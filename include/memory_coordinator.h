#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <libvirt/libvirt.h>
#include "utils.h"
#include "vcpu_stats.h"
#include "pcpu_stats.h"

namespace virt {
    /**
     * Memory Coordinator is to connect to hybervisor and
     * balance all active running virtual machines
     */
     class MemoryCoordinator {
         public:
         DISABLE_COPY(MemoryCoordinator);

         MemoryCoordinator(const char* name = "qemu:///system") {
            v_conn_ptr_ = connect2Hybervisor(name);
            p_cpu_ptr_ = new PCpuInfo(v_conn_ptr_, std::move(new virNodeInfo));
            LOG(INFO) << "Connect to hybervisor successful.\n";
         }
         ~MemoryCoordinator() {}

         void run(size_t timeIntervals = 10);

         private:
         inline virConnectPtr connect2Hybervisor(const char* uri);
         virConnectPtr  v_conn_ptr_;
         PCpuInfoPtr    p_cpu_ptr_;
     };  // class MemoryCoordinator

     typedef std::shared_ptr<MemoryCoordinator> MemoryCoordinatorPtr;
}  // namespace virt