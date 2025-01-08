#include "FileSystem.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <cstring>
#include <utility>

void FileSystem::createDisk(const std::string& diskName, u_int32_t size_MB){
    this->diskName = diskName;
    allocateDiskSpace(size_MB);
    createDiskInfo(size_MB);
    saveDiskInfo();
}
void FileSystem::deleteDisk(const std::string& diskName){
    std::remove(diskName.c_str());
}
void FileSystem::loadDisk(const std::string& diskName){
    this->diskName = diskName;
    loadDiskInfo();
    calculateTablesSizes(diskSuperBlockInfo.diskSize);
    if ((sizeof(SuperBlock) + INodesBitMapBytesSize + DataBlocksBitMapBytesSize) != diskSuperBlockInfo.INodesSectionStartAddr)
        throw "invalid INodes Section Start Address";
    if ((sizeof(SuperBlock) + INodesBitMapBytesSize + DataBlocksBitMapBytesSize + INodesSectionBytesSize) != diskSuperBlockInfo.DataBlocksSectionStartAddr)
        throw "invalid DataBlocks Section Start Address";
    loadINodesBitMap();
    loadDataBlocksBitMap();
}

void FileSystem::showDiskBitMaps(){
    std::cout<<"\tINodes BitMap\n------------------------------\n";
    for (const auto& n : INodesBitMap)
        std::cout<<n<<" ";
    std::cout<<"\n------------------------------\n";
    std::cout<<"\tDataBlocks BitMap\n------------------------------\n";
    for (const auto& db : DataBlocksBitMap)
        std::cout<<db<<" ";
    std::cout<<"\n------------------------------\n";
}

const bool FileSystem::addFile(const std::string& fileName){
    if (fileName.size() > MAX_FILENAME_SIZE){
        std::cout<<"Error: File Name is too long\n";
        return false;
    }
    std::ifstream file(fileName, std::ios::binary | std::ios::in);
    if (!file){
        std::cerr<<"Error: Unable to get file";
        return false;
    }
    std::cout<<"File Name: "<<fileName<<"\n";
    std::streampos fSize = 0;
    fSize = file.tellg();
    file.seekg(0, std::ios::end );
    fSize = file.tellg() - fSize;
    file.close();
    std::cout<<"File size [B]: "<<fSize<<"\n";
    size_t neededDataBlocksNum = (u_int32_t(fSize) + DATABLOCK_DATA_SIZE - 1)/DATABLOCK_DATA_SIZE;
    std::cout<<"Needed DataBlocks: "<<neededDataBlocksNum<<"\n";
    const auto space = availableSpace();
    if (!(space.first > 0) || !(space.second >= neededDataBlocksNum)){
        std::cerr<<"Error: Disk is full, delete files first\n";
        return false;
    }
    //files fit into disk
    size_t INodeIndex = getFreeINodeIndex();
    std::vector<size_t> DataBlocksIndexes = getFreeDataBlocksIndexes(neededDataBlocksNum);
    //modify INodesBitMap
    INodesBitMap[INodeIndex] = true;
    //modify DataBlocksBitMap and make DataBlocks
    
    //open file again
    file.open(fileName, std::ios::binary | std::ios::in);
    if (!file){
        std::cerr<<"Error: Unable to get file";
        return false;
    }
    std::vector<DataBlock> _DataBlocks;
    for (size_t i = 0; i < DataBlocksIndexes.size(); i++){
        u_int8_t buffer[DATABLOCK_DATA_SIZE] = {0};
        size_t cdbi = DataBlocksIndexes[i];
        DataBlocksBitMap[cdbi] = true;
        file.read((char*)buffer, DATABLOCK_DATA_SIZE);
        u_int32_t nextDataBlockAddr = 0;
        if (i < DataBlocksIndexes.size() - 1)
            nextDataBlockAddr = DataBlocksIndexes[i+1] * sizeof(DataBlock) + diskSuperBlockInfo.DataBlocksSectionStartAddr;
        DataBlock _DataBlock;
        _DataBlock.nextDataBlockAddr = nextDataBlockAddr;
        // std::memcpy((char*)_DataBlock.data, (char*)buffer, file.gcount());
        std::memcpy((char*)_DataBlock.data, (char*)buffer, DATABLOCK_DATA_SIZE);

        _DataBlocks.push_back(_DataBlock);
    }
    u_int32_t firstDataBlockAddr = DataBlocksIndexes[0] * sizeof(DataBlock) + diskSuperBlockInfo.DataBlocksSectionStartAddr;
    saveDataBlocks(_DataBlocks, firstDataBlockAddr);
    
    //make INode
    INode _INode = {firstDataBlockAddr, u_int32_t(fSize), ""};  
    std::strncpy(_INode.fileName, fileName.c_str(), fileName.size());
    u_int32_t _INodeAddr = INodeIndex * sizeof(INode) + diskSuperBlockInfo.INodesSectionStartAddr;
    saveINode(_INode, _INodeAddr);
    saveINodesBitMap();
    saveDataBlocksBitMap();
    std::cout<<"File added successfully.\n";
    return true;
}
void FileSystem::listFiles(){
    for(size_t i=0; i <INodesBitMap.size(); i++){
        if (!INodesBitMap[i])
            continue;
        INode _INode = loadINode(i*sizeof(INode) + diskSuperBlockInfo.INodesSectionStartAddr);
        std::cout<<"\nFile INode Index: "<<i<<"\n----------------------------------\n";
        std::cout<<"Name: "<<_INode.fileName<<"\nSize [B]: "<<_INode.fileSize_B<<"\nFirst DataBlock Address: "<<_INode.firstDataBlockAddr;
        std::cout<<"\n----------------------------------\n";

    }
}
const bool FileSystem::deleteFile(size_t fileINodeIndex){
    if (!INodesBitMap[fileINodeIndex]){
        std::cerr<<"Error: File with this Index does not exist on disk.\n";
        return false;
    }
    u_int32_t _INodeAddres = fileINodeIndex * sizeof(INode) + diskSuperBlockInfo.INodesSectionStartAddr;
    INode _INode = loadINode(_INodeAddres);
    std::vector<DataBlock> _DataBlocks = loadDataBlocks(_INode.firstDataBlockAddr);
    //clear the DataBlocks? - not needed however could be good

    //get indexes of DataBlocks
    u_int32_t nextDataBlockAddr = _INode.firstDataBlockAddr; 
    for (const auto& db : _DataBlocks){
        size_t DataBlockIndex = (nextDataBlockAddr - diskSuperBlockInfo.DataBlocksSectionStartAddr) / sizeof(DataBlock);
        nextDataBlockAddr = db.nextDataBlockAddr; 
        //could clear DataBlock
        DataBlocksBitMap[DataBlockIndex] = 0;
    }
    size_t INodeIndex = (_INodeAddres - diskSuperBlockInfo.INodesSectionStartAddr) / sizeof(INode);
    INodesBitMap[INodeIndex] = 0;
    saveINodesBitMap();
    saveDataBlocksBitMap();
    return true;
}
void FileSystem::getFile(size_t fileINodeIndex, const std::string& targetFileName){
    if (!INodesBitMap[fileINodeIndex]){
        std::cerr<<"Error: File with this Index does not exist on disk.\n";
        return;
    }
    u_int32_t _INodeAddres = fileINodeIndex * sizeof(INode) + diskSuperBlockInfo.INodesSectionStartAddr;
    INode _INode = loadINode(_INodeAddres);
    std::vector<DataBlock> _DataBlocks = loadDataBlocks(_INode.firstDataBlockAddr);
    //save to outputfile
    std::ofstream file;
    if (targetFileName.empty())
         file.open(_INode.fileName, std::ios::binary | std::ios::out);
    else
        file.open(targetFileName, std::ios::binary | std::ios::out);
    u_int32_t bytesToWrite = _INode.fileSize_B;
    for (const auto & db : _DataBlocks){
        if (!(bytesToWrite > 0)){
            std::cerr<<"Error: File size smaller than DataBlocks saved info.\n";
            throw "corrupted disk";
        }
        if (bytesToWrite >= DATABLOCK_DATA_SIZE){
            file.write((char*)db.data, DATABLOCK_DATA_SIZE);
            bytesToWrite-=DATABLOCK_DATA_SIZE;
        }
        else{
            file.write((char*)db.data, bytesToWrite);
            bytesToWrite-=bytesToWrite; // bytesToWrite = 0;
        }
    }
    file.close();
}

void FileSystem::calculateTablesSizes(u_int32_t size_MB){
    u_int32_t size_B = size_MB * 1048576;
    INodesBitMapBytesSize = (INODES_NUM + 7) / 8;
    INodesSectionBytesSize = INODES_NUM * sizeof(INode);
    DataBlocksNum = (8 * (size_B - (sizeof(SuperBlock) + INodesBitMapBytesSize + INodesSectionBytesSize)) - 7) / (sizeof(DataBlock) * 8 + 1);
    DataBlocksBitMapBytesSize = (DataBlocksNum + 7) / 8;
}
void FileSystem::createDiskInfo(u_int32_t size_MB){
    calculateTablesSizes(size_MB);
    u_int32_t _INodesSectionStartAddr = sizeof(SuperBlock) + INodesBitMapBytesSize + DataBlocksBitMapBytesSize;
    u_int32_t _DataBlocksSectionStartAddr = _INodesSectionStartAddr + INodesSectionBytesSize;
    diskSuperBlockInfo = {size_MB, _INodesSectionStartAddr, _DataBlocksSectionStartAddr};
}
void FileSystem::saveDiskInfo(){
    saveSuperBlock();
}
void FileSystem::allocateDiskSpace(int size_MB){
    std::ofstream disk(diskName, std::ios::binary | std::ios::out);
    std::vector<char> empty(1024, 0); // 1kib of 0's
    for(int i = 0; i < 1024*size_MB; i++)
    {
        if (!disk.write(&empty[0], empty.size()))
        {
            std::cerr << "Error: Problem with writing empty disk.\n" << std::endl;
            throw;
        }
    }
    disk.close();
}
void FileSystem::loadDiskInfo(){
    std::ifstream disk(diskName, std::ios::binary | std::ios::in);
    if(!disk){
        std::cerr<<"Error: No disk with such name\n";
        throw;
    }
    disk.read((char*)&diskSuperBlockInfo, sizeof(SuperBlock));
    std::cout<<"\tLoaded Disk Info\n----------------------------------\nDisk Name: "<<diskName;
    std::cout<<"\nDisk Size [MB]: "<<diskSuperBlockInfo.diskSize << "\nINodes Section Addres: "<< diskSuperBlockInfo.INodesSectionStartAddr;
    std::cout<<"\nDataBlocks Section Address: "<<diskSuperBlockInfo.DataBlocksSectionStartAddr<< "\n----------------------------------\n";
}
BytesVector FileSystem::convertBitsToBytes(const BitsVector& bitsVector) const{
    BytesVector bytesVector;
    size_t bitsNum = bitsVector.size();
    size_t bytesCount = (bitsNum + 7) / 8;
    bytesVector.resize(bytesCount, 0);

    for (size_t i = 0; i < bitsNum; ++i) {
        size_t byteIndex = i / 8; 
        size_t bitPosition = i % 8;
        if (bitsVector[i]) {
            bytesVector[byteIndex] |= (1 << bitPosition);
        }
    }
    return bytesVector;
}
BitsVector FileSystem::convertBytesToBits(const BytesVector& bytesVector, size_t bitsNum) const{
    BitsVector bitsVector;
    int bitsCount = 0;
    for (u_int8_t byte : bytesVector) {
        for (int i = 0; i < 8; ++i) {
            if (bitsCount++ >bitsNum)
                return bitsVector;
            bool bit = (byte >> i) & 1; // Extract the i-th bit
            bitsVector.push_back(bit);
        }
    }
    return bitsVector;
}
void FileSystem::saveBitsVector(const BitsVector& bitsVector, u_int32_t bytesVectorAddr) const{
    std::ofstream disk(diskName, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk) {
        std::cerr << "Error: Could not open disk file.\n";
        throw;
    }
    BytesVector bytesVector = convertBitsToBytes(bitsVector);
    disk.seekp(bytesVectorAddr);
    disk.write((char*)bytesVector.data(), bytesVector.size());
    disk.close();
}
BytesVector FileSystem::loadBytesVector(size_t bytesNum, u_int32_t bytesVectorAddr){
    BytesVector bytesVector(bytesNum, 0);
    std::ifstream disk(diskName, std::ios::binary | std::ios::in);
    if (!disk) {
        std::cerr << "Error: Could not open disk file.\n";
        throw;
    }
    disk.seekg(bytesVectorAddr);
    disk.read((char*)bytesVector.data(), bytesNum);
    return bytesVector;
}

void FileSystem::loadINodesBitMap(){
    u_int32_t INodesBitMapAddr = sizeof(SuperBlock);
    BytesVector INodesBytesMap = loadBytesVector(INodesBitMapBytesSize, INodesBitMapAddr);
    INodesBitMap = convertBytesToBits(INodesBytesMap, INODES_NUM);
}

void FileSystem::loadDataBlocksBitMap(){
    u_int32_t DataBlocksBitMapAddr = sizeof(SuperBlock) + INodesBitMapBytesSize;
    BytesVector DataBlocksBytesMap = loadBytesVector(DataBlocksBitMapBytesSize, DataBlocksBitMapAddr);
    DataBlocksBitMap = convertBytesToBits(DataBlocksBytesMap, DataBlocksNum);
}
void FileSystem::saveINodesBitMap(){
    u_int32_t INodesBitMapAddr = sizeof(SuperBlock);
    saveBitsVector(INodesBitMap, INodesBitMapAddr);
}

void FileSystem::saveDataBlocksBitMap(){
    u_int32_t DataBlocksBitMapAddr = sizeof(SuperBlock) + INodesBitMapBytesSize;
    saveBitsVector(DataBlocksBitMap, DataBlocksBitMapAddr);
}
void FileSystem::saveSuperBlock() const {
    std::ofstream disk(diskName, std::ios::binary | std::ios::out | std::ios::in);
    disk.write((char*)&diskSuperBlockInfo, sizeof(SuperBlock));
    disk.close();
}
const u_int FileSystem::freeINodesNum() const{
    u_int freeINodes = 0;
    for (const auto& n: INodesBitMap){
        if(!n)
            freeINodes++;
    }
    return freeINodes;
}

const u_int FileSystem::freeDataBlocksNum() const{
    u_int freeDataBlocks = 0;
    for (const auto& n: DataBlocksBitMap){
        if(!n)
            freeDataBlocks++;
    }
    return freeDataBlocks;
}

const std::pair<size_t, size_t> FileSystem::availableSpace() const{
    return {freeINodesNum(), freeDataBlocksNum()};
}

const size_t FileSystem::getFreeINodeIndex() const {
    for (size_t i=0; i < INodesBitMap.size(); i++)
        if (!INodesBitMap[i])
            return i;
    std::cerr<<"Error: No free INode found";
    throw;
}

const std::vector<size_t> FileSystem::getFreeDataBlocksIndexes(size_t DataBlocksNum) const{
    std::vector<size_t> indexes;
    for (size_t i=0; i < DataBlocksBitMap.size(); i++)
        if (!DataBlocksBitMap[i] && indexes.size() != DataBlocksNum)
            indexes.push_back(i);
    if (indexes.size() < DataBlocksNum){
        std::cerr<<"Error: Not enough free DataBlocks found";
        throw;
    }
    return indexes; 
}
void FileSystem::saveINode(const INode& _INode, u_int32_t INodeAddr){
    std::ofstream disk(diskName, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk) {
        std::cerr << "Error: Could not open disk file.\n";
        throw;
    }
    disk.seekp(INodeAddr);
    disk.write((char*)&_INode, sizeof(INode));
    disk.close();
}
void FileSystem::saveDataBlocks(const std::vector<DataBlock>& _DataBlocks, u_int32_t firstDataBlockAddr){
    std::ofstream disk(diskName, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk) {
        std::cerr << "Error: Could not open disk file.\n";
        throw;
    }
    disk.seekp(firstDataBlockAddr);
    disk.write((char*)&_DataBlocks[0], sizeof(DataBlock));
    for (size_t i = 1; i < _DataBlocks.size(); i++){
        disk.seekp(_DataBlocks[i-1].nextDataBlockAddr);
        disk.write((char*)&_DataBlocks[i], sizeof(DataBlock));
    }
    disk.close();
}
INode FileSystem::loadINode(u_int32_t INodeAddr){
    std::ifstream disk(diskName, std::ios::binary | std::ios::in);
    if (!disk) {
        std::cerr << "Error: Could not open disk file.\n";
        throw;
    }
    disk.seekg(INodeAddr);
    INode _INode;
    disk.read((char*)&_INode, sizeof(INode));
    disk.close();
    return _INode;
}
std::vector<DataBlock> FileSystem::loadDataBlocks(u_int32_t firstDataBlockAddr){
    std::ifstream disk(diskName, std::ios::binary | std::ios::in);
    if (!disk) {
        std::cerr << "Error: Could not open disk file.\n";
        throw;
    }
    u_int32_t nextDataBlockAddr = firstDataBlockAddr;
    std::vector<DataBlock> _DataBlocks;
    while (nextDataBlockAddr != 0){
    disk.seekg(nextDataBlockAddr);
    DataBlock _DataBlock;
    disk.read((char*)&_DataBlock, sizeof(DataBlock));
    nextDataBlockAddr = _DataBlock.nextDataBlockAddr;
    _DataBlocks.push_back(_DataBlock);
    }
    return _DataBlocks;
}