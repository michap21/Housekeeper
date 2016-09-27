#include "memory_coordinator.h"

using namespace log;
using namespace virt;

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