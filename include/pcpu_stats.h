#pragma once

#include <memory>
#include <libvirt/libvirt.h>
#include "utils.h"

namespace virt
{
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
        void   getHostCpusInfo() {
            LOG(INFO) << "###### Host Name: "<< getHostName() << " ######\n";
            LOG(INFO) << "CPU Model: " << getHostCpuModel() << std::endl;
            LOG(INFO) << "CPU Numbers: " << getHostCpuNum() << std::endl;
            LOG(INFO) << "CPU Frequency (Mhz): " << getHostFrequency() << std::endl;
            LOG(INFO) << "CPU Memory:" << GetHostMemory() / 1000.0 << " GB" << std::endl; 
        }

        private:
        virConnectPtr  v_conn_ptr_;
        virNodeInfoPtr v_node_ptr_;
    };
    
    typedef PCpuInfo* PCpuInfoPtr;
}