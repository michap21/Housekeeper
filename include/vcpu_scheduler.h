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
            v_node_info_ptr_ = new virNodeInfo();
            CHECK_EQ(virNodeGetInfo(v_conn_ptr_, v_node_info_ptr_), 0)
                << "Failed to Get Host Node Info.\n";
            LOG(INFO) << "Connect to hybervisor successful.\n";
        }
        ~CpuScheduler() {
            delete v_node_info_ptr_;
            freeDomainsResource();
            CHECK_NE(virConnectClose(v_conn_ptr_), -1)
                << "Close hybervisor connection failed.\n";
        }
        virConnectPtr  getVirConnectPtr() { return v_conn_ptr_; }
        virDomainPtr*  getVirDomainPtr () { return v_domains_; }

        size_t getDomainsNum()   { return v_domains_num_; }
        size_t getHostCpuNum()   { return v_node_info_ptr_->cpus;}
        size_t GetHostMemory()   { return v_node_info_ptr_->memory/1000;} /// MB
        size_t getHostFrequency(){ return v_node_info_ptr_->mhz;}
        char*  getHostCpuModel() { return v_node_info_ptr_->model;}
        void   getHostCpusInfo();

        void run(size_t time_intervals = 10);

        private:
        inline void freeDomainsResource();
        /// connect to hybervisor
        inline virConnectPtr connect2Hybervisor(const char* name);
        /// get all active running virtual machine
        inline void getAllActiveRunningVMs();

        virConnectPtr  v_conn_ptr_;
        virNodeInfoPtr v_node_info_ptr_;
        virDomainPtr*  v_domains_;
        size_t v_domains_num_;

        /// VCpu info
        std::unordered_map<virDomainPtr, CpuInfoPtr> vec_vcpu_info_;
    };

    typedef std::shared_ptr<CpuScheduler> CpuSchedulerPtr;
}
