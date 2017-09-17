#pragma once

#include <memory>
#include <libvirt/libvirt.h>
#include "utils.h"

namespace virt {
    class PCpuInfo {
        public:
        DISABLE_COPY(PCpuInfo);
        PCpuInfo(virConnectPtr v_conn_ptr, virNodeInfoPtr v_node_ptr) {
            v_conn_ptr_ = v_conn_ptr;
            v_node_ptr_ = v_node_ptr;
            CHECK_EQ(virNodeGetInfo(v_conn_ptr, v_node_ptr_), 0)
                << "Failed to Get Host Node Info.\n";
        }
        ~PCpuInfo() {
            delete v_node_ptr_;
        }

        char* getHostName() {
            CHECK_NOTNULL(v_conn_ptr_);
            return virConnectGetHostname(v_conn_ptr_);
        }

        size_t getHostCpuNum()   { return v_node_ptr_->cpus; }
        size_t GetHostMemory()   { return v_node_ptr_->memory/1000.0; }
        size_t getHostFrequency(){ return v_node_ptr_->mhz; }
        char*  getHostCpuModel() { return v_node_ptr_->model; }

        void   getHostMemsInfo() {
            LOG(INFO) << "---------- Host Name: "<< getHostName() << " ----------\n";
            LOG(INFO) << "CPU Memory:" << GetHostMemory() / 1024.0 << " GB" << std::endl;
            
            int cellNum = VIR_NODE_MEMORY_STATS_ALL_CELLS;
            int nparams = 0;
            virNodeMemoryStatsPtr params = nullptr;
            if (virNodeGetMemoryStats(v_conn_ptr_, cellNum, NULL, &nparams, 0) == 0 &&
                nparams != 0) {
                constexpr int len = sizeof(virNodeMemoryStats);    
                params = (virNodeMemoryStatsPtr)malloc(len * nparams);
                CHECK_NOTNULL(params) << "Failed to allocate memory for Host Memory Stats.\n";
                memset(params, 0, len * nparams);
                CHECK_EQ(virNodeGetMemoryStats(v_conn_ptr_, cellNum, params, &nparams, 0), 0)
                    << "Failed to get host memory stats." << std::endl;
            }

            for (int32_t i = 0; i < nparams; ++i) {
                LOG(INFO) << params[i].field << ": " << params[i].value / 1024 << "MB\n";
            }
            assignHostUnusedMem(params, nparams);
            free(params);
        }

        size_t getHostUnusedMem() {
            return p_unused_mem_;
        }

        void   getHostCpusInfo() {
            LOG(INFO) << "---------- Host Name: "<< getHostName() << " ----------\n";
            LOG(INFO) << "CPU Model: " << getHostCpuModel() << std::endl;
            LOG(INFO) << "CPU Numbers: " << getHostCpuNum() << std::endl;
            LOG(INFO) << "CPU Frequency (Mhz): " << getHostFrequency() << std::endl;
            LOG(INFO) << "CPU Memory:" << GetHostMemory() / 1024.0 << " GB" << std::endl; 
        }

        private:
        inline void assignHostUnusedMem(virNodeMemoryStatsPtr params, int nparams) {
            size_t unused_mem = params[0].value;
            for (int32_t i = 1; i < nparams; ++i) {
                unused_mem -= params[i].value;
            }
            p_unused_mem_ = unused_mem;
            LOG(INFO) << "unused: " << p_unused_mem_ / 1024.0 << "MB\n";
        }

        size_t p_unused_mem_;
        virConnectPtr  v_conn_ptr_;
        virNodeInfoPtr v_node_ptr_;
    };
    
    typedef PCpuInfo* PCpuInfoPtr;
}