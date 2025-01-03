#include "client.h"

void clients::Client::upload() {
    while (true) {
        // Receive client request
        MPI_Status status;
        int requestType;

        MPI_Recv(&requestType, 1, MPI_INT, MPI_ANY_SOURCE, UPLOAD_TAG, MPI_COMM_WORLD, &status);

        ClientRequest request = indexToClientRequest(requestType);
        int clientIndex = status.MPI_SOURCE;

        if (request == LOG_OFF && clientIndex == TRACKER_RANK) {
            // Log
            this->clientLog << "[UPLOAD CLIENT " << this->id << "] received LOG_OFF signal\n" << std::flush;
            break;
        }

        if (request != REQUEST_SEGMENT) {
            continue;
        }

        // Log
        this->clientLog << "[UPLOAD CLIENT " << this->id << "] received REQUEST_SEGMENT from " << clientIndex << '\n' << std::flush;

        // Receive desired file size, name & segment index
        int fileSize;
        int segmentIndex;

        MPI_Recv(&fileSize, 1, MPI_INT, clientIndex, UPLOAD_TAG, MPI_COMM_WORLD, &status);

        char fileName[fileSize];

        MPI_Recv(fileName, fileSize, MPI_CHAR, clientIndex, UPLOAD_TAG, MPI_COMM_WORLD, &status);

        MPI_Recv(&segmentIndex, 1, MPI_INT, clientIndex, UPLOAD_TAG, MPI_COMM_WORLD, &status);

        // Find file
        for (File *file: this->ownedFiles) {
            if (file->name == fileName) {
                // If segment is unavailable, decline request
                if (file->segments.find(segmentIndex) == file->segments.end()) {
                    // Log
                    this->clientLog << "[UPLOAD CLIENT " << this->id << "] declined request for segment " << segmentIndex << " from " << clientIndex << '\n' << std::flush;

                    int reply = clientRequestIndex(DECLINED);

                    MPI_Send(&reply, 1, MPI_INT, clientIndex, DOWNLOAD_TAG, MPI_COMM_WORLD);

                    break;
                }

                // Otherwise, accept request
                // Log
                this->clientLog << "[UPLOAD CLIENT " << this->id << "] accepted request for segment " << segmentIndex << " from " << clientIndex << '\n' << std::flush;

                int reply = clientRequestIndex(ACCEPTED);

                MPI_Send(&reply, 1, MPI_INT, clientIndex, DOWNLOAD_TAG, MPI_COMM_WORLD);

                // Send segment
                MPI_Send(file->segments[segmentIndex].c_str(), SEGMENT_SIZE, MPI_CHAR, clientIndex, DOWNLOAD_TAG, MPI_COMM_WORLD);

                break;
            }
        }
    }
}