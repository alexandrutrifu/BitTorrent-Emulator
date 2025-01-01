build:
	mpicxx -std=c++17 -o tema2 tema2.cpp client/client.cpp tracker/tracker.cpp -pthread -Wall

run:
	mpirun -np 4 ./tema2

clean:
	rm -rf tema3
