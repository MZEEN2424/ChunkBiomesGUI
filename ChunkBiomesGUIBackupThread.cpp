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
#include <chrono>
#include <fstream>
#include <Windows.h>

extern "C" {
#include "cubiomes/generator.h"
#include "cubiomes/finders.h"
#include "Bfinders.h"
}

class StructureFinder {
private:
    // Add thread-safe generator initialization
    std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_int_distribution<int64_t> seedDist;
    const int numThreads = std::max(1, (int)std::thread::hardware_concurrency());

    // Make sure Generator is thread-safe
    Generator g;
    std::mutex generatorMutex;  // Add mutex for generator access

    std::atomic<bool> shouldStop{false};
    std::vector<std::thread> searchThreads;
    std::vector<std::string> structureNames;  // Change to std::vector<std::string>
    std::vector<Pos> positions;
    std::string currentStatus;
    bool isSearching = false;
    int64_t currentSeed = 0;
    std::mutex structuresMutex;
    int selectedStructure = Village;
    int searchRadius = 256;

    // Add these to track search performance
    std::atomic<uint64_t> seedsChecked{0};
    std::chrono::steady_clock::time_point searchStartTime;
    std::chrono::steady_clock::time_point searchEndTime;
    std::vector<int64_t> foundSeeds;
    double lastCalculatedSeedsPerSecond{0.0};

    // Add a new member for continuous search
    bool continuousSearch = false;

    void resetSearchMetrics() {
        seedsChecked = 0;
        foundSeeds.clear();
        searchStartTime = std::chrono::steady_clock::now();
        lastCalculatedSeedsPerSecond = 0.0;
    }

    double calculateSeedsPerSecond() {
        auto now = std::chrono::steady_clock::now();
        auto endTime = isSearching ? now : searchEndTime;
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - searchStartTime).count();
        
        if (duration > 0) {
            lastCalculatedSeedsPerSecond = seedsChecked * 1000.0 / duration;
        }
        
        return lastCalculatedSeedsPerSecond;
    }

    void searchThread() {
        while (!shouldStop) {
            if (isSearching) {
                int64_t seedToCheck = currentSeed++;
                seedsChecked++;

                try {
                    Pos pos;
                    if (findStructure(seedToCheck, &pos, searchRadius)) {
                        std::lock_guard<std::mutex> lock(structuresMutex);
                        structureNames.push_back(struct2str(selectedStructure));
                        positions.push_back(pos);
                        foundSeeds.push_back(seedToCheck);
                        currentStatus = "✨ Found " + std::string(struct2str(selectedStructure)) + 
                                      " at X: " + std::to_string(pos.x) + ", Z: " + std::to_string(pos.z) +
                                      " (Distance: " + std::to_string((int)sqrt(pow(pos.x, 2) + pow(pos.z, 2))) + " blocks)";
                        
                        // Check if continuous search is disabled
                        if (!continuousSearch) {
                            shouldStop = true;
                        }
                    }
                } catch (const std::exception& e) {
                    currentStatus = "Error: " + std::string(e.what());
                    shouldStop = true;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    bool isWithinRadius(const Pos& pos, int radius) {
        return (int)sqrt(pow(pos.x, 2) + pow(pos.z, 2)) <= radius;
    }

    bool checkSurroundingBiomes(int centerX, int centerZ, int biomeId, int radius) {
        int count = 0;
        int totalChecks = 0;
        
        for(int r = 0; r <= radius; r++) {
            for(int x = centerX - r; x <= centerX + r; x++) {
                for(int z = centerZ - r; z <= centerZ + r; z++) {
                    if(abs(x - centerX) == r || abs(z - centerZ) == r) {
                        totalChecks++;
                        int currentBiome = getBiomeAt(&g, 4, x >> 2, 319>>2, z >> 2);
                        if(currentBiome == biomeId) {
                            count++;
                        } else {
                            if(r < radius/2) {
                                return false;
                            }
                        }
                    }
                }
            }
            if(r == radius/2) {
                if(count < totalChecks * 0.9) {
                    return false;
                }
            }
        }
        
        return count >= totalChecks * 0.8;
    }

    bool findStructure(int64_t seed, Pos* pos, int radius) {
        std::lock_guard<std::mutex> lock(generatorMutex);
        
        setupGenerator(&g, MC_NEWEST, 0);
        g.seed = seed;
        g.dim = DIM_OVERWORLD;
        applySeed(&g, DIM_OVERWORLD, seed);

        int regionRadius = (radius / 512) + 1;

        for (int regionX = -regionRadius; regionX <= regionRadius; ++regionX) {
            for (int regionZ = -regionRadius; regionZ <= regionRadius; ++regionZ) {
                StructureConfig sconf;
                if (!getBedrockStructureConfig(selectedStructure, g.mc, &sconf)) {
                    continue;
                }

                Pos p;
                if (!getBedrockStructurePos(selectedStructure, g.mc, seed, regionX, regionZ, &p)) {
                    continue;
                }

                if (!isWithinRadius(p, radius)) {
                    continue;
                }

                if (!isViableStructurePos(selectedStructure, &g, p.x, p.z, 0)) {
                    continue;
                }

                if (!isViableStructureTerrain(selectedStructure, &g, p.x, p.z)) {
                    continue;
                }

                // Additional biome checks for Monument and Mansion
                int biomeId = getBiomeAt(&g, 4, p.x >> 2, 319>>2, p.z >> 2);
                
                if (selectedStructure == Monument) {
                    if (!isDeepOcean(biomeId)) continue;
                    if (!checkSurroundingBiomes(p.x, p.z, biomeId, 32)) continue;
                }
                else if (selectedStructure == Mansion) {
                    if (biomeId != dark_forest) continue;
                    if (!checkSurroundingBiomes(p.x, p.z, dark_forest, 64)) continue;
                }
                else if (selectedStructure == Shipwreck) {
                    if (!isShipwreckBiome(biomeId)) continue;
                    if (!checkSurroundingBiomes(p.x, p.z, biomeId, 32)) continue;
                }
                else if (selectedStructure == Village) {
                    if (!isVillageBiome(biomeId)) continue;
                    if (!checkSurroundingBiomes(p.x, p.z, biomeId, 16)) continue;
                }

                *pos = p;
                return true;
            }
        }
        return false;
    }

    bool isDeepOcean(int biomeId) {
        return biomeId == deep_ocean || 
               biomeId == deep_frozen_ocean || 
               biomeId == deep_cold_ocean || 
               biomeId == deep_lukewarm_ocean;
    }

    bool isShipwreckBiome(int biomeId) {
        return biomeId == beach ||
               biomeId == snowy_beach ||
               biomeId == ocean;
    }

    bool isVillageBiome(int biomeId) {
        return biomeId == desert ||
               biomeId == plains ||
               biomeId == meadow ||
               biomeId == savanna ||
               biomeId == snowy_plains ||
               biomeId == taiga ||
               biomeId == snowy_taiga ||
               biomeId == sunflower_plains;
    }

public:
    StructureFinder() : 
        gen(rd()),
        seedDist(INT64_MIN, INT64_MAX) 
    {
        setupGenerator(&g, MC_NEWEST, 0);  // Initialize generator in constructor
    }

    ~StructureFinder() {
        stopSearch();
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
            case Shipwreck:        return "Shipwreck";
            default:               return "Unknown";
        }
    }

    void startSearch() {
        if (isSearching) {
            stopSearch();  // Stop any existing search first
        }

        // Reset search metrics
        resetSearchMetrics();

        {
            std::lock_guard<std::mutex> lock(structuresMutex);
            structureNames.clear();
            positions.clear();
            foundSeeds.clear();
        }

        shouldStop = false;
        isSearching = true;
        
        try {
            searchThreads.clear();  // Clear any old threads
            
            // Create new threads
            for (int i = 0; i < numThreads; i++) {
                searchThreads.emplace_back([this, i]() {
                    try {
                        int attempts = 0;
                        std::mt19937_64 localGen(rd() + i);  // Thread-local random generator
                        std::uniform_int_distribution<int64_t> localSeedDist(INT64_MIN, INT64_MAX);
                        
                        while (!shouldStop) {
                            attempts++;
                            int64_t seedToCheck = localSeedDist(localGen);
                            seedsChecked++;
                            
                            {
                                std::lock_guard<std::mutex> lock(structuresMutex);
                                currentSeed = seedToCheck;
                                currentStatus = "🔍 Checking seed: " + std::to_string(seedToCheck) + 
                                             " (Thread " + std::to_string(i) + ", Attempt " + std::to_string(attempts) + ")";
                            }

                            Pos pos;
                            if (findStructure(seedToCheck, &pos, searchRadius)) {
                                std::lock_guard<std::mutex> lock(structuresMutex);
                                structureNames.push_back(struct2str(selectedStructure));
                                positions.push_back(pos);
                                foundSeeds.push_back(seedToCheck);
                                currentStatus = "✨ Found " + std::string(struct2str(selectedStructure)) + 
                                              " at X: " + std::to_string(pos.x) + ", Z: " + std::to_string(pos.z) +
                                              " (Distance: " + std::to_string((int)sqrt(pow(pos.x, 2) + pow(pos.z, 2))) + " blocks)";
                                
                                // Check if continuous search is disabled
                                if (!continuousSearch) {
                                    shouldStop = true;
                                    break;
                                }
                            }
                        }
                    } catch (const std::exception& e) {
                        std::lock_guard<std::mutex> lock(structuresMutex);
                        currentStatus = "⚠️ Thread error: " + std::string(e.what());
                        shouldStop = true;
                    }
                });
            }
        } catch (const std::exception& e) {
            currentStatus = "⚠️ Failed to start search: " + std::string(e.what());
            isSearching = false;
            shouldStop = true;
        }
    }

    void stopSearch() {
        shouldStop = true;
        isSearching = false;
        searchEndTime = std::chrono::steady_clock::now();
        
        // Properly join all threads
        for (auto& thread : searchThreads) {
            if (thread.joinable()) {
                try {
                    thread.join();
                } catch (const std::exception& e) {
                    // Handle any thread joining errors
                }
            }
        }
        searchThreads.clear();
        currentStatus = "⚠️ Search stopped";
    }

    void renderAboutTab() {
        ImGui::Text("Chunk Biomes - Minecraft Seed Finder");
        ImGui::Separator();

        // Project Description
        ImGui::TextWrapped("A powerful tool for finding Minecraft Bedrock seeds with specific structures and biome characteristics.");

        // Key Features Section
        ImGui::Text("\nKey Features:");
        ImGui::BulletText("Multi-threaded seed searching");
        ImGui::BulletText("Support for multiple structure types");
        ImGui::BulletText("Customizable search radius");
        ImGui::BulletText("Continuous or single-stop search modes");
        ImGui::BulletText("Seed and structure export functionality");

        // Technologies Used
        ImGui::Text("\nTechnologies:");
        ImGui::Columns(2, "TechColumns", false);
        
        ImGui::Text("Libraries:");
        ImGui::BulletText("Cubiomes - Minecraft Java Seedfinding Library");
        ImGui::BulletText("GUI: Dear ImGui");
        ImGui::BulletText("C++ Standard Library");
        
        ImGui::NextColumn();
        
        ImGui::Text("Techniques:");
        ImGui::BulletText("Multi-threading");
        ImGui::BulletText("Parallel seed generation");
        ImGui::BulletText("Efficient structure checking");
        ImGui::BulletText("Random seed sampling");
        
        ImGui::Columns(1);

        // Performance Metrics
        ImGui::Separator();
        ImGui::Text("Performance Metrics:");
        ImGui::BulletText("Concurrent seed checking across multiple threads");
        ImGui::BulletText("Dynamic seeds per second calculation");

        // Contributors
        ImGui::Separator();
        ImGui::Text("Contributors:");
        ImGui::BulletText("Project Creator: NelS");
        ImGui::BulletText("Project Helpers: Fragrant_Result_186");
        ImGui::BulletText("GUI: MZEEN");
        ImGui::BulletText("Cubiomes Library: Cubitect");

        // Version and License
        ImGui::Separator();
        ImGui::Text("Version: BetaV1");
        ImGui::Text("License: Open Source (MIT)");

        // GitHub and Support
        ImGui::Separator();
        if (ImGui::Button("GitHub Repository")) {
            // Open GitHub link (replace with actual repository link)
            system("start https://github.com/yourusername/chunkbiomes");
        }
        ImGui::SameLine();
        if (ImGui::Button("Report an Issue")) {
            // Open issues page
            system("start https://github.com/yourusername/chunkbiomes/issues");
        }
    }

    void renderRavineTab() {
        ImGui::Text("Ravine Finder (Coming Soon)");
        ImGui::TextWrapped("This feature will help you locate ravines in Minecraft Bedrock seeds.");
        
        // Placeholder for future ravine search functionality
        ImGui::Separator();
        ImGui::TextDisabled("Ravine search functionality is not yet implemented.");
    }

    void renderBiomeTab() {
        ImGui::Text("Biome Finder (Coming Soon)");
        ImGui::TextWrapped("This feature will help you find specific biomes in Minecraft Java and Bedrock seeds.");
        
        // Placeholder for future biome search functionality
        ImGui::Separator();
        ImGui::TextDisabled("Biome search functionality is not yet implemented.");
    }

    void saveSeedsToFile() {
        if (foundSeeds.empty()) {
            currentStatus = "⚠️ No seeds to save";
            return;
        }

        // Open file dialog to choose save location
        OPENFILENAMEA ofn;
        char szFile[260] = {0};
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = "txt";

        if (GetSaveFileNameA(&ofn)) {
            std::ofstream outFile(szFile);
            if (outFile.is_open()) {
                // Write header
                outFile << "Chunk Biomes - Found Seeds\n";
                outFile << "Structure: " << struct2str(selectedStructure) << "\n";
                outFile << "Search Radius: " << searchRadius << "\n";
                outFile << "------------------------\n";

                // Write seeds with their structure details
                for (size_t i = 0; i < foundSeeds.size(); ++i) {
                    outFile << "Seed: " << foundSeeds[i];
                    
                    // Add structure details if available
                    if (i < structureNames.size() && i < positions.size()) {
                        outFile << " - " << structureNames[i] 
                                << " (X: " << positions[i].x 
                                << ", Z: " << positions[i].z << ")";
                    }
                    outFile << "\n";
                }

                outFile.close();
                currentStatus = "✅ Seeds saved successfully to " + std::string(szFile);
            } else {
                currentStatus = "⚠️ Failed to open file for writing";
            }
        }
    }

    void renderSearchTab() {
        ImGui::Text("Search Settings");
        ImGui::Separator();

        // Structure selection combo
        const char* structures[] = {
            "Village", "Desert Pyramid", "Jungle Pyramid", "Swamp Hut",
            "Igloo", "Monument", "Mansion", "Outpost", 
            "Ancient City", "Ruined Portal", "Shipwreck"
        };
        static int structureIndex = 0; // Default to Village
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
                case 10: selectedStructure = Shipwreck; break;
            }
        }
        
        ImGui::PushItemWidth(120);
        ImGui::InputInt("Search Radius", &searchRadius, 16, 100);
        if (searchRadius < 16) searchRadius = 16;
        if (searchRadius > 10000) searchRadius = 10000;
        ImGui::PopItemWidth();

        // Continuous Search Checkbox
        ImGui::Separator();
        ImGui::Checkbox("Continuous Search", &continuousSearch);
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted("Continue searching after finding a structure");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }

        // Start/Stop Search Buttons
        ImGui::Separator();
        if (!isSearching) {
            if (ImGui::Button("Start Search")) {
                startSearch();
            }
        } else {
            if (ImGui::Button("Stop Search")) {
                stopSearch();
            }
        }

        // Search Progress and Results
        ImGui::Separator();
        ImGui::Text("Search Status:");
        ImGui::TextColored(ImVec4(1,1,0,1), "%s", currentStatus.c_str());

        // Search Performance Metrics
        ImGui::Separator();
        ImGui::Text("Search Performance:");
        ImGui::Text("Seeds Checked: %llu", seedsChecked.load());
        ImGui::Text("Seeds/Second: %.2f", calculateSeedsPerSecond());

        // Found Seeds Display and Copy
        if (!foundSeeds.empty()) {
            ImGui::Separator();
            
            // Found Seeds header with Clear and Save buttons
            ImGui::Text("Found Seeds:");
            
            // Align buttons to the right
            float windowWidth = ImGui::GetWindowWidth();
            ImGui::SameLine(windowWidth - 200);  // Adjust spacing for two buttons
            if (ImGui::Button("Clear Seeds")) {
                foundSeeds.clear();
                structureNames.clear();
                positions.clear();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Save Seeds")) {
                saveSeedsToFile();
            }

            // Create a scrollable region for found seeds
            ImGui::BeginChild("Found Seeds List", ImVec2(0, 150), true, ImGuiWindowFlags_HorizontalScrollbar);
            
            // Display each seed on a separate line with a copy button
            for (size_t i = 0; i < foundSeeds.size(); ++i) {
                // Display seed with additional structure info if available
                if (i < structureNames.size() && i < positions.size()) {
                    ImGui::Text("%lld - %s (X: %d, Z: %d)", 
                        foundSeeds[i], 
                        structureNames[i].c_str(), 
                        positions[i].x, 
                        positions[i].z
                    );
                } else {
                    ImGui::Text("%lld", foundSeeds[i]);
                }
                
                // Copy Seed button
                ImGui::SameLine();
                if (ImGui::Button(("Copy Seed##" + std::to_string(i)).c_str())) {
                    char seedStr[32];
                    snprintf(seedStr, sizeof(seedStr), "%lld", foundSeeds[i]);
                    ImGui::SetClipboardText(seedStr);
                }
            }
            
            ImGui::EndChild();
        }

        // Display found structures
        if (!structureNames.empty()) {
            ImGui::Separator();
            
            // Found Structures header
            ImGui::Text("Found Structures:");

            // Create a scrollable region for found structures
            ImGui::BeginChild("Found Structures List", ImVec2(0, 200), true, ImGuiWindowFlags_HorizontalScrollbar);
            
            // Display each structure
            for (size_t i = 0; i < structureNames.size(); ++i) {
                if (i < positions.size()) {
                    ImGui::Text("%s - X: %d, Z: %d", 
                        structureNames[i].c_str(), 
                        positions[i].x, 
                        positions[i].z
                    );
                } else {
                    ImGui::Text("%s", structureNames[i].c_str());
                }
            }
            
            ImGui::EndChild();
        }
    }

    void renderGUI() {
        ImGui::Begin("ChunkBiomes GUI", nullptr, ImGuiWindowFlags_NoCollapse);

        if (ImGui::BeginTabBar("MainTabs")) {
            // Search Tab
            if (ImGui::BeginTabItem("Structure Search")) {
                renderSearchTab();
                ImGui::EndTabItem();
            }

            // Ravine Tab
            if (ImGui::BeginTabItem("Ravine Finder")) {
                renderRavineTab();
                ImGui::EndTabItem();
            }

            // Biome Tab
            if (ImGui::BeginTabItem("Biome Finder")) {
                renderBiomeTab();
                ImGui::EndTabItem();
            }

            // About Tab
            if (ImGui::BeginTabItem("About")) {
                renderAboutTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    // Add these to the class members
    bool showSearchTab = true;
    bool showAboutTab = false;
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
    GLFWwindow* window = glfwCreateWindow(800, 600, "ChunkBiomes GUI", NULL, NULL);
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
    
    // Customize colors to use green instead of blue
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Header] = ImVec4(0.2f, 0.7f, 0.2f, 0.7f);           // Green header
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.3f, 0.8f, 0.3f, 0.8f);    // Lighter green on hover
    colors[ImGuiCol_HeaderActive] = ImVec4(0.4f, 0.9f, 0.4f, 0.9f);     // Brightest green when active
    
    colors[ImGuiCol_Button] = ImVec4(0.2f, 0.7f, 0.2f, 0.6f);           // Green buttons
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.8f, 0.3f, 0.7f);    // Lighter green on hover
    colors[ImGuiCol_ButtonActive] = ImVec4(0.4f, 0.9f, 0.4f, 0.8f);     // Brightest green when active
    
    colors[ImGuiCol_Tab] = ImVec4(0.2f, 0.7f, 0.2f, 0.6f);              // Green tabs
    colors[ImGuiCol_TabHovered] = ImVec4(0.3f, 0.8f, 0.3f, 0.7f);       // Lighter green on hover
    colors[ImGuiCol_TabActive] = ImVec4(0.4f, 0.9f, 0.4f, 0.8f);        // Brightest green when active
    
    // Checkbox and other interactive elements
    colors[ImGuiCol_CheckMark] = ImVec4(0.1f, 0.9f, 0.1f, 1.0f);        // Bright green checkmark
    colors[ImGuiCol_FrameBg] = ImVec4(0.2f, 0.7f, 0.2f, 0.5f);          // Green frame background
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3f, 0.8f, 0.3f, 0.6f);   // Lighter green frame background on hover
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.4f, 0.9f, 0.4f, 0.7f);    // Brightest green frame background when active
    
    // Sliders and scrollbars
    colors[ImGuiCol_SliderGrab] = ImVec4(0.1f, 0.9f, 0.1f, 1.0f);       // Bright green slider grab
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2f, 1.0f, 0.2f, 1.0f); // Even brighter green when active
    
    // Highlight and selection colors
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2f, 0.7f, 0.2f, 0.5f);   // Green text selection background

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Create structure finder
    StructureFinder finder;

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Update ImGui IO with the current window size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        io.DisplaySize = ImVec2((float)display_w, (float)display_h);

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a fullscreen window that fills the entire screen
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("ChunkBiomes GUI", nullptr, 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoScrollbar | 
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoBackground);  // Remove background

        // Render the GUI
        finder.renderGUI();

        ImGui::End();

        // Rendering
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);  // Transparent background
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
