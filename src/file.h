#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

class File {
    public:
        File() = default;
        ~File() = default;

        string name;
        int segmentCount;
        unordered_map<int, string> segments;    // (segmentIndex, segmentHash)
};