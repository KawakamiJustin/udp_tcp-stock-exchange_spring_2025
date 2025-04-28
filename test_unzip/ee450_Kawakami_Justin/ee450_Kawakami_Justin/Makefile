CC = g++
CFLAGS = -std=c++11

TARGETS = C M A Q P

all: $(TARGETS)

M: serverM.cpp
	$(CC) $(CFLAGS) -o serverM serverM.cpp

A: serverA.cpp
	$(CC) $(CFLAGS) -o serverA serverA.cpp

Q: serverQ.cpp
	$(CC) $(CFLAGS) -o serverQ serverQ.cpp

P: serverP.cpp
	$(CC) $(CFLAGS) -o serverP serverP.cpp

C: client.cpp
	$(CC) $(CFLAGS) -o client client.cpp

clean:
	rm -rf $(TARGETS)

.PHONY: all clean 