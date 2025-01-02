#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "client/client.h"
#include "tracker/tracker.h"
#include "common.h"

using namespace clients;
using namespace trackers;

void *download_thread_func(void *arg)
{
    Client *client = (Client *) arg;

    if (client->getDesiredFiles().empty()) {
        // Send finished signal to tracker
        int finished = trackerRequestIndex(TrackerRequest::FINISHED);

        MPI_Send(&finished, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

        // Await confirmation
        char reply[4];
        MPI_Status status;

        MPI_Recv(reply, 4, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD, &status);

        return NULL;
    }

    client->download();

    return NULL;
}

void *upload_thread_func(void *arg)
{
    Client *client = (Client *) arg;

    client->upload();

    return NULL;
}

void tracker(int numtasks, int rank) {
    // Create new tracker instance
    Tracker *tracker = new Tracker(numtasks - 1);

    // Open log file
    tracker->trackerLog.open("tracker.log");
    if (!tracker->trackerLog.is_open()) {
        cout << "Error opening log file\n";
        exit(-1);
    }

    // Wait for clients to send their lists of files
    tracker->awaitClientInput();

    // Handle requests
    tracker->handleRequests();

    // Debug
    tracker->trackerLog << "[TRACKER] all clients are done\n";
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;

    MPI_Status mpiStatus;
    void *status;
    int r;

    // Create new client instance
    Client *client = new Client(rank);

    // Open log file
    client->clientLog.open("download.log");
    if (!client->clientLog.is_open()) {
        cout << "Error opening log file\n";
        exit(-1);
    }

    // Start parsing input file
    client->parseInput();

    // Send input to tracker
    client->sendInputToTracker();

    // Await tracker confirmation
    char reply[4];

    MPI_Bcast(reply, 4, MPI_CHAR, TRACKER_RANK, MPI_COMM_WORLD);

    client->clientLog << "[CLIENT " << rank << "] received " << reply << " from TRACKER.\n";

    r = pthread_create(&download_thread, NULL, download_thread_func, (void *) client);
    if (r) {
        printf("Eroare la crearea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_create(&upload_thread, NULL, upload_thread_func, (void *) client);
    if (r) {
        printf("Eroare la crearea thread-ului de upload\n");
        exit(-1);
    }

    r = pthread_join(download_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de download\n");
        exit(-1);
    }

    r = pthread_join(upload_thread, &status);
    if (r) {
        printf("Eroare la asteptarea thread-ului de upload\n");
        exit(-1);
    }
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
 
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        fprintf(stderr, "MPI nu are suport pentru multi-threading\n");
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
