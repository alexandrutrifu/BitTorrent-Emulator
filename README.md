# Topic 2 - APD

## Structure

The implementation was modularized, focusing on the main objects we worked with:

- `file.h` - contains the definition of the `File` class[1]; it includes the main attributes necessary for file manipulation.
- `client/` - contains the definition of the `Client` class[2], along with methods associated with its workflow:
  - `clientDownload.cpp` - defines functions used in the **download loop** for files (`download()`, `querySwarm()`, etc.).
  - `clientUpload.cpp` - defines functions used in the **upload loop**, i.e., in the process of sending segments between clients.
  - `clientFileManagement.cpp` - defines **auxiliary functions**, such as parsing input, saving output, or sending signals to the tracker.
- `tracker/` - contains the definition of the `Tracker` class[3], along with methods associated with its workflow:
  - `tracker.cpp` - defines functions responsible for client communication, request parsing, etc.
- `common.h` - contains `enum` structures that model signals transmitted between clients/tracker and tags used for MPI communication.

## Signal Modeling

The following `enum` types were used:

- `ClientRequest` (signals used in client-client communication + for logging off):
  - `REQUEST_SEGMENT` - sent to a peer from which we want to retrieve a segment.
  - `DECLINED` - response from a peer when it does not possess the requested segment.
  - `ACCEPTED` - response from a peer before sending the requested segment.
  - `LOG_OFF` - signal sent by the tracker once all clients have downloaded the desired files.

- `TrackerRequest` (signals used in client-tracker communication):
  - `REQUEST_SEEDS` - signals a request for a list of seeds.
  - `DOWNLOAD_COMPLETE` - signals the completion of a file download.
  - `FINISHED` - signals the complete download of all desired files.

- `FileStatus` - COMPLETE/INCOMPLETE, based on the number of segments possessed by the client.

- `ClientStatus` - SEED/DONE, based on each client's status.

- `Tag` (tags for MPI communication):
  - `COMMUNICATION_TAG` - used in general messages between clients and the tracker (ACKs, LOG_OFF signals, input transmission).
  - `DOWNLOAD_TAG` - used in messages handled by the **download thread**.
  - `UPLOAD_TAG` - used in messages handled by the **upload thread**.
  - `SEED_REQUEST_TAG` - used in requests to update seed lists for files.

## [1] The `File` Class

Files were modeled using the following attributes:

- name
- status (COMPLETE if owned by a seed, or INCOMPLETE if owned by a peer missing certain segments).
- total number of segments
- number of segments the owning client needs
- *segments* - a hashmap associating each segment index with its corresponding hash
- a list of seeds that own the file

## [2] Client Workflow

### The `Client` Class

Each client is described by an ID (equivalent to the process rank) and two vectors containing the files it owns/wants.

At process initialization, the client's structures are populated via the `parseInput()` function. The information is then sent to the tracker (`sendInputToTracker()`), and clients wait for a confirmation message from it. Subsequently, each client initializes specific threads responsible for the *download* and *upload* loops.

### Download Thread

The file download loop is managed in the `download()` function.

Initially, seed lists for each desired file are requested (`requestSeeds()` & `getFileSeeds()`), using `COMMUNICATION_TAG` and `SEED_REQUEST_TAG` tags alternately. This request initializes, on the first call, the *segmentsLacked* field in the `File` structures (= *segmentCount*).

While desired files are not fully downloaded, missing segments are identified in the `nextDesiredSegment()` function and requested sequentially.

The querying mechanism (`querySwarm()`) selects a random seed from the file's swarm and sends a request to it. The request contains the file name, its size, and the index of the desired segment. The client then waits for a response from the chosen peer:

- For a `DECLINED` response, another seed is selected, and the query is repeated.
- For an `ACCEPTED` response, the file's segment map and missing segment count are updated, then the main loop resumes.

Once the desired segment is received, the process moves to the next one. Every 10 successfully received segments, the `requestSeeds()` function is called again to update the recorded swarms.

The complete download of a desired file generates a `DOWNLOAD_COMPLETE` signal sent to the tracker. Additionally, the received segments are saved in the corresponding output file.

The complete download of all desired files generates a `FINISHED` signal, followed by exiting the loop and closing the client's *download* thread.

### Upload Thread

The file upload loop is managed in the `upload()` function.

The thread responsible for this workflow continuously waits for requests from other clients. For each request, it first searches for the desired file, then checks if the peer possesses the requested segment.

- If yes, it sends an `ACCEPTED` message, followed by the segment hash.
- Otherwise, it sends a `DECLINED` message.

The process repeats until a `LOG_OFF` signal is received from the tracker. This signal triggers an exit from the loop and the closure of the client's *upload* thread.

## [3] Tracker Workflow

### The `Tracker` Class

The tracker is described by several structures that store:

- *clientStates* - the states of the clients (SEED/DONE).
- *swarms* - registered files along with their associated swarms.
- *segmentCounts* - the segment count of each registered file.

### Workflow

Before starting the loop that manages client requests (`handleRequests()`), the tracker waits for client input and populates its own structures (`awaitClientInput()`).

As long as there are clients that have not finished downloading, the tracker receives messages from them and processes them accordingly.

Most requests require a simple ACK response from the tracker. In particular, requests to update swarms are handled by sending the requested information.

Each `FINISHED` message decrements the *pendingClients* counter—once it reaches 0, the tracker can send a `LOG_OFF` message to all clients, stopping the upload threads and ending the program's execution.
