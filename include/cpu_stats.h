#pragma once

#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <libvirt/libvirt.h>
#include "utils.h"

namespace virt
{
    class CpuInfo{
        public:
        DISABLE_COPY(CpuInfo);

        CpuInfo(virDomainPtr v_domain_ptr) : v_domain_ptr_(v_domain_ptr){
            init();
            vCpuUsageInfo();
        }
        ~CpuInfo(){
            /// free resource
            virTypedParamsFree(begin_params_, begin_nparams_ * max_id_);
            virTypedParamsFree(end_params_, end_nparams_ * max_id_);
        }

        private:
        /// initialize CpuInfo
        void init();
        /// stats vCpu usage
        void vCpuUsageInfo();
        
        virDomainPtr v_domain_ptr_;         /// vcpu domain pointer
        int max_id_;                        /// number of vCpus
        int nparams_;                       /// number of params per vCPU
        int begin_nparams_;                 /// number of params for different state of vCPU
        int end_nparams_;                   /// number of params for different state of vCPU
        virTypedParameterPtr begin_params_; /// param ptr for different state of vCPU
        virTypedParameterPtr end_params_;   /// param ptr for different state of vCPU
        struct timeval begin_, end_;        /// time intervals for cpu usage stats
    };
    typedef std::shared_ptr<CpuInfo> CpuInfoPtr;
}