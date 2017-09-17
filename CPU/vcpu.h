#pragma once

#include <string.h>
#include <unistd.h>

#include "log.h"
#include "virt_domain.h"

namespace cloud {
class VCPU {
public:
  explicit VCPU(virConnectPtr conn) : conn_(conn) {}

  void mapping(DomainStats *dom_stats, size_t num) {
    size_t nr_cpus = virNodeGetCPUMap(conn_, NULL, NULL, 0);
    size_t cpumaplen = VIR_CPU_MAPLEN(nr_cpus);
    unsigned char nr_cpus_mask = 0x1;
    unsigned char temp = 0x1;

    for (size_t i = 0; i < nr_cpus; i++) {
      temp <<= 0x1;
      nr_cpus_mask ^= temp;
    }

    for (size_t i = 0; i < num; ++i) {
      unsigned char map = 0x1;
      for (size_t j = 0; j < dom_stats[i].vcpus_num; ++j) {
        printf("  - CPUmap: 0x%x\n", map);
        virDomainPinVcpu(dom_stats[i].domain, j, &map, cpumaplen);
        map <<= 0x1;
        map %= nr_cpus_mask; // equivalent to map % # of pCPUs
      }
    }
  }

  void pinning() {
    void pinPcpus(double *usage, int nr_cpus, struct DomainStats *domain_stats,
                  int domains_count) {
      char do_nothing = 1;
      int freest;
      double freest_usage = 100.0;
      int busiest;
      double busiest_usage = 0.0;
      // Do not pin anything if all pCpus are above the threshold,
      // no room available to change pinnings
      for (int i = 0; i < nr_cpus; i++) {
        do_nothing &= (usage[i] < USAGE_THRESHOLD);
        if (usage[i] > busiest_usage) {
          busiest_usage = usage[i];
          busiest = i;
        }
        if (usage[i] < freest_usage) {
          freest_usage = usage[i];
          freest = i;
        }
      }
      if (do_nothing) {
        printf("Cannot or should not change pinnings\n");
        printf("Busiest CPU: %d - Freest CPU: %d\n", busiest, freest);
        return;
      }
      printf("Busiest CPU: %d - Freest CPU: %d\n", busiest, freest);
      printf("Busiest CPU above usage threshold of %f%%\n", USAGE_THRESHOLD);
      printf("Changing pinnings...\n");
      virVcpuInfoPtr cpuinfo;
      unsigned char *cpumaps;
      size_t cpumaplen;
      unsigned char freest_map = 0x1 << freest;
      unsigned char busiest_map = 0x1 << busiest;

      // To do so iterate over all domains, over all vcpus and change
      // the busiest CPU vcpus for the freest ones and viceversa
      for (int i = 0; i < domains_count; i++) {
        cpuinfo = calloc(domain_stats[i].vcpus_count, sizeof(virVcpuInfo));
        cpumaplen = VIR_CPU_MAPLEN(nr_cpus);
        cpumaps = calloc(domain_stats[i].vcpus_count, cpumaplen);
        check(virDomainGetVcpus(domain_stats[i].domain, cpuinfo,
                                domain_stats[i].vcpus_count, cpumaps,
                                cpumaplen) > 0,
              "Could not retrieve vCpus affinity info");
        for (int j = 0; j < domain_stats[i].vcpus_count; j++) {
          if (cpuinfo[j].cpu == busiest) {
            printf("%s vCPU %d is one of the busiest\n",
                   virDomainGetName(domain_stats[i].domain), j);
            virDomainPinVcpu(domain_stats[i].domain, j, &freest_map, cpumaplen);
          } else if (cpuinfo[j].cpu == freest) {
            printf("%s vCPU %d is one of the freest\n",
                   virDomainGetName(domain_stats[i].domain), j);
            virDomainPinVcpu(domain_stats[i].domain, j, &busiest_map,
                             cpumaplen);
          }
        }
        free(cpuinfo);
        free(cpumaps);
      }
    }

    void domain_dump(DomainStats * prev, DomainStats * curt, size_t num,
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
          LOG(INFO) << "  - vCPU " << j << " usage: " << curt[i].usage[j]
                    << "%";
          avg_usage += curt[i].usage[j];
        }
        curt[i].avg_usage = avg_usage / curt[i].vcpus_num;
        LOG(INFO) << "  - Average vCPU usage: " << curt[i].avg_usage << "%";
      }
    }

    void v2p_dump(DomainStats * stats, size_t num) {
      size_t maxcpus = virNodeGetCPUMap(conn_, NULL, NULL, 0);
      double *cpu_usage = new double[maxcpus];

      virVcpuInfoPtr cpuinfo;
      unsigned char *cpumaps;
      size_t cpumaplen;

      int *vcpus_per_cpu = new int[maxcpus];
      memset(vcpus_per_cpu, 0, sizeof(int) * maxcpus);

      for (size_t i = 0; i < num; i++) {
        LOG(INFO) << "Domain: " << virDomainGetName(stats[i].domain);
        cpuinfo = new virVcpuInfo[stats[i].vcpus_num];
        cpumaplen = VIR_CPU_MAPLEN(maxcpus);
        cpumaps = (unsigned char *)calloc(stats[i].vcpus_num, cpumaplen);
        virDomainGetVcpus(stats[i].domain, cpuinfo, stats[i].vcpus_num, cpumaps,
                          cpumaplen);
        for (size_t j = 0; j < stats[i].vcpus_num; j++) {
          cpu_usage[cpuinfo[j].cpu] += stats[i].usage[j];
          vcpus_per_cpu[cpuinfo[j].cpu] += 1;
          printf("  - CPUmap: 0x%x", cpumaps[j]);
          printf(" - CPU: %d", cpuinfo[j].cpu);
          printf(" - vCPU %ld affinity: ", j);
          for (size_t m = 0; m < maxcpus; m++) {
            printf("%c", VIR_CPU_USABLE(cpumaps, cpumaplen, j, m) ? 'y' : '-');
          }
          printf("\n");
        }
        delete[] cpuinfo;
        free(cpumaps);
      }

      LOG(INFO) << "--------------------------------";

      for (size_t i = 0; i < maxcpus; i++) {
        if (vcpus_per_cpu[i] != 0) {
          cpu_usage[i] = cpu_usage[i] / ((double)vcpus_per_cpu[i]);
          printf("CPU %ld - # vCPUs assigned %d - usage %f%%\n", i,
                 vcpus_per_cpu[i], cpu_usage[i]);
        }
      }

      delete[] vcpus_per_cpu;
      delete[] cpu_usage;
    }

    DomainStats *stats(Domains list) {
      size_t stats = VIR_DOMAIN_STATS_VCPU;
      virDomainStatsRecordPtr *records = nullptr;

      CHECK_GT(virDomainListGetStats(list.domains, stats, &records, 0), 0)
          << "Could not get domains stats";

      DomainStats *domain_stats = new DomainStats[list.num];

      virDomainStatsRecordPtr *next;
      size_t i = 0;
      for (next = records; *next; next++, i++) {
        domain_stats[i] = stats_impl(*next);
      }
      virDomainStatsRecordListFree(records);
      return domain_stats;
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
    virConnectPtr conn_ = nullptr;
  };
} // namespace cloud
