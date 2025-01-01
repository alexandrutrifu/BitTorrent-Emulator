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
        enum ClientStatus {
            SEED,
            PEER,
            DONE
        };

        void printSwarms();

    public:
        Tracker() = default;
        Tracker(int numClients) { clientCount = numClients; }
        ~Tracker() = default;

        void awaitClientInput();

        int clientCount;
        char ACK[4] = "ACK";

        std::unordered_map<int, ClientStatus> clientStates;
        std::unordered_map<File *, unordered_set<int> > swarms;   // (File, Set<clientIndex>)
    };
}