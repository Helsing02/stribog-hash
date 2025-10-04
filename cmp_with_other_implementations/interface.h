#ifndef HASH_INTERFACE_H
#define HASH_INTERFACE_H

#include <stdint.h>
#include <stddef.h>

// Универсальная структура для любой реализации
typedef struct {
    const char* name;
    int (*init_256)(void** context);
    int (*init_512)(void** context);
    int (*update)(void* context, const uint8_t* data, size_t len);
    int (*final)(void* context, uint8_t* hash);
    void (*cleanup)(void* context);
} hash_implementation;

// Результат тестирования
typedef struct {
    const char* implementation;
    const char* test_file;
    size_t data_size;
    uint8_t hash_256[32];
    uint8_t hash_512[64];
    int status; // 0 = success, -1 = error
} test_result;

#endif