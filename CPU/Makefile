CC=g++
CFLAGS=-Wall -I. -pthread -lvirt -std=c++11

all: vcpu_scheduler

vcpu_scheduler: vcpu_scheduler.cc
	$(CC) -o vcpu_scheduler log.cc vcpu_scheduler.cc $(CFLAGS)

clean:
	rm vcpu_scheduler
