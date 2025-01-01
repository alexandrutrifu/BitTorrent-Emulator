#include "client.h"

void clients::Client::download() {
    // Request seeds for desired files
    this->requestSeeds();

    int pendingDownloads = this->getDesiredFiles().size();
    int fileIndex = 0;
    int segmentsReceived = 0;
    File *currentDownload = this->getDesiredFiles()[0];

    while (pendingDownloads) {
        // Request segment from seeds
        int segmentIndex = this->nextDesiredSegment(currentDownload);

        int ret = this->querySwarm(currentDownload, segmentIndex);

        if (ret == -1) {    // Error
            cerr << "Error querying swarm\n";
            return;
        }

        if (ret == -2) { // No seeds available
            continue;
        }

        // Move to next segment/file
        if (currentDownload->segmentsLacked == 0) {
            pendingDownloads--;
            
            if (pendingDownloads == 0) {
                break;
            }

            currentDownload = this->getDesiredFiles()[++fileIndex];
            continue;
        }

        // Every 10 segments, update list of file seeds
        segmentsReceived++;

        if (segmentsReceived == 10) {
            this->requestSeeds();
            segmentsReceived = 0;
        }
    }
}

int clients::Client::querySwarm(File *file, int segmentIndex) {
    for (int i = 0; i < file->seedCount; i++) {
        int seed = file->seeds[i];

        // Ignore self (we are peers for others)
        if (seed == this->id) {
            continue;
        }

        // Send request for segment
        int request = clientRequestIndex(REQUEST_SEGMENT);

        MPI_Send(&request, 1, MPI_INT, seed, 0, MPI_COMM_WORLD);

        // Send file name & segment index
        MPI_Send(file->name.c_str(), file->name.size(), MPI_CHAR, seed, 0, MPI_COMM_WORLD);
        MPI_Send(&segmentIndex, 1, MPI_INT, seed, 0, MPI_COMM_WORLD);

        // Receive reply
        int reply;
        MPI_Status status;

        MPI_Recv(&reply, 1, MPI_INT, seed, 0, MPI_COMM_WORLD, &status);

        // Parse reply
        ClientRequest clientRequest = indexToClientRequest(reply);

        switch (clientRequest) {
            case DECLINED:
                continue;
            case ACCEPTED:
                this->updateFile(file, segmentIndex, seed);
                return 0;
            default:
                return -1;
        }
    }

    return -2;
}

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

void clients::Client::requestSeeds() {
    for (File *file: this->desiredFiles) {
        file->seeds = getFileSeeds(file);
    }
}

int *clients::Client::getFileSeeds(File *file) {
    MPI_Status status;

    // Send request for seeds
    int request = trackerRequestIndex(REQUEST_SEEDS);

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

    file->seedCount = seedCount;

    int *seeds = new int[seedCount];

    MPI_Recv(seeds, seedCount, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, &status);

    return seeds;
}

int clients::Client::nextDesiredSegment(File *file) {
    for (int segmentIndex = 0; segmentIndex < file->segmentCount; segmentIndex++) {
        if (file->segments.find(segmentIndex) == file->segments.end()) {
            return segmentIndex;
        }
    }
}
