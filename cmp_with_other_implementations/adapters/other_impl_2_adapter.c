#include <stdlib.h>

#include "../interface.h"
#include "../other_impl_2/gost_3411_2012_calc.h"

static int other_init_256(void** context) {
    TGOSTHashContext* ctx = malloc(sizeof(TGOSTHashContext));
    if (!ctx) return -1;
    GOSTHashInit(ctx, 256);
    *context = ctx;
    return 0;
}

static int other_init_512(void** context) {
    TGOSTHashContext* ctx = malloc(sizeof(TGOSTHashContext));
    if (!ctx) return -1;
    GOSTHashInit(ctx, 512);
    *context = ctx;
    return 0;
}

static int other_update(void* context, const uint8_t* data, size_t len) {
    TGOSTHashContext* ctx = (TGOSTHashContext*)context;
    GOSTHashUpdate(ctx, data, len);
    return 0;
}

static int other_final(void* context, uint8_t* hash) {
    TGOSTHashContext* ctx = (TGOSTHashContext*)context;
    GOSTHashFinal(ctx);
    if (ctx->hash_size) {
        memcpy(hash, ctx->hash  + ctx->hash_size / 8, ctx->hash_size / 8);
    } else {
        memcpy(hash, ctx->hash, ctx->hash_size / 8);
    }
    return 0;
}

static void other_cleanup(void* context) {
    free(context);
}

// Экспортируем нашу реализацию
hash_implementation other_2_implementation = {
    .name = "other_2",
    .init_256 = other_init_256,
    .init_512 = other_init_512,
    .update = other_update,
    .final = other_final,
    .cleanup = other_cleanup
};

hash_implementation* get_hash_implementation(void) {
    return &other_2_implementation;
}