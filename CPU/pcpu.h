#pragma once

#include <string.h>
#include <unistd.h>

#include "log.h"
#include "virt_domain.h"

namespace cloud {

class PCPU {
public:
  explicit PCPU(virConnectPtr conn) : conn_(conn) { busy_time_ = SampleTime(); }

  void usage(size_t secs) {
    uint64_t new_time = SampleTime();
    double diff = new_time - busy_time_;
    busy_time_ = new_time;

    virNodeInfo info;
    virNodeGetInfo(conn_, &info);
    LOG(INFO) << "PCPU usage: "
              << 100 * (diff / ((double)secs * 1000000000)) / (double)info.cpus
              << "%";
  }

private:
  // Samples the global CPU time
  uint64_t SampleTime() {
    int nr_params = 0;
    int nr_cpus = VIR_NODE_CPU_STATS_ALL_CPUS;
    virNodeCPUStatsPtr params;
    uint64_t busy_time = 0;

    CHECK_EQ(virNodeGetCPUStats(conn_, nr_cpus, NULL, &nr_params, 0), 0);
    CHECK_NE(nr_params, 0);

    params = new virNodeCPUStats[nr_params];
    CHECK_NOTNULL(params) << "Could not allocate PCPU params";

    memset(params, 0, sizeof(virNodeCPUStats) * nr_params);
    CHECK_EQ(virNodeGetCPUStats(conn_, nr_cpus, params, &nr_params, 0), 0);

    for (int i = 0; i < nr_params; i++) {
      if (strcmp(params[i].field, VIR_NODE_CPU_STATS_USER) == 0 ||
          strcmp(params[i].field, VIR_NODE_CPU_STATS_KERNEL) == 0) {
        busy_time += params[i].value;
      }
    }

    delete[] params;

    return busy_time;
  }

private:
  virConnectPtr conn_ = nullptr;
  uint64_t busy_time_ = 0;
};
} // namespace PCPU
