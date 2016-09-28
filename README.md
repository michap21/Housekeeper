# CPU-Scheduler-and-Memory-Coordinator-in-KVM
## Build Project

```bash
mkdir build && cd build
cmake .. && make
```

## Run Program

```bash
cd build/bin
# run vcpu_scheduler
./vcpu_scheduler 12
# run memory_coordinator
./memory_coordinator 12
```

## Functionality

### Log Info

It provides glog-like's print, debug, fatal information and interface.

### Local Engine

Local Engine provides fundamental APIs for query KVM physical and virtual resource including
``` 
getPCpusInfo
getPMemsInfo 
getVCpusInfo
getVMemsInfo
```
### CPU Scheduler
1. Inherit from Local Engine, CPU Scheduler can query PCPUs and VCPUs information, for instance,
CPU Model, CPU numbers and CPU usage.
2. According to the CPUs stats info, CPU Scheduler can choose the "best" scheduling algorithm to 
balance computing resources.

### Memory Coordinator
1. Inherit from Local Engine, Memory Coordinator can query PMems and VMems information, for instance,
Used Memory, Unused Memory, Swap in/out, etc.
2. According to the Memory stats info, Memory Coordinato can choose the "best" scheduling algorithm to 
balance memory resources.
