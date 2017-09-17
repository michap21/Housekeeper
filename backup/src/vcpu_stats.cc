#include <iomanip>
#include "vcpu_stats.h"
#include "log.h"

namespace virt {
    inline void CpuInfo::getSupportVCPUsNum() {
        max_id_ = virDomainGetMaxVcpus(vcpu_domain_ptr_);
        LOG(INFO) << "This Domain supports maximum VCPUs num: "
                  << max_id_ << std::endl;
        use_id_ = virDomainGetVcpusFlags(vcpu_domain_ptr_,
                  VIR_DOMAIN_AFFECT_CURRENT | VIR_DOMAIN_AFFECT_LIVE);          
        LOG(INFO) << "This Domain uses VCPUs num: "
                  << use_id_ << std::endl;
    }

    inline void CpuInfo::getVCpusMapinfo() {
        LOG(INFO) << "---------PCPUs-to-VCPUs-from-Bitmaps----------" << std::endl;
        int cnt = 0;
        for (int32_t i = 0; i < max_id_; ++i) {
            if (vcpu_maps_[i] == 0) {
                LOG(INFO) << "VCPU No." << i << " is offline!" << std::endl;
            } else {
                cnt = 0;
                while(vcpu_maps_[i]) {
                    if (vcpu_maps_[i] & 1) {
                        LOG(INFO) << "PCPU No." << cnt << " map to VCPU No."
                                  << i << std::endl;
                    }
                    vcpu_maps_[i] >>= 1;
                    cnt++;
                }
            }
        }
    }

    inline void CpuInfo::getVCpusInfo() {
        LOG(INFO) << "---------virVcpuInfo----------" << std::endl;
        for (int32_t i = 0; i < max_id_; ++i) {
            LOG(INFO) << "Virtual CPU Number: " << vcpu_info_ptr_[i].number << std::endl;
            LOG(INFO) << "Real CPU Number: " << vcpu_info_ptr_[i].cpu << std::endl;

            if (vcpu_info_ptr_[i].state == VIR_VCPU_OFFLINE)
                LOG(INFO) << "VCPU State is offline." << std::endl;
            else if (vcpu_info_ptr_[i].state == VIR_VCPU_RUNNING)
                LOG(INFO) << "VCPU State is running." << std::endl;
            else if (vcpu_info_ptr_[i].state == VIR_VCPU_BLOCKED)
                LOG(INFO) << "VCPU State is blocked on resource." << std::endl;
            else
                LOG(INFO) << "VCPU state is unknown." << std::endl;
    
            LOG(INFO) << "CPU Time Used: " << vcpu_info_ptr_[i].cpuTime << " nanoseconds\n"; 
        }   
    }

    void CpuInfo::init() {
        getOSType();
        getSupportVCPUsNum();

        nparams_ = virDomainGetCPUStats(vcpu_domain_ptr_, NULL, 0, -1, max_id_, 0);
        CHECK(nparams_ >= 0 && max_id_ >= 0) << "Failed to get CPU stats.\n";

        begin_params_ = (virTypedParameterPtr)calloc(
                            nparams_ * max_id_, sizeof(*begin_params_));
        end_params_ = (virTypedParameterPtr)calloc(
                            nparams_ * max_id_, sizeof(*end_params_));
        CHECK(begin_params_ && end_params_)
            << "Failed to allocate memory.\n";

        /// get current time
        CHECK_GE(gettimeofday(&begin_, NULL), 0) << "Failed to get time.\n";
        /// get current total cpu stats
        begin_nparams_ = virDomainGetCPUStats(
                    vcpu_domain_ptr_, begin_params_, nparams_, -1, max_id_, 0);
        CHECK_GE(begin_nparams_, 0) << "Failed to get cpu stats.\n";

        usleep(300 * 1000); /// 300 microseconds
        
        /// get next time
        CHECK_GE(gettimeofday(&end_, NULL), 0) << "Failed to get time.\n";
        /// get next total cpu stats
        end_nparams_ = virDomainGetCPUStats(
                    vcpu_domain_ptr_, end_params_, nparams_, -1, max_id_, 0);
        CHECK_GE(end_nparams_, 0) << "Failed to get cpu stats.\n";        
    }

    void CpuInfo::vCpuMapsInfo() {
        vcpu_info_ptr_ = new virVcpuInfo[max_id_];
        virNodeInfo vCpuNodeInfo;
        virNodeGetInfo(vcpu_conn_ptr_, &vCpuNodeInfo);
        size_t cpuMapLen = VIR_CPU_MAPLEN(vCpuNodeInfo.cpus);
        vcpu_maps_ = (unsigned char*)calloc(max_id_, cpuMapLen);

        CHECK_NE(virDomainGetVcpus(vcpu_domain_ptr_, vcpu_info_ptr_,
                 max_id_, vcpu_maps_, cpuMapLen), -1)
                 << "virDomainGetVcpus function failed.\n";
        getVCpusMapinfo();
        getVCpusInfo();              
    }

    float CpuInfo::getVCpusUsage() {
        return total_vcpus_usage_;
    }

    void CpuInfo::vCpuUsageInfo() {
        CHECK_GE(max_id_, 0);
        CHECK_NOTNULL(begin_params_);
        CHECK_NOTNULL(end_params_);
        CHECK_EQ(begin_nparams_, end_nparams_);
        double vCpuUsage;
        double cpuDiff, realDiff;

        // OS Type
        LOG(INFO) << "OS Type: " << vcpu_os_type_ << std::endl;

        // CPU Usage
        total_vcpus_usage_ = 0;
        for (int32_t i = 0; i < max_id_; ++i) {
            cpuDiff = (end_params_[end_nparams_ * i].value.ul -
                begin_params_[begin_nparams_ * i].value.ul) / 1000;
            realDiff = 1000000 * (end_.tv_sec - begin_.tv_sec) +
                (end_.tv_usec - begin_.tv_usec);
            vCpuUsage = cpuDiff / realDiff * 100;       
            
            LOG(INFO) << "vCPU No." << i << " Usage: "
                      << std::setprecision(3) << std::fixed
                      << vCpuUsage << "%" << std::endl;
            total_vcpus_usage_ += vCpuUsage;
        }
        total_vcpus_usage_ /= static_cast<double>(max_id_);
    }
}