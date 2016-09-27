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

namespace virt {
    #define STARVE_DOMAIN_THREHOLD  100 * 1024 * 1024   /* starvation threshold KB */
    #define WASTES_DOMAIN_THREHOLD  300 * 1024 * 1024   /* waste threhold KB */

    typedef enum {
        VIR_MEM_STARVE = 0,
        VIR_MEM_WASTES = 1,
        VIR_MEM_NORMAL = 2,
        VIR_MEM_BOTHSW = 3,
        VIR_MEM_END
    } vir_mem_state_t;

    typedef struct {
        double          v_cpu_usage_;        /* domain vcpus usage: % */
        size_t          v_unused_memory_;    /* KB unused memory in domain */
        vir_mem_state_t v_memory_state_;     /* memory state: starve, waste or both */

    } vir_domain_st;
    typedef std::unique_ptr<vir_domain_st> virDomainInfoPtr;

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
                if (m_domain_info_[v_domains_[i]] == nullptr) {
                    virDomainInfoPtr t_domain_ptr(new vir_domain_st);
                    m_domain_info_[v_domains_[i]] = std::move(t_domain_ptr);
                }

                LOG(INFO) << "---------- Domain Name: "<< virDomainGetName(v_domains_[i])
                          << " ----------\n";
                CpuInfoPtr vcpu_info_ptr(new CpuInfo(v_conn_ptr_, v_domains_[i]));
                vcpu_info_ptr->vCpuUsageInfo();
                vcpu_info_ptr->vCpuMapsInfo();
               
                m_domain_info_[v_domains_[i]]->v_cpu_usage_ = vcpu_info_ptr->getVCpusUsage();
            }
        }

        void getVMemsInfo() {
            getAllVMsList();
            for (int32_t i = 0; i < v_domains_num_; ++i) {
                if (m_domain_info_[v_domains_[i]] == nullptr) {
                    virDomainInfoPtr t_domain_ptr(new vir_domain_st);
                    m_domain_info_[v_domains_[i]] = std::move(t_domain_ptr);
                }

                virDomainMemoryStatStruct mem_stats[VIR_DOMAIN_MEMORY_STAT_NR];

                CHECK_GE(virDomainSetMemoryStatsPeriod(v_domains_[i], 1,
                    VIR_DOMAIN_AFFECT_LIVE | VIR_DOMAIN_AFFECT_CURRENT), 0)
                    << "Failed to call virDomainSetMemoryStatsPeriod().\n";
                CHECK_NE(virDomainMemoryStats(v_domains_[i], mem_stats,
                    VIR_DOMAIN_MEMORY_STAT_NR, 0), -1)
                    << "Failed to collect domains virt memory." << std::endl;
                
                LOG(INFO) << "----------Domains Memory Info Stats-----------\n";
                LOG(INFO) << "Domain Name: "
                          << virDomainGetName(v_domains_[i]) << std::endl;
                
                size_t mem_unused = mem_stats[VIR_DOMAIN_MEMORY_STAT_UNUSED].val;
                m_domain_info_[v_domains_[i]]->v_unused_memory_ = mem_unused;
          
                LOG(INFO) << "Unused Memory: " <<  mem_unused / 1024.0 << " MB\n";
                LOG(INFO) << "Swap Out: " << mem_stats[VIR_DOMAIN_MEMORY_STAT_SWAP_OUT].val
                          << std::endl;
                LOG(INFO) << "Swap In: " << mem_stats[VIR_DOMAIN_MEMORY_STAT_SWAP_IN].val
                          << std::endl;
                LOG(INFO) << "Available: " << mem_stats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val
                          << std::endl;         
                
                if (mem_unused <= STARVE_DOMAIN_THREHOLD) {
                    m_domain_info_[v_domains_[i]]->v_memory_state_ = VIR_MEM_STARVE;
                } else if (mem_unused >= WASTES_DOMAIN_THREHOLD) {
                    m_domain_info_[v_domains_[i]]->v_memory_state_ = VIR_MEM_WASTES;
                } else {
                    m_domain_info_[v_domains_[i]]->v_memory_state_ = VIR_MEM_NORMAL;
                }         

            }
        }

        void getPCpusInfo() { p_cpu_ptr_->getHostCpusInfo();}
        void getPMemsInfo() { p_cpu_ptr_->getHostMemsInfo();}
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
            m_domain_info_.clear();
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

        /// Vir Domains Info
        std::unordered_map<virDomainPtr, virDomainInfoPtr> m_domain_info_; 

    };  /// class LocalEngine
}   /// namespace virt