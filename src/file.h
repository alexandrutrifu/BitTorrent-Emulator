#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "common.h"

using namespace std;

class File {
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

        // Override hash
        struct Hash {
            size_t operator()(const File &file) const {
                return hash<string>{}(file.name);
            }
        };

        string name;
        int segmentCount;
        int segmentsLacked = 0;
        FileStatus status = INCOMPLETE;

        unordered_map<int, string> segments;    // (segmentIndex, segmentHash)

        int seedCount = 0;
        int *seeds;
};