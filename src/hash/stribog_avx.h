/*
 * Данный файл содержит оптимизированную реализацию криптографических преобразований
 * с использованием SIMD-инструкций SSE2/SSE3/AVX для 256-битных операций.
 */

#include <mmintrin.h>    // MMX intrinsics
#include <emmintrin.h>   // SSE2 intrinsics
#ifdef __SSE3__
# include <pmmintrin.h>  // SSE3 intrinsics (если доступно)
#endif

/*
 * Выбор оптимальной инструкции для чтения из невыровненной памяти:
 * Для SSE3 используем _mm256_lddqu_si256 (лучше при пересечении границ кэш-линии)
 * Для SSE2 используем _mm256_loadu_si256 (обычная невыровненная загрузка)
 */
#ifdef __SSE3__
# define MEM_READ_I256 _mm256_lddqu_si256
#else /* SSE2 */
# define MEM_READ_I256 _mm256_loadu_si256
#endif

/*
 * LOAD - загрузка 512 бит (64 байт) из невыровненной памяти в два 256-битных регистра
 * Аргументы:
 *   P     - указатель на данные для загрузки
 *   xmm0  - первый 256-битный регистр (нижние 32 байта)
 *   xmm1  - второй 256-битный регистр (верхние 32 байта)
 */
#define LOAD(P, xmm0, xmm1) { \
    const __m256i *__m256p = (const __m256i *) P; \
    xmm0 = MEM_READ_I256(&__m256p[0]); \
    xmm1 = MEM_READ_I256(&__m256p[1]); \
}

/* Инструкция для записи в невыровненную память */
# define MEM_WRITE_I256  _mm256_storeu_si256

/*
 * STORE - сохранение 512 бит (64 байт) из двух 256-битных регистров в память
 * Аргументы:
 *   P     - указатель на буфер назначения
 *   xmm0  - первый 256-битный регистр (нижние 32 байта)
 *   xmm1  - второй 256-битный регистр (верхние 32 байта)
 */
#define STORE(P, xmm0, xmm1) { \
    __m256i *__m256p = (__m256i *) &P[0]; \
    MEM_WRITE_I256(&__m256p[0], xmm0); \
    MEM_WRITE_I256(&__m256p[1], xmm1); \
}

/*
 * X256R - побитовый XOR двух пар 256-битных регистров
 * Результат сохраняется в первых двух регистрах:
 *   xmm0 = xmm0 XOR xmm2
 *   xmm1 = xmm1 XOR xmm3
 */
#define X256R(xmm0, xmm1, xmm2, xmm3) { \
    xmm0 = _mm256_xor_si256(xmm0, xmm2); \
    xmm1 = _mm256_xor_si256(xmm1, xmm3); \
}

/*
 * X256M - побитовый XOR 256-битных регистров с данными из памяти
 *   xmm0 = xmm0 XOR (память по адресу P[0])
 *   xmm1 = xmm1 XOR (память по адресу P[32])
 */
#define X256M(P, xmm0, xmm1) { \
    const __m256i *__m256p = (const __m256i *) &P[0]; \
    xmm0 = _mm256_xor_si256(xmm0, MEM_READ_I256(&__m256p[0])); \
    xmm1 = _mm256_xor_si256(xmm1, MEM_READ_I256(&__m256p[1])); \
}

/*
 * XLPS256M - комбинированная операция: XOR с памятью + преобразование LPS
 * Выполняет:
 *   1. X256M(P, xmm0, xmm1) - XOR регистров с данными из памяти
 *   2. LPS(xmm0, xmm1)     - нелинейное преобразование
 */
#define XLPS256M(P, xmm0, xmm1) { \
    X256M(P, xmm0, xmm1); \
    LPS(xmm0, xmm1); \
}

/*
 * XLPS256R - комбинированная операция: XOR регистров + преобразование LPS
 * Особенность: результат записывается во ВТОРУЮ пару регистров
 * Выполняет:
 *   1. X256R(xmm2, xmm3, xmm0, xmm1) - xmm2/xmm3 = xmm2/xmm3 XOR xmm0/xmm1
 *   2. LPS(xmm2, xmm3)              - преобразование результата
 */
#define XLPS256R(xmm0, xmm1, xmm2, xmm3) { \
    X256R(xmm2, xmm3, xmm0, xmm1); \
    LPS(xmm2, xmm3); \
}

/*
 * ROUND256 - один раунд преобразования
 * Выполняет:
 *   1. XLPS256M с константой раунда C[i]
 *   2. XLPS256R с переданными регистрами
 *
 * Аргументы:
 *   i     - номер раунда (индекс в массиве констант C)
 *   xmm0-xmm3 - 4 регистра, представляющие состояние
 */
#define ROUND256(i, xmm0, xmm1, xmm2, xmm3) { \
    XLPS256M((&C[i]), xmm0, xmm1); \
    XLPS256R(xmm0, xmm1, xmm2, xmm3); \
}

/*
 * LPS (Linear-Permutation-Substitution) - базовое нелинейное преобразование
 * Выполняет линейное преобразование над 64-байтным блоком данных
 * с использованием заранее вычисленной матрицы L_MATRIX_PRECALC_BYTES
 *
 * Алгоритм:
 *   1. Сохраняет 512 бит из регистров во временный буфер
 *   2. Обнуляет регистры-аккумуляторы
 *   3. Для каждого байта в блоке (8 групп по 8 байт):
 *      - вычисляет 64-битное значение из таблицы подстановки
 *      - выполняет XOR с аккумуляторами
 *
 * L_MATRIX_PRECALC_BYTES - предвычисленная матрица 8x256, где каждая строка
 * соответствует своей позиции байта в 64-байтном блоке
 */
#define LPS(xmm0, xmm1) { \
    uint8_t block[64]; \
    STORE(block, xmm0, xmm1); \
    xmm0 = _mm256_setzero_si256(); \
    xmm1 = _mm256_setzero_si256(); \
    __m256i tmm0, tmm1; \
    for (int i = 0; i < 8; i++){ \
        /* Обработка старших 4 байт группы (байты 7-4) */ \
        tmm1 = _mm256_set_epi64x( \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 7]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 6]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 5]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 4]]  \
        ); \
        /* Обработка младших 4 байт группы (байты 3-0) */ \
        tmm0 = _mm256_set_epi64x( \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 3]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 2]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 1]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i]]      \
        ); \
        X256R(xmm0, xmm1, tmm0, tmm1); \
    } \
} \
