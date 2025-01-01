#pragma once

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

class File {
    private:
        enum FileStatus {
            INCOMPLETE,
            COMPLETE
        };

    public:
        File() = default;
        File(string name) { this->name = name; }
        ~File() = default;

        void completeStatus() { status = COMPLETE; }

        bool operator==(const File &file) const {
            return name == file.name;
        }
        bool operator!=(const File &file) const {
            return name != file.name;
        }

        string name;
        int segmentCount;
        int segmentsLacked = 0;
        FileStatus status = INCOMPLETE;

        unordered_map<int, string> segments;    // (segmentIndex, segmentHash)

        int seedCount = 0;
        int *seeds;
};