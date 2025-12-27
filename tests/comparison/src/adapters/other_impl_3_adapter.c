#include <stdlib.h>

#include "../interface.h"
#include "../../other_impl_3/stribog.h"
// Контекст для накопления данных (так как их API работает за один вызов)
typedef struct {
    unsigned char* data;        // Накопленные данные
    size_t data_size;           // Текущий размер данных
    size_t capacity;            // Емкость буфера
    int hash_size;           // 256 или 512
    struct stribog_ctx_t lib_context; // Контекст из хедерфайла, нужный функциям этой реализации Стрибога
} help_context_t;

static int strange_init_256(void** context) {
    help_context_t* ctx = malloc(sizeof(help_context_t));
    if (!ctx) return -1;
    // Ну такая реализация что Стрибог-256=0, а Стрибог-512=1 ¯\_(ツ)_/¯
    init(&(ctx->lib_context), 0);

    ctx->data = NULL;
    ctx->data_size = 0;
    ctx->capacity = 0;
    ctx->hash_size = 256;

    *context = ctx;
    return 0;
}

static int strange_init_512(void** context) {
    help_context_t* ctx = malloc(sizeof(help_context_t));
    if (!ctx) return -1;
    init(&ctx->lib_context, 1);

    ctx->data = NULL;
    ctx->data_size = 0;
    ctx->capacity = 0;
    ctx->hash_size = 512;

    *context = ctx;
    return 0;
}

static int strange_update(void* context, const uint8_t* data, size_t len) {
    help_context_t* ctx = (help_context_t*)context;
    
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

static int strange_final(void* context, uint8_t* hash) {
    help_context_t* ctx = (help_context_t*)context;

    stribog(&ctx->lib_context, ctx->data, ctx->data_size);
    
    // Перевернуть порядок байт результата
    int hash_len = ctx->hash_size / 8;
    for (size_t i = 0; i < hash_len; i++) {
        hash[i] = (ctx->lib_context).h[hash_len - 1 - i];
    }

    // Очищаем буфер для возможного переиспользования контекста
    free(ctx->data);
    ctx->data = NULL;
    ctx->data_size = 0;
    ctx->capacity = 0;
    ctx->hash_size = 0;

    return 0;
}

static void strange_cleanup(void* context) {
    free(context);
}

// Экспортируем эту странную реализацию
hash_implementation strange_implementation = {
    .name = "really_strange",
    .init_256 = strange_init_256,
    .init_512 = strange_init_512,
    .update = strange_update,
    .final = strange_final,
    .cleanup = strange_cleanup
};

hash_implementation* get_hash_implementation(void) {
    return &strange_implementation;
}