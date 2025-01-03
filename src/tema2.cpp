#include <mpi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "client/client.h"
#include "tracker/tracker.h"

using namespace clients;
using namespace trackers;

void *download_thread_func(void *arg)
{
    Client *client = (Client *) arg;

    if (client->getDesiredFiles().empty()) {
        // Send finished signal to tracker
        int finished = trackerRequestIndex(TrackerRequest::FINISHED);

        MPI_Send(&finished, 1, MPI_INT, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD);    

        // Await confirmation
        char reply[4];
        MPI_Status status;
        int count;
        
        MPI_Recv(reply, 4, MPI_CHAR, TRACKER_RANK, COMMUNICATION_TAG, MPI_COMM_WORLD, &status);

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

    // Log
    tracker->trackerLog << "[TRACKER] all clients are done\n" << std::flush;

    // Close log file
    tracker->trackerLog.close();
}

void peer(int numtasks, int rank) {
    pthread_t download_thread;
    pthread_t upload_thread;

    void *status;
    int r;

    // Create new client instance
    Client *client = new Client(rank);

    // Open log file
    client->clientLog.open("client" + to_string(rank) + ".log");
    if (!client->clientLog.is_open()) {
        cout << "Error opening log file\n";
        exit(-1);
    }

    // Test log
    client->clientLog << "[CLIENT " << rank << "] opened log file\n" << std::flush;

    // Start parsing input file
    client->parseInput();

    // Send input to tracker
    client->sendInputToTracker();

    // Await tracker confirmation
    char reply[4];

    MPI_Bcast(reply, 4, MPI_CHAR, TRACKER_RANK, MPI_COMM_WORLD);

    client->clientLog << "[CLIENT " << rank << "] received " << reply << " from TRACKER.\n" << std::flush;

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

    // Close log file
    client->clientLog.close();
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
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_ARE_FATAL);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
