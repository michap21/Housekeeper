#include "memory.h"
#include "domain.h"
#include "log.h"
#include "printf.h"

using namespace cloud;

int main(int argc, char **argv) {
  CHECK_EQ(argc, 2) << "ERROR: please provide one argument time interval (secs): "
                    << "for instance, ./bin/memory_corrdinator 12");

  auto secs = atoi(argv[1]);
  auto conn = LocalConnect();

  LOG(INFO) << string::Sprintf("- Memory Coordinator - Interval: %s seconds",
                               argv[1]);
  LOG(INFO) << string::Sprintf(
      "Connecting to Libvirt ... Connected Successfully!");

  Domains list;
  while ((list = GetActiveDomains(conn)).num > 0) {
    DomainMemory *domains;
    DomainMemory wasted;
    DomainMemory starve;

    domains = FindWasteStarveDomains(list);

    wasted = domains[0];
    starve = domains[1];

    if (starve.memory <= STARVE_THRESHOLD) {
      if (wasted.memory >= WASTED_THRESHOLD) {
        LOG(INFO) << string::Sprintf("Taking memory from wasted domain %s",
                                     virDomainGetName(wasted.domain));
        virDomainSetMemory(wasted.domain, wasted.memory - wasted.memory >> 1);

        LOG(INFO) << string::Sprintf("Adding memory to  starved domain %s",
                                     virDomainGetName(starve.domain));
        virDomainSetMemory(starve.domain, starve.memory + wasted.memory >> 1);
      } else {
        LOG(INFO) << string::Sprintf("Adding memory to  starved domain %s",
                                     virDomainGetName(starve.domain));
        virDomainSetMemory(starve.domain, starve.memory + WASTE_THRESHOLD);
      }
    } else if (wasted.memory >= WASTE_THRESHOLD) {
      LOG(INFO) << "Returning memory back to host";
      virDomainSetMemory(wasted.domain, wasted.memory - WASTE_THRESHOLD);
    }
    delete[] demains;
  }

  LOG(INFO) << "No active domains - closing... See you next time!";

  virConnectClose(conn);

  return 0;
}
