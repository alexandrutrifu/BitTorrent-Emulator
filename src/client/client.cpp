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

        // Query file swarm for missing segments
        for (int i = 0; i < currentDownload->seedCount; i++) {
            int seed = currentDownload->seeds[i];

            // Send request for segment
            // TODO

            // Receive reply - either segment or decline
            // TODO

            // If segment received, update file
            segmentsReceived++;

            // Every 10 segments, update list of file seeds
            if (segmentsReceived == 10) {
                this->requestSeeds();
                segmentsReceived = 0;
            }

            if (currentDownload->segmentsLacked == 0) {
                pendingDownloads--;
                
                if (pendingDownloads == 0) {
                    break;
                }

                currentDownload = this->getDesiredFiles()[++fileIndex];
                continue;
            }

            break;

            // Otherwise, continue requesting segments
        }
    }
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
