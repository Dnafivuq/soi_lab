#include "FileSystem.hpp"

void printHelp() {
    std::cout << "Available Commands:\n";
    std::cout << "  crt <diskname> <disksize>\t\t- Create a new disk with the specified name and size [MB].\n";
    std::cout << "  del <diskname>\t\t\t- Delete the specified disk.\n";
    std::cout << "  bm <diskname>\t\t\t\t- Show the bitmap of the specified disk.\n";
    std::cout << "  lf <diskname>\t\t\t\t- List files on the specified disk.\n";
    std::cout << "  af <diskname> <filename>\t\t- Add a file to the specified disk.\n";
    std::cout << "  df <diskname> <fileindex>\t\t- Delete a file from the specified disk by index.\n";
    std::cout << "  gf <diskname> <fileindex> [filename]\t- Get a file from the disk by index and optionally save it to a filename.\n";
    std::cout << "  h\t\t\t\t\t- Print this help message.\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: Not enough arguments.\n";
        std::cerr << "Usage: <command> [arguments]\n";
        std::cerr << "Use 'h' for a list of available commands.\n";
        return 1;
    }
    FileSystem f;
    std::string command = argv[1];
    if (command == "h") {
        printHelp();
    } else if (command == "lf") {
        if (argc < 3) {
            std::cerr << "Error: 'lf' requires <diskname>.\n";
            return 1;
        }
        std::string diskname = argv[2];
        f.loadDisk(diskname);
        f.listFiles();
    } else if (command == "crt") {
        if (argc < 4) {
            std::cerr << "Error: 'crt' requires <diskname> and <disksize>.\n";
            return 1;
        }
        std::string diskname = argv[2];
        int disksize = std::stoi(argv[3]);
        if (disksize <= 0) {
            std::cerr << "Error: <disksize> must be a positive integer.\n";
            return 1;
        }
        f.createDisk(diskname, disksize);
    } else if (command == "bm") {
        if (argc < 3) {
            std::cerr << "Error: 'bm' requires <diskname>.\n";
            return 1;
        }
        std::string diskname = argv[2];
        f.loadDisk(diskname);
        f.showDiskBitMaps();
    } else if (command == "del") {
        if (argc < 3) {
            std::cerr << "Error: 'del' requires <diskname>.\n";
            return 1;
        }
        std::string diskname = argv[2];
        f.deleteDisk(diskname);
    } else if (command == "af") {
        if (argc < 4) {
            std::cerr << "Error: 'af' requires <diskname> and <filename>.\n";
            return 1;
        }
        std::string diskname = argv[2];
        std::string filename = argv[3];
        f.loadDisk(diskname);
        f.addFile(filename);
    } else if (command == "df") {
        if (argc < 4) {
            std::cerr << "Error: 'df' requires <diskname> and <fileindex>.\n";
            return 1;
        }
        std::string diskname = argv[2];
        int fileIndex = std::stoi(argv[3]);
        f.loadDisk(diskname);
        f.deleteFile(fileIndex);
    } else if (command == "gf") {
        if (argc < 4) {
            std::cerr << "Error: 'gf' requires <diskname>, <fileindex>, and optional <filename>.\n";
            return 1;
        }
        std::string diskname = argv[2];
        int fileIndex = std::stoi(argv[3]);
        std::string filename = (argc > 4) ? argv[4] : "";
        f.loadDisk(diskname);
        f.getFile(fileIndex, filename);
    } else {
        std::cerr << "Error: Unknown command '" << command << "'.\n";
        std::cerr << "Use 'h' for a list of available commands.\n";
        return 1;
    }

    return 0;
}