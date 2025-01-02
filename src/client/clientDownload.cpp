#include "client.h"

void clients::Client::download() {
    // Debug
    this->clientLog << "[CLIENT " << this->id << "] starting download\n";

    // Request seeds for desired files
    this->requestSeeds();

    int pendingDownloads = this->getDesiredFiles().size();
    int fileIndex = 0;
    int segmentsReceived = 0;
    File *currentDownload = this->getDesiredFiles()[0];

    while (pendingDownloads) {
        // Request segment from seeds
        int segmentIndex = this->nextDesiredSegment(currentDownload);

        // Debug
        this->clientLog << "[CLIENT " << this->id << "] requesting segment " << segmentIndex << " from " << currentDownload->name << '\n';

        int ret = this->querySwarm(currentDownload, segmentIndex);

        if (ret == -1) {    // Error
            cerr << "Error querying swarm\n";
            return;
        }

        // Debug
        this->clientLog << "[CLIENT " << this->id << "] needs " << currentDownload->segmentsLacked << " more segments from " << currentDownload->name << '\n';

        // Move to next segment/file
        if (currentDownload->segmentsLacked == 0) {
            pendingDownloads--;

            this->sendDownloadCompleteSignal();
            
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

        // Debug
        this->clientLog << "[CLIENT " << this->id << "] querying seed " << seed << '\n';

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

        // Debug
        this->clientLog << "[CLIENT " << this->id << "] received reply " << clientRequest << " from seed " << seed << '\n';

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