#include "vcpu.h"
#include "log.h"
#include "pcpu.h"
#include "virt_domain.h"

using namespace cloud;

int main(int argc, const char **argv) {
  log::initializeLogging(argc, argv);

  CHECK_EQ(argc, 2) << "Please provide one argument time internal (sec): "
                       "./bin/vcpu_scheduler 12";

  size_t secs = atoi(argv[1]);

  auto conn = LocalConnect();
  PCPU pcpu(conn);
  VCPU vcpu(conn);

  size_t previous_dom_num = 0;
  DomainStats *previous_dom_stats;
  DomainStats *current_dom_stats;

  Domains list;
  while ((list = GetActiveDomains(conn)).num > 0) {
    pcpu.usage(secs);
    current_dom_stats = vcpu.stats(list);

    if (previous_dom_num == list.num) {
      // Pinning vCpus to pCpus
      vcpu.usage(previous_dom_stats, current_dom_stats, list.num, secs);
    } else {
      // Do not calculate usage or change pinning, we don't have stats yet
      vcpu.mapping(current_dom_stats, list.num);

      // previous domain stats
      previous_dom_stats = new DomainStats[list.num];
    }

    memcpy(previous_dom_stats, current_dom_stats,
           list.num * sizeof(DomainStats));

    // previous domain nums
    previous_dom_num = list.num;

    // free current domain stats for update
    delete[] current_dom_stats;

    // time interval
    sleep(secs);
  }

  return 0;
}