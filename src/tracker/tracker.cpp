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

                this->swarms[file] = clients;

                continue;
            }

            // If file was already in records, update swarm
            this->swarms[file].insert(index);
        }
    }

    // Print swarms
    // this->printSwarms();

    // After every client has sent their input, broadcast ACK signal
    MPI_Bcast(this->ACK, 4, MPI_CHAR, 0, MPI_COMM_WORLD);
}