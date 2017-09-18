#pragma once

#include <string.h>
#include <unistd.h>

#include "domain.h"
#include "log.h"
#include "printf.h"

namespace cloud {

/**
 * Return A pointer to two DomainMemory:
 *  1. most wasted one
 *  2. most starve one
 */
DomainMemory *FindWasteStarveDomains(Domains list) {
  DomainMemory *ret = new DomainMemory[2];
  DomainMemory wasted;
  DomainMemory starve;

  wasted.size = 0;
  starve.size = 0;

  for (size_t i = 0; i < list.num; i++) {
    virDomainMemoryStatStruct memstats[VIR_DOMAIN_MEMORY_STAT_NR];
    size_t flags = VIR_DOMAIN_AFFECT_CURRENT;

    CHECK_GE(virDomainSetMemoryStatsPeriod(list.domains[i], 1, flags), 0)
        << "ERROR: Could not change balloon collecting period";
    CHECK_NE(-1, virDomainMemoryStats(list.domains[i], memstats,
                                      VIR_DOMAIN_MEMORY_STAT_NR, 0))
        << "ERROR: Could not collect memory stats for domain "
        << virDomainGetName(list.domains[i]);

    LOG(INFO) << string::Sprintf(
        "%s : %llu MB available", virDomainGetName(list.domains[i]),
        memstats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val / 1024);

    if (memstats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val > wasted.size) {
      wasted.domain = list.domains[i];
      wasted.size = memstats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val;
    }

    if (memstats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val < starve.size ||
        starve.size == 0) {
      starve.domain = list.domains[i];
      starve.size = memstats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val;
    }
  }

  LOG(INFO) << string::Sprintf(
      "%s is the most wasted domain - %ld MB available",
      virDomainGetName(wasted.domain), wasted.size / 1024);

  LOG(INFO) << string::Sprintf(
      "%s is the most starve domain - %ld MB available",
      virDomainGetName(starve.domain), starve.size / 1024);

  ret[0] = wasted;
  ret[1] = starve;

  return ret;
}
} // namespace cloud
