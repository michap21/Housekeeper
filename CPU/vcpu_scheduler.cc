#include "vcpu.h"
#include "log.h"
#include "pcpu.h"
#include "virt_domain.h"

using namespace cloud;

int main(int argc, const char **argv) {
  log::initializeLogging(argc, argv);

  CHECK_EQ(argc, 2) << "Please provide one argument time internal (sec): "
                       "./bin/vcpu_scheduler 12";

  auto secs = atoi(argv[1]);
  auto conn = LocalConnect();

  PCPU pcpu(conn);
  VCPU vcpu(conn);

  size_t prev_num = 0;

  DomainStats *prev;
  DomainStats *curt;

  Domains list;
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

  delete[] prev;

  return 0;
}