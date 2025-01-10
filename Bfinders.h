#ifndef __BFINDERS_H
#define __BFINDERS_H

#include "cubiomes/finders.h"
#include "Brng.h"

bool getBedrockStructureConfig(const int structureType, const int mc, StructureConfig *sconf);

static inline ATTR(const)
Pos getBedrockFeatureChunkInRegion(const StructureConfig *config, uint64_t seed, int regX, int regZ) {
	MersenneTwister mt;
	mSetSeed(&mt, regX*UINT64_C(341873128712) + regZ*UINT64_C(132897987541) + seed + config->salt, 2);
	Pos pos;
	pos.x = mNextInt(&mt, config->chunkRange);
	pos.z = mNextInt(&mt, config->chunkRange);
	return pos;
}

static inline ATTR(const)
Pos getBedrockFeaturePos(const StructureConfig *config, uint64_t seed, int regX, int regZ) {
	Pos pos = getBedrockFeatureChunkInRegion(config, seed, regX, regZ);
	// Bedrock features are offset by +8.
	pos.x = (((uint64_t)regX*config->regionSize + pos.x) << 4) + 8;
	pos.z = (((uint64_t)regZ*config->regionSize + pos.z) << 4) + 8;
	return pos;
}

static inline ATTR(const)
Pos getBedrockLargeStructureChunkInRegion(const StructureConfig *config, uint64_t seed, int regX, int regZ) {
	MersenneTwister mt;
	mSetSeed(&mt, regX*UINT64_C(341873128712) + regZ*UINT64_C(132897987541) + seed + config->salt, 4);
	Pos pos;
	pos.x = (mNextInt(&mt, config->chunkRange) + mNextInt(&mt, config->chunkRange))/2;
	pos.z = (mNextInt(&mt, config->chunkRange) + mNextInt(&mt, config->chunkRange))/2;
	return pos;
}

static inline ATTR(const)
Pos getBedrockLargeStructurePos(const StructureConfig *config, uint64_t seed, int regX, int regZ) {
	Pos pos = getBedrockLargeStructureChunkInRegion(config, seed, regX, regZ);
	// Bedrock features are offset by +8.
	pos.x = (((uint64_t)regX*config->regionSize + pos.x) << 4) + 8;
	pos.z = (((uint64_t)regZ*config->regionSize + pos.z) << 4) + 8;
	return pos;
}

bool getBedrockStructurePos(int structureType, int mc, uint64_t seed, int regX, int regZ, Pos *pos);

/* Returns the number of ravines found.
   For 1.17 and earlier, two ravine checks are performed per coordinate: one "ordinary" one and a second for ocean ravines.
   If the second is desired, `g` must be provided and have been initialized; in all other cases, `g` can be set to NULL.
   If not NULL, the position and size of the ravines is stored in `ravine1` and `ravine2`.*/
int getRavinePos(int mc, uint64_t seed, int x, int z, const Generator *g, StructureVariant *ravine1, StructureVariant *ravine2);

/* Returns if a ravine was found.
   If not NULL, the position and size of the ravine, and whether it is giant, are stored in `ravine`.*/
// Needs fixing
// bool getBedrockRavinePos(uint64_t seed, int x, int z, StructureVariant *ravine);

/* Returns the number of potential strongholds for a given seed */
int getBedrockStronghold(uint64_t seed);

#endif