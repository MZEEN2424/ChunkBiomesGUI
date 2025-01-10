#include "Bfinders.h"
#include "cubiomes/util.h"
#include <stdio.h>
#include <string.h>


bool getBedrockStructureConfig(const int structureType, const int mc, StructureConfig *sconf) {
	static const StructureConfig
	s_ancient_city    = { 20083232, 24, 16, Ancient_City,    DIM_OVERWORLD,0}, // Found by Fragrant_Result_186
	s_desert_pyramid  = { 14357617, 32, 24, Desert_Pyramid,  DIM_OVERWORLD,0},
	s_igloo           = { 14357617, 32, 24, Igloo,           DIM_OVERWORLD,0},
	s_jungle_pyramid  = { 14357617, 32, 24, Jungle_Pyramid,  DIM_OVERWORLD,0},
	s_mansion         = { 10387319, 80, 60, Mansion,         DIM_OVERWORLD,0},
	s_monument        = { 10387313, 32, 27, Monument,        DIM_OVERWORLD,0},
	s_outpost         = {165745296, 80, 56, Outpost,         DIM_OVERWORLD,0},
	s_ruined_portal   = { 40552231, 40, 25, Ruined_Portal,   DIM_OVERWORLD,0},
	s_shipwreck_117   = {165745295, 10,  5, Shipwreck,       DIM_OVERWORLD,0},
	s_shipwreck       = {165745295, 24, 20, Shipwreck,       DIM_OVERWORLD,0},
	s_swamp_hut       = { 14357617, 32, 24, Swamp_Hut,       DIM_OVERWORLD,0},
	// s_treasure        = { 16842397,  4,  2, Feature,         DIM_OVERWORLD,0},
	s_village_117     = { 10387312, 27, 17, Village,         DIM_OVERWORLD,0},
	s_village         = { 10387312, 34, 26, Village,         DIM_OVERWORLD,0},
	// Nether structures
	s_bastion         = { 30084232, 30, 26, Bastion,         DIM_NETHER,0},
	s_fortress        = { 30084232, 30, 26, Fortress,        DIM_NETHER,0},
	s_ruined_portal_n = { 40552231, 25, 15, Ruined_Portal_N, DIM_NETHER,0},
	// End structures
	s_end_city        = { 10387313, 20,  9, End_City,        DIM_END,0}
	;

	// Chunkbase only goes back to Bedrock 1.14
	switch (structureType) {
	case Ancient_City:
		*sconf = s_ancient_city;
		return mc >= MC_1_19_2;
	case Desert_Pyramid:
		*sconf = s_desert_pyramid;
		return mc >= MC_1_14;
	case Igloo:
		*sconf = s_igloo;
		return mc >= MC_1_14;
	case Jungle_Pyramid:
		*sconf = s_jungle_pyramid;
		return mc >= MC_1_14;
	case Mansion:
		*sconf = s_mansion;
		return mc >= MC_1_14;
	case Monument:
		*sconf = s_monument;
		return mc >= MC_1_14;
	case Outpost:
		*sconf = s_outpost;
		return mc >= MC_1_14;
	case Ruined_Portal:
		*sconf = s_ruined_portal;
		return mc >= MC_1_14;
	case Shipwreck:
		*sconf = mc <= MC_1_17 ? s_shipwreck_117 : s_shipwreck;
		return mc >= MC_1_14;
	case Swamp_Hut:
		*sconf = s_swamp_hut;
		return mc >= MC_1_14;
	// case Treasure:
	// 	*sconf = s_treasure;
	// 	return mc >= MC_1_14;
	case Village:
		*sconf = mc <= MC_1_17 ? s_village_117 : s_village;
		return mc >= MC_1_14;
	case Bastion:
		*sconf = s_bastion;
		return mc >= MC_1_14;
	case Fortress:
		*sconf = s_fortress;
		return mc >= MC_1_14;
	case Ruined_Portal_N:
		*sconf = s_ruined_portal_n;
		return mc >= MC_1_14;
	case End_City:
		*sconf = s_end_city;
		return mc >= MC_1_14;
	default:
		memset(sconf, 0, sizeof(StructureConfig));
		return false;
	}
}

bool getBedrockStructurePos(int structureType, int mc, uint64_t seed, int regX, int regZ, Pos *pos) {
	StructureConfig sconf;
#if STRUCT_CONFIG_OVERRIDE
	if (!getBedrockStructureConfig_override(structureType, mc, &sconf))
#else
	if (!getBedrockStructureConfig(structureType, mc, &sconf))
#endif
	return false;
 
	switch (structureType) {
	case Desert_Pyramid:
	case Igloo:
	case Jungle_Pyramid:
	case Ruined_Portal:
	case Swamp_Hut:
	case Bastion:
	case Fortress:
	case Ruined_Portal_N:
		*pos = getBedrockFeaturePos(&sconf, seed, regX, regZ);
		return true;

	case Ancient_City:
	case Mansion:
	case Monument:
	case Outpost:
	// case Treasure:
	case Village:
		*pos = getBedrockLargeStructurePos(&sconf, seed, regX, regZ);
		return true;

	case Shipwreck:
		*pos = (mc <= MC_1_17 ? getBedrockLargeStructurePos : getBedrockFeaturePos)(&sconf, seed, regX, regZ);
		return true;

	case End_City:
		*pos = getBedrockLargeStructurePos(&sconf, seed, regX, regZ);
		// Not verified
		return (pos->x*(int64_t)pos->x + pos->z*(int64_t)pos->z >= 1008*INT64_C(1008));

	// case Treasure:
	// 	pos->x = 16*regX + 8;
	// 	pos->z = 16*regZ + 8;
	// 	MersenneTwister treasureMt;
	// 	mSetSeed(&treasureMt, pos->x*UINT64_C(341873128712) + pos->z*UINT64_C(132897987541) + seed + sconf.salt, 1);
	// 	return mNextFloat(&treasureMt) < 0.01;

	// case Bastion:
	// 	*pos = getBedrockFeaturePos(&sconf, seed, regX, regZ);
	// 	MersenneTwister bastionMt;
	// 	mSetSeed(&bastionMt, seed, 2);
	// 	uint64_t call1 = mNextIntUnbound(&bastionMt);
	// 	mSetSeed(&bastionMt, (call1 * (pos->x >> 4)) ^ (mNextIntUnbound(&bastionMt) * (pos->z >> 4)) ^ seed, 1);
	// 	return mNextInt(&bastionMt, 5) >= 2;

	default:
		fprintf(stderr, "ERROR: getStructurePos: unsupported structure type %s\n", struct2str(structureType));
		exit(1);
	}
	return 0;
}

//TODO: Check if x/z are blocks or chunks
int getRavinePos(int mc, uint64_t seed, int x, int z, const Generator *g, StructureVariant *ravine1, StructureVariant *ravine2) {
	uint64_t random;
	if (mc < MC_1_18) {
		int count = 0;
		for (int i = 0; i < 2; ++i) {
			setSeed(&random, seed + 1 - i);
			setSeed(&random, (x * nextLong(&random)) ^ (z * nextLong(&random)) ^ (seed + 1 - i));
			if (nextFloat(&random) >= 0.02 || (count && (!g || !isOceanic(getBiomeAt(g, 1, 16*x, 0, 16*z))))) continue;
			StructureVariant *ravinePointer = count ? ravine2 : ravine1;
			if (ravinePointer) {
				ravinePointer->x = 16 * x + nextInt(&random, 16);
				int temp = nextInt(&random, 40);
				ravinePointer->y = 20     + nextInt(&random, temp + 8);
				ravinePointer->z = 16 * z + nextInt(&random, 16);
				nextFloat(&random);
				nextFloat(&random);
				ravinePointer->size = 4*nextFloat(&random) + 2*nextFloat(&random); //TODO: Check if float->uint conversion is intended
			}
			++count;
		}
		return count;
	} else {
		setSeed(&random, seed + 2);
		setSeed(&random, (x * nextLong(&random)) ^ (z * nextLong(&random)) ^ (seed + 2));
		if (nextFloat(&random) >= 0.01) return 0;
		if (ravine1) {
			ravine1->x = 16 * x + nextInt(&random, 16);
			ravine1->y = 10     + nextInt(&random, 58);
			ravine1->z = 16 * z + nextInt(&random, 16);
			nextFloat(&random); nextFloat(&random);
			ravine1->size = 4*nextFloat(&random) + 2*nextFloat(&random); //TODO: Check if float->uint conversion is intended
		}
		return 1;
	}
}

 bool getBedrockRavinePos(uint64_t seed, int x, int z, StructureVariant *ravine) {
	MersenneTwister mt;
	mSetSeed(&mt, seed, 2);
	uint32_t call1 = mNextIntUnbound(&mt);
	mSetSeed(&mt, (seed ^ x*(call1 | 1)) + z*(mNextIntUnbound(&mt) | 1), 1 + 10*!!ravine);
	if (mNextInt(&mt, 150)) return false;
 	if (ravine) {
		ravine->x = 16*x + mNextInt(&mt, 16);
		ravine->y = 20   + mNextInt(&mt, mNextInt(&mt, 40) + 8);
	mNextIntUnbound(&mt);
	ravine->z = 16*z + mNextInt(&mt, 16);
		mNextFloat(&mt);
	mNextFloat(&mt);
		ravine->size = 3*(mNextFloat(&mt) + mNextFloat(&mt));
		if (mNextFloat(&mt) < .05) {
 		ravine->giant = 1;
 			ravine->size *= 2;
		}
	}
	return true;
 }

int getBedrockStronghold(uint64_t seed) {
    static const double PI = 3.1415926535897932384626433;
    MersenneTwister mt;
    mSetSeed(&mt, seed, 1);

    int strongholdCount = 0;
    double angle = 2 * PI * mNextFloat(&mt);
    int distance = 32 + mNextInt(&mt, 16);

    for (int i = 0; i < 3; ++i) {
        int x = (int)(distance * cos(angle));
        int z = (int)(distance * sin(angle));

        // Check if stronghold can generate
        if (x != 0 || z != 0) {
            strongholdCount++;
        }

        // Adjust angle and distance for next stronghold
        angle += PI / 4;
        distance += 8;
    }

    return strongholdCount;
}

