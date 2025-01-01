#include "client.h"

int clients::Client::parseInput() {
    string inputFileName = "in" + to_string(id) + ".txt";

    // Debug
    cout << "[CLIENT " << id << "] parsing input file " << inputFileName << "\n";
    
    ifstream inputFile("/Users/alex/facultate/anul3/apd/tema2-apd/checker/tests/test1/" + inputFileName);
    if (!inputFile.is_open()) {
        cerr << "File opening error\n";
        return -1;
    }

    // Parse file contents
    inputFile >> this->ownedFileCount;

    for (int index = 0; index < this->ownedFileCount; index++) {
        File *file = new File();

        inputFile >> file->name;
        inputFile >> file->segmentCount;

        for (int segmentIndex = 0; segmentIndex < file->segmentCount; segmentIndex++) {
            inputFile >> file->segments[segmentIndex];
        }

        file->completeStatus();

        // Add file to client structure
        this->addOwnedFile(file);
    }

    // Parse desired files
    inputFile >> this->desiredFileCount;

    for (int index = 0; index < this->desiredFileCount; index++) {
        string desiredFileName;

        inputFile >> desiredFileName;

        // Add to desired files
        this->addDesiredFile(new File(desiredFileName));
    }

    // Debug
    cout << "[CLIENT " << id << "] parsed " << this->ownedFileCount << " owned files and " << this->desiredFileCount << " desired files\n";

    // Close input file
    inputFile.close();

    return 0;
}

int clients::Client::sendInputToTracker() {
    // Send how many files the client has
    MPI_Send(&this->ownedFileCount, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

    // Send owned files
    for (auto& file: this->ownedFiles) {
        // Send file name
        MPI_Send(file->name.c_str(), file->name.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);

        // Send segment count
        MPI_Send(&file->segmentCount, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

        // Send segments
        for (int segmentIndex = 0; segmentIndex < file->segmentCount; segmentIndex++) {
            // Extract segment and size
            string segment = file->segments[segmentIndex];

            // Send segment
            MPI_Send(segment.c_str(), SEGMENT_SIZE, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
        }
    }

    return 0;
}

int *clients::Client::requestSeeds(File *file) {
    MPI_Status status;

    // Send request for seeds
    int request = requestIndex(REQUEST_SEEDS);

    MPI_Send(&request, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

    // Send file name
    MPI_Send(file->name.c_str(), file->name.size(), MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);

    // Receive segment count
    int segmentCount;

    MPI_Recv(&segmentCount, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, &status);

    file->segmentCount = segmentCount;

    // Receive seeds
    int seedCount;

    MPI_Recv(&seedCount, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, &status);

    int *seeds = new int[seedCount];

    MPI_Recv(seeds, seedCount, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, &status);

    return seeds;
}
