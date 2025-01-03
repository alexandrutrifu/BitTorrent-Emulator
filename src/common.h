#pragma once

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

#define SEGMENT_SIZE 33

enum FileStatus {
    INCOMPLETE,
    COMPLETE
};

enum TrackerRequest {
    REQUEST_SEEDS,
    DOWNLOAD_COMPLETE,
    FINISHED
};

enum ClientRequest {
    REQUEST_SEGMENT,
    DECLINED,
    ACCEPTED,
    LOG_OFF
};

enum ClientStatus {
    SEED,
    DONE
};

enum Tag {
    COMMUNICATION_TAG,
    UPLOAD_TAG,
    DOWNLOAD_TAG,
    SEED_REQUEST_TAG
};

inline int trackerRequestIndex(TrackerRequest request) {
    switch (request) {
        case REQUEST_SEEDS:
            return 0;
        case DOWNLOAD_COMPLETE:
            return 1;
        case FINISHED:
            return 2;
        default:
            return -1;
    }
}

inline TrackerRequest indexToTrackerRequest(int index) {
    switch (index) {
        case 0:
            return TrackerRequest::REQUEST_SEEDS;
        case 1:
            return TrackerRequest::DOWNLOAD_COMPLETE;
        case 2:
            return TrackerRequest::FINISHED;
        default:
            return TrackerRequest::REQUEST_SEEDS;
    }
}

inline int clientRequestIndex(ClientRequest request) {
    switch (request) {
        case REQUEST_SEGMENT:
            return 0;
        case DECLINED:
            return 1;
        case ACCEPTED:
            return 2;
        case LOG_OFF:
            return 3;
        default:
            return -1;
    }
}

inline ClientRequest indexToClientRequest(int index) {
    switch (index) {
        case 0:
            return ClientRequest::REQUEST_SEGMENT;
        case 1:
            return ClientRequest::DECLINED;
        case 2:
            return ClientRequest::ACCEPTED;
        case 3:
            return ClientRequest::LOG_OFF;
        default:
            return ClientRequest::REQUEST_SEGMENT;
    }
}