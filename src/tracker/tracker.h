#pragma once

#include <mpi.h>
#include <iostream>

#include <unordered_map>
#include <unordered_set>

#include "../file.h"
#include "../common.h"

using namespace std;

namespace trackers {
    class Tracker {
    private:
        void printSwarms();

    public:
        Tracker() = default;
        Tracker(int numClients) { clientCount = numClients; }
        ~Tracker() = default;

        void awaitClientInput();
        void handleRequests();
        void handleSeedRequest(int clientIndex);
        void handleDownloadComplete(int clientIndex);
        void handleFinishedClient(int clientIndex);
        void handleUpdateSwarm(int clientIndex, string fileName);

        void sendACK(int clientIndex);

        int clientCount;
        char ACK[4] = "ACK";

        unordered_map<int, ClientStatus> clientStates;
        unordered_map<File *, unordered_set<int> > swarms;   // (File, Set<clientIndex>)
        unordered_map<string, int> segmentCounts;            // (fileName, segmentCount)

        // Log files
        ofstream trackerLog;
    };
}