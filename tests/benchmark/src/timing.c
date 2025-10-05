#include <time.h>
#include <stdint.h>
#include "benchmark.h"

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

// Наносекунды с помощью clock_gettime (переносимо)
uint64_t nanoseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Тактов процессора (только x86)
uint64_t cpu_cycles(void) {
#if defined(__x86_64__) || defined(__i386__)
    return __rdtsc();
#else
    return nanoseconds(); // Fallback для других архитектур
#endif
}

// Калибровка таймера
uint64_t calibrate_timer(void) {
    const int iterations = 1000000;
    uint64_t start = nanoseconds();
    
    for (int i = 0; i < iterations; i++) {
        // Пустая работа для калибровки
        __asm__ volatile("" ::: "memory");
    }
    
    uint64_t end = nanoseconds();
    return (end - start) / iterations;
}

// Точное ожидание (для стабилизации частоты)
void precise_delay(uint64_t delay_ns) {
    uint64_t start = nanoseconds();
    while (nanoseconds() - start < delay_ns) {
        __asm__ volatile("pause" ::: "memory");
    }
}
