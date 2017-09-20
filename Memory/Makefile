CC=g++
CFLAGS=-Wall -I. -pthread -lvirt -std=c++11

all: memory_coordinator

memory_coordinator: memory_coordinator.cc
	$(CC) -o ../bin/memory_coordinator ../log.cc memory_coordinator.cc $(CFLAGS)

clean:
	../bin/memory_coordinator
