#include <iomanip>
#include "cpu_stats.h"
#include "log.h"

namespace virt {
    void CpuInfo::init() {
        max_id_ = virDomainGetCPUStats(v_domain_ptr_, NULL, 0, 0, 0, 0);
        LOG(INFO) << "vCPUs num: " << max_id_;
        nparams_ = virDomainGetCPUStats(v_domain_ptr_, NULL, 0, 0, 1, 0);

        CHECK(nparams_ >= 0 && max_id_ >= 0) << "Failed to get CPU stats.\n";
        begin_params_ = (virTypedParameterPtr)calloc(
                            nparams_ * max_id_, sizeof(*begin_params_));
        end_params_ = (virTypedParameterPtr)calloc(
                            nparams_ * max_id_, sizeof(*end_params_));
        CHECK(begin_params_ && end_params_)
            << "Failed to allocate memory.\n";

        /// get current time
        CHECK_GE(gettimeofday(&begin_, NULL), 0) << "Failed to get time.\n";
        /// get current cpu stats
        begin_nparams_ = virDomainGetCPUStats(
                    v_domain_ptr_, begin_params_, nparams_, 0, max_id_, 0);
        CHECK_GE(begin_nparams_, 0) << "Failed to get cpu stats.\n";

        usleep(300 * 1000); /// 300 microseconds
        
        /// get next time
        CHECK_GE(gettimeofday(&end_, NULL), 0) << "Failed to get time.\n";
        /// get next cpu stats
        end_nparams_ = virDomainGetCPUStats(
                    v_domain_ptr_, end_params_, nparams_, 0, max_id_, 0);
        CHECK_GE(end_nparams_, 0) << "Failed to get cpu stats.\n";        
    }
    
    void CpuInfo::vCpuUsageInfo() {
        CHECK_GE(max_id_, 0);
        CHECK_NOTNULL(begin_params_);
        CHECK_NOTNULL(end_params_);
        CHECK_EQ(begin_nparams_, end_nparams_);
        double vCpuUsage;
        double cpuDiff, realDiff;
        for (int32_t i = 0; i < max_id_; ++i) {
            cpuDiff = (end_params_[end_nparams_ * i].value.ul -
                begin_params_[begin_nparams_ * i].value.ul) / 1000;
            realDiff = 1000000 * (end_.tv_sec - begin_.tv_sec) +
                (end_.tv_usec - begin_.tv_usec);
             vCpuUsage = cpuDiff / realDiff * 100;       
            LOG(INFO) << "vCPU No." << i << " Usage: "
                      << std::setprecision(3) << std::fixed
                      << vCpuUsage << "%" << std::endl;
        }
    }
}