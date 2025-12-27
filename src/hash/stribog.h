#ifndef STRIBOG_H
#define STRIBOG_H

#include <stdint.h> // Для uint64_t и других целочисленных типов
#include <stddef.h> // Для size_t

// Размер блока в байтах для Stribog (512 бит)
#define STRIBOG_BLOCK_SIZE 64
// Размер хеша в байтах для режима 512 бит
#define STRIBOG_512_HASH_SIZE 64
// Размер хеша в байтах для режима 256 бит
#define STRIBOG_256_HASH_SIZE 32


// Контекст хеширования. Содержит промежуточное состояние.
typedef struct {
    // Текущее значение хеша (h)
    uint8_t h[STRIBOG_BLOCK_SIZE];
    // Текущее значение N
    uint8_t N[STRIBOG_BLOCK_SIZE];
    // Текущее значение Сигмы 
    uint8_t Sigma[STRIBOG_BLOCK_SIZE];
    // Накопитель для данных, которые еще не обработаны (блок размером 512 бит)
    uint8_t buffer[STRIBOG_BLOCK_SIZE];
    // Количество байтов, находящихся в буфере
    uint8_t buffer_size;
    // Общая длина сообщения в битах (используется на этапе дополнения)
    uint64_t total_bits;
    // Размер выходного хеша: 256 или 512
    uint16_t hash_size;
} stribog_ctx_t;

// Функции инициализации контекста
void stribog_init(stribog_ctx_t *ctx, uint16_t hash_size);
// Функция обновления состояния. Добавляет данные для хеширования.
// Может вызываться многократно для больших данных.
void stribog_update(stribog_ctx_t *ctx, const uint8_t *data, size_t len);
// Функция финализации. Выполняет дополнение сообщения и финальные вычисления.
// Результат помещается в `hash`. Указатель `hash` должен указывать на область памяти
// размером не менее STRIBOG_512_HASH_SIZE байтов.
void stribog_final(stribog_ctx_t *ctx, uint8_t *hash);


#endif // STRIBOG_H
