#pragma once

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <random>

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

        // Download methods
        void download();
        int querySwarm(File *file, int segmentIndex);
        void updateFile(File *file, int segmentIndex, int seed);

        // Upload methods
        void upload();

        // Tracker communication
        void sendDownloadCompleteSignal();
        void sendFinishedSignal();

        // File management
        int parseInput();
        int sendInputToTracker();
        void requestSeeds();
        int *getFileSeeds(File *file);
        int nextDesiredSegment(File *file);

        // Output methods
        void saveHashes(File *file);

        // Getters & setters
        vector<File *> getOwnedFiles() { return ownedFiles; }
        vector<File *> getDesiredFiles() { return desiredFiles; }
        void addOwnedFile(File *file) { ownedFiles.push_back(file); }
        void addDesiredFile(File *file) { desiredFiles.push_back(file); }

        int id;
        int ownedFileCount;
        int desiredFileCount;

        // Log files
        ofstream clientLog;

    private:
        vector<File *> ownedFiles;
        vector<File *> desiredFiles;
    };
}