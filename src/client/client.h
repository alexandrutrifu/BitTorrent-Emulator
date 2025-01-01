#include <iostream>
#include <fstream>
#include <string>

#include <unordered_map>
#include <vector>

#include "../file.h"

using namespace std;

namespace clients {
    class Client {
    public:
        Client() = default;
        Client(int rank) { id = rank; }
        ~Client() = default;

        int parseInput();

        vector<File *> getOwnedFiles() { return ownedFiles; }
        vector<string> getDesiredFiles() { return desiredFiles; }
        void addOwnedFile(File *file) { ownedFiles.push_back(file); }
        void addDesiredFile(string name) { desiredFiles.push_back(name); }

        int id;
        int ownedFileCount;
        int desiredFileCount;

    private:
        vector<File *> ownedFiles;
        vector<string> desiredFiles;
    };
}