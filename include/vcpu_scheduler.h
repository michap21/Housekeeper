#pragma once

#include <memory>
#include <vector>
#include <unordered_map>
#include <libvirt/libvirt.h>
#include "utils.h"
#include "cpu_stats.h"

namespace virt {
    /**
     * Cpu Scheduler class is to connect to hybervisor and
     * get all active running virtual machines
     */
    class CpuScheduler {
        public:
        DISABLE_COPY(CpuScheduler);
        CpuScheduler(const char* name = "qemu:///system") {
            v_conn_ptr_ = connect2Hybervisor(name);
            LOG(INFO) << "Connect to hybervisor successful";
        }
        ~CpuScheduler() {
            freeDomainsResource();
            CHECK_NE(virConnectClose(v_conn_ptr_), -1)
                << "Close hybervisor connection failed";
        }
        virConnectPtr getVirConnectPtr() { return v_conn_ptr_; }
        virDomainPtr*  getVirDomainPtr () { return v_domains_; }
        size_t getDomainsNum() { return v_domains_num_; }
        /// get all active running virtual machine
        void getAllActiveRunningVMs();

        private:
        inline void freeDomainsResource();
        /// connect to hybervisor
        inline virConnectPtr connect2Hybervisor(const char* name);

        virConnectPtr v_conn_ptr_;
        size_t v_domains_num_;
        virDomainPtr* v_domains_;
        /// VCpu info
        std::unordered_map<virDomainPtr, CpuInfoPtr> vec_vcpu_info_;
    };

    typedef std::shared_ptr<CpuScheduler> CpuSchedulerPtr;
}
