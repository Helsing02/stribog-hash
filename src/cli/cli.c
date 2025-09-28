#include "cli.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

void print_help(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Calculate Stribog hash (GOST R 34.11-2012) for input data.\n\n");
    printf("Options:\n");
    printf("  -s, --size SIZE      Hash size (256 or 512, default: 512)\n");
    printf("  -i, --input FILE     Input file (default: stdin)\n");
    printf("  -o, --output FILE    Output file (default: stdout)\n");
    printf("  -h, --help           Display this help and exit\n\n");
    printf("Examples:\n");
    printf("  %s -s 256 -i file.txt -o hash.txt\n", program_name);
    printf("  cat file.txt | %s -s 512 > hash.txt\n", program_name);
    printf("  echo -n \"hello\" | %s\n", program_name);
}

int parse_arguments(int argc, char *argv[], Config *config) {
    // Устанавливаем значения по умолчанию
    config->input_file = NULL;
    config->output_file = NULL;
    config->hash_size = 512; // По умолчанию 512 бит
    config->output_bytes = 0;
    config->help_requested = 0;

    // Длинные опции
    static struct option long_options[] = {
        {"size", required_argument, 0, 's'},
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"bytes", no_argument, 0, 'b'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "s:i:o:hb", long_options, &option_index)) != -1) {
        switch (c) {
            case 's':
                if (strcmp(optarg, "256") == 0) {
                    config->hash_size = 256;
                } else if (strcmp(optarg, "512") == 0) {
                    config->hash_size = 512;
                } else {
                    fprintf(stderr, "Error: Invalid hash size. Must be 256 or 512.\n");
                    return -1;
                }
                break;
            case 'i':
                config->input_file = optarg;
                break;
            case 'o':
                config->output_file = optarg;
                break;
            case 'b':
                config->output_bytes = 1;
                break;
            case 'h':
                config->help_requested = 1;
                break;
            case '?':
                // getopt уже вывел сообщение об ошибке
                return -1;
            default:
                fprintf(stderr, "Error: Unknown option.\n");
                return -1;
        }
    }

    // Проверяем, что нет лишних аргументов
    if (optind < argc) {
        fprintf(stderr, "Error: Unexpected argument: %s\n", argv[optind]);
        return -1;
    }

    return 0;
}
