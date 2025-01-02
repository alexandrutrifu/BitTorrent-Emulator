#include "client.h"

void clients::Client::updateFile(File *file, int segmentIndex, int seed) {
    // Receive segment
    char segment[SEGMENT_SIZE];
    MPI_Status status;

    MPI_Recv(segment, SEGMENT_SIZE, MPI_CHAR, seed, DOWNLOAD_TAG, MPI_COMM_WORLD, &status);

    // Update file
    file->segments[segmentIndex] = segment;
    file->segmentsLacked--;
}

int clients::Client::parseInput()
{
    string inputFileName = "in" + to_string(id) + ".txt";

    // Log
    this->clientLog << "[CLIENT " << id << "] parsing input file " << inputFileName << "\n" << std::flush;
    
    ifstream inputFile(inputFileName);
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

        File *file = new File(desiredFileName);

        // Add to desired files & owned files (no segments yet)
        this->addDesiredFile(file);
        this->addOwnedFile(file);
    }

    // Log
    this->clientLog << "[CLIENT " << id << "] parsed " << this->ownedFileCount << " owned files and " << this->desiredFileCount << " desired files\n" << std::flush;

    // Close input file
    inputFile.close();

    return 0;
}

int clients::Client::sendInputToTracker() {
    // Send how many files the client has
    MPI_Send(&this->ownedFileCount, 1, MPI_INT, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD);

    // Send owned files
    for (auto& file: this->ownedFiles) {
        if (file->status == INCOMPLETE) {
            continue;
        }

        // Send file size
        int fileSize = file->name.size() + 1;
        char fileName[fileSize];

        strcpy(fileName, file->name.c_str());

        MPI_Send(&fileSize, 1, MPI_INT, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD);

        // Send file name
        MPI_Send(fileName, fileSize, MPI_CHAR, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD);

        // Send segment count
        MPI_Send(&file->segmentCount, 1, MPI_INT, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD);

        // Send segments
        for (int segmentIndex = 0; segmentIndex < file->segmentCount; segmentIndex++) {
            // Extract segment and size
            string segment = file->segments[segmentIndex];

            // Send segment
            MPI_Send(segment.c_str(), SEGMENT_SIZE, MPI_CHAR, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD);
        }
    }

    return 0;
}

void clients::Client::sendDownloadCompleteSignal() {
    int request = trackerRequestIndex(DOWNLOAD_COMPLETE);

    MPI_Send(&request, 1, MPI_INT, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD);

    // Await confirmation
    char reply[4];
    MPI_Status status;

    MPI_Recv(reply, 4, MPI_CHAR, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD, &status);
}

void clients::Client::sendFinishedSignal() {
    int request = trackerRequestIndex(FINISHED);

    MPI_Send(&request, 1, MPI_INT, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD);

    // Await confirmation
    char reply[4];
    MPI_Status status;

    MPI_Recv(reply, 4, MPI_CHAR, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD, &status);
}

void clients::Client::saveHashes(File *file) {
    ofstream outputFile("client" + to_string(id) + "_" + file->name);
    if (!outputFile.is_open()) {
        cerr << "File opening error\n";
        return;
    }

    for (int segmentIndex = 0; segmentIndex < file->segmentCount; segmentIndex++) {
        outputFile << file->segments[segmentIndex];
        
        if (segmentIndex < file->segmentCount - 1) {
            outputFile << '\n';
        }
    }

    outputFile.close();
}