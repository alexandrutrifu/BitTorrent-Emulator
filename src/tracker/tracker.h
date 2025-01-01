#include <mpi.h>
#include <iostream>

#include <unordered_map>
#include <unordered_set>

#include "../file.h"

namespace trackers {
    class Tracker {
    private:
        enum ClientStatus {
            SEED,
            PEER,
            DONE
        };

    public:
        Tracker() = default;
        Tracker(int numClients) { clientCount = numClients; }
        ~Tracker() = default;

        void awaitClientInput();

        int clientCount;
        char *ACK = "ACK";

        std::unordered_map<int, ClientStatus> clientStates;
        std::unordered_map<File *, unordered_set<int>> swarms;   // (File, Set<clientIndex>)
    };
}