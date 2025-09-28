#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 4096

int process_input_output(const char *input_file, const char *output_file, int hash_size, int output_bytes_mode) {
    FILE *input = stdin;
    FILE *output = stdout;
    stribog_ctx_t ctx;
    uint8_t buffer[BUFFER_SIZE];
    unsigned char hash[STRIBOG_512_HASH_SIZE];
    size_t bytes_read;
    int result = 0;

    // Открываем входной файл если указан
    if (input_file) {
        input = fopen(input_file, "rb");
        if (!input) {
            fprintf(stderr, "Error: Cannot open input file '%s': %s\n", 
                    input_file, strerror(errno));
            return -1;
        }
    }

    // Открываем выходной файл если указан
    if (output_file) {
        output = fopen(output_file, "wb");
        if (!output) {
            fprintf(stderr, "Error: Cannot open output file '%s': %s\n", 
                    output_file, strerror(errno));
            if (input != stdin) fclose(input);
            return -1;
        }
    }

    // Инициализируем контекст хеширования
    stribog_init(&ctx, hash_size);

    // Читаем и обрабатываем данные
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, input)) > 0) {
        stribog_update(&ctx, buffer, bytes_read);
    }

    // Проверяем ошибки чтения
    if (ferror(input)) {
        fprintf(stderr, "Error reading input: %s\n", strerror(errno));
        result = -1;
        goto cleanup;
    }

    // Финализируем хеш
    stribog_final(&ctx, hash);

    // Записываем результат
    size_t hash_bytes = (hash_size == 256) ? STRIBOG_256_HASH_SIZE : STRIBOG_512_HASH_SIZE;
    if (output_bytes_mode) {
        // выводим бинарные байты:
        if (fwrite(hash, 1, hash_bytes, output) != hash_bytes) {
            fprintf(stderr, "Error writing output: %s\n", strerror(errno));
            result = -1;
            goto cleanup;
        }
    } else {
        // выводим как hex:
        for (int i = 0; i < (int)hash_bytes; i++) {
            fprintf(output, "%02x", hash[i]);
        }
        fprintf(output, "\n");
    }

cleanup:
    // Закрываем файлы если они были открыты
    if (input != stdin) fclose(input);
    if (output != stdout) fclose(output);

    return result;
}
