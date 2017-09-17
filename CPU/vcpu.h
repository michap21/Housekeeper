#pragma once

#include <string.h>
#include <unistd.h>

#include "log.h"
#include "printf.h"
#include "virt_domain.h"

namespace cloud {

#define USAGE_THRESHOLD 50.0

class VCPU {
public:
  explicit VCPU(virConnectPtr conn) : conn_(conn) {
    maxcpus_ = virNodeGetCPUMap(conn_, NULL, NULL, 0);
    maplens_ = VIR_CPU_MAPLEN(maxcpus_);
  }

  ~VCPU() {
    virDomainStatsRecordListFree(records_);
    delete[] cpu_usage_;
  }

  void mapping(DomainStats *dom_stats, size_t num) {
    unsigned char maxcpus_mask = 0x1;
    unsigned char temp = 0x1;

    for (size_t i = 0; i < maxcpus_; i++) {
      temp <<= 0x1;
      maxcpus_mask ^= temp;
    }

    for (size_t i = 0; i < num; ++i) {
      unsigned char map = 0x1;
      for (size_t j = 0; j < dom_stats[i].vcpus_num; ++j) {
        // LOG(INFO) << string::Sprintf(" - CPUmap: 0x%x", map);
        virDomainPinVcpu(dom_stats[i].domain, j, &map, maplens_);
        map <<= 0x1;
        map %= maxcpus_mask; // equivalent to map % # of pCPUs
      }
    }
  }

  void pinning(DomainStats *stats, size_t num) {
    char do_nothing = 1;

    int freest;
    int busiest;

    double freest_usage = 100.0;
    double busiest_usage = 0.0;

    // Do not pin anything if all pCpus are above the threshold,
    // no room available to change pinnings
    for (size_t i = 0; i < maxcpus_; i++) {
      do_nothing &= (cpu_usage_[i] < USAGE_THRESHOLD);
      if (cpu_usage_[i] > busiest_usage) {
        busiest_usage = cpu_usage_[i];
        busiest = i;
      }
      if (cpu_usage_[i] < freest_usage) {
        freest_usage = cpu_usage_[i];
        freest = i;
      }
    }

    if (do_nothing) {
      LOG(INFO) << string::Sprintf("Cannot or should not change pinnings");
      LOG(INFO) << string::Sprintf("Busiest CPU: %d - Freest CPU: %d", busiest, freest);
      return;
    }

    LOG(INFO) << string::Sprintf("Busiest CPU: %d - Freest CPU: %d", busiest, freest);
    LOG(INFO) << string::Sprintf("Busiest CPU above usage threshold of %f%%", USAGE_THRESHOLD);
    LOG(INFO) << string::Sprintf("Changing pinnings...");

    virVcpuInfoPtr cpuinfo;
    unsigned char *cpumaps;
    unsigned char freest_map = 0x1 << freest;
    unsigned char busiest_map = 0x1 << busiest;

    // To do so iterate over all domains, over all vcpus and change
    // the busiest CPU vcpus for the freest ones and viceversa
    for (size_t i = 0; i < num; i++) {
      cpuinfo = (virVcpuInfoPtr)calloc(stats[i].vcpus_num, sizeof(virVcpuInfo));
      cpumaps = (unsigned char *)calloc(stats[i].vcpus_num, maplens_);
      virDomainGetVcpus(stats[i].domain, cpuinfo, stats[i].vcpus_num, cpumaps,
                        maplens_);
      for (size_t j = 0; j < stats[i].vcpus_num; j++) {
        if (cpuinfo[j].cpu == busiest) {
          LOG(INFO) << string::Sprintf("%s vCPU %ld is one of the busiest\n",
                 virDomainGetName(stats[i].domain), j);
          virDomainPinVcpu(stats[i].domain, j, &freest_map, maplens_);
        } else if (cpuinfo[j].cpu == freest) {
          LOG(INFO) << string::Sprintf("%s vCPU %ld is one of the freest\n",
                 virDomainGetName(stats[i].domain), j);
          virDomainPinVcpu(stats[i].domain, j, &busiest_map, maplens_);
        }
      }
      free(cpuinfo);
      free(cpumaps);
    }
  }

  DomainStats *stats(Domains list) {
    size_t stats = VIR_DOMAIN_STATS_VCPU;

    CHECK_GT(virDomainListGetStats(list.domains, stats, &records_, 0), 0)
        << "Could not get domains stats";

    DomainStats *domain_stats = new DomainStats[list.num];

    virDomainStatsRecordPtr *next;
    size_t i = 0;
    for (next = records_; *next; next++, i++) {
      domain_stats[i] = stats_impl(*next);
    }
    return domain_stats;
  }

  void usage(DomainStats *prev, DomainStats *curt, size_t num, size_t secs) {
    domain_dump(prev, curt, num, secs);
    v2pmap_dump(curt, num);
  }

private:
  void domain_dump(DomainStats *prev, DomainStats *curt, size_t num,
                   size_t secs) {
    double avg_usage;
    // i: represents domain number
    for (size_t i = 0; i < num; i++) {
      curt[i].usage = new double[curt[i].vcpus_num];
      avg_usage = 0.0;
      LOG(INFO) << "Domain: " << virDomainGetName(curt[i].domain);
      // j: represents vcpu number
      for (size_t j = 0; j < curt[i].vcpus_num; j++) {
        double diff = curt[i].vcpus[j] - prev[i].vcpus[j];
        double period = (double)secs * 1000000000;
        curt[i].usage[j] = 100 * (diff / period);
        LOG(INFO) << " - vCPU " << j << " usage: " << curt[i].usage[j] << "%";
        avg_usage += curt[i].usage[j];
      }
      curt[i].avg_usage = avg_usage / curt[i].vcpus_num;
      LOG(INFO) << " - Average vCPU usage: " << curt[i].avg_usage << "%";
    }
  }

  void v2pmap_dump(DomainStats *stats, size_t num) {
    virVcpuInfoPtr cpuinfo;
    unsigned char *cpumaps;

    cpu_usage_ = new double[maxcpus_];
    vcpus_per_cpu_ = new int[maxcpus_];

    memset(cpu_usage_, 0, sizeof(double) * maxcpus_);
    memset(vcpus_per_cpu_, 0, sizeof(int) * maxcpus_);

    for (size_t i = 0; i < num; i++) {
      LOG(INFO) << "Domain: " << virDomainGetName(stats[i].domain);
      cpuinfo = new virVcpuInfo[stats[i].vcpus_num];
      maplens_ = VIR_CPU_MAPLEN(maxcpus_);
      cpumaps = (unsigned char *)calloc(stats[i].vcpus_num, maplens_);
      virDomainGetVcpus(stats[i].domain, cpuinfo, stats[i].vcpus_num, cpumaps,
                        maplens_);
      for (size_t j = 0; j < stats[i].vcpus_num; j++) {
        cpu_usage_[cpuinfo[j].cpu] += stats[i].usage[j];
        vcpus_per_cpu_[cpuinfo[j].cpu] += 1;

        std::cout << string::Sprintf(" - CPUmap: 0x%x", cpumaps[j])
                  << string::Sprintf(" - CPU: %d", cpuinfo[j].cpu)
                  << string::Sprintf(" - vCPU %ld affinity: ", j);

        for (size_t m = 0; m < maxcpus_; m++) {
          printf("%c", VIR_CPU_USABLE(cpumaps, maplens_, j, m) ? 'y' : '-');
        }
        printf("\n");
      }
      delete[] cpuinfo;
      free(cpumaps);
    }

    LOG(INFO) << "--------------------------------";

    for (size_t i = 0; i < maxcpus_; i++) {
      if (vcpus_per_cpu_[i] != 0) {
        cpu_usage_[i] = cpu_usage_[i] / ((double)vcpus_per_cpu_[i]);
        LOG(INFO) << string::Sprintf(
            "CPU %ld - # vCPUs assigned %d - usage %f%%", i,
            vcpus_per_cpu_[i], cpu_usage_[i]);
      }
    }

    delete[] vcpus_per_cpu_;
  }

private:
  DomainStats stats_impl(virDomainStatsRecordPtr record) {
    DomainStats ret;
    size_t vcpus_num = 0;
    size_t vcpu_number, field_len;
    uint64_t *current_vcpus;
    const char *last_four;

    for (int i = 0; i < record->nparams; i++) {
      if (strcmp(record->params[i].field, "vcpu.current") == 0) {
        vcpus_num = record->params[i].value.i;
        current_vcpus = new uint64_t[vcpus_num];
        CHECK_NOTNULL(current_vcpus)
            << "Could not allocate memory for stats struct";
      }
      field_len = strlen(record->params[i].field);
      if (field_len >= 4) {
        last_four = &record->params[i].field[field_len - 4];
        if (strcmp(last_four, "time") == 0) {
          vcpu_number = atoi(&record->params[i].field[field_len - 6]);
          current_vcpus[vcpu_number] = record->params[i].value.ul;
        }
      }
    }
    ret.domain = record->dom;
    ret.vcpus_num = vcpus_num;
    ret.vcpus = current_vcpus;
    return ret;
  }

private:
  virConnectPtr conn_;
  virDomainStatsRecordPtr *records_;

  size_t maxcpus_;
  size_t maplens_;

  double *cpu_usage_;
  int *vcpus_per_cpu_;
};
} // namespace cloud
