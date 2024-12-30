#include <stdio.h>
#include "Bfinders.h"

const char* struct2str(int structureType) {
    switch(structureType) {
        case Ancient_City: return "Ancient City";
        case Desert_Pyramid: return "Desert Pyramid";
        case Igloo: return "Igloo";
        case Jungle_Pyramid: return "Jungle Pyramid";
        case Mansion: return "Mansion";
        case Monument: return "Monument";
        case Outpost: return "Outpost";
        case Ruined_Portal: return "Ruined Portal";
        case Shipwreck: return "Shipwreck";
        case Swamp_Hut: return "Swamp Hut";
        case Village: return "Village";
        case Bastion: return "Bastion";
        case Fortress: return "Fortress";
        case Ruined_Portal_N: return "Nether Ruined Portal";
        case End_City: return "End City";
        default: return "Unknown Structure";
    }
}

int main() {
    const uint64_t SEED = 8675309;
    const int OVERWORLD_STRUCTURES[] = {
        Ancient_City,
        Desert_Pyramid,
        Igloo,
        Jungle_Pyramid,
        Mansion,
        Monument,
        Outpost,
        Ruined_Portal, 
        Shipwreck,
        Swamp_Hut,
        /* Treasure, */ // Not supported yet
        Village
    };
    const int NETHER_STRUCTURES[] = {
        Bastion,  // Not differentiated from fortresses yet
        Fortress, // Not differentiated from bastions yet
        Ruined_Portal_N
    };
    const int END_STRUCTURES[] = {
        End_City
    };
    const int REGION_RADIUS = 7;

    Generator g;
    setupGenerator(&g, MC_NEWEST, 0);
    applySeed(&g, DIM_OVERWORLD, SEED);
    // g.platform = PLATFORM_JAVA;

    StructureConfig sconf;
    Pos pos;

    printf("Searching for structures with seed: %llu\n", SEED);
    printf("Region radius: %d chunks\n\n", REGION_RADIUS);

    printf("=== OVERWORLD STRUCTURES ===\n");
    for (int regionX = -REGION_RADIUS; regionX <= REGION_RADIUS; ++regionX) {
        for (int regionZ = -REGION_RADIUS; regionZ <= REGION_RADIUS; ++regionZ) {
            for (size_t i = 0; i < sizeof(OVERWORLD_STRUCTURES)/sizeof(*OVERWORLD_STRUCTURES); ++i) {
                if (!getBedrockStructureConfig(OVERWORLD_STRUCTURES[i], g.mc, &sconf)) {
                    printf("ERR: %s's configuration could not be found.\n", struct2str(OVERWORLD_STRUCTURES[i]));
                    continue;
                }
                if (!getBedrockStructurePos(OVERWORLD_STRUCTURES[i], g.mc, g.seed, regionX, regionZ, &pos)) {
                    continue;
                }
                if (OVERWORLD_STRUCTURES[i] && (!isViableStructurePos(OVERWORLD_STRUCTURES[i], &g, pos.x, pos.z, 0) || !isViableStructureTerrain(OVERWORLD_STRUCTURES[i], &g, pos.x, pos.z))) continue;
                printf("%s: (%d, %d)\n", struct2str(OVERWORLD_STRUCTURES[i]), pos.x, pos.z);
            }
        }
    }

    printf("\n=== NETHER STRUCTURES ===\n");
    applySeed(&g, DIM_NETHER, SEED);
    for (int regionX = -REGION_RADIUS; regionX <= REGION_RADIUS; ++regionX) {
        for (int regionZ = -REGION_RADIUS; regionZ <= REGION_RADIUS; ++regionZ) {
            for (size_t i = 0; i < sizeof(NETHER_STRUCTURES)/sizeof(*NETHER_STRUCTURES); ++i) {
                if (!getBedrockStructureConfig(NETHER_STRUCTURES[i], g.mc, &sconf)) {
                    printf("ERR: %s's configuration could not be found.\n", struct2str(NETHER_STRUCTURES[i]));
                    continue;
                }
                if (!getBedrockStructurePos(NETHER_STRUCTURES[i], g.mc, g.seed, regionX, regionZ, &pos)) {
                    continue;
                }
                printf("%s: (%d, %d)\n", struct2str(NETHER_STRUCTURES[i]), pos.x, pos.z);
            }
        }
    }

    printf("\n=== END STRUCTURES ===\n");
    applySeed(&g, DIM_END, SEED);
    for (int regionX = -REGION_RADIUS; regionX <= REGION_RADIUS; ++regionX) {
        for (int regionZ = -REGION_RADIUS; regionZ <= REGION_RADIUS; ++regionZ) {
            for (size_t i = 0; i < sizeof(END_STRUCTURES)/sizeof(*END_STRUCTURES); ++i) {
                if (!getBedrockStructureConfig(END_STRUCTURES[i], g.mc, &sconf)) {
                    printf("ERR: %s's configuration could not be found.\n", struct2str(END_STRUCTURES[i]));
                    continue;
                }
                if (!getBedrockStructurePos(END_STRUCTURES[i], g.mc, g.seed, regionX, regionZ, &pos)) {
                    continue;
                }
                printf("%s: (%d, %d)\n", struct2str(END_STRUCTURES[i]), pos.x, pos.z);
            }
        }
    }

    return 0;
}