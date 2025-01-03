#include "client.h"

void clients::Client::download() {
    // Log
    this->clientLog << "[CLIENT " << this->id << "] starting download\n" << std::flush;

    // Request seeds for desired files
    this->requestSeeds();

    int pendingDownloads = this->getDesiredFiles().size();
    int fileIndex = 0;
    int segmentsReceived = 0;
    File *currentDownload = this->getDesiredFiles()[0];

    // Log
    this->clientLog << "[CLIENT " << this->id << "] File " << currentDownload->name << " has following seeds: ";

    for (int i = 0; i < currentDownload->seedCount; i++) {
        this->clientLog << currentDownload->seeds[i] << " ";
    }

    this->clientLog << '\n' << std::flush;

    while (pendingDownloads) {
        // Request segment from seeds
        int segmentIndex = this->nextDesiredSegment(currentDownload);

        // Log
        this->clientLog << "[CLIENT " << this->id << "] requesting segment " << segmentIndex << " from " << currentDownload->name << '\n' << std::flush;

        int ret = this->querySwarm(currentDownload, segmentIndex);

        if (ret == -1) {    // Error
            cerr << "Client " << this->id << " encountered an error while downloading\n";
            return;
        }

        // Log
        this->clientLog << "[CLIENT " << this->id << "] needs " << currentDownload->segmentsLacked << " more segments from " << currentDownload->name << '\n' << std::flush;

        // Move to next segment/file
        if (currentDownload->segmentsLacked == 0) {
            pendingDownloads--;

            // Log
            this->clientLog << "[CLIENT " << this->id << "] finished downloading " << currentDownload->name << '\n' << std::flush;

            this->sendDownloadCompleteSignal();

            // Write hashes to client output file
            saveHashes(currentDownload);
            
            if (pendingDownloads == 0) {
                break;
            }

            currentDownload = this->getDesiredFiles()[++fileIndex];

            // Log
            this->clientLog << "[CLIENT " << this->id << "] starting download of " << currentDownload->name << '\n' << std::flush;

            continue;
        }

        // Every 10 segments, update list of file seeds
        segmentsReceived++;

        if (segmentsReceived == 10) {
            // Log
            this->clientLog << "[CLIENT " << this->id << "] updating seeds\n" << std::flush;
            
            this->requestSeeds();
            segmentsReceived = 0;
        }
    }

    this->sendFinishedSignal();
}

int clients::Client::querySwarm(File *file, int segmentIndex) {
    while (true) {
        // Choose a random seed
        int seed = file->seeds[rand() % file->seedCount];

        // Ignore self (we are peers for others)
        if (seed == this->id) {
            continue;
        }

        // Log
        this->clientLog << "[DOWNLOAD CLIENT " << this->id << "] querying seed " << seed << '\n' << std::flush;

        // Send request for segment
        int request = clientRequestIndex(REQUEST_SEGMENT);

        MPI_Send(&request, 1, MPI_INT, seed, UPLOAD_TAG, MPI_COMM_WORLD);

        int fileSize = file->name.size() + 1;
        char fileName[fileSize];
        strcpy(fileName, file->name.c_str());

        // Send file size, name & segment index
        MPI_Send(&fileSize, 1, MPI_INT, seed, UPLOAD_TAG, MPI_COMM_WORLD);
        MPI_Send(fileName, fileSize, MPI_CHAR, seed, UPLOAD_TAG, MPI_COMM_WORLD);
        MPI_Send(&segmentIndex, 1, MPI_INT, seed, UPLOAD_TAG, MPI_COMM_WORLD);

        // Receive reply
        int reply;
        MPI_Status status;

        MPI_Recv(&reply, 1, MPI_INT, seed, DOWNLOAD_TAG, MPI_COMM_WORLD, &status);

        // Parse reply
        ClientRequest clientRequest = indexToClientRequest(reply);

        // Log
        this->clientLog << "[DOWNLOAD CLIENT " << this->id << "] received reply " << clientRequest << " from seed " << seed << '\n' << std::flush;

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
}

void clients::Client::requestSeeds() {
    for (File *file: this->desiredFiles) {
        this->clientLog << "[DOWNLOAD CLIENT " << this->id << "] requesting seeds for " << file->name << '\n' << std::flush;
        file->seeds = getFileSeeds(file);
    }
}

int *clients::Client::getFileSeeds(File *file) {
    MPI_Status status;

    // Send request for seeds
    int request = trackerRequestIndex(REQUEST_SEEDS);

    MPI_Send(&request, 1, MPI_INT, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD);

    // Send file size and name
    int fileSize = file->name.size() + 1;
    char fileName[fileSize];

    strcpy(fileName, file->name.c_str());

    MPI_Send(&fileSize, 1, MPI_INT, TRACKER_RANK, SEED_REQUEST_TAG, MPI_COMM_WORLD);
    MPI_Send(fileName, fileSize, MPI_CHAR, TRACKER_RANK, SEED_REQUEST_TAG, MPI_COMM_WORLD);

    // Receive segment count
    int segmentCount = 0;

    MPI_Recv(&segmentCount, 1, MPI_INT, TRACKER_RANK, SEED_REQUEST_TAG, MPI_COMM_WORLD, &status);

    if (file->segmentCount == 0) {
        file->segmentCount = segmentCount;
        file->segmentsLacked = segmentCount;
    }

    // Log
    this->clientLog << "[DOWNLOAD CLIENT " << this->id << "] received " << segmentCount << " segments for " << file->name << '\n' << std::flush;

    // Receive seeds
    int seedCount;

    MPI_Recv(&seedCount, 1, MPI_INT, TRACKER_RANK, SEED_REQUEST_TAG, MPI_COMM_WORLD, &status);

    file->seedCount = seedCount;

    int *seeds = new int[seedCount];

    MPI_Recv(seeds, seedCount, MPI_INT, TRACKER_RANK, SEED_REQUEST_TAG, MPI_COMM_WORLD, &status);

    return seeds;
}

int clients::Client::nextDesiredSegment(File *file) {
    for (int segmentIndex = 0; segmentIndex < file->segmentCount; segmentIndex++) {
        if (file->segments.find(segmentIndex) == file->segments.end()) {
            return segmentIndex;
        }
    }
    return 0;
}
