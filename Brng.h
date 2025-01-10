#ifndef __BRNG_H
#define __BRNG_H

#include "cubiomes/rng.h"
#include <stdbool.h>

#ifndef min
    #define min(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

/* ==================
    Mersenne Twister
   ================== */

STRUCT(MersenneTwister) {
    uint_fast32_t array[624];
    uint_fast16_t currentIndex;
};

// Initializes a Mersenne Twister for `n` advancements, or fully initializes it if `n` <= 0.
static inline void mSetSeed(MersenneTwister *mt, uint64_t seed, int n) {
    if (n > 0) n += 397;
    mt->array[0] = seed;
    // (size_t)(n - 1) intentionally underflows if n <= 0
    for (size_t i = 1; i <= min(sizeof(mt->array)/sizeof(*mt->array) - 1, (size_t)(n - 1)); ++i) {
        seed = mt->array[i - 1] ^ (mt->array[i - 1] >> 30);
        mt->array[i] = 1812433253 * seed + i;
    }
    mt->currentIndex = sizeof(mt->array)/sizeof(*mt->array);
}

// Twist the PRNG, "scrambling" the internal state.
static inline void _mTwist(MersenneTwister *mt) {
    const size_t M = 397, J = sizeof(mt->array)/sizeof(*mt->array) - M;

    for (size_t i = 0; i < J; ++i) {
        uint32_t val = (mt->array[i] & 0x80000000) | (mt->array[i + 1] & 0x7fffffff);
        mt->array[i] = mt->array[i + M] ^ (val >> 1) ^ (val & 1 ? 2567483615 : 0);
    }
    for (size_t i = J; i < sizeof(mt->array)/sizeof(*mt->array) - 1; ++i) {
        uint32_t val = (mt->array[i] & 0x80000000) | (mt->array[i + 1] & 0x7fffffff);
        mt->array[i] = mt->array[i - J] ^ (val >> 1) ^ (val & 1 ? 2567483615 : 0);
    }
    uint32_t val = (mt->array[sizeof(mt->array)/sizeof(*mt->array) - 1] & 0x80000000) | (mt->array[0] & 0x7fffffff);
    mt->array[sizeof(mt->array)/sizeof(*mt->array) - 1] = mt->array[M - 1] ^ (val >> 1) ^ (val & 1 ? 2567483615 : 0);
    mt->currentIndex = 0;
}

// Returns the next unsigned 32-bit integer from the PRNG.
static inline uint32_t _mNext(MersenneTwister *mt) {
    // If the current index is greater than the length of the array, twist the array's values.
    if (mt->currentIndex >= sizeof(mt->array)/sizeof(*mt->array)) _mTwist(mt);
    uint32_t val = mt->array[mt->currentIndex];
    ++mt->currentIndex;
    val ^= val >> 11;
    val ^= (val << 7) & 2636928640;
    val ^= (val << 15) & 4022730752;
    return val ^ (val >> 18);
}

// Returns the next integer in the range [0,n).
static inline int mNextInt(MersenneTwister *mt, const int n) {
    return _mNext(mt) % n;
}

// static inline int mNextInt2(MersenneTwister *mt, const int n) {
//     int bits, val;
//     const int m = n - 1;

//     if (!(m & n)) {
//         uint64_t x = n * (uint64_t)(_mNext(mt) >> 1);
//         return (int64_t)x >> 31;
//     }
//     do {
//         bits = _mNext(mt) >> 1;
//         val = bits % n;
//     }
//     while (bits - val + m < 0);
//     return val;
// }

// static inline int mNextInt3(MersenneTwister *mt, const int n) {
//     uint64_t r = (uint64_t)_mNext(mt) * n;
//     if ((uint32_t)r < n) {
//         while ((uint32_t)r < (~n + 1) % n) {
//             r = (uint64_t)_mNext(mt) * n;
//         }
//     }
//     return r >> 32;
// }

// Returns the next non-negative integer.
static inline int mNextIntUnbound(MersenneTwister *mt) {
    return _mNext(mt) >> 1;
}

// Returns the next double in the range [0,1).
static inline double mNextDouble(MersenneTwister *mt) {
    return _mNext(mt) * 2.3283064365386963E-10;
}

// Returns the next float in the range [0,1).
static inline float mNextFloat(MersenneTwister *mt) {
    return mNextDouble(mt);
}

// Returns a pseudorandom boolean value.
static inline bool mNextBool(MersenneTwister *mt) {
    return _mNext(mt) >> 31;
}

// Jumps the Mersenne Twister forward `n` calls.
static inline void mSkipN(MersenneTwister *mt, uint64_t n) {
    uint64_t mIndex = mt->currentIndex + n; // Separate variable because mt->currentIndex is only 16 bits, while n can be up to 64
    for (uint64_t i = 0; i < mIndex / (sizeof(mt->array)/sizeof(*mt->array)); ++i) _mTwist(mt);
    mt->currentIndex = mIndex % sizeof(mt->array)/sizeof(*mt->array);
}

#endif