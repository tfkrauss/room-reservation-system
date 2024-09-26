CC=g++
CFLAGS=-std=c++11 -Wall
LDFLAGS=

all: serverS serverD serverU serverM client

serverS: serverS.o backendserver.o
	$(CC) $(CFLAGS) -o serverS serverS.o backendserver.o $(LDFLAGS)

serverD: serverD.o backendserver.o
	$(CC) $(CFLAGS) -o serverD serverD.o backendserver.o $(LDFLAGS)

serverU: serverU.o backendserver.o
	$(CC) $(CFLAGS) -o serverU serverU.o backendserver.o $(LDFLAGS)

serverM: serverM.o
	$(CC) $(CFLAGS) -o serverM serverM.o $(LDFLAGS)

client: client.o
	$(CC) $(CFLAGS) -o client client.o $(LDFLAGS)

serverS.o: serverS.cpp backendserver.h
	$(CC) $(CFLAGS) -c serverS.cpp

serverD.o: serverD.cpp backendserver.h
	$(CC) $(CFLAGS) -c serverD.cpp

serverU.o: serverU.cpp backendserver.h
	$(CC) $(CFLAGS) -c serverU.cpp

serverM.o: serverM.cpp serverM.h
	$(CC) $(CFLAGS) -c serverM.cpp

client.o: client.cpp client.h
	$(CC) $(CFLAGS) -c client.cpp

backendserver.o: backendserver.cpp backendserver.h
	$(CC) $(CFLAGS) -c backendserver.cpp

clean:
	rm -f *.o serverS serverD serverU serverM client