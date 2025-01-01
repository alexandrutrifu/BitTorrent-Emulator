#include "client.h"

int client::Client::parseInput() {
    string inputFileName = "in" + to_string(id) + ".txt";
    
    ifstream inputFile(inputFileName);
    if (!inputFile.is_open()) {
        cerr << "File opening error\n";
        return -1;
    }

    // Parse file contents
    inputFile >> this->ownedFileCount;

    for (int index = 0; index < this->ownedFileCount; index++) {
        File *file = new File();

        inputFile >> file->name;
        inputFile >> file->segmentCount;

        for (int segmentIndex = 0; segmentIndex < file->segmentCount; segmentIndex) {
            inputFile >> file->segments[segmentIndex];
        }

        // Add file to client structure
        this->addOwnedFile(file);
    }

    // Parse desired files
    inputFile >> this->desiredFileCount;

    for (int index = 0; index < this->desiredFileCount; index++) {
        string desiredFileName;

        inputFile >> desiredFileName;

        // Add to desired files
        this->addDesiredFile(desiredFileName);
    }

    // Close input file
    inputFile.close();

    return 0;
}