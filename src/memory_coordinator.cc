#include <iostream>
#include "log.h"
#include "memory_coordinator.h"

using namespace log;
using namespace virt;

namespace virt {
    inline virConnectPtr MemoryCoordinator::connect2Hybervisor(const char* uri) {
        virConnectPtr conn;
        conn = virConnectOpen(uri);
        CHECK_NOTNULL(conn) << "Failed to connect to Hypervisor!\n";
        return std::move(conn);
    }

    void MemoryCoordinator::run(size_t timeIntervals) {
        sleep(timeIntervals);
    }        
}

int main (int argc, const char** argv) {
    initializeLogging(argc, argv);
    CHECK_EQ(argc, 2) << "Please provide one argument "
        <<"[time internal (secs)] for schedulers.\n";

    size_t timeIntervals = atoi(argv[1]);
    MemoryCoordinatorPtr memoryCoordinator(new MemoryCoordinator);
    while(1) {
        // intialize CpuScheduler and connect to hybervisor
        memoryCoordinator->run(timeIntervals);
    }
    return 0;    
}