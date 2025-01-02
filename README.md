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
  - `UPDATE_SWARM`

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

Odată cu inițializarea procesului, structurile clientului sunt populate printr-u apel al funcției `parseInput()`.

### Download Thread

### Upload Thread

## [3] Tracker Workflow

### Clasa `Tracker`

### Workflow
