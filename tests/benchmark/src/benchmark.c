#define _GNU_SOURCE  // Должно быть ПЕРВОЙ строкой для включения всех GNU/Linux функций
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "benchmark.h"

#include <pthread.h>       // для работы с pthread_self, pthread_setaffinity_np
#include <sched.h>         // для struct sched_param, sched_setscheduler, SCHED_FIFO и др.
#include <unistd.h>        

// Привязка к CPU ядру
int bind_to_cpu(int cpu_core) {
#ifdef __linux__
    if (cpu_core >= 0) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu_core, &cpuset);
        
        if (pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0) {
            printf("✓ Bound to CPU core %d\n", cpu_core);
            return 0;
        } else {
            perror("pthread_setaffinity_np");
        }
    }
#else
    (void)cpu_core;
    printf("⚠️  CPU binding not supported on this platform\n");
#endif
    return -1;
}

// // Realtime приоритет
// int set_realtime_priority(void) {
// #ifdef __linux__
//     struct sched_param param = {
//         .sched_priority = sched_get_priority_max(SCHED_FIFO)
//     };
    
//     if (sched_setscheduler(0, SCHED_FIFO, &param) == 0) {
//         printf("✓ Set realtime priority\n");
//         return 0;
//     } else {
//         perror("sched_setscheduler");
//     }
// #endif
//     printf("⚠️  Realtime priority not available (need root?)\n");
//     return -1;
// }

// Точное время в наносекундах
uint64_t get_nanoseconds(void) {
    struct timespec ts;
    
#ifdef CLOCK_MONOTONIC_RAW
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Быстрая сортировка для медианы
static void sort_double(double *arr, int n) {
    for (int i = 0; i < n-1; i++) {
        for (int j = 0; j < n-i-1; j++) {
            if (arr[j] > arr[j+1]) {
                double temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
    }
}

// Расчет медианы
static double calculate_median(double *times, int n) {
    if (n == 0) return 0.0;
    
    double *copy = malloc(n * sizeof(double));
    if (!copy) return 0.0;
    
    memcpy(copy, times, n * sizeof(double));
    sort_double(copy, n);
    
    double median = (n % 2 == 0) ? 
        (copy[n/2 - 1] + copy[n/2]) / 2.0 : 
        copy[n/2];
    
    free(copy);
    return median;
}

// Генерация тестовых данных
static void generate_data(uint8_t *data, size_t size) {
    // Детерминистичные, но "случайные" данные
    for (size_t i = 0; i < size; i++) {
        data[i] = (i * 2654435761UL) % 256;
    }
}

int run_benchmark(const benchmark_config_t *config) {
    printf("=== Stribog Performance Benchmark ===\n\n");
    
    // Настройка окружения
    if (config->cpu_core >= 0) {
        bind_to_cpu(config->cpu_core);
    }
    
    if (config->use_realtime) {
        set_realtime_priority();
    }
    
    printf("Configuration:\n");
    printf("  Data sizes: %zu - %zu bytes\n", config->min_size, config->max_size);
    printf("  Iterations: %d measurements, %d warmup\n", 
           config->iterations, config->warmup_iterations);
    printf("  Hash size: %d-bit\n", config->hash_size);
    printf("  Output: %s\n\n", config->output_file ? config->output_file : "console");
    
    // Прогрев
    printf("Warming up...\n");
    uint8_t warmup_data[1024];
    stribog_ctx_t warmup_ctx;
    
    for (int i = 0; i < config->warmup_iterations; i++) {
        generate_data(warmup_data, sizeof(warmup_data));
        stribog_init(&warmup_ctx, config->hash_size);
        stribog_update(&warmup_ctx, warmup_data, sizeof(warmup_data));
        uint8_t hash[64];
        stribog_final(&warmup_ctx, hash);
    }
    printf("✓ Warmup completed\n");
    
    // Подготовка размеров данных
    int num_sizes = 0;
    size_t size = config->min_size;
    
    while (size <= config->max_size) {
        num_sizes++;
        if (config->step_size > 0) {
            size += config->step_size;
        } else {
            size *= 2;
        }
    }
    
    benchmark_result_t *results = malloc(num_sizes * sizeof(benchmark_result_t));
    if (!results) {
        fprintf(stderr, "❌ Memory allocation failed for results\n");
        return -1;
    }
    
    int result_count = 0;
    
    // Основные измерения
    size = config->min_size;
    for (int i = 0; i < num_sizes; i++) {
        printf("Testing %9zu bytes...", size);
        fflush(stdout);
        
        uint8_t *test_data = malloc(size);
        if (!test_data) {
            fprintf(stderr, "❌ Failed to allocate %zu bytes for test data\n", size);
            free(results);
            return -1;
        }
        
        generate_data(test_data, size);
        
        double *times = malloc(config->iterations * sizeof(double));
        if (!times) {
            fprintf(stderr, "❌ Failed to allocate times array\n");
            free(test_data);
            free(results);
            return -1;
        }
        
        double sum = 0.0, min = 1e20, max = 0.0;
        
        for (int iter = 0; iter < config->iterations; iter++) {
            stribog_ctx_t ctx;
            uint8_t hash[64];
            
            stribog_init(&ctx, config->hash_size);
            
            uint64_t start = get_nanoseconds();
            stribog_update(&ctx, test_data, size);
            stribog_final(&ctx, hash);
            uint64_t end = get_nanoseconds();
            
            double time_ns = (double)(end - start);
            times[iter] = time_ns;
            sum += time_ns;
            
            if (time_ns < min) min = time_ns;
            if (time_ns > max) max = time_ns;
        }
        
        double avg = sum / config->iterations;
        double median = calculate_median(times, config->iterations);
        double throughput = (size / (1024.0 * 1024.0)) / (avg / 1e9);  // MB/s
        
        results[result_count] = (benchmark_result_t){
            .data_size = size,
            .min_time_ns = min,
            .max_time_ns = max,
            .avg_time_ns = avg,
            .median_time_ns = median,
            .throughput_mbs = throughput
        };
        result_count++;
        
        printf(" %.2f MB/s\n", throughput);
        
        free(test_data);
        free(times);
        
        if (config->step_size > 0) {
            size += config->step_size;
        } else {
            size *= 2;
        }
        
        // Защита от переполнения
        if (size < config->min_size) {
            break;
        }
    }
    
    // Вывод результатов
    printf("\n=== Results ===\n");
    print_results(results, result_count);
    
    if (config->output_file) {
        save_results_csv(results, result_count, config->output_file);
    }
    
    free(results);
    printf("✓ Benchmark completed successfully\n");
    return 0;
}

void print_results(const benchmark_result_t *results, int count) {
    printf("\n%12s %14s %14s %14s %14s %12s\n", 
           "Size (bytes)", "Min (ns)", "Max (ns)", "Avg (ns)", "Median (ns)", "Throughput (MB/s)");
    printf("%12s %14s %14s %14s %14s %12s\n", 
           "------------", "---------", "---------", "---------", "-----------", "----------------");
    
    for (int i = 0; i < count; i++) {
        const benchmark_result_t *r = &results[i];
        printf("%12zu %14.2f %14.2f %14.2f %14.2f %12.2f\n",
               r->data_size, r->min_time_ns, r->max_time_ns, 
               r->avg_time_ns, r->median_time_ns, r->throughput_mbs);
    }
}

int save_results_csv(const benchmark_result_t *results, int count, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("❌ Failed to open results file");
        return -1;
    }
    
    fprintf(f, "data_size,min_time_ns,max_time_ns,avg_time_ns,median_time_ns,throughput_mbs\n");
    
    for (int i = 0; i < count; i++) {
        const benchmark_result_t *r = &results[i];
        fprintf(f, "%zu,%.2f,%.2f,%.2f,%.2f,%.2f\n",
                r->data_size, r->min_time_ns, r->max_time_ns,
                r->avg_time_ns, r->median_time_ns, r->throughput_mbs);
    }
    
    fclose(f);
    printf("✓ Results saved to %s\n", filename);
    return 0;
}