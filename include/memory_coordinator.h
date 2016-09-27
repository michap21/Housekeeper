#pragma once

#include "local_engine.h"

namespace virt {
    /**
     * Memory Coordinator is to connect to hybervisor and
     * balance all active running virtual machines
     */
     class MemoryCoordinator : public LocalEngine {
         public:
         DISABLE_COPY(MemoryCoordinator);
         MemoryCoordinator(const char* name = "qemu:///system")
            : LocalEngine(name) {}

         void run(size_t timeIntervals = 10) {
            CHECK_GE(timeIntervals, 0);
            getPCpusInfo();
            getPMemsInfo();
            getVMemsInfo();

            // default memory coordinator
            defaultCoordinator();
            sleep(timeIntervals);
         }

         private:
         inline void getStarveWasteVecs() {
             for (int32_t i = 0; i < m_domain_info_.size(); ++i) {
                 if (m_domain_info_[v_domains_[i]]->v_mem_state_ == VIR_MEM_STARVE) {
                     starve_domains_[v_domains_[i]] = m_domain_info_[v_domains_[i]];
                 } else if (m_domain_info_[v_domains_[i]]->v_mem_state_ == VIR_MEM_WASTES) {
                     wastes_domains_[v_domains_[i]] = m_domain_info_[v_domains_[i]];
                 }
             }
         }

         inline void defaultCoordinator() {
             getStarveWasteVecs();

             LOG(INFO) << "------------Default Coordinator Start to Balance Memory--------------\n";
             int swap_size = starve_domains_.size() < wastes_domains_.size()
                ? starve_domains_.size() : wastes_domains_.size();

             // exchange pairs of starve and waste mem to balance mem utilization
             std::unordered_map<virDomainPtr, virDomainInfoPtr>::iterator wastes_it, starve_it;
             wastes_it = wastes_domains_.begin();
             starve_it = starve_domains_.begin();
             for (int32_t i = 0; i < swap_size; ++i, ++wastes_it, ++starve_it) {
                 size_t wastes_mem = wastes_it->second->v_unused_mem_;
                 size_t starve_mem = starve_it->second->v_unused_mem_;
                 CHECK_EQ(virDomainSetMemory(wastes_it->first, wastes_mem - wastes_mem/2), 0)
                    << "Failed to set domain memory.\n";
                 CHECK_EQ(virDomainSetMemory(starve_it->first, starve_mem + wastes_mem/2), 0)
                    << "Failed to set domain memory.\n";
                 LOG(INFO) << virDomainGetName(wastes_it->first) << " Memory is waste [Exchanged].\n";
                 LOG(INFO) << virDomainGetName(starve_it->first) << " Memory is starve [Exchanged].\n";
             }

             // rest of unpair unnormal domains: starve or waste
             int rest_size = 0;
             if (starve_domains_.size() > wastes_domains_.size()) {
                 rest_size = starve_domains_.size() - swap_size;
                 for (int32_t i = 0; i < rest_size; ++i, ++starve_it) {
                     LOG(INFO) << virDomainGetName(starve_it->first)
                               << " Memory is starvation. [Exchanged]\n";
                     CHECK_EQ(virDomainSetMemory(starve_it->first,
                        starve_it->second->v_unused_mem_ + WASTES_DOMAIN_THREHOLD), 0)
                        << "Failed to set domain memory.\n";
                 }
             } else if (starve_domains_.size() < wastes_domains_.size()) {
                 rest_size = wastes_domains_.size() - swap_size;
                 for (int32_t i = 0; i < rest_size; ++i, ++wastes_it) {
                     LOG(INFO) << virDomainGetName(wastes_it->first)
                               << " Memory is waste. [Exchanged]\n";
                     CHECK_EQ(virDomainSetMemory(wastes_it->first,
                        wastes_it->second->v_unused_mem_ - WASTES_DOMAIN_THREHOLD), 0)
                        << "Failed to set domain memory.\n";
                 }
             }

             // free resource
             starve_domains_.clear();
             wastes_domains_.clear();
         }

         std::unordered_map<virDomainPtr, virDomainInfoPtr> starve_domains_;
         std::unordered_map<virDomainPtr, virDomainInfoPtr> wastes_domains_;
     
     };  // class MemoryCoordinator

     typedef std::shared_ptr<MemoryCoordinator> MemoryCoordinatorPtr;
}  // namespace virt