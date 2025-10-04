#include <stdlib.h>

#include "../interface.h"
#include "../../src/hash/stribog.h"

static int our_init_256(void** context) {
    stribog_ctx_t* ctx = malloc(sizeof(stribog_ctx_t));
    if (!ctx) return -1;
    stribog_init(ctx, 256);
    *context = ctx;
    return 0;
}

static int our_init_512(void** context) {
    stribog_ctx_t* ctx = malloc(sizeof(stribog_ctx_t));
    if (!ctx) return -1;
    stribog_init(ctx, 512);
    *context = ctx;
    return 0;
}

static int our_update(void* context, const uint8_t* data, size_t len) {
    stribog_ctx_t* ctx = (stribog_ctx_t*)context;
    stribog_update(ctx, data, len);
    return 0;
}

static int our_final(void* context, uint8_t* hash) {
    stribog_ctx_t* ctx = (stribog_ctx_t*)context;
    stribog_final(ctx, hash);
    return 0;
}

static void our_cleanup(void* context) {
    free(context);
}

// Экспортируем нашу реализацию
hash_implementation my_implementation = {
    .name = "our_stribog",
    .init_256 = our_init_256,
    .init_512 = our_init_512,
    .update = our_update,
    .final = our_final,
    .cleanup = our_cleanup
};

// Функция для динамической загрузки
hash_implementation* get_hash_implementation(void) {
    return &my_implementation;
}