#include <sched.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "benchmark.h"

#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

int set_cpu_affinity(int cpu_core) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_core, &cpuset);
    
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
        perror("pthread_setaffinity_np");
        return -1;
    }
    
    // Проверяем, что привязка произошла
    CPU_ZERO(&cpuset);
    if (pthread_getaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) != 0) {
        perror("pthread_getaffinity_np");
        return -1;
    }
    
    if (!CPU_ISSET(cpu_core, &cpuset)) {
        fprintf(stderr, "Failed to set CPU affinity to core %d\n", cpu_core);
        return -1;
    }
    
    printf("Bound to CPU core %d\n", cpu_core);
    return 0;
}

int set_realtime_priority(void) {
    struct sched_param param;
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);
    
    if (sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
        perror("sched_setscheduler");
        fprintf(stderr, "Warning: Cannot set realtime priority (need root?)\n");
        return -1;
    }
    
    printf("Set realtime priority (SCHED_FIFO)\n");
    return 0;
}

#ifdef __linux__
int disable_turbo_boost(void) {
    int fd = open("/sys/devices/system/cpu/intel_pstate/no_turbo", O_WRONLY);
    if (fd == -1) {
        // Попробуем альтернативный путь
        fd = open("/sys/devices/system/cpu/cpufreq/boost", O_WRONLY);
        if (fd == -1) {
            return -1; // Не поддерживается на этой системе
        }
    }
    
    write(fd, "1", 1);
    close(fd);
    printf("Disabled turbo boost\n");
    return 0;
}

int enable_turbo_boost(void) {
    int fd = open("/sys/devices/system/cpu/intel_pstate/no_turbo", O_WRONLY);
    if (fd == -1) {
        fd = open("/sys/devices/system/cpu/cpufreq/boost", O_WRONLY);
        if (fd == -1) {
            return -1;
        }
    }
    
    write(fd, "0", 1);
    close(fd);
    printf("Enabled turbo boost\n");
    return 0;
}
#else
// Заглушки для не-Linux систем
int disable_turbo_boost(void) { return -1; }
int enable_turbo_boost(void) { return -1; }
#endif
