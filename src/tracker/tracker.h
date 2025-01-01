#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "../file.h"

namespace tracker {
    class Tracker {
    private:
        enum ClientStatus {
            SEED,
            PEER,
            DONE
        };

    public:
        Tracker() = default;
        ~Tracker() = default;

        std::unordered_map<int, ClientStatus> clientStates;
        std::unordered_map<File, unordered_set<int>> swarms;   // (File, Set<clientIndex>)
    };
}