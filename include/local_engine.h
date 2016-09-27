#pragma once

#include <iostream>
#include <unistd.h>
#include <memory>
#include <unordered_map>
#include <libvirt/libvirt.h>
#include "utils.h"
#include "log.h"
#include "vcpu_stats.h"
#include "pcpu_stats.h"

namespace virt
{
    class LocalEngine {
        public:
        DISABLE_COPY(LocalEngine);
        explicit LocalEngine(const char* name = "qemu:///system") {
            v_conn_ptr_ = connect2Hybervisor(name);
            p_cpu_ptr_ = new PCpuInfo(v_conn_ptr_, std::move(new virNodeInfo));
            // did not query domains in here
            v_domains_ = nullptr;                
            LOG(INFO) << "Connect to hybervisor successful.\n";
        }
        virtual ~LocalEngine() {
            freeHostResource();
            freeDomainsResource();
            CHECK_NE(virConnectClose(v_conn_ptr_), -1)
                << "Close hybervisor connection failed.\n";
        }

        virConnectPtr  getVirConnectPtr() { return v_conn_ptr_; }
        virDomainPtr*  getVirDomainPtr () { return v_domains_; }

        size_t getDomainsNum() { return v_domains_num_; }

        void getVCpusInfo() {
            getAllVMsList();
            /// for each domain, query and print all needed info.
            for (int32_t i = 0; i < v_domains_num_; ++i) {
                LOG(INFO) << "###### Domain Name: "<< virDomainGetName(v_domains_[i])
                          << " ######\n";
                CpuInfoPtr vcpu_info_ptr(new CpuInfo(v_conn_ptr_, v_domains_[i]));
                vec_vcpu_info_[v_domains_[i]] = std::move(vcpu_info_ptr);
            }
        }

        void getPCpusInfo() { p_cpu_ptr_->getHostCpusInfo();}

        virtual void run(size_t time_intervals = 10) = 0;

        private:
        inline void freeHostResource() {
            delete p_cpu_ptr_;
            p_cpu_ptr_ = nullptr;
        }

        inline void freeDomainsResource() {
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

        /// connect to hybervisor
        inline virConnectPtr connect2Hybervisor(const char* name) {
            virConnectPtr conn;
            conn = virConnectOpen(name);
            CHECK_NOTNULL(conn) << "Failed to connect to Hypervisor!\n";
            return std::move(conn);
        }

        /// get all active running virtual machine
        inline void getAllVMsList(unsigned int flags =
            VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING) {
            /// before query VMs stats info, first need free previous resource.
            freeDomainsResource();
            /// query all active and running domains.
            v_domains_num_ = virConnectListAllDomains(v_conn_ptr_, &v_domains_, flags);                   
            CHECK_GE(v_domains_num_, 0) << "Failed to list all active and running domains!\n";
            LOG(INFO) << "Get all active and runnning virtual machines successful!\n";
            LOG(INFO) << "CpuScheduler contains " << v_domains_num_ << " VM domains.\n";
        }

        size_t v_domains_num_;
        virDomainPtr*  v_domains_;
        virConnectPtr  v_conn_ptr_;
        PCpuInfoPtr    p_cpu_ptr_;

        /// VCpu info
        std::unordered_map<virDomainPtr, CpuInfoPtr> vec_vcpu_info_;
    };  /// class LocalEngine
}   /// namespace virt