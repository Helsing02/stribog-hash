#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "benchmark.h"

static void print_help(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Benchmark Stribog hash implementation performance\n\n");
    printf("Options:\n");
    printf("  -s, --min-size BYTES    Minimum data size (default: 1024)\n");
    printf("  -S, --max-size BYTES    Maximum data size (default: 1048576)\n");
    printf("  -t, --step BYTES        Step size (0 = exponential, default: 0)\n");
    printf("  -i, --iterations N      Measurement iterations (default: 100)\n");
    printf("  -w, --warmup N          Warmup iterations (default: 100)\n");
    printf("  -z, --hash-size 256|512 Hash size (default: 512)\n");
    printf("  -c, --cpu CORE          Bind to CPU core (-1 = all, default: -1)\n");
    printf("  -r, --realtime          Use realtime priority\n");
    printf("  -o, --output FILE       Save results to CSV file\n");
    printf("  -h, --help              Show this help\n\n");
    printf("Examples:\n");
    printf("  %s -s 1024 -S 1048576 -i 1000\n", program_name);
    printf("  %s --min-size 1024 --max-size 1048576 --cpu 0 --realtime --output results.csv\n", program_name);
}

int main(int argc, char *argv[]) {
    benchmark_config_t config = {
        .min_size = 1024,
        .max_size = 1048576,
        .step_size = 0,
        .iterations = 100,
        .warmup_iterations = 100,
        .hash_size = 512,
        .cpu_core = -1,
        .use_realtime = 0,
        .output_file = NULL
    };
    
    static struct option long_options[] = {
        {"min-size", required_argument, 0, 's'},
        {"max-size", required_argument, 0, 'S'},
        {"step", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"warmup", required_argument, 0, 'w'},
        {"hash-size", required_argument, 0, 'z'},
        {"cpu", required_argument, 0, 'c'},
        {"realtime", no_argument, 0, 'r'},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "s:S:t:i:w:z:c:ro:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 's':
                config.min_size = atol(optarg);
                break;
            case 'S':
                config.max_size = atol(optarg);
                break;
            case 't':
                config.step_size = atol(optarg);
                break;
            case 'i':
                config.iterations = atoi(optarg);
                break;
            case 'w':
                config.warmup_iterations = atoi(optarg);
                break;
            case 'z':
                config.hash_size = atoi(optarg);
                if (config.hash_size != 256 && config.hash_size != 512) {
                    fprintf(stderr, "Error: Hash size must be 256 or 512\n");
                    return 1;
                }
                break;
            case 'c':
                config.cpu_core = atoi(optarg);
                break;
            case 'r':
                config.use_realtime = 1;
                break;
            case 'o':
                config.output_file = optarg;
                break;
            case 'h':
                print_help(argv[0]);
                return 0;
            default:
                print_help(argv[0]);
                return 1;
        }
    }
    
    return run_benchmark(&config);
}
