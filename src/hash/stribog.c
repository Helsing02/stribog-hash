#include <immintrin.h>
#include <stdalign.h>
#include "stribog.h"
#include "stribog_const.h"
#include "stribog_avx.h"
#include <string.h> // Для memcpy, memset
#include <stdio.h>


// Функция сжатия g
static void stribog_g_transform(uint8_t *N, uint8_t *h, const uint8_t *m, uint8_t *result) {
    /*
    Функция реализует преобразование ``g`` ГОСТа.
    Ожидает все массивы с порядком байтов от младшего к старшему (байт с нулевым индексом содержит биты с 7 по 0).
    Возвращает массив с аналогичным порядоком байтов.

    В данной реализации использует макросы над SIMD инструкциями.
    */

    __m256i xmm0, xmm1;
    __m256i xmm2, xmm3;
    unsigned int i;

    LOAD(N, xmm0, xmm1);
    XLPS256M(h, xmm0, xmm1);            // Выполняем LPSX с N и h для выработки первого ключа K

    LOAD(m, xmm2, xmm3);
    XLPS256R(xmm0, xmm1, xmm2, xmm3);   // Выполняем LPSX с m и K в качестве первого раунда

    for (i = 0; i < 11; i++) {
        ROUND256(i, xmm0, xmm1, xmm2, xmm3);    // Выполняем еще 11 раундов
    }

    XLPS256M((&C[11]), xmm0, xmm1);     // Вычисляем 12 ключ K
    X256R(xmm0, xmm1, xmm2, xmm3);      // Складываем его с результатом раундовой обработки

    X256M(h, xmm0, xmm1);               // Складываем с h
    X256M(m, xmm0, xmm1);               // Складываем с m

    STORE(result, xmm0, xmm1);
}

// Сложение двух 512-битных чисел по модулю 2^512
static void stribog_add_array_modulo_512(uint8_t *a, const uint8_t *b) {
    /*
    Функция сложения (по модулю 2^512) двух 512 битных чисел, представленных массивами байтов.
    Ожидает все массивы с порядком байтов от младшего к старшему (байт с нулевым индексом содержит биты с 7 по 0).
    Возвращает массив с аналогичным порядоком байтов.
    */
    unsigned long long *pa = (unsigned long long *)a;
    const unsigned long long *pb = (const unsigned long long *)b;

    // Используем встроенные функции для лучшей оптимизации
    unsigned long long carry = 0ULL;

    for (int i = 0; i < STRIBOG_BLOCK_SIZE / 8; i++) {
        carry = _addcarry_u64(carry, pa[i], pb[i], &pa[i]);
    }
}

static void stribog_add_number_modulo_512(uint8_t *a, uint64_t b) {
    /*
    Функция сложения (по модулю 2^512) двух чисел, одно из которых представлено массивом, а второе 64-битным числом.
    Ожидает массив с порядком байтов от младшего к старшему (байт с нулевым индексом содержит биты с 7 по 0).
    Возвращает массив с аналогичным порядоком байтов.
    */
    unsigned long long *pa = (unsigned long long *)a;
    // Используем встроенные функции для лучшей оптимизации
    unsigned long long carry = _addcarry_u64(0, pa[0], b, &pa[0]);

    for (int i = 1; i < STRIBOG_BLOCK_SIZE / 8 && carry; i++) {
        carry = _addcarry_u64(carry, pa[i], 0, &pa[i]);
    }
}

void stribog_init(stribog_ctx_t *ctx, uint16_t hash_size) {
    /*
    Функция инициализации контекста хеширования.
    Обнуляет структуру контекста, записывает и выставляет переменные,
    указанные в 1 этапе алгоритма из ГОСТа.
    */
    // Обнуляем структуру контекста
    memset(ctx, 0, sizeof(stribog_ctx_t));

    // Устанавливаем размер хеша
    ctx->hash_size = hash_size;

    // Инициализируем начальное значение h.
    // Для Stribog-512: h = 0x00...00 (инициализация нулями уже выполнена memset'ом)
    // Для Stribog-256: h = 0x010101...01
    if (hash_size == 256) {
        memset(ctx->h, 0x01, STRIBOG_BLOCK_SIZE);
    }
}

void stribog_process_block(stribog_ctx_t *ctx, const uint8_t *m) {
    /*
    Вспомогательная функция обработки блока.
    Выполняет основные преобразования 2 этапа ГОСТа.
    */
    // Вызываем преобразование gN(h, m)
    stribog_g_transform(ctx->N, ctx->h, m, ctx->h);
    // Прибавляем 512 к N по модулю 2^512
    stribog_add_number_modulo_512(ctx->N, (uint64_t)512);
    // Прибавляем числовое представление m к Сигме
    stribog_add_array_modulo_512(ctx->Sigma, m);
}

void stribog_update(stribog_ctx_t *ctx, const uint8_t *data, size_t len){
    /*
    Принимает конфигурацию хеша, массив байтов, и размер массива.
    Важно заметить, что по нулевому индексу массива должен лежать первый байт блока сообщения,
    несмотря на то, что в ГОСТе все 64 байтовые массивы представлены в обратном порядке байтов.
    */

    // Увеличиваем счетчик бит исходного сообщения на длину очередного блока в битах
    ctx->total_bits += (uint64_t)len * 8;

    // Быстрая обработка полных блоков
    while (len >= STRIBOG_BLOCK_SIZE) {
        if (ctx->buffer_size == 0) {
            // Прямая обработка без копирования в буфер
            stribog_process_block(ctx, data);
            data += STRIBOG_BLOCK_SIZE;
            len -= STRIBOG_BLOCK_SIZE;
        } else {
            // Дозаполняем буфер и обрабатываем
            size_t to_copy = STRIBOG_BLOCK_SIZE - ctx->buffer_size;
            memcpy(ctx->buffer + ctx->buffer_size, data, to_copy);
            stribog_process_block(ctx, ctx->buffer);
            ctx->buffer_size = 0;
            data += to_copy;
            len -= to_copy;
        }
    }

    // Остаток в буфер
    if (len > 0) {
        memcpy(ctx->buffer + ctx->buffer_size, data, len);
        ctx->buffer_size += len;
    }
}

void stribog_final(stribog_ctx_t *ctx, uint8_t *hash) {
    /*
    Функция завершения хеширования. Вызывается в случае,
    когда данные для хеширования закончились, и необходимо получить значение хеша.
    Реализует 3 этап в алгоритме из ГОСТа.
    Возвращает хеш в порядке: в нулевой ячейке массива лежит нулевой (последний в нотации ГОСТа) байт хеша.
    */
    // Дополнение сообщения
    // Добавляем бит '1'
    ctx->buffer[ctx->buffer_size] = 0x01;
    ctx->buffer_size++;

    // Заполняем оставшуюся часть буфера нулями
    memset(ctx->buffer + ctx->buffer_size, 0, STRIBOG_BLOCK_SIZE - ctx->buffer_size);

    // Обрабатываем последнюю часть сообщения
    stribog_g_transform(ctx->N, ctx->h, ctx->buffer, ctx->h);
    // Прибавляем к N длину обработанного сообщения в битах
    stribog_add_number_modulo_512(ctx->N, (ctx->buffer_size - 1) * 8);
    // Прибавляем к Сигме последний блок сообщения, преобразованный в число
    stribog_add_array_modulo_512(ctx->Sigma, ctx->buffer);

    // Вызываем g с нулевым индексом и параметрами g0(h, N)
    uint8_t zero[STRIBOG_BLOCK_SIZE] = {0};
    stribog_g_transform(zero, ctx->h, ctx->N, ctx->h);
    // Вызываем g с нулевым индексом и параметрами g0(h, Sigma)
    stribog_g_transform(zero, ctx->h, ctx->Sigma, hash);

    // Для Stribog-256 берем только первые 256 бит (4 элемента из 8)
    if (ctx->hash_size == 256) {
        memcpy(hash, &hash[32], 32);
        memset(&hash[32], 0, 32);
    }
}
