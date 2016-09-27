#pragma once

#include "local_engine.h"

namespace virt {
    /**
     * Cpu Scheduler class is to connect to hybervisor and
     * get all active running virtual machines
     */
    class CpuScheduler : public LocalEngine {
        public:
        DISABLE_COPY(CpuScheduler);
        CpuScheduler(const char* name = "qemu:///system") : LocalEngine(name) {}

        void run(size_t timeIntervals) {
            CHECK_GE(timeIntervals, 0);
            getPCpusInfo();
            getVCpusInfo();
            sleep(timeIntervals);
        }

        private:
        void defaultScheduler() {}
    };

    typedef std::unique_ptr<CpuScheduler> CpuSchedulerPtr;
}
