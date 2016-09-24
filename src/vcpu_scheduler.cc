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

    void CpuScheduler::getAllActiveRunningVMs() {
        freeDomainsResource();
        unsigned int flags = VIR_CONNECT_LIST_DOMAINS_ACTIVE
                            | VIR_CONNECT_LIST_DOMAINS_RUNNING;

        v_domains_num_ = virConnectListAllDomains(v_conn_ptr_, &v_domains_, flags);                   
        CHECK_GE(v_domains_num_, 0)
            << "Failed to list all active and running domains!\n";
        LOG(INFO) << "Get all active and runnning virtual machines successful!\n";
        LOG(INFO) << "CpuScheduler contains " << v_domains_num_ << " VM domains.\n";

        for (int32_t i = 0; i < v_domains_num_; ++i) {
            LOG(INFO) << "###### Domain Name: "<< virDomainGetName(v_domains_[i])
                      << " ######\n";
            CpuInfoPtr vcpu_info_ptr(new CpuInfo(v_domains_[i]));
            vec_vcpu_info_[v_domains_[i]] = vcpu_info_ptr;
        }
    }
}

int main (int argc, const char** argv) {
    initializeLogging(argc, argv);
    CHECK_EQ(argc, 2) << "Please provide one argument "
        <<"[time internal (secs)] for schedulers.\n";

    size_t time_intervals = atoi(argv[1]);
    CpuSchedulerPtr cpuScheduler(new CpuScheduler());
    while(1) {
        // intialize CpuScheduler and connect to hybervisor
        cpuScheduler->getAllActiveRunningVMs();
        sleep(time_intervals);
    }
    return 0;
}