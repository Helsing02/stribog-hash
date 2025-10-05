#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>


#define MAX_DATA_SIZE (100 * 1024 * 1024) // 100MiB максимум

// генератор случайных данных
int generate_random_data(const char* filename, size_t size) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        perror("Failed to open output file");
        return -1;
    }

    FILE* urandom = fopen("/dev/urandom", "rb");
    if (!urandom) {
        perror("Failed to open /dev/urandom");
        fclose(file);
        return -1;
    }

    uint8_t* buffer = malloc(size);
    if (!buffer) {
        perror("Memory allocation failed");
        fclose(file);
        fclose(urandom);
        return -1;
    }

    // Читаем из /dev/urandom
    size_t bytes_read = fread(buffer, 1, size, urandom);
    if (bytes_read != size) {
        fprintf(stderr, "Failed to read enough random data: %zu/%zu\n", bytes_read, size);
        free(buffer);
        fclose(file);
        fclose(urandom);
        return -1;
    }

    // Записываем в файл
    size_t bytes_written = fwrite(buffer, 1, size, file);
    if (bytes_written != size) {
        fprintf(stderr, "Failed to write all data: %zu/%zu\n", bytes_written, size);
        free(buffer);
        fclose(file);
        fclose(urandom);
        return -1;
    }

    free(buffer);
    fclose(file);
    fclose(urandom);
    
    printf("Generated %zu bytes of random data: %s\n", size, filename);
    return 0;
}


int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <output_file> <size_in_bytes>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* filename = argv[1];
    char* endptr;
    errno = 0;
    unsigned long long size_ull = strtoull(argv[2], &endptr, 10);

    if (errno != 0 || *endptr != '\0') {
        fprintf(stderr, "Invalid size argument: '%s'\n", argv[2]);
        return EXIT_FAILURE;
    }

    if (size_ull > MAX_DATA_SIZE) {
        fprintf(stderr, "Requested size exceeds maximum allowed: %llu > %d\n", size_ull, MAX_DATA_SIZE);
        return EXIT_FAILURE;
    }

    size_t size = (size_t)size_ull;

    if (generate_random_data(filename, size) != 0) {
        fprintf(stderr, "Failed to generate random data\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}