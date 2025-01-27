#ifndef CHUNKBIOMES_BRNG_H_
#define CHUNKBIOMES_BRNG_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cubiomes/rng.h"

#ifndef MIN
    #define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

// Only define optimization macros if not already defined
#if !defined(LIKELY) && (defined(__GNUC__) || defined(__clang__))
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define FORCE_INLINE __attribute__((always_inline)) static inline
    #define ALIGN_CACHE __attribute__((aligned(64)))
    #define HOT_FUNC __attribute__((hot))
    #define PURE_FUNC __attribute__((pure))
#else
    #define LIKELY(x) (x)
    #define UNLIKELY(x) (x)
    #define FORCE_INLINE static inline
    #define ALIGN_CACHE
    #define HOT_FUNC
    #define PURE_FUNC
#endif

#define MT_SIZE 624
#define MT_M 397
#define MT_MATRIX_A 0x9908b0dfU
#define MT_UPPER_MASK 0x80000000U
#define MT_LOWER_MASK 0x7fffffffU

typedef struct ALIGN_CACHE {
    uint32_t array[MT_SIZE];
    uint_fast16_t index;
} MersenneTwister;

// Optimized twist operation
FORCE_INLINE HOT_FUNC void _mTwist(MersenneTwister* const mt) {
    static const uint32_t mag01[2] = {0, MT_MATRIX_A};
    uint_fast16_t i;

    for (i = 0; i < MT_SIZE - MT_M; i++) {
        uint32_t y = (mt->array[i] & MT_UPPER_MASK) | (mt->array[i + 1] & MT_LOWER_MASK);
        mt->array[i] = mt->array[i + MT_M] ^ (y >> 1) ^ mag01[y & 1];
    }
    for (; i < MT_SIZE - 1; i++) {
        uint32_t y = (mt->array[i] & MT_UPPER_MASK) | (mt->array[i + 1] & MT_LOWER_MASK);
        mt->array[i] = mt->array[i - (MT_SIZE - MT_M)] ^ (y >> 1) ^ mag01[y & 1];
    }
    uint32_t y = (mt->array[MT_SIZE - 1] & MT_UPPER_MASK) | (mt->array[0] & MT_LOWER_MASK);
    mt->array[MT_SIZE - 1] = mt->array[MT_M - 1] ^ (y >> 1) ^ mag01[y & 1];

    mt->index = 0;
}

// Initialize with a seed
FORCE_INLINE HOT_FUNC void mSetSeed(MersenneTwister* const mt, const uint64_t seed, const int n) {
    if (LIKELY(n > 0)) {
        const size_t end = MIN(MT_SIZE - 1, (size_t)(n + 396));
        mt->array[0] = seed & 0xFFFFFFFF;

        for (size_t i = 1; i <= end; i++) {
            uint32_t prev = mt->array[i - 1];
            mt->array[i] = (1812433253U * (prev ^ (prev >> 30)) + i) & 0xFFFFFFFF;
        }
    }
    mt->index = MT_SIZE;
}

// Generate the next random value
FORCE_INLINE HOT_FUNC PURE_FUNC uint32_t _mNext(MersenneTwister* const mt) {
    if (UNLIKELY(mt->index >= MT_SIZE)) {
        _mTwist(mt);
    }

    uint32_t y = mt->array[mt->index++];
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680U;
    y ^= (y << 15) & 0xefc60000U;
    return y ^ (y >> 18);
}

// Generate a random integer in [0, n)
FORCE_INLINE HOT_FUNC PURE_FUNC int mNextInt(MersenneTwister* const mt, const int n) {
    if (LIKELY((n & (n - 1)) == 0)) {
        // Fast path for power-of-2 n
        return _mNext(mt) & (n - 1);
    }
    return _mNext(mt) % n;
}

// Generate an unbounded random integer
FORCE_INLINE HOT_FUNC PURE_FUNC int mNextIntUnbound(MersenneTwister* const mt) {
    return _mNext(mt) >> 1;
}

// Generate a random double in [0.0, 1.0)
FORCE_INLINE HOT_FUNC PURE_FUNC double mNextDouble(MersenneTwister* const mt) {
    return _mNext(mt) * (1.0 / 4294967296.0);
}

// Generate a random float in [0.0f, 1.0f)
FORCE_INLINE HOT_FUNC PURE_FUNC float mNextFloat(MersenneTwister* const mt) {
    return _mNext(mt) * (1.0f / 4294967296.0f);
}

// Generate a random boolean value
FORCE_INLINE HOT_FUNC PURE_FUNC bool mNextBool(MersenneTwister* const mt) {
    return _mNext(mt) & 1;
}

// Skip N values in the sequence
FORCE_INLINE HOT_FUNC void mSkipN(MersenneTwister* const mt, uint64_t n) {
    const uint64_t twists = (mt->index + n) / MT_SIZE;
    for (uint64_t i = 0; i < twists; i++) {
        _mTwist(mt);
    }
    mt->index = (mt->index + n) % MT_SIZE;
}

#endif // CHUNKBIOMES_BRNG_H_
