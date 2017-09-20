CC=g++
CFLAGS=-Wall -I. -pthread -lvirt -std=c++11

all: vcpu_scheduler

vcpu_scheduler: vcpu_scheduler.cc
	$(CC) -o ../bin/vcpu_scheduler ../log.cc vcpu_scheduler.cc $(CFLAGS)

clean:
	rm ../bin/vcpu_scheduler
