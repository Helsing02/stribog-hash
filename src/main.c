#include <stdio.h>
#include <stdlib.h>
#include "cli/cli.h"
#include "io/io.h"

int main(int argc, char *argv[]) {
    Config config;
    
    // Разбираем аргументы командной строки
    if (parse_arguments(argc, argv, &config) != 0) {
        return 1;
    }

    // Если запрошена помощь, выводим и выходим
    if (config.help_requested) {
        print_help(argv[0]);
        return 0;
    }

    // Обрабатываем ввод-вывод
    if (process_input_output(
            config.input_file, 
            config.output_file, 
            config.hash_size, 
            config.hex_input, 
            config.hex_output
        ) != 0) {
        return 1;
    }

    return 0;
}
