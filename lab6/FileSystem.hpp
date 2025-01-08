#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <cstring>
#include <utility>


using BitsVector = std::vector<bool>;
using BytesVector = std::vector<u_int8_t>;

#define DATABLOCK_DATA_SIZE 4092
#define MAX_FILENAME_SIZE 52
#define INODES_NUM 64 // Number of files

struct SuperBlock{  //12B
    u_int32_t diskSize=0;
    u_int32_t INodesSectionStartAddr=0;
    u_int32_t DataBlocksSectionStartAddr=0;
};

struct INode{   //60B
    u_int32_t firstDataBlockAddr=0;
    u_int32_t fileSize_B=0;
    char fileName[MAX_FILENAME_SIZE]={0};
};

struct DataBlock{   //4096B
    u_int32_t nextDataBlockAddr=0;
    u_int8_t data[DATABLOCK_DATA_SIZE]={0};
};

class FileSystem{
    public:
    void createDisk(const std::string& diskName, u_int32_t size_MB);
    void deleteDisk(const std::string& diskName);
    void loadDisk(const std::string& diskName);
    void showDiskBitMaps();
    const bool addFile(const std::string& fileName);
    void listFiles();
    const bool deleteFile(size_t fileINodeIndex);
    void getFile(size_t fileINodeIndex, const std::string& targetFileName);
    private:
    void calculateTablesSizes(u_int32_t size_MB);
    void createDiskInfo(u_int32_t size_MB);
    void saveDiskInfo();
    void allocateDiskSpace(int size_MB);
    void loadDiskInfo();
    BytesVector convertBitsToBytes(const BitsVector& bitsVector) const;
    BitsVector convertBytesToBits(const BytesVector& bytesVector, size_t bitsNum) const;
    void saveBitsVector(const BitsVector& bitsVector, u_int32_t bytesVectorAddr) const;
    BytesVector loadBytesVector(size_t bytesNum, u_int32_t bytesVectorAddr);
    void loadINodesBitMap();
    void loadDataBlocksBitMap();
    void saveINodesBitMap();
    void saveDataBlocksBitMap();
    void saveSuperBlock() const;
    const u_int freeINodesNum() const;
    const u_int freeDataBlocksNum() const;
    const std::pair<size_t, size_t> availableSpace() const;
    const size_t getFreeINodeIndex() const;
    const std::vector<size_t> getFreeDataBlocksIndexes(size_t DataBlocksNum) const;
    void saveINode(const INode& _INode, u_int32_t INodeAddr);
    void saveDataBlocks(const std::vector<DataBlock>& _DataBlocks, u_int32_t firstDataBlockAddr);
    INode loadINode(u_int32_t INodeAddr);
    std::vector<DataBlock> loadDataBlocks(u_int32_t firstDataBlockAddr);

    private:
    std::string diskName;
    SuperBlock diskSuperBlockInfo;
    BitsVector INodesBitMap;
    BitsVector DataBlocksBitMap;
    u_int32_t INodesBitMapBytesSize;
    u_int32_t INodesSectionBytesSize;
    u_int32_t DataBlocksNum;
    u_int32_t DataBlocksBitMapBytesSize;
    // no need to keep INodes or DataBlocks in Memory, tables are sufficient
};
#endif
