#include <iostream>
#include <unordered_map>
#include <vector>

#include "../file.h"

using namespace std;

namespace client {
    class Client {
    public:
        Client() = default;
        ~Client() = default;

        vector<File> ownedFiles;
        vector<string> desiredFiles;
    };
}