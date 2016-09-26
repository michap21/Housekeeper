#pragma once

#include <memory>
#include <unordered_map>
#include <libvirt/libvirt.h>
#include "utils.h"
#include "vcpu_stats.h"
#include "pcpu_stats.h"

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
            p_cpu_ptr_ = new PCpuInfo(v_conn_ptr_, std::move(new virNodeInfo));
            // did not query domains in here
            v_domains_ = nullptr;                
            LOG(INFO) << "Connect to hybervisor successful.\n";
        }
        ~CpuScheduler() {
            freeHostResource();
            freeDomainsResource();
            CHECK_NE(virConnectClose(v_conn_ptr_), -1)
                << "Close hybervisor connection failed.\n";
        }

        virConnectPtr  getVirConnectPtr() { return v_conn_ptr_; }
        virDomainPtr*  getVirDomainPtr () { return v_domains_; }

        size_t getDomainsNum()   { return v_domains_num_; }

        void run(size_t time_intervals = 10);

        private:
        inline void freeHostResource();
        inline void freeDomainsResource();
        /// connect to hybervisor
        inline virConnectPtr connect2Hybervisor(const char* name);
        /// get all active running virtual machine
        inline void getAllActiveRunningVMs(unsigned int flags =
            VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING);

        size_t v_domains_num_;
        virDomainPtr*  v_domains_;
        virConnectPtr  v_conn_ptr_;
        PCpuInfoPtr    p_cpu_ptr_;

        /// VCpu info
        std::unordered_map<virDomainPtr, CpuInfoPtr> vec_vcpu_info_;
    };

    typedef std::unique_ptr<CpuScheduler> CpuSchedulerPtr;
}
