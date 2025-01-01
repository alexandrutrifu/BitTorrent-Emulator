#pragma once

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>

#include <unordered_map>
#include <vector>

#include "../file.h"
#include "../common.h"

using namespace std;

namespace clients {
    class Client {
    public:
        Client() = default;
        Client(int rank) { id = rank; }
        ~Client() = default;

        int parseInput();
        int sendInputToTracker();
        int *requestSeeds(File *file);

        vector<File *> getOwnedFiles() { return ownedFiles; }
        vector<File *> getDesiredFiles() { return desiredFiles; }
        void addOwnedFile(File *file) { ownedFiles.push_back(file); }
        void addDesiredFile(File *file) { desiredFiles.push_back(file); }

        int id;
        int ownedFileCount;
        int desiredFileCount;

    private:
        vector<File *> ownedFiles;
        vector<File *> desiredFiles;
    };
}