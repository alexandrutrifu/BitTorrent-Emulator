#include "tracker.h"

void trackers::Tracker::printSwarms() {
    for (auto& swarm: this->swarms) {
        this->trackerLog << "[TRACKER] " << swarm.first->name << " is owned by clients: ";

        for (auto& client: swarm.second) {
            this->trackerLog << client << " ";
        }

        this->trackerLog << '\n' << std::flush;

        // Print segments
        for (int segmentIndex = 0; segmentIndex < swarm.first->segmentCount; segmentIndex++) {
            this->trackerLog << "[TRACKER] " << swarm.first->segments[segmentIndex] << '\n';
        }

        this->trackerLog << std::flush;
    }
}

void trackers::Tracker::awaitClientInput()
{
    MPI_Status status;

    for (int index = 1; index <= this->clientCount; index++) {
        int fileCount;

        // Receive file count
        MPI_Recv(&fileCount, 1, MPI_INT, index, COMMUNICATION_TAG, MPI_COMM_WORLD, &status);

        // Receive files
        for (int fileIndex = 0; fileIndex < fileCount; fileIndex++) {
            // Receive file size, name and segment count
            // Receive file size and name
            int fileSize;

            MPI_Recv(&fileSize, 1, MPI_INT, index, COMMUNICATION_TAG, MPI_COMM_WORLD, &status);

            int segmentCount;
            char fileName[fileSize];

            MPI_Recv(fileName, fileSize, MPI_CHAR, index, COMMUNICATION_TAG, MPI_COMM_WORLD, &status);
            MPI_Recv(&segmentCount, 1, MPI_INT, index, COMMUNICATION_TAG, MPI_COMM_WORLD, &status);

            this->trackerLog << "[TRACKER] received " << fileName << " with " << segmentCount << " segments from CLIENT " << index << "\n" << std::flush;

            File *file = new File(fileName);

            this->segmentCounts[fileName] = segmentCount;

            char segments[segmentCount][SEGMENT_SIZE];

            // Receive file segments
            for (int segmentIndex = 0; segmentIndex < segmentCount; segmentIndex++) {
                MPI_Recv(segments[segmentIndex], SEGMENT_SIZE, MPI_CHAR, index, COMMUNICATION_TAG, MPI_COMM_WORLD, &status);

                file->segments[segmentIndex] = segments[segmentIndex];
            }

            // Check if file is in tracker records
            if (this->swarms.find(file) == this->swarms.end()) {
                // Add file to tracker records
                file->segmentCount = segmentCount;
                file->completeStatus();

                unordered_set<int> clients;
                clients.insert(index);

                this->clientStates[index] = SEED;

                this->swarms[file] = clients;

                continue;
            }

            // If file was already in records, update swarm
            this->swarms[file].insert(index);
            this->clientStates[index] = SEED;
        }
    }

    // After every client has sent their input, broadcast ACK signal
    MPI_Bcast(this->ACK, 4, MPI_CHAR, TRACKER_RANK, MPI_COMM_WORLD);
}

void trackers::Tracker::handleRequests() {
    int pendingClients = this->clientCount;

    // Answer client requests until all clients are done
    while (pendingClients) {
        MPI_Status status;
        int requestType;
        int clientIndex;

        unordered_set<int> swarm;

        // Parse request
        MPI_Recv(&requestType, 1, MPI_INT, MPI_ANY_SOURCE, COMMUNICATION_TAG, MPI_COMM_WORLD, &status);

        TrackerRequest request = indexToTrackerRequest(requestType);

        // Log
        this->trackerLog << "[TRACKER] received request " << request << " from CLIENT " << status.MPI_SOURCE << '\n' << std::flush;

        // Get client index
        clientIndex = status.MPI_SOURCE;

        // Handle request cases
        switch (request) {
            case REQUEST_SEEDS:
                handleSeedRequest(clientIndex);
            case DOWNLOAD_COMPLETE:
                handleDownloadComplete(clientIndex);
                break;
            case FINISHED:
                handleFinishedClient(clientIndex);
                pendingClients--;
                break;
        }
    }

    // Send LOG_OFF signal to all clients
    // Not through broadcast, so that clients can handle the signal inside upload() loop
    int logOff = clientRequestIndex(LOG_OFF);

    for (int index = 1; index <= this->clientCount; index++) {
        MPI_Send(&logOff, 1, MPI_INT, index, UPLOAD_TAG, MPI_COMM_WORLD);
    }

    // Log
    this->trackerLog << "[TRACKER] sent LOG_OFF signal to all clients\n" << std::flush;
}

void trackers::Tracker::handleSeedRequest(int clientIndex) {
    MPI_Status status;

    // Receive file size and name
    int fileSize;

    MPI_Recv(&fileSize, 1, MPI_INT, clientIndex, SEED_REQUEST_TAG, MPI_COMM_WORLD, &status);

    char fileName[fileSize];

    MPI_Recv(fileName, fileSize, MPI_CHAR, clientIndex, SEED_REQUEST_TAG, MPI_COMM_WORLD, &status);

    // Log
    this->trackerLog << "[TRACKER] handling REQUEST_SEEDS for " << fileName << " from CLIENT " << clientIndex << '\n' << std::flush;

    int segmentCount = this->segmentCounts[fileName];

    // Send file segment count
    MPI_Send(&segmentCount, 1, MPI_INT, clientIndex, SEED_REQUEST_TAG, MPI_COMM_WORLD);

    // Log
    this->trackerLog << "[TRACKER] sent segment count " << segmentCount << " to CLIENT " << clientIndex << '\n' << std::flush;

    // Send file seeds
    handleUpdateSwarm(clientIndex, fileName);
}

void trackers::Tracker::handleUpdateSwarm(int clientIndex, string fileName) {
    // Find file in swarms
    File *file= NULL;
    unordered_set<int> swarm;

    for (auto& entry: this->swarms) {
        if (entry.first->name == fileName) {
            file = entry.first;
            swarm = entry.second;
            break;
        }
    }

    // If client is not in swarm, add it
    if (swarm.find(clientIndex) == swarm.end()) {
        swarm.insert(clientIndex);
        this->swarms[file] = swarm;
    }

    int swarmSize = swarm.size();

    // Get (int *) array associated with set
    int *clientArray = new int[swarmSize];

    int index = 0;
    for (int client: swarm) {
        clientArray[index++] = client;
    }

    // Send swarm size
    MPI_Send(&swarmSize, 1, MPI_INT, clientIndex, SEED_REQUEST_TAG, MPI_COMM_WORLD);

    // Send seeds to client
    MPI_Send(clientArray, swarmSize, MPI_INT, clientIndex, SEED_REQUEST_TAG, MPI_COMM_WORLD);

    // Log
    this->trackerLog << "[TRACKER] updated swarm for " << fileName << " with " << swarmSize << " clients at the request of CLIENT " << clientIndex << '\n' << std::flush;
}

void trackers::Tracker::handleDownloadComplete(int clientIndex) {
    this->clientStates[clientIndex] = SEED;
    sendACK(clientIndex);
}

void trackers::Tracker::handleFinishedClient(int clientIndex) {
    this->clientStates[clientIndex] = DONE;
    sendACK(clientIndex);
}

void trackers::Tracker::sendACK(int clientIndex) {
    // Send ACK to client
    MPI_Send(this->ACK, 4, MPI_CHAR, clientIndex, COMMUNICATION_TAG, MPI_COMM_WORLD);
}