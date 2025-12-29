#include <mmintrin.h>
#include <emmintrin.h>
#ifdef __SSE3__
# include <pmmintrin.h>
#endif

#ifdef __SSE3__
/*
 * "This intrinsic may perform better than _mm_loadu_si256 when
 * the data crosses a cache line boundary."
 */
# define MEM_READ_I256 _mm256_lddqu_si256
#else /* SSE2 */
# define MEM_READ_I256 _mm256_loadu_si256
#endif

/* load 512bit from unaligned memory  */
#define LOAD(P, xmm0, xmm1) { \
    const __m256i *__m256p = (const __m256i *) P; \
    xmm0 = MEM_READ_I256(&__m256p[0]); \
    xmm1 = MEM_READ_I256(&__m256p[1]); \
}

# define MEM_WRITE_I256  _mm256_storeu_si256

#define STORE(P, xmm0, xmm1) { \
    __m256i *__m256p = (__m256i *) &P[0]; \
    MEM_WRITE_I256(&__m256p[0], xmm0); \
    MEM_WRITE_I256(&__m256p[1], xmm1); \
}

#define X256R(xmm0, xmm1, xmm2, xmm3) { \
    xmm0 = _mm256_xor_si256(xmm0, xmm2); \
    xmm1 = _mm256_xor_si256(xmm1, xmm3); \
}

#define X256M(P, xmm0, xmm1) { \
    const __m256i *__m256p = (const __m256i *) &P[0]; \
    xmm0 = _mm256_xor_si256(xmm0, MEM_READ_I256(&__m256p[0])); \
    xmm1 = _mm256_xor_si256(xmm1, MEM_READ_I256(&__m256p[1])); \
}

#define XLPS256M(P, xmm0, xmm1) { \
    X256M(P, xmm0, xmm1); \
    LPS(xmm0, xmm1); \
}

#define XLPS256R(xmm0, xmm1, xmm2, xmm3) { \
    X256R(xmm2, xmm3, xmm0, xmm1); \
    LPS(xmm2, xmm3); \
}

#define ROUND256(i, xmm0, xmm1, xmm2, xmm3) { \
    XLPS256M((&C[i]), xmm0, xmm1); \
    XLPS256R(xmm0, xmm1, xmm2, xmm3); \
}

#define LPS(xmm0, xmm1) { \
    uint8_t block[64]; \
    STORE(block, xmm0, xmm1); \
    xmm0 = _mm256_setzero_si256(); \
    xmm1 = _mm256_setzero_si256(); \
    __m256i tmm0, tmm1; \
    for (int i = 0; i < 8; i++){ \
        tmm1 = _mm256_set_epi64x( \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 7]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 6]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 5]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 4]]  \
        ); \
        tmm0 = _mm256_set_epi64x( \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 3]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 2]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i + 1]], \
            L_MATRIX_PRECALC_BYTES[i][block[8 * i]]      \
        ); \
        X256R(xmm0, xmm1, tmm0, tmm1); \
    } \
} \
