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
         MemoryCoordinator(const char* name = "qemu:///system") : LocalEngine(name) {}

         void run(size_t timeIntervals = 10) {
            CHECK_GE(timeIntervals, 0);
            getPCpusInfo();
            sleep(timeIntervals);
         }

         private:
         void defaultCoordinator() {}
     };  // class MemoryCoordinator

     typedef std::shared_ptr<MemoryCoordinator> MemoryCoordinatorPtr;
}  // namespace virt