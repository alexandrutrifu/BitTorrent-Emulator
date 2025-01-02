#include "client.h"

void clients::Client::upload() {
    while (true) {
        // Receive client request
        MPI_Status status;
        int requestType;

        MPI_Recv(&requestType, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        ClientRequest request = indexToClientRequest(requestType);
        int clientIndex = status.MPI_SOURCE;

        if (request == LOG_OFF) {
            // Debug
            this->clientLog << "[CLIENT " << this->id << "] received LOG_OFF signal\n";
            break;
        }

        if (request != REQUEST_SEGMENT) {
            continue;
        }

        // Receive desired file name & segment index
        char fileName[255];
        int segmentIndex;

        MPI_Recv(fileName, 255, MPI_CHAR, clientIndex, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Recv(&segmentIndex, 1, MPI_INT, clientIndex, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

        // Find file
        for (File *file: this->ownedFiles) {
            if (file->name == fileName) {
                // If segment is unavailable, decline request
                if (file->segments.find(segmentIndex) == file->segments.end()) {
                    int reply = clientRequestIndex(DECLINED);

                    MPI_Send(&reply, 1, MPI_INT, clientIndex, 0, MPI_COMM_WORLD);

                    break;
                }

                // Otherwise, accept request
                int reply = clientRequestIndex(ACCEPTED);

                MPI_Send(&reply, 1, MPI_INT, clientIndex, 0, MPI_COMM_WORLD);

                // Send segment
                MPI_Send(file->segments[segmentIndex].c_str(), SEGMENT_SIZE, MPI_CHAR, clientIndex, 0, MPI_COMM_WORLD);

                break;
            }
        }
    }
}