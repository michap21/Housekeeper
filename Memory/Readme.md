# Memory Coordinator

### Compile

```bash
cd CPU
make
```

### Run

```bash
./memory_coordinator 2
```

### Code Description

The algorithm used to calculate fairness is the following:

* On every coordination period, find the most starve and the most wasted domains.
* If the most starved domain is below the starvation threshold, we have to assign memory to it
  * 1. If wasted memory above the waste threshold, halve that domain's memory
    and assign the same amount to the starve domain.
  * 2. If there is no starve domain but there is a wasted domain, return memory from the
    wasted domain back to the host.
  * 3. if there is no starve domain nor wasted domains, don't do anything.
* This is repeated until there are no active domains

When we want to schedule and balance VMs memory, we can simply use the above functions according to the active domains. The source code could be as follows:

```c++
  while ((list = GetActiveDomains(conn)).num > 0) {
    DomainMemory *domains;
    DomainMemory wasted;
    DomainMemory starve;

    domains = FindWasteStarveDomains(list);

    wasted = domains[0];
    starve = domains[1];

    if (starve.size <= STARVE_THRESHOLD) {
      if (wasted.size >= WASTED_THRESHOLD) {
        LOG(INFO) << string::Sprintf("Taking memory from wasted domain %s",
                                     virDomainGetName(wasted.domain));
        virDomainSetMemory(wasted.domain, wasted.size - (wasted.size >> 1));

        LOG(INFO) << string::Sprintf("Adding memory to  starved domain %s",
                                     virDomainGetName(starve.domain));
        virDomainSetMemory(starve.domain, starve.size + (wasted.size >> 1));
      } else {
        LOG(INFO) << string::Sprintf("Adding memory to  starved domain %s",
                                     virDomainGetName(starve.domain));
        virDomainSetMemory(starve.domain, starve.size + WASTED_THRESHOLD);
      }
    } else if (wasted.size >= WASTED_THRESHOLD) {
      LOG(INFO) << "Returning memory back to host";
      virDomainSetMemory(wasted.domain, wasted.size - WASTED_THRESHOLD);
    }
    delete[] domains;
    sleep(secs);
  }
```

### Test

This binary `memory_coordinator` has been verified by `test cases`. And the log files already generated in this folder, like `memory_coordiantor1.log`. Please checkout them to find out more details.

- [memory_coordinator1.log](memory_coordinator1.log)
- [memory_coordinator2.log](memory_coordinator2.log)
- [memory_coordinator3.log](memory_coordinator3.log)
