#pragma once

#include <mpi.h>
#include <iostream>

#include <unordered_map>
#include <unordered_set>

#include "../file.h"
#include "../common.h"

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

        std::unordered_map<int, ClientStatus> clientStates;
        std::unordered_map<File *, unordered_set<int> > swarms;   // (File, Set<clientIndex>)
        std::unordered_map<string, int> segmentCounts;            // (fileName, segmentCount)
    };
}