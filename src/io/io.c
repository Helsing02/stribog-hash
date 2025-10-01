#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#define BUFFER_SIZE 4096


// Вспомогательные функции для работы с hex
static int hex_char_to_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static size_t hex_string_to_bytes(const char *hex_str, size_t hex_len, uint8_t *bytes) {
    size_t bytes_len = 0;
    
    // Пропускаем не-hex символы
    for (size_t i = 0; i < hex_len; ) {
        // Пропускаем пробелы и переводы строк
        while (i < hex_len && isspace((unsigned char)hex_str[i])) {
            i++;
        }
        if (i >= hex_len) break;
        
        // Проверяем, что осталось как минимум 2 символа
        if (i + 1 >= hex_len) {
            // Нечетное количество hex символов
            return 0;
        }
        
        // Пропускаем не-hex символы
        if (!isxdigit((unsigned char)hex_str[i]) || !isxdigit((unsigned char)hex_str[i+1])) {
            return 0;
        }
        
        int high = hex_char_to_value(hex_str[i]);
        int low = hex_char_to_value(hex_str[i+1]);
        
        if (high == -1 || low == -1) {
            return 0;
        }
        
        bytes[bytes_len++] = (high << 4) | low;
        i += 2;
    }
    
    return bytes_len;
}

static int read_hex_data(FILE *input, stribog_ctx_t *ctx) {
    char hex_buffer[BUFFER_SIZE * 2]; // В 2 раза больше для hex
    uint8_t byte_buffer[BUFFER_SIZE];
    size_t bytes_read;
    
    while ((bytes_read = fread(hex_buffer, 1, sizeof(hex_buffer), input)) > 0) {
        size_t bytes_converted = hex_string_to_bytes(hex_buffer, bytes_read, byte_buffer);
        if (bytes_converted == 0 && bytes_read > 0) {
            fprintf(stderr, "Error: Invalid hex string in input\n");
            return -1;
        }
        
        if (bytes_converted > 0) {
            stribog_update(ctx, byte_buffer, bytes_converted);
        }
    }
    
    if (ferror(input)) {
        fprintf(stderr, "Error reading input: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

static int read_binary_data(FILE *input, stribog_ctx_t *ctx) {
    uint8_t buffer[BUFFER_SIZE];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, input)) > 0) {
        stribog_update(ctx, buffer, bytes_read);
    }
    
    if (ferror(input)) {
        fprintf(stderr, "Error reading input: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

static int write_hex_output(FILE *output, const uint8_t *hash, size_t hash_len) {
    for (size_t i = 0; i < hash_len; i++) {
        if (fprintf(output, "%02x", hash[i]) != 2) {
            fprintf(stderr, "Error writing hex output: %s\n", strerror(errno));
            return -1;
        }
    }
    if (fputc('\n', output) == EOF) {
        fprintf(stderr, "Error writing hex output: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int write_binary_output(FILE *output, const uint8_t *hash, size_t hash_len) {
    if (fwrite(hash, 1, hash_len, output) != hash_len) {
        fprintf(stderr, "Error writing binary output: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int process_input_output(const char *input_file, const char *output_file, int hash_size, int hex_input, int hex_output) {
    FILE *input = stdin;
    FILE *output = stdout;
    stribog_ctx_t ctx;
    uint8_t hash[STRIBOG_512_HASH_SIZE];
    int result = 0;

    // Открываем входной файл если указан
    if (input_file) {
        const char *mode = hex_input ? "r" : "rb";
        input = fopen(input_file, mode);
        if (!input) {
            fprintf(stderr, "Error: Cannot open input file '%s': %s\n", 
                    input_file, strerror(errno));
            return -1;
        }
    }

    // Открываем выходной файл если указан
    if (output_file) {
        const char *mode = hex_output ? "w" : "wb";
        output = fopen(output_file, mode);
        if (!output) {
            fprintf(stderr, "Error: Cannot open output file '%s': %s\n", 
                    output_file, strerror(errno));
            if (input != stdin) fclose(input);
            return -1;
        }
    }

    // Инициализируем контекст хеширования
    stribog_init(&ctx, hash_size);

    // Читаем данные в зависимости от формата
    if (hex_input) {
        if (read_hex_data(input, &ctx) != 0) {
            result = -1;
            goto cleanup;
        }
    } else {
        if (read_binary_data(input, &ctx) != 0) {
            result = -1;
            goto cleanup;
        }
    }

    // Финализируем хеш
    stribog_final(&ctx, hash);

    // Записываем результат
    size_t hash_bytes = (hash_size == 256) ? STRIBOG_256_HASH_SIZE : STRIBOG_512_HASH_SIZE;
    if (hex_output) {
        if (write_hex_output(output, hash, hash_bytes) != 0) {
            result = -1;
            goto cleanup;
        }
    } else {
        if (write_binary_output(output, hash, hash_bytes) != 0) {
            result = -1;
            goto cleanup;
        }
    }

cleanup:
    // Закрываем файлы если они были открыты
    if (input != stdin) fclose(input);
    if (output != stdout) fclose(output);

    return result;
}
