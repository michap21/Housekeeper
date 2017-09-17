CC=g++
CFLAGS=-Wall -I. -pthread -lvirt -std=c++11

all: vcpu_scheduler memory_coordinator

vcpu_scheduler: CPU/vcpu_scheduler.cc
	$(CC) -o bin/vcpu_scheduler log.cc CPU/vcpu_scheduler.cc $(CFLAGS)

#memory_coordinator: Memory/memory_coordinator.cc
#	$(CC) -o bin/memory_coordinator Memory/memory_coordinator.cc $(CFLAGS)

clean:
	rm bin/vcpu_scheduler bin/memory_coordinator
