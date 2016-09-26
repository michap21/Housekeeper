#include <iostream>
#include <unistd.h>

#include "log.h"
#include "vcpu_scheduler.h"

using namespace log;
using namespace virt;

namespace virt {
    inline virConnectPtr CpuScheduler::connect2Hybervisor(const char* uri) {
        virConnectPtr conn;
        conn = virConnectOpen(uri);
        CHECK_NOTNULL(conn) << "Failed to connect to Hypervisor!\n";
        return conn;
    }
    
    inline void CpuScheduler::freeHostResource() {
        delete p_cpu_ptr_;
        p_cpu_ptr_ = nullptr;
    }

    inline void CpuScheduler::freeDomainsResource() {
        if (v_domains_ != nullptr && v_domains_num_ != 0) {
            for (int32_t i = 0; i < v_domains_num_; ++i) {
                virDomainFree(v_domains_[i]);
            }
            free(v_domains_);
            v_domains_num_ = 0;
            v_domains_ = nullptr;
        }   
        vec_vcpu_info_.clear();
    }

    inline void CpuScheduler::getAllActiveRunningVMs(unsigned int flags) {
        /// before query VMs stats info, first need free previous resource.
        freeDomainsResource();

        /// query all active and running domains.
        v_domains_num_ = virConnectListAllDomains(v_conn_ptr_, &v_domains_, flags);                   
        CHECK_GE(v_domains_num_, 0)
            << "Failed to list all active and running domains!\n";
        LOG(INFO) << "Get all active and runnning virtual machines successful!\n";
        LOG(INFO) << "CpuScheduler contains " << v_domains_num_ << " VM domains.\n";
        
        /// for each domain, query and print all needed info.
        for (int32_t i = 0; i < v_domains_num_; ++i) {
            LOG(INFO) << "###### Domain Name: "<< virDomainGetName(v_domains_[i])
                      << " ######\n";
            CpuInfoPtr vcpu_info_ptr(new CpuInfo(v_conn_ptr_, v_domains_[i]));
            vec_vcpu_info_[v_domains_[i]] = std::move(vcpu_info_ptr);
        }
    }

    void CpuScheduler::run(size_t timeIntervals) {
        CHECK_GE(timeIntervals, 0);
        this->p_cpu_ptr_->getHostCpusInfo();
        this->getAllActiveRunningVMs();
        sleep(timeIntervals);
    }
}

int main (int argc, const char** argv) {
    initializeLogging(argc, argv);
    CHECK_EQ(argc, 2) << "Please provide one argument "
        <<"[time internal (secs)] for schedulers.\n";

    size_t timeIntervals = atoi(argv[1]);
    CpuSchedulerPtr cpuScheduler(new CpuScheduler);
    while(1) {
        // intialize CpuScheduler and connect to hybervisor
        cpuScheduler->run(timeIntervals);
    }
    return 0;
}