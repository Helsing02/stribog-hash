#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../src/hash/stribog.h"


void print_hash(const uint8_t* hash, size_t len) {
    for (size_t i = 0; i < len; i++) {
        printf("%02x", hash[i]);
    }
}

// Функция преобразования hex строки в байты
int hex_to_bytes(const char* hex, uint8_t* bytes, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (sscanf(hex + i * 2, "%2hhx", &bytes[i]) != 1) {
            return 0;
        }
    }
    return 1;
}

int compare_hash(const uint8_t* hash1, const uint8_t* hash2, size_t len) {
    return memcmp(hash1, hash2, len) == 0;
}


// Универсальная функция тестирования
int test_gost_example(const char *input, const uint8_t *expected_256, const uint8_t *expected_512) {    
    stribog_ctx_t ctx;
    uint8_t hash[64];
    int success = 1;
    
    // Тестируем 256-битную версию
    if (expected_256) {
        stribog_init(&ctx, 256);
        stribog_update(&ctx, (const uint8_t*)input, strlen(input));
        stribog_final(&ctx, hash);
        
        if (!compare_hash(hash, expected_256, 32)) {
            printf("  Stribog-256 failed\n");
            printf("  Expected: "); print_hash(expected_256, 32); printf("\n");
            printf("  Got:      "); print_hash(hash, 32); printf("\n");
            success = 0;
        }
    }
    
    // Тестируем 512-битную версию
    if (expected_512) {
        stribog_init(&ctx, 512);
        stribog_update(&ctx, (const uint8_t*)input, strlen(input));
        stribog_final(&ctx, hash);
        
        if (!compare_hash(hash, expected_512, 64)) {
            printf("  Stribog-512 failed\n");
            printf("  Expected: "); print_hash(expected_512, 64); printf("\n");
            printf("  Got:      "); print_hash(hash, 64); printf("\n");
            success = 0;
        }
    }
    
    printf("result %d\n", success);
    return success;
}

// Макрос для простого тестирования
#define RUN_TEST(name, input, expected_256_hex, expected_512_hex) do { \
    printf("TEST: %s\n", name); \
    \
    uint8_t expected_256[32] = {0}; \
    uint8_t expected_512[64] = {0}; \
    \
    if (!hex_to_bytes(expected_256_hex, expected_256, 32)) { \
        printf("FAIL: Invalid 256-bit hex string\n\n"); \
        failed++; \
    } else if (!hex_to_bytes(expected_512_hex, expected_512, 64)) { \
        printf("FAIL: Invalid 512-bit hex string\n\n"); \
        failed++; \
    } else if (test_gost_example(input, expected_256, expected_512)) { \
        printf("PASS\n\n"); \
        passed++; \
    } else { \
        printf("FAIL\n\n"); \
        failed++; \
    } \
    total++; \
} while(0)


// Основная функция
int main() {
    printf("=== Stribog Control Examples Test ===\n\n");
    
    int total = 0, passed = 0, failed = 0;
    
    RUN_TEST(
    	"Numbers (less then one block)", 
    	"012345678901234567890123456789012345678901234567890123456789012",
    	"9d151eefd8590b89daa6ba6cb74af9275dd051026bb149a452fd84e5e57b5500",
    	"1b54d01a4af5b9d5cc3d86d68d285462b19abc2475222f35c085122be4ba1ffa00ad30f8767b3a82384c6574f024c311e2a481332b08ef7f41797891c1646f48"
    );

    const unsigned char second_test[] = {
        0xd1, 0xe5, 0x20, 0xe2, 0xe5, 0xf2, 0xf0, 0xe8, 0x2c, 
        0x20, 0xd1, 0xf2, 0xf0, 0xe8, 0xe1, 0xee, 0xe6, 0xe8, 
        0x20, 0xe2, 0xed, 0xf3, 0xf6, 0xe8, 0x2c, 0x20, 0xe2, 
        0xe5, 0xfe, 0xf2, 0xfa, 0x20, 0xf1, 0x20, 0xec, 0xee, 
        0xf0, 0xff, 0x20, 0xf1, 0xf2, 0xf0, 0xe5, 0xeb, 0xe0, 
        0xec, 0xe8, 0x20, 0xed, 0xe0, 0x20, 0xf5, 0xf0, 0xe0, 
        0xe1, 0xf0, 0xfb, 0xff, 0x20, 0xef, 0xeb, 0xfa, 0xea, 
        0xfb, 0x20, 0xc8, 0xe3, 0xee, 0xf0, 0xe5, 0xe2, 0xfb, 
        0x00
    };
    RUN_TEST(
        "Words (more then one block)", 
        (const char *)second_test,
        "9dd2fe4e90409e5da87f53976d7405b0c0cac628fc669a741d50063c557e8f50",
        "1e88e62226bfca6f9994f1f2d51569e0daf8475a3b0fe61a5300eee46d961376035fe83549ada2b8620fcd7c496ce5b33f0cb9dddc2b6460143b03dabac9fb28"
    );
    
        
    printf("=== Results: %d/%d tests passed ===\n", passed, total);
    
    return failed > 0 ? 1 : 0;
}
