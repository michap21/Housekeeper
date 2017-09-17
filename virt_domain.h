#pragma once

#include "log.h"

#include <libvirt/libvirt.h>

namespace cloud {

/* clang-format off */
struct Domains {
  size_t        num;        /* Number of domains in the *domains array */
  virDomainPtr *domains;    /* Pointer to array of Libvirt domains */
};

struct DomainStats {
  virDomainPtr  domain;     /* The pointer to domian */
  size_t        vcpus_num;  /* The number of virtual CPUS */
  uint64_t     *vcpus;      /* The pointer to all virtual CPUs in this domain */
  double       *usage;      /* The Pointer to usage of all virtual CPUs in this domain */
  double        avg_usage;  /* The average usage of virtual CPUs in this domain */
};
/* clang-format on */

/**
 * \brief	Initiates a connection to local qemu libvirt.
 *
 * \return	A pointer to that connection.
 */
inline virConnectPtr LocalConnect() {
  virConnectPtr conn = virConnectOpen("qemu:///system");
  CHECK_NOTNULL(conn);
  return conn;
}

/**
 * \brief	Query a list of domains with flags constraint.
 *
 * \param[in]	conn	A pointer to the local connection.
 * \param[in]	flags	flag constraint.
 */
inline Domains GetDomains(virConnectPtr conn, size_t flags) {
  virDomainPtr *domains;
  int num;

  num = virConnectListAllDomains(conn, &domains, flags);

  CHECK_GT(num, 0) << "Failed to list all domains";

  /* clang-format off */
  Domains list;
  list.num     = num;
  list.domains = domains;
  /* clang-format on */

  return list;
}

/* Query a list of active domains. */
inline Domains GetActiveDomains(virConnectPtr conn) {
  size_t flags =
      VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING;
  return GetDomains(conn, flags);
}

} // namespace cloud
