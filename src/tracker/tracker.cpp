#include "tracker.h"

void trackers::Tracker::printSwarms() {
    for (auto& swarm: this->swarms) {
        cout << "[TRACKER] " << swarm.first->name << " is owned by clients: ";

        for (auto& client: swarm.second) {
            cout << client << " ";
        }

        cout << '\n';

        // Print segments
        for (int segmentIndex = 0; segmentIndex < swarm.first->segmentCount; segmentIndex++) {
            cout << "[TRACKER] " << swarm.first->segments[segmentIndex] << '\n';
        }
    }
}

void trackers::Tracker::awaitClientInput()
{
    MPI_Status status;
    char fileName[255];
    int segmentCount;

    for (int index = 1; index <= this->clientCount; index++) {
        int fileCount;

        // Receive file count
        MPI_Recv(&fileCount, 1, MPI_INT, index, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        // Receive files
        for (int fileIndex = 0; fileIndex < fileCount; fileIndex++) {
            // Receive file name and segment count
            MPI_Recv(fileName, 255, MPI_CHAR, index, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            MPI_Recv(&segmentCount, 1, MPI_INT, index, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            cout << "[TRACKER] received " << fileName << " with " << segmentCount << " segments from CLIENT " << index << "\n";

            File *file = new File(fileName);

            char segments[segmentCount][SEGMENT_SIZE];

            // Receive file segments
            for (int segmentIndex = 0; segmentIndex < segmentCount; segmentIndex++) {
                MPI_Recv(segments[segmentIndex], SEGMENT_SIZE, MPI_CHAR, index, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

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

    // Print swarms
    // this->printSwarms();

    // After every client has sent their input, broadcast ACK signal
    MPI_Bcast(this->ACK, 4, MPI_CHAR, 0, MPI_COMM_WORLD);
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
        MPI_Recv(&requestType, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        ClientRequest request = indexToRequest(requestType);

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
            case UPDATE_SWARM:
                // Receive file name
                char fileName[255];

                MPI_Recv(fileName, 255, MPI_CHAR, clientIndex, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                handleUpdateSwarm(clientIndex, fileName);
                break;
        }
    }
}

void trackers::Tracker::handleSeedRequest(int clientIndex) {
    MPI_Status status;

    // Receive file name
    char fileName[255];

    MPI_Recv(fileName, 255, MPI_CHAR, clientIndex, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    // Send file segment count
    MPI_Send(&this->segmentCounts[fileName], 1, MPI_INT, clientIndex, 0, MPI_COMM_WORLD);

    // Send file seeds
    handleUpdateSwarm(clientIndex, fileName);
}

void trackers::Tracker::handleUpdateSwarm(int clientIndex, string fileName) {
    // Get client swarm for requested file
    File *file = new File(fileName);
    unordered_set<int> swarm = this->swarms[file];

    // Get (int *) array associated with set
    int *clientArray = new int[swarm.size()];

    int index = 0;
    for (int client: swarm) {
        clientArray[index++] = client;
    }

    // Send seeds to client
    MPI_Send(clientArray, swarm.size(), MPI_INT, clientIndex, 0, MPI_COMM_WORLD);
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
    MPI_Send(this->ACK, 4, MPI_CHAR, clientIndex, 0, MPI_COMM_WORLD);
}