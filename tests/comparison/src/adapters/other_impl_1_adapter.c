#include <stdlib.h>
#include <string.h>

#include "../interface.h"
#include "../../other_impl_1/C/stribog.h"
// Контекст для накопления данных (так как их API работает за один вызов)
typedef struct {
    unsigned char* data;      // Накопленные данные
    size_t data_size;         // Текущий размер данных
    size_t capacity;          // Емкость буфера
    int hash_size;           // 256 или 512
} simple_context_t;

static int simple_init_256(void** context) {
    simple_context_t* ctx = malloc(sizeof(simple_context_t));
    if (!ctx) return -1;
    
    ctx->data = NULL;
    ctx->data_size = 0;
    ctx->capacity = 0;
    ctx->hash_size = 256;
    
    *context = ctx;
    return 0;
}

static int simple_init_512(void** context) {
    simple_context_t* ctx = malloc(sizeof(simple_context_t));
    if (!ctx) return -1;
    
    ctx->data = NULL;
    ctx->data_size = 0;
    ctx->capacity = 0;
    ctx->hash_size = 512;
    
    *context = ctx;
    return 0;
}

static int simple_update(void* context, const uint8_t* data, size_t len) {
    simple_context_t* ctx = (simple_context_t*)context;
    
    // Увеличиваем буфер если нужно
    if (ctx->data_size + len > ctx->capacity) {
        size_t new_capacity = ctx->capacity * 2;
        if (new_capacity < ctx->data_size + len) {
            new_capacity = ctx->data_size + len;
        }
        if (new_capacity < 4096) {
            new_capacity = 4096; // Минимальный размер буфера
        }
        
        unsigned char* new_data = realloc(ctx->data, new_capacity);
        if (!new_data) {
            return -1; // Ошибка выделения памяти
        }
        
        ctx->data = new_data;
        ctx->capacity = new_capacity;
    }
    
    // Сдвигаем уже накопленные данные вправо на len байт
    memmove(ctx->data + len, ctx->data, ctx->data_size);

    // Копируем данные в обратном порядке в начало буфера
    for (size_t i = 0; i < len; i++) {
        ctx->data[i] = data[len - 1 - i];
    }

    ctx->data_size += len;
    
    return 0;
}

static int simple_final(void* context, uint8_t* hash) {
    simple_context_t* ctx = (simple_context_t*)context;
    int result = 0;
    
    if (ctx->hash_size == 256) {
        hash_256(ctx->data, ctx->data_size * 8, hash);
    } else {
        hash_512(ctx->data, ctx->data_size * 8, hash);
    }

    // Перевернуть порядок байт результата
    int hash_len = ctx->hash_size / 8;
    for (size_t i = 0; i < hash_len / 2; i++) {
        uint8_t tmp = hash[i];
        hash[i] = hash[hash_len - 1 - i];
        hash[hash_len - 1 - i] = tmp;
    }
    
    // Очищаем буфер для возможного переиспользования контекста
    free(ctx->data);
    ctx->data = NULL;
    ctx->data_size = 0;
    ctx->capacity = 0;
    
    return result;
}

static void simple_cleanup(void* context) {
    simple_context_t* ctx = (simple_context_t*)context;
    free(ctx->data);
    free(ctx);
}

// Экспортируем реализацию
hash_implementation other_1_implementation = {
    .name = "other_1",
    .init_256 = simple_init_256,
    .init_512 = simple_init_512,
    .update = simple_update,
    .final = simple_final,
    .cleanup = simple_cleanup
};

// Функция для динамической загрузки
hash_implementation* get_hash_implementation(void) {
    return &other_1_implementation;
}