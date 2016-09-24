ADD_LIBRARY(utils SHARED
        log.cc
        cpu_stats.cc)

ADD_EXECUTABLE(memory_coordinator memory_coordinator.cc)
TARGET_LINK_LIBRARIES(memory_coordinator utils ${LibVirt_LIBRARY})                

ADD_EXECUTABLE(vcpu_scheduler vcpu_scheduler.cc)
TARGET_LINK_LIBRARIES(vcpu_scheduler utils ${LibVirt_LIBRARY})
