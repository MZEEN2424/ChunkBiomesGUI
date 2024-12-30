#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstring>
#include <cmath>
#include <random>
#include <limits>

extern "C" {
#include "cubiomes/generator.h"
#include "cubiomes/finders.h"
#include "Bfinders.h"
}

class StructureFinder {
private:
    Generator g;
    std::atomic<bool> shouldStop{false};
    std::thread searchThread;
    std::vector<const char*> structureNames;
    std::vector<Pos> positions;
    std::string currentStatus;
    bool isSearching = false;
    int64_t currentSeed = 0;
    std::mutex structuresMutex;
    int selectedStructure = Ruined_Portal;
    int searchRadius = 256;
    
    // Random number generation
    std::random_device rd;  // Hardware random number generator
    std::mt19937_64 gen;    // Mersenne Twister generator
    std::uniform_int_distribution<int64_t> seedDist;  // Distribution for seeds

    bool isWithinRadius(const Pos& pos, int radius) {
        double distance = sqrt(pow(pos.x, 2) + pow(pos.z, 2));
        return distance <= radius;
    }

    bool checkSurroundingBiomes(int centerX, int centerZ, int biomeId, int radius) {
        // Check in a square area around the point
        int count = 0;
        int totalChecks = 0;
        
        // For mansions, we need a large continuous dark forest area
        // Check in expanding squares from center to detect continuous area
        for(int r = 0; r <= radius; r++) {
            for(int x = centerX - r; x <= centerX + r; x++) {
                for(int z = centerZ - r; z <= centerZ + r; z++) {
                    // Only check points on the current square perimeter
                    if(abs(x - centerX) == r || abs(z - centerZ) == r) {
                        totalChecks++;
                        int currentBiome = getBiomeAt(&g, 4, x >> 2, 319>>2, z >> 2);
                        if(currentBiome == biomeId) {
                            count++;
                        } else {
                            // If we find non-dark forest early, it's not a good spot
                            if(r < radius/2) {
                                return false;
                            }
                        }
                    }
                }
            }
            // Need very high percentage of dark forest in inner radius
            if(r == radius/2) {
                if(count < totalChecks * 0.9) { // 90% requirement for inner area
                    return false;
                }
            }
        }
        
        // Need good percentage overall
        return count >= totalChecks * 0.8; // 80% requirement overall
    }

    bool findStructure(int64_t seed, Pos* pos, int radius) {
        // Initialize the generator properly
        setupGenerator(&g, MC_NEWEST, 0);  // First setup with seed 0
        g.seed = seed;  // Set the world seed
        g.dim = DIM_OVERWORLD;  // Set dimension
        applySeed(&g, DIM_OVERWORLD, seed);  // Apply the seed properly

        // Convert radius to region size
        int regionRadius = (radius / 512) + 1; // Add 1 to ensure we cover the full radius

        for (int regionX = -regionRadius; regionX <= regionRadius; ++regionX) {
            for (int regionZ = -regionRadius; regionZ <= regionRadius; ++regionZ) {
                StructureConfig sconf;
                if (!getBedrockStructureConfig(selectedStructure, g.mc, &sconf)) {
                    continue;
                }
                
                if (!getBedrockStructurePos(selectedStructure, g.mc, seed, regionX, regionZ, pos)) {
                    continue;
                }

                // Check if the position is within the specified radius
                if (!isWithinRadius(*pos, radius)) {
                    continue;
                }

                // Get the biome at the structure position
                int biomeId = getBiomeAt(&g, 4, pos->x >> 2, 319>>2, pos->z >> 2);
                
                // Check if the position is viable for the structure
                if (!isViableStructurePos(selectedStructure, &g, pos->x, pos->z, 0)) {
                    continue;
                }

                // Check if the terrain is suitable
                if (!isViableStructureTerrain(selectedStructure, &g, pos->x, pos->z)) {
                    continue;
                }

                // Additional biome check for specific structures
                if (selectedStructure == Mansion) {
                    // For mansions, need large dark forest area (128 block radius)
                    if (biomeId != dark_forest && biomeId != dark_forest_hills) {
                        continue;
                    }
                    // Check surrounding area for dark forest - larger radius and stricter check
                    if (!checkSurroundingBiomes(pos->x, pos->z, dark_forest, 128)) {
                        continue;
                    }
                }
                else if (selectedStructure == Desert_Pyramid && biomeId != desert && biomeId != desert_hills) {
                    continue;
                }
                else if (selectedStructure == Jungle_Pyramid && 
                    biomeId != jungle && biomeId != jungle_hills && 
                    biomeId != bamboo_jungle && biomeId != bamboo_jungle_hills) {
                    continue;
                }
                else if (selectedStructure == Swamp_Hut && biomeId != swamp) {
                    continue;
                }

                return true;
            }
        }
        return false;
    }

public:
    StructureFinder() : 
        gen(rd()),  // Initialize generator with random device
        seedDist(INT64_MIN, INT64_MAX) 
    {
        // Constructor initialization
    }

    ~StructureFinder() {
        stopSearch();
        if (searchThread.joinable()) {
            searchThread.join();
        }
    }

    const char* struct2str(int structureType) {
        switch (structureType) {
            case Village:           return "Village";
            case Desert_Pyramid:    return "Desert Pyramid";
            case Jungle_Pyramid:    return "Jungle Pyramid";
            case Swamp_Hut:        return "Swamp Hut";
            case Igloo:            return "Igloo";
            case Monument:         return "Monument";
            case Mansion:          return "Mansion";
            case Outpost:          return "Outpost";
            case Ancient_City:     return "Ancient City";
            case Ruined_Portal:    return "Ruined Portal";
            default:               return "Unknown";
        }
    }

    void startSearch(int radius) {
        if (isSearching) return;
        
        searchRadius = radius;
        {
            std::lock_guard<std::mutex> lock(structuresMutex);
            structureNames.clear();
            positions.clear();
        }
        shouldStop = false;
        isSearching = true;
        
        searchThread = std::thread([this]() {
            int attempts = 0;
            
            while (!shouldStop) {
                attempts++;
                // Generate a completely random seed each time
                currentSeed = seedDist(gen);
                currentStatus = "🔍 Checking seed: " + std::to_string(currentSeed) + " (Attempt " + std::to_string(attempts) + ")";

                Pos pos;
                if (findStructure(currentSeed, &pos, searchRadius)) {
                    std::lock_guard<std::mutex> lock(structuresMutex);
                    structureNames.push_back(struct2str(selectedStructure));
                    positions.push_back(pos);
                    currentStatus = "✨ Found " + std::string(struct2str(selectedStructure)) + 
                                  " at X: " + std::to_string(pos.x) + ", Z: " + std::to_string(pos.z) +
                                  " (Distance: " + std::to_string((int)sqrt(pow(pos.x, 2) + pow(pos.z, 2))) + " blocks)";
                    break;
                }
                
                // Optional: Add a small sleep to prevent CPU overload
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            if (shouldStop) {
                currentStatus = "⚠️ Search stopped by user after " + std::to_string(attempts) + " attempts";
            }
            
            isSearching = false;
        });

        searchThread.detach();
    }

    void stopSearch() {
        shouldStop = true;
        currentStatus = "⚠️ Search stopped by user";
    }

    void renderGUI() {
        static int radius = 256;

        // Main window
        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Minecraft Structure Finder", nullptr, ImGuiWindowFlags_NoCollapse);

        // Style and colors
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.3f, 0.7f, 1.0f));

        // Search controls
        ImGui::Text("Search Settings");
        ImGui::Separator();

        // Structure selection combo
        const char* structures[] = {
            "Village", "Desert Pyramid", "Jungle Pyramid", "Swamp Hut",
            "Igloo", "Monument", "Mansion", "Outpost", 
            "Ancient City", "Ruined Portal"
        };
        static int structureIndex = 9; // Default to Ruined Portal
        if (ImGui::Combo("Structure Type", &structureIndex, structures, IM_ARRAYSIZE(structures))) {
            switch(structureIndex) {
                case 0:  selectedStructure = Village; break;
                case 1:  selectedStructure = Desert_Pyramid; break;
                case 2:  selectedStructure = Jungle_Pyramid; break;
                case 3:  selectedStructure = Swamp_Hut; break;
                case 4:  selectedStructure = Igloo; break;
                case 5:  selectedStructure = Monument; break;
                case 6:  selectedStructure = Mansion; break;
                case 7:  selectedStructure = Outpost; break;
                case 8:  selectedStructure = Ancient_City; break;
                case 9:  selectedStructure = Ruined_Portal; break;
            }
        }
        
        ImGui::PushItemWidth(120);
        ImGui::InputInt("Search Radius", &radius, 16, 100);
        if (radius < 16) radius = 16;
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (!isSearching) {
            if (ImGui::Button("Search", ImVec2(120, 0))) {
                startSearch(radius);
            }
        } else {
            if (ImGui::Button("Stop", ImVec2(120, 0))) {
                stopSearch();
            }
        }

        // Status and seed info with copy button
        ImGui::Text("%s", currentStatus.c_str());
        if (currentSeed != 0) {
            char seedStr[32];
            snprintf(seedStr, sizeof(seedStr), "%lld", currentSeed);
            ImGui::Text("Current Seed: %s", seedStr);
            ImGui::SameLine();
            if (ImGui::Button("Copy Seed")) {
                ImGui::SetClipboardText(seedStr);
            }
        }

        ImGui::Separator();

        // Results table
        if (ImGui::BeginTable("Structures", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Structure");
            ImGui::TableSetupColumn("X");
            ImGui::TableSetupColumn("Z");
            ImGui::TableSetupColumn("Distance");
            ImGui::TableHeadersRow();

            std::lock_guard<std::mutex> lock(structuresMutex);
            for (size_t i = 0; i < structureNames.size(); i++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", structureNames[i]);
                ImGui::TableNextColumn();
                ImGui::Text("%d", positions[i].x);
                ImGui::TableNextColumn();
                ImGui::Text("%d", positions[i].z);
                ImGui::TableNextColumn();
                int distance = (int)sqrt(pow(positions[i].x, 2) + pow(positions[i].z, 2));
                ImGui::Text("%d", distance);
            }
            ImGui::EndTable();
        }

        ImGui::PopStyleColor(3);
        ImGui::End();
    }
};

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

int main(int argc, char** argv) {
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(800, 600, "Minecraft Structure Finder", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Create structure finder
    StructureFinder finder;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render the GUI
        finder.renderGUI();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}