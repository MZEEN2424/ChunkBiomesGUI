#ifndef CHUNKBIOMES_BFINDERS_H_
#define CHUNKBIOMES_BFINDERS_H_

#include "cubiomes/finders.h"
#include "Brng.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX
    #define MAX(X, Y) ((X) > (Y) ? (X) : (Y))
#endif

// Constants for seed mixing - using static const for better optimization
static const uint64_t REGION_SALT_X = UINT64_C(341873128712);
static const uint64_t REGION_SALT_Z = UINT64_C(132897987541);
static const int CHUNK_OFFSET = 8;

static inline uint64_t mix_seed(const uint64_t seed, const int regX, const int regZ, const uint64_t salt) {
    return regX * REGION_SALT_X + regZ * REGION_SALT_Z + seed + salt;
}

static inline Pos getBedrockFeatureChunkInRegion(const StructureConfig * const config, const uint64_t seed, const int regX, const int regZ) {
    MersenneTwister mt __attribute__((aligned(64)));
    const uint64_t mixedSeed = mix_seed(seed, regX, regZ, config->salt);
    mSetSeed(&mt, mixedSeed, 2);
    
    // Get both random numbers together for better pipelining
    const int range = config->chunkRange;
    Pos pos = {mNextInt(&mt, range), mNextInt(&mt, range)};
    return pos;
}

static inline Pos getBedrockFeaturePos(const StructureConfig * const config, const uint64_t seed, const int regX, const int regZ) {
    Pos pos = getBedrockFeatureChunkInRegion(config, seed, regX, regZ);
    
    // Use SIMD-friendly operations
    const uint64_t regionSize = config->regionSize;
    const uint64_t xBase = (uint64_t)regX * regionSize;
    const uint64_t zBase = (uint64_t)regZ * regionSize;
    
    // Combine operations for better vectorization
    pos.x = ((xBase + pos.x) << 4) + CHUNK_OFFSET;
    pos.z = ((zBase + pos.z) << 4) + CHUNK_OFFSET;
    
    return pos;
}

static inline Pos getBedrockLargeStructureChunkInRegion(const StructureConfig * const config, const uint64_t seed, const int regX, const int regZ) {
    MersenneTwister mt __attribute__((aligned(64)));
    const uint64_t mixedSeed = mix_seed(seed, regX, regZ, config->salt);
    mSetSeed(&mt, mixedSeed, 4);
    
    // Get all random numbers in one batch
    const int range = config->chunkRange;
    const int x1 = mNextInt(&mt, range);
    const int x2 = mNextInt(&mt, range);
    const int z1 = mNextInt(&mt, range);
    const int z2 = mNextInt(&mt, range);
    
    // Use bit shifts for division where possible
    Pos pos = {
        (x1 + x2) >> 1,  // Faster than /2
        (z1 + z2) >> 1
    };
    return pos;
}

static inline Pos getBedrockLargeStructurePos(const StructureConfig * const config, const uint64_t seed, const int regX, const int regZ) {
    Pos pos = getBedrockLargeStructureChunkInRegion(config, seed, regX, regZ);
    
    // SIMD-friendly operations
    const uint64_t regionSize = config->regionSize;
    const uint64_t xBase = (uint64_t)regX * regionSize;
    const uint64_t zBase = (uint64_t)regZ * regionSize;
    
    // Combine operations
    pos.x = ((xBase + pos.x) << 4) + CHUNK_OFFSET;
    pos.z = ((zBase + pos.z) << 4) + CHUNK_OFFSET;
    
    return pos;
}

static inline void bedrockChunkGenerateRnd(const uint64_t worldseed, const int chunkX, const int chunkZ, const int n, MersenneTwister * const mt) {
    mSetSeed(mt, worldseed, 2);
    
    // Get both random numbers at once
    const uint64_t r1 = mNextIntUnbound(mt);
    const uint64_t r2 = mNextIntUnbound(mt);
    
    // Combine operations
    const uint64_t mixedSeed = (r1 * chunkX) ^ (r2 * chunkZ) ^ worldseed;
    mSetSeed(mt, mixedSeed, n);
}

// Main interface functions
bool getBedrockStructureConfig(int structureType, int mc, StructureConfig *sconf);
bool getBedrockStructurePos(int structureType, int mc, uint64_t seed, int regX, int regZ, Pos *pos);
bool isViableBedrockStructurePos(int structureType, Generator *g, int blockX, int blockZ, uint32_t flags);
int getRavinePos(int mc, uint64_t seed, int x, int z, const Generator *g, StructureVariant *ravine1, StructureVariant *ravine2);

#ifdef __cplusplus
}
#endif

#endif // CHUNKBIOMES_BFINDERS_H_
