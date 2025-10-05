#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stddef.h>
#include <stdint.h>
#include "../../../src/hash/stribog.h"  // Наша реализация

// Конфигурация бенчмарка
typedef struct {
    size_t min_size;
    size_t max_size; 
    size_t step_size;
    int iterations;
    int warmup_iterations;
    int hash_size;  // 256 или 512
    int cpu_core;   // -1 = все ядра, 0+ = конкретное ядро
    int use_realtime;
    const char *output_file;
} benchmark_config_t;

// Результаты
typedef struct {
    size_t data_size;
    double min_time_ns;
    double max_time_ns;
    double avg_time_ns;
    double median_time_ns;
    double throughput_mbs;
} benchmark_result_t;

// Основные функции
int run_benchmark(const benchmark_config_t *config);
void print_results(const benchmark_result_t *results, int count);
int save_results_csv(const benchmark_result_t *results, int count, const char *filename);

// Утилиты
int bind_to_cpu(int cpu_core);
int set_realtime_priority(void);
uint64_t get_nanoseconds(void);

#endif