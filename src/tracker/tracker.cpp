#include "tracker.h"

void trackers::Tracker::awaitClientInput() {
    MPI_Status status;
    char fileName[255];
    int segmentCount;

    for (int index = 1; index <= this->clientCount; index++) {
        MPI_Recv(fileName, 255, MPI_CHAR, index, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Recv(&segmentCount, 1, MPI_INT, index, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        File *file = new File(fileName);

        char segments[segmentCount][255];

        // Receive file segments
        for (int segmentIndex = 0; segmentIndex < segmentCount; segmentIndex++) {
            MPI_Recv(segments[segmentIndex], 255, MPI_CHAR, index, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            file->segments.insert({segmentIndex, segments[segmentIndex]});
        }

        // Check if file is in tracker records
        if (this->swarms.find(file) == this->swarms.end()) {
            // Add file to tracker records
            file->segmentCount = segmentCount;
            file->completeStatus();

            unordered_set<int> clients;
            clients.insert(index);

            this->swarms.insert({file, clients});

            continue;
        }

        // If file was already in records, update swarm
        this->swarms[file].insert(index);
    }

    // After every client has sent their input, broadcast ACK signal
    MPI_Bcast(this->ACK, 3, MPI_CHAR, 0, MPI_COMM_WORLD);
}