# Tema 2 - APD

## Structura

Implementarea a fost modularizată având în atenție principalele obiecte cu care am lucrat:

- `file.h` - conține definiția clasei `File`[1]; aceasta cuprinde principalele atribute necesare în manipularea fișierelor
- `client/` - conține definiția clasei `Client`[2], respectiv metodele asociate cu workflow-ul acestuia:
  - `clientDownload.cpp` - definește funcțiile apelate în cadrul **loop-ului de descărcare** a fișierelor (`download()`, `querySwarm()` etc.)
  - `clientUpload.cpp` - definește funcțiile apelate în cadrul **loop-ului de upload** al fișierelor, i.e. în procesul de trimitere a segmentelor între clienți
  - `clientFileManagement.cpp` - definește **funcții auxiliare**, precum cele de parsare a input-ului, salvare a output-ului, sau trimitere a semnalelor către tracker
- `tracker/` - conține definiția clasei `Tracker`[3], respectiv metodele asociate cu workflow-ul acestuia:
  - `tracker.cpp` - definește funcțiile responsabile de comunicarea cu clienții, parsarea request-urilor etc.
- `common.h` - conține structuri de tip `enum` ce modelează semnalele transmise între clienți/tracker, respectiv tag-urile folosite în comunicarea prin MPI

## Modelarea semnalelor

S-au folosit următoarele `enum`-uri:

- `ClientRequest` (semnale folosite în comunicarea client-client + pentru logging off)
  - `REQUEST_SEGMENT` - trimis către un peer de la care vrem să preluăm un segment
  - `DECLINED` - răspuns al unui peer atunci când nu deține segmentul cerut
  - `ACCEPTED` - răspuns oferit de peer înainte de a trimite segmentul cerut
  - `LOG_OFF` - semnal trimis de către tracker odată ce toți clienții au descărcat fișierele dorite

- `TrackerRequest` (semnale folosite în comunicarea client-tracker):
  - `REQUEST_SEEDS` - semnalează cererea unei liste de seed-uri
  - `DOWNLOAD_COMPLETE` - semnalează finalizarea descărcării unui fișier
  - `FINISHED` - semnalează descărcarea completă a tuturor fișierelor dorite

- `FileStatus` - COMPLETE/INCOMPLETE, conform numărului de segmente deținute de către client

- `ClientStatus` - SEED/DONE, conform statusului fiecărui client

- `Tag` (tag-uri pentru comunicarea prin MPI):
  - `COMMUNICATION_TAG` - folosit în mesajele generale dintre clienți și tracker (ACK-uri, semnale de LOG_OFF, transmitere de input)
  - `DOWNLOAD_TAG` - folosit în mesajele preluate de către thread-ul responsabil de **download**
  - `UPLOAD_TAG` - folosit în mesajele preluate de către thread-ul responsabil de **upload**
  - `SEED_REQUEST_TAG` - folosit în request-urile de actualizare a listelor de seed-uri pentru fișiere

## [1] Clasa `File`

Fișierele au fost modelate prin intermediul următoarelor atribute:

- nume
- status (COMPLETE - dacă se află în posesia unui seed, sau INCOMPLETE - dacă aparține unui peer căruia îi lipsesc anumite segmente)
- număr total de segmente
- număr de segmente de care clientul-posesor are nevoie
- *segments* - un hashmap ce asociază fiecărui index al unui segment hash-ul său corespunzător
- o lista cu seed-urile ce dețin fișierul

## [2] Client Workflow

### Clasa `Client`

Fiecare client este descris printr-un id (echivalent cu rangul procesului), respectiv doi vectori ce conțin fișierele deținute/dorite de către acesta.

Odată cu inițializarea procesului, structurile clientului sunt populate printr-u apel al funcției `parseInput()`. Informațiile sunt trimise, mai apoi, către tracker (`sendInputToTracker()`); clienții așteaptă un mesaj de confirmare din partea acestuia, urmând ca fiecare să-și inițializeze thread-urile specifice, responsabile de loop-urile *download* & *upload*.

### Download Thread

Loop-ul de descărcare a fișierelor este gestionat în cadrul funcției `download()`.

Inițial, sunt solicitate listele de seed-uri pentru fiecare fișier dorit (`requestSeeds()` & `getFileSeeds()`), folosindu-se alternativ tag-urile `COMMUNICATION_TAG`, respectiv `SEED_REQUEST_TAG`. O astfel de cerere inițializează, la primul apel, câmpul *segmentsLacked* al structurilor `File` (= *segmentCount*).

Cât timp fișierele dorite nu au fost descărcate complet, segmentele lipsă ale acestora sunt identificate în funcția `nextDesiredSegment()` și solicitate secvențial.

Mecanismul de querying (`querySwarm()`) selectează un seed random din swarm-ul fișierului target, trimițând un request către acesta. Cererea conține numele fișierului, dimensiunea acestuia, respectiv indexul segmentului dorit. Clientul așteaptă, ulterior, un răspuns din partea peer-ului ales:

- pentru un răspuns de tip `DECLINED`, se selectează un nou seed și se repetă interogarea
- pentru un răspuns de tip `ACCEPTED`, se actualizează map-ul de segmente al fișierului și numărul de segmente lipsă, apoi se revine la loop-ul principal

Odată ce am primit segmentul dorit, putem trece la următorul. La fiecare 10 segmente primite cu succes, se apelează din nou funcția `requestSeeds()`, pentru a actualiza swarm-urile înregistrate.

Descărcarea completă a unui fișier dorit generează un semnal de tip `DOWNLOAD_COMPLETE`, trimis către tracker. Totodată, segmentele primite sunt salvate în fișierul de output aferent.

Descărcarea completă a tuturor fișierelor dorite generează un semnal de tip `FINISHED`, urmat de ieșirea din loop și închiderea thread-ului de *download* al clientului.

### Upload Thread

Loop-ul de upload al fișierelor este gestionat în cadrul funcției `upload()`.

Thread-ul responsabil de acest workflow așteaptă în permanență request-uri din partea altor clienți. Pentru fiecare cerere, se caută mai întâi fișierul dorit, apoi se verifică dacă peer-ul deține segmentul solicitat.

- în caz afirmativ, se va trimite un mesaj de tip `ACCEPTED`, urmat de hash-ul segmentului
- altfel, se va trimite un mesaj de tip `DECLINED`

Procesul se repetă până la primirea unui semnal de tip `LOG_OFF` din partea tracker-ului. Acesta din urmă declanșează ieșirea din loop și închiderea thread-ului de *upload*.

## [3] Tracker Workflow

### Clasa `Tracker`

Tracker-ul este descris printr-o serie de structuri ce rețin următoarele:

- *clientStates* - stările clienților (SEED/DONE)
- *swarms* - fișierele înregistrate, alături de swarm-ul asociat fiecăruia
- *segmentCounts* - numărul de segmente al fiecărui fișier înregistrat

### Workflow

Înainte de pornirea loop-ului de gestionează cererile clienților (`handleRequests()`), tracker-ul așteaptă input-ul acestora și își populează structurile proprii (`awaitClientInput()`).

Cât timp există clienți ce nu și-au finalizat procesul de descărcare, tracker-ul primește mesaje din partea acestora, pe care le tratează corespunzător.

Majoritatea request-urilor necesită un simplu răspuns de tip ACK din partea tracker-ului. În particular, cererile de actualizare a swarm-urilor sunt gestionate prin trimiterea informațiilor solicitate.

Fiecare mesaj de tip `FINISHED` decrementează contorul *pendingClients* - odată ce acesta devine 0, tracker-ul poate trimite un mesaj de `LOG_OFF` către toți clienții, determinând oprirea thread-urilor de upload și încheiând execuția programului.
