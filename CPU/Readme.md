# CPU Scheduler

### Compile

```bash
cd CPU
make
```

### Run

```bash
./vcpu_scheduler 2
```

### Code Description

To "balance" VCPUs to PCPUs pinning, two abstractions are defined, that's `PCPU` and `VCPU` classes.

- `PCPU`: response for physical cpu's usage. It only include one public function `usage`;
- `VCPU`: response for all virtual cpus' usage. It should include more public functions like `pinning`, `mapping`, `usage` and `stats`.

To do so, it changes the vCPU affinity and pinning of pCPU to vCPU according to the following algorithm:

* On every scheduler period it calculates:
  - vCPU usage (%) for all domains and  Usage (%) for all pCPUs
  - Busiest pCPU and Freest pCPU (lowest usage)
* Once we have this information, the fairness algorithm is applied using `pinning`.
  - In order to minimize pin changes, they will only happen when the busiest pCPU
  usage is above the USAGE_THRESHOLD (50% by default)
  - If the busiest pCPU is **above** the USAGE_THRESHOLD
    - Find the vCPUs pinned to the freest pCPU and swap the pins of the
      busiest pCPU with the freest pCPU.
  - If the busiest pCPU is **below** the USAGE_THRESHOLD, don't do anything
  - If all pCPUs are **above** or **below** the USAGE_THRESHOLD, don't do anything
    - There is no room for action in this case

When we want to schedule and balance CPUs pinning, we can simply use the above functions according to the active domains. The source code could be as follows:

```c++
  while ((list = GetActiveDomains(conn)).num > 0) {
    pcpu.usage(secs); // dump physical cpu cores usage

    curt = vcpu.stats(list);

    if (prev_num == list.num) {
      vcpu.usage(prev, curt, list.num, secs);
      vcpu.pinning(curt, list.num);
    } else {
      vcpu.mapping(curt, list.num); // initialize vcpu2pcpu pinning
      prev = new DomainStats[list.num];
    }

    memcpy(prev, curt, list.num * sizeof(DomainStats));

    prev_num = list.num;

    delete[] curt;

    sleep(secs);
  }
```

### Test

This binary `vcpu_scheduler` has been verified by `test cases`. And the log files already generated in this folder, like `vcpu_scheduler1.log`. Please checkout them to find out more details.

- [vcpu_scheduler1.log](vcpu_scheduler1.log)
- [vcpu_scheduler2.log](vcpu_scheduler2.log)
- [vcpu_scheduler3.log](vcpu_scheduler3.log)
- [vcpu_scheduler4.log](vcpu_scheduler4.log)
- [vcpu_scheduler5.log](vcpu_scheduler5.log)
