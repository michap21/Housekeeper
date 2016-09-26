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

        CpuInfo(virConnectPtr connPtr, virDomainPtr domainPtr)
            : vcpu_conn_ptr_(connPtr), vcpu_domain_ptr_(domainPtr){
            init();
            vCpuUsageInfo();
            vCpuMapsInfo();
        }
        ~CpuInfo(){
            /// free resource
            delete [] vcpu_info_ptr_;
            free(vcpu_maps_);
            virTypedParamsFree(begin_params_, begin_nparams_ * max_id_);
            virTypedParamsFree(end_params_, end_nparams_ * max_id_);
        }

        private:
        /// initialize CpuInfo
        void init();
        /// stats vCpu usage
        void vCpuUsageInfo();
        /// the current mao bewteen VCPU and PCPU
        void vCpuMapsInfo();
        /// get vm support VCPUs num
        inline void getSupportVCPUsNum();
        /// get OS type
        inline void getOSType() {vcpu_os_type_= virDomainGetOSType(vcpu_domain_ptr_);}
        /// get VCpus bitmap info
        inline void getVCpusMapinfo();
        /// get virVcpusInfo structure info
        inline void getVCpusInfo();

        virConnectPtr vcpu_conn_ptr_;
        virDomainPtr  vcpu_domain_ptr_;     /// vcpu domain pointer
        std::string   vcpu_os_type_;        /// domain os type
        int max_id_;                        /// number of vCpus
        int use_id_;                        /// number of used vCpus
        int nparams_;                       /// number of params per vCPU
        int begin_nparams_;                 /// number of params for different state of vCPU
        int end_nparams_;                   /// number of params for different state of vCPU
        virTypedParameterPtr begin_params_; /// param ptr for different state of vCPU
        virTypedParameterPtr end_params_;   /// param ptr for different state of vCPU
        struct timeval begin_, end_;        /// time intervals for cpu usage stats

        unsigned char  *vcpu_maps_;         /// vcpu bitmaps
        virVcpuInfoPtr  vcpu_info_ptr_;     /// vcpu info
    };
    typedef std::unique_ptr<CpuInfo> CpuInfoPtr;
}