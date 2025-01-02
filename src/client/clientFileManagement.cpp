#include "client.h"

void clients::Client::updateFile(File *file, int segmentIndex, int seed) {
    // Receive segment
    char segment[SEGMENT_SIZE];
    MPI_Status status;

    MPI_Recv(segment, SEGMENT_SIZE, MPI_CHAR, seed, 0, MPI_COMM_WORLD, &status);

    // Update file
    file->segments[segmentIndex] = segment;
    file->segmentsLacked--;
}

int clients::Client::parseInput()
{
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

        File *file = new File(desiredFileName);

        file->segmentsLacked = file->segmentCount;

        // Add to desired files
        this->addDesiredFile(file);
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

void clients::Client::sendDownloadCompleteSignal() {
    int request = trackerRequestIndex(DOWNLOAD_COMPLETE);

    MPI_Send(&request, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

    // Await confirmation
    char reply[4];
    MPI_Status status;

    MPI_Recv(reply, 4, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD, &status);
}

void clients::Client::sendFinishedSignal() {
    int request = trackerRequestIndex(FINISHED);

    MPI_Send(&request, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

    // Await confirmation
    char reply[4];
    MPI_Status status;

    MPI_Recv(reply, 4, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD, &status);
}