#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

#define SEGMENT_SIZE 33

enum ClientRequest {
    REQUEST_SEEDS,
    DOWNLOAD_COMPLETE,
    FINISHED,
    UPDATE_SWARM
};

enum ClientStatus {
    SEED,
    DONE
};

int requestIndex(ClientRequest request) {
    switch (request) {
        case REQUEST_SEEDS:
            return 0;
        case DOWNLOAD_COMPLETE:
            return 1;
        case FINISHED:
            return 2;
        case UPDATE_SWARM:
            return 3;
        default:
            return -1;
    }
}

ClientRequest indexToRequest(int index) {
    switch (index) {
        case 0:
            return ClientRequest::REQUEST_SEEDS;
        case 1:
            return ClientRequest::DOWNLOAD_COMPLETE;
        case 2:
            return ClientRequest::FINISHED;
        case 3:
            return ClientRequest::UPDATE_SWARM;
        default:
            return ClientRequest::REQUEST_SEEDS;
    }
}