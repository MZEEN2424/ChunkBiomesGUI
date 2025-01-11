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
#include <set>
#include <random>
#include <chrono>
#include <format>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <cmath>
#include <limits>
#include <Windows.h>
#include <algorithm> // Include algorithm header
#include "Brng.h" // Include Brng.h for MersenneTwister

// Function to show Windows Save File Dialog
std::string ShowSaveFileDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Color Settings\0*.txt\0All\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileNameA(&ofn)) {
        std::string filename = ofn.lpstrFile;
        if (filename.find(".txt") == std::string::npos) {
            filename += ".txt";
        }
        return filename;
    }
    return "";
}

// Function to show Windows Open File Dialog
std::string ShowOpenFileDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Color Settings\0*.txt\0All\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameA(&ofn)) {
        return ofn.lpstrFile;
    }
    return "";
}

extern "C" {
#include "cubiomes/generator.h"
#include "cubiomes/finders.h"
#include "Bfinders.h"
}

// Forward declare ApplyCustomColors
void ApplyCustomColors();

struct UIColors {
    ImVec4 text = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    ImVec4 background = ImVec4(0.05f, 0.05f, 0.05f, 1.00f);
    ImVec4 frame = ImVec4(0.00f, 0.20f, 0.00f, 1.00f);
    ImVec4 button = ImVec4(0.00f, 0.40f, 0.00f, 1.00f);
    ImVec4 header = ImVec4(0.00f, 0.30f, 0.00f, 1.00f);
    ImVec4 border = ImVec4(0.00f, 0.50f, 0.00f, 0.50f);
    ImVec4 accent = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);

    // Target colors for smooth transitions
    ImVec4 targetText = text;
    ImVec4 targetBackground = background;
    ImVec4 targetFrame = frame;
    ImVec4 targetButton = button;
    ImVec4 targetHeader = header;
    ImVec4 targetBorder = border;
    ImVec4 targetAccent = accent;
} uiColors;

// Global transition speed
static float transitionSpeed = 0.1f;

// Helper function to smoothly interpolate between colors
ImVec4 LerpColor(const ImVec4& current, const ImVec4& target, float t) {
    return ImVec4(
        current.x + (target.x - current.x) * t,
        current.y + (target.y - current.y) * t,
        current.z + (target.z - current.z) * t,
        current.w + (target.w - current.w) * t
    );
}

void UpdateColors() {
    // Smoothly transition each color
    uiColors.text = LerpColor(uiColors.text, uiColors.targetText, transitionSpeed);
    uiColors.background = LerpColor(uiColors.background, uiColors.targetBackground, transitionSpeed);
    uiColors.frame = LerpColor(uiColors.frame, uiColors.targetFrame, transitionSpeed);
    uiColors.button = LerpColor(uiColors.button, uiColors.targetButton, transitionSpeed);
    uiColors.header = LerpColor(uiColors.header, uiColors.targetHeader, transitionSpeed);
    uiColors.border = LerpColor(uiColors.border, uiColors.targetBorder, transitionSpeed);
    uiColors.accent = LerpColor(uiColors.accent, uiColors.targetAccent, transitionSpeed);
    
    ApplyCustomColors();
}

void ApplyCustomColors() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Apply custom colors
    colors[ImGuiCol_Text] = uiColors.text;
    colors[ImGuiCol_TextDisabled] = ImVec4(uiColors.text.x, uiColors.text.y, uiColors.text.z, 0.50f);
    
    // Background colors
    colors[ImGuiCol_WindowBg] = uiColors.background;
    colors[ImGuiCol_ChildBg] = uiColors.background;
    colors[ImGuiCol_PopupBg] = uiColors.background;
    colors[ImGuiCol_MenuBarBg] = uiColors.background;
    colors[ImGuiCol_ScrollbarBg] = ImVec4(uiColors.background.x * 0.8f, uiColors.background.y * 0.8f, uiColors.background.z * 0.8f, 0.60f);
    
    colors[ImGuiCol_Border] = uiColors.border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    
    // Frame backgrounds
    colors[ImGuiCol_FrameBg] = uiColors.frame;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(uiColors.frame.x + 0.1f, uiColors.frame.y + 0.1f, uiColors.frame.z + 0.1f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(uiColors.frame.x + 0.2f, uiColors.frame.y + 0.2f, uiColors.frame.z + 0.2f, 1.0f);
    
    // Title backgrounds
    colors[ImGuiCol_TitleBg] = uiColors.header;
    colors[ImGuiCol_TitleBgActive] = ImVec4(uiColors.header.x + 0.1f, uiColors.header.y + 0.1f, uiColors.header.z + 0.1f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(uiColors.header.x, uiColors.header.y, uiColors.header.z, 0.51f);
    
    // Scrollbar
    colors[ImGuiCol_ScrollbarGrab] = uiColors.button;
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(uiColors.button.x + 0.1f, uiColors.button.y + 0.1f, uiColors.button.z + 0.1f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(uiColors.button.x + 0.2f, uiColors.button.y + 0.2f, uiColors.button.z + 0.2f, 1.0f);
    
    // Interactive elements
    colors[ImGuiCol_CheckMark] = uiColors.accent;
    colors[ImGuiCol_SliderGrab] = uiColors.accent;
    colors[ImGuiCol_SliderGrabActive] = ImVec4(uiColors.accent.x + 0.2f, uiColors.accent.y + 0.2f, uiColors.accent.z + 0.2f, 1.0f);
    
    // Buttons
    colors[ImGuiCol_Button] = uiColors.button;
    colors[ImGuiCol_ButtonHovered] = ImVec4(uiColors.button.x + 0.1f, uiColors.button.y + 0.1f, uiColors.button.z + 0.1f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(uiColors.button.x + 0.2f, uiColors.button.y + 0.2f, uiColors.button.z + 0.2f, 1.0f);
    
    // Headers
    colors[ImGuiCol_Header] = uiColors.header;
    colors[ImGuiCol_HeaderHovered] = ImVec4(uiColors.header.x + 0.1f, uiColors.header.y + 0.1f, uiColors.header.z + 0.1f, 1.0f);
    colors[ImGuiCol_HeaderActive] = ImVec4(uiColors.header.x + 0.2f, uiColors.header.y + 0.2f, uiColors.header.z + 0.2f, 1.0f);
    
    // Tabs
    colors[ImGuiCol_Tab] = uiColors.button;
    colors[ImGuiCol_TabHovered] = ImVec4(uiColors.button.x + 0.1f, uiColors.button.y + 0.1f, uiColors.button.z + 0.1f, 1.0f);
    colors[ImGuiCol_TabActive] = ImVec4(uiColors.button.x + 0.2f, uiColors.button.y + 0.2f, uiColors.button.z + 0.2f, 1.0f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(uiColors.button.x * 0.8f, uiColors.button.y * 0.8f, uiColors.button.z * 0.8f, 1.0f);
    colors[ImGuiCol_TabUnfocusedActive] = uiColors.button;
}

// Function to save UI colors to a file
void SaveUIColors(const char* filename) {
    std::ofstream file(filename);
    if (file.is_open()) {
        // Save each color component with precision
        file << std::fixed << std::setprecision(6);
        file << uiColors.text.x << " " << uiColors.text.y << " " << uiColors.text.z << " " << uiColors.text.w << "\n";
        file << uiColors.background.x << " " << uiColors.background.y << " " << uiColors.background.z << " " << uiColors.background.w << "\n";
        file << uiColors.frame.x << " " << uiColors.frame.y << " " << uiColors.frame.z << " " << uiColors.frame.w << "\n";
        file << uiColors.button.x << " " << uiColors.button.y << " " << uiColors.button.z << " " << uiColors.button.w << "\n";
        file << uiColors.header.x << " " << uiColors.header.y << " " << uiColors.header.z << " " << uiColors.header.w << "\n";
        file << uiColors.border.x << " " << uiColors.border.y << " " << uiColors.border.z << " " << uiColors.border.w << "\n";
        file << uiColors.accent.x << " " << uiColors.accent.y << " " << uiColors.accent.z << " " << uiColors.accent.w << "\n";
        file.close();
    }
}

// Function to load UI colors from a file
bool LoadUIColors(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    try {
        float r, g, b, a;
        UIColors newColors = uiColors;  // Create a temporary copy
        
        // Load text color
        if (file >> r >> g >> b >> a) newColors.text = ImVec4(r, g, b, a);
        // Load background color
        if (file >> r >> g >> b >> a) newColors.background = ImVec4(r, g, b, a);
        // Load frame color
        if (file >> r >> g >> b >> a) newColors.frame = ImVec4(r, g, b, a);
        // Load button color
        if (file >> r >> g >> b >> a) newColors.button = ImVec4(r, g, b, a);
        // Load header color
        if (file >> r >> g >> b >> a) newColors.header = ImVec4(r, g, b, a);
        // Load border color
        if (file >> r >> g >> b >> a) newColors.border = ImVec4(r, g, b, a);
        // Load accent color
        if (file >> r >> g >> b >> a) newColors.accent = ImVec4(r, g, b, a);

        file.close();
        
        // Update both current and target colors
        uiColors = newColors;
        uiColors.targetText = uiColors.text;
        uiColors.targetBackground = uiColors.background;
        uiColors.targetFrame = uiColors.frame;
        uiColors.targetButton = uiColors.button;
        uiColors.targetHeader = uiColors.header;
        uiColors.targetBorder = uiColors.border;
        uiColors.targetAccent = uiColors.accent;
        
        ApplyCustomColors();
        return true;
    }
    catch (...) {
        file.close();
        return false;
    }
}

struct FoundStructure {
    uint64_t seed;
    Pos pos;
    bool isZombieVillage;
};

class StructureFinder {
private:
    std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_int_distribution<uint64_t> dis;
    const int numThreads = std::max(1, (int)std::thread::hardware_concurrency());

    // Make sure Generator is thread-safe
    // Generator g;
    std::mutex generatorMutex;  // Add mutex for generator access

    std::atomic<bool> shouldStop{false};
    std::vector<std::thread> searchThreads;
    std::vector<std::string> structureNames;  // Change to std::vector<std::string>
    std::vector<Pos> positions;
    std::string currentStatus;
    bool isSearching = false;
    std::atomic<int64_t> seedsChecked{0};
    std::atomic<int64_t> currentSeed{0};
    std::mutex structuresMutex;
    std::vector<int64_t> foundSeeds;
    int selectedStructure = Village;
    int searchRadius = 256;

    // Add these to track search performance
    std::chrono::steady_clock::time_point searchStartTime;
    double lastCalculatedSeedsPerSecond{0.0};
    double elapsedSearchTime = 0.0;
    bool timerRunning = false;

    // Add a new member for continuous search
    bool continuousSearch = false;

    // Add a new member for seed range selection
    bool useBedrockRange = false;  // Default to 64-bit range

    // Optimize batch size for thorough checking
    int OPTIMAL_BATCH_SIZE = 200000;  // Increased batch size
    const int STATUS_UPDATE_INTERVAL = 5000;  // Less frequent updates
    int generatorPoolSize = 64; // Keep multiple generators

    // New structure for multiple structure search
    struct AttachedStructure {
        int structureType;
        int maxDistance;
        bool required;
        bool found;
        Pos foundPos;

        AttachedStructure() : 
            structureType(Village), maxDistance(256), required(false), found(false), foundPos({0, 0}) {}
        
        AttachedStructure(int type, int dist, bool req) : 
            structureType(type), maxDistance(dist), required(req), found(false), foundPos({0, 0}) {}
    };
    
    bool multiStructureMode = false;
    std::vector<AttachedStructure> attachedStructures;
    int baseStructureType = Village;  // The main structure to search around

public:
    StructureFinder() : 
        gen(rd())
    {
        // setupGenerator(&g, MC_NEWEST, 0);  // Initialize generator in constructor
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

    int getStructureTypeFromIndex(int index) {
        switch(index) {
            case 0: return Village;
            case 1: return Desert_Pyramid;
            case 2: return Jungle_Pyramid;
            case 3: return Swamp_Hut;
            case 4: return Igloo;
            case 5: return Monument;
            case 6: return Mansion;
            case 7: return Outpost;
            case 8: return Ancient_City;
            case 9: return Ruined_Portal;
            case 10: return Shipwreck;
            default: return Village;
        }
    }

    int getIndexFromStructureType(int structureType) {
        switch(structureType) {
            case Village: return 0;
            case Desert_Pyramid: return 1;
            case Jungle_Pyramid: return 2;
            case Swamp_Hut: return 3;
            case Igloo: return 4;
            case Monument: return 5;
            case Mansion: return 6;
            case Outpost: return 7;
            case Ancient_City: return 8;
            case Ruined_Portal: return 9;
            case Shipwreck: return 10;
            default: return 0;
        }
    }

    void startSearch() {
        if (isSearching) {
            stopSearch();
        }

        resetSearchMetrics();
        
        {
            std::lock_guard<std::mutex> lock(structuresMutex);
            structureNames.clear();
            positions.clear();
            foundSeeds.clear();
            
            if (multiStructureMode) {
                for (auto& attached : attachedStructures) {
                    if (attached.required) {
                        attached.found = false;
                    }
                }
            }
        }

        // Start the timer
        searchStartTime = std::chrono::steady_clock::now();
        timerRunning = true;
        elapsedSearchTime = 0.0;

        shouldStop = false;
        isSearching = true;
        
        try {
            searchThreads.clear();
            
            // Pre-generate seed batches for each thread
            std::vector<std::vector<int64_t>> threadSeeds(numThreads);
            std::mt19937_64 globalGen(rd());
            std::uniform_int_distribution<int64_t> dist;
            
            if (useBedrockRange) {
                dist = std::uniform_int_distribution<int64_t>(INT32_MIN, INT32_MAX);
            } else {
                dist = std::uniform_int_distribution<int64_t>(INT64_MIN, INT64_MAX);
            }

            // Pre-generate seeds for each thread
            for (int i = 0; i < numThreads; i++) {
                threadSeeds[i].reserve(OPTIMAL_BATCH_SIZE);
                for (int j = 0; j < OPTIMAL_BATCH_SIZE; j++) {
                    threadSeeds[i].push_back(dist(globalGen));
                }
            }
            
            // Create threads with pre-generated seeds
            for (int i = 0; i < numThreads; i++) {
                searchThreads.emplace_back([this, i, seedBatch = std::move(threadSeeds[i])]() {
                    try {
                        std::mt19937_64 localGen(rd() + i);
                        std::uniform_int_distribution<int64_t> localDist;
                        
                        if (useBedrockRange) {
                            localDist = std::uniform_int_distribution<int64_t>(INT32_MIN, INT32_MAX);
                        } else {
                            localDist = std::uniform_int_distribution<int64_t>(INT64_MIN, INT64_MAX);
                        }

                        int statusCounter = 0;
                        size_t seedIndex = 0;
                        
                        while (!shouldStop) {
                            int64_t seedToCheck = (seedIndex < seedBatch.size()) ? 
                                                seedBatch[seedIndex++] : 
                                                localDist(localGen);

                            if (++statusCounter >= STATUS_UPDATE_INTERVAL) {
                                std::lock_guard<std::mutex> lock(structuresMutex);
                                currentSeed = seedToCheck;
                                std::string rangeType = useBedrockRange ? "2^32" : "2^64";
                                currentStatus = "[T" + std::to_string(i) + "] Processing seed " + std::to_string(seedToCheck);
                                statusCounter = 0;
                            }

                            Pos pos;
                            bool found = false;

                            try {
                                if (multiStructureMode) {
                                    found = findMultipleStructures(seedToCheck, &pos);
                                } else {
                                    found = findStructure(seedToCheck, &pos, searchRadius);
                                }
                            } catch (const std::exception& e) {
                                continue;
                            }

                            seedsChecked++;
                            
                            if (found) {
                                std::lock_guard<std::mutex> lock(structuresMutex);
                                
                                if (multiStructureMode) {
                                    // Create combined structure description
                                    std::string structures = std::string(struct2str(baseStructureType)) + 
                                                           " [" + std::to_string(pos.x) +
                                                           ", " + std::to_string(pos.z) + "]";
                                    
                                    // Count enabled and found structures
                                    int enabledCount = 0;
                                    int foundCount = 0;
                                    for (const auto& attached : attachedStructures) {
                                        if (attached.required) {
                                            enabledCount++;
                                            if (attached.found) {
                                                foundCount++;
                                                structures += "\n+ " + std::string(struct2str(attached.structureType)) +
                                                            " [" + std::to_string(attached.foundPos.x) +
                                                            ", " + std::to_string(attached.foundPos.z) + "]";
                                            }
                                        }
                                    }
                                    
                                    // Only add to results if we found all required structures
                                    if (foundCount == enabledCount || !continuousSearch) {
                                        structureNames.push_back(structures);
                                        positions.push_back(pos);
                                        foundSeeds.push_back(seedToCheck);
                                        
                                        // Update status with found information
                                        std::string foundMsg = "[FOUND] Seed: " + std::to_string(seedToCheck) + "\n";
                                        foundMsg += "Base " + std::string(struct2str(baseStructureType)) +
                                                  ": [" + std::to_string(pos.x) +
                                                  ", " + std::to_string(pos.z) + "]";
                                        
                                        for (const auto& attached : attachedStructures) {
                                            if (attached.required && attached.found) {
                                                int dx = attached.foundPos.x - pos.x;
                                                int dz = attached.foundPos.z - pos.z;
                                                int distance = (int)sqrt(dx*dx + dz*dz);
                                                
                                                foundMsg += "\n" + std::string(struct2str(attached.structureType)) +
                                                          ": [" + std::to_string(attached.foundPos.x) +
                                                          ", " + std::to_string(attached.foundPos.z) + "]" +
                                                          " (Distance: " + std::to_string(distance) + "m)";
                                            }
                                        }
                                        currentStatus = foundMsg;
                                        
                                        if (!continuousSearch) {
                                            shouldStop = true;
                                            break;
                                        }
                                    }
                                } else {
                                    structureNames.push_back(struct2str(selectedStructure));
                                    positions.push_back(pos);
                                    foundSeeds.push_back(seedToCheck);
                                    currentStatus = "[FOUND] Seed: " + std::to_string(seedToCheck) +
                                                  " | Coords: [" + std::to_string(pos.x) +
                                                  ", " + std::to_string(pos.z) + "]" +
                                                  " | Distance: " + std::to_string((int)sqrt(pow(pos.x, 2) + pow(pos.z, 2))) + "m";
                                    
                                    if (!continuousSearch) {
                                        shouldStop = true;
                                        break;
                                    }
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
        
        // Stop the timer and calculate final time
        if (timerRunning) {
            elapsedSearchTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - searchStartTime).count();
            timerRunning = false;
        }
        
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
        ImGui::Text("ChunkBiomes - Minecraft Seed Finder");
        ImGui::Separator();

        // Important Notice about Chunkbase
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "IMPORTANT NOTICE:");
        ImGui::TextWrapped(
            "This application is a desktop port of Chunkbase's seed finding functionality. "
            "The core functionality and algorithms are based on Chunkbase (https://www.chunkbase.com/), "
            "created by Alexander Gundermann and the Chunkbase development team. "
            "Please support the original project by visiting their website."
        );

        // Project Description
        ImGui::Spacing();
        ImGui::TextWrapped("A desktop tool for finding Minecraft Bedrock seeds with specific structures and biome characteristics, "
                          "based on Chunkbase's functionality.");

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
        ImGui::BulletText("Chunkbase - Core Functionality");
        ImGui::BulletText("Cubiomes - World Generation Library");
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
        ImGui::Text("Credits and Contributors:");
        ImGui::BulletText("Alexander Gundermann - Original Creator (Chunkbase)");
        ImGui::BulletText("Chunkbase Development Team - Original Implementation");
        ImGui::BulletText("NelS - Porter");
        ImGui::BulletText("MZEEN - GUI Contributions");
        ImGui::BulletText("Fragrant_Result_186 - Project Helper");
        ImGui::BulletText("Cubitect - Cubiomes Library (Public Domain)");

        // Version and License
        ImGui::Separator();
        ImGui::Text("Version: BetaV3");
        ImGui::TextWrapped(
            "License Information:\n"
            "- Core functionality and algorithms: Rights reserved by Chunkbase\n"
            "- Cubiomes components: Public Domain\n"
            "- Third-party libraries: Various open source licenses"
        );

        // GitHub and Support
        ImGui::Separator();
        if (ImGui::Button("Visit Chunkbase")) {
            system("start https://www.chunkbase.com/");
        }
        ImGui::SameLine();
        if (ImGui::Button("Report an Issue")) {
            system("start https://github.com/MZEEN2424/ChunkBiomesGUI/issues");
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

        // Structure Type Selection
        ImGui::Text("Structure Finding Type:");
        ImGui::SameLine();
        
        // Mode selection
        if (ImGui::RadioButton("Single", !multiStructureMode)) {
            multiStructureMode = false;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Multiple", multiStructureMode)) {
            multiStructureMode = true;
            if (attachedStructures.empty()) {
                // Initialize with some default attached structures
                attachedStructures.resize(3, AttachedStructure());
            }
        }

        // Seed Range Selection
        ImGui::Text("Seed Range:");
        float spacing = 10.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(spacing, 0));
        
        if (ImGui::RadioButton("32-Bit Range", useBedrockRange)) {
            useBedrockRange = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("?##bedrock", ImVec2(25, 0))) {}
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(
                "32-Bit Range:\n"
                "Range: -2,147,483,648 to 2,147,483,647\n"
                "Total: 4,294,967,296 seeds (2^32)"
            );
            ImGui::EndTooltip();
        }
        
        ImGui::SameLine();
        if (ImGui::RadioButton("64-Bit Range", !useBedrockRange)) {
            useBedrockRange = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("?##full", ImVec2(25, 0))) {}
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(
                "64-Bit Range:\n"
                "Range: -9,223,372,036,854,775,808 to\n"
                "       9,223,372,036,854,775,807\n"
                "Total: 18,446,744,073,709,551,616 seeds (2^64)"
            );
            ImGui::EndTooltip();
        }
        
        ImGui::PopStyleVar();
        ImGui::Separator();

        if (!multiStructureMode) {
            // Original single structure selection
            const char* structures[] = {
                "Village", "Desert Pyramid", "Jungle Pyramid", "Swamp Hut",
                "Igloo", "Monument", "Mansion", "Outpost", 
                "Ancient City", "Ruined Portal", "Shipwreck"
            };
            static int structureIndex = 0;
            if (ImGui::Combo("Structure Type", &structureIndex, structures, IM_ARRAYSIZE(structures))) {
                selectedStructure = getStructureTypeFromIndex(structureIndex);
            }
        } else {
            // Multi-structure mode UI
            ImGui::BeginChild("MultiStructureConfig", ImVec2(0, 250), true);
            
            // Base structure selection
            const char* structures[] = {
                "Village", "Desert Pyramid", "Jungle Pyramid", "Swamp Hut",
                "Igloo", "Monument", "Mansion", "Outpost", 
                "Ancient City", "Ruined Portal", "Shipwreck"
            };
            static int baseIndex = 0;
            if (ImGui::Combo("Base Structure", &baseIndex, structures, IM_ARRAYSIZE(structures))) {
                baseStructureType = getStructureTypeFromIndex(baseIndex);
                selectedStructure = baseStructureType;
            }

            ImGui::Text("Attached Structures:");
            ImGui::Separator();

            // Table for attached structures
            if (ImGui::BeginTable("AttachedStructures", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Enable");
                ImGui::TableSetupColumn("Structure Type");
                ImGui::TableSetupColumn("Distance");
                ImGui::TableSetupColumn("Status");
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < attachedStructures.size(); i++) {
                    ImGui::TableNextRow();
                    
                    // Enable checkbox
                    ImGui::TableNextColumn();
                    bool enabled = attachedStructures[i].required;
                    if (ImGui::Checkbox(("##enable" + std::to_string(i)).c_str(), &enabled)) {
                        attachedStructures[i].required = enabled;
                    }

                    // Structure type selection
                    ImGui::TableNextColumn();
                    int structIndex = getIndexFromStructureType(attachedStructures[i].structureType);
                    if (ImGui::Combo(("##type" + std::to_string(i)).c_str(), &structIndex, structures, IM_ARRAYSIZE(structures))) {
                        attachedStructures[i].structureType = getStructureTypeFromIndex(structIndex);
                    }

                    // Distance input
                    ImGui::TableNextColumn();
                    int radius = attachedStructures[i].maxDistance;
                    if (ImGui::InputInt(("##radius" + std::to_string(i)).c_str(), &radius, 16, 100)) {
                        if (radius < 16) radius = 16;
                        if (radius > 10000) radius = 10000;
                        attachedStructures[i].maxDistance = radius;
                    }

                    // Status column
                    ImGui::TableNextColumn();
                    if (attachedStructures[i].found) {
                        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Found!");
                        if (ImGui::IsItemHovered()) {
                            ImGui::BeginTooltip();
                            ImGui::Text("X: %d, Z: %d", attachedStructures[i].foundPos.x, attachedStructures[i].foundPos.z);
                            ImGui::EndTooltip();
                        }
                    } else if (isSearching) {
                        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Searching...");
                    } else {
                        ImGui::TextDisabled("-");
                    }
                }
                ImGui::EndTable();
            }

            if (ImGui::Button("Add Structure")) {
                if (attachedStructures.size() < 5) { // Limit to 5 attached structures
                    attachedStructures.push_back(AttachedStructure());
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Structure") && !attachedStructures.empty()) {
                attachedStructures.pop_back();
            }

            ImGui::EndChild();
        }

        // Rest of the original UI (radius, continuous search, etc.)
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

        // Display timer
        if (timerRunning) {
            elapsedSearchTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - searchStartTime).count();
        }
        ImGui::Text("Search Time: %.2f seconds", elapsedSearchTime);

        // Status message
        if (isSearching) {
            std::string status;
            {
                std::lock_guard<std::mutex> lock(structuresMutex);
                status = currentStatus;
            }
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", status.c_str());

            // Display seeds per second
            double seedsPerSecond = calculateSeedsPerSecond();
            ImGui::Text("Processing %.0f seeds/second", seedsPerSecond);
            
            // Display total seeds checked
            ImGui::Text("Total Seeds Checked: %lld", seedsChecked.load());
        } else if (seedsChecked > 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Search Stopped");
            ImGui::Text("Total Seeds Checked: %lld", seedsChecked.load());
        }

        ImGui::Separator();

        // Display found seeds in a table format
        if (!foundSeeds.empty()) {
            ImGui::Text("Found Seeds:");
            
            // Align buttons to the right
            float windowWidth = ImGui::GetWindowWidth();
            ImGui::SameLine(windowWidth - 200);
            if (ImGui::Button("Clear Seeds")) {
                foundSeeds.clear();
                structureNames.clear();
                positions.clear();
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Save Seeds")) {
                saveSeedsToFile();
            }

            // Create a scrollable table for seeds
            if (ImGui::BeginTable("SeedsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | 
                                                  ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | 
                                                  ImGuiTableFlags_Reorderable, ImVec2(0, 300))) {
                
                // Setup columns
                ImGui::TableSetupColumn("No.", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Seed", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Structure", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Coordinates", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                ImGui::TableHeadersRow();

                // Display seeds in rows
                for (size_t i = 0; i < foundSeeds.size(); ++i) {
                    ImGui::TableNextRow();
                    
                    // Number column
                    ImGui::TableNextColumn();
                    ImGui::Text("#%zu", i + 1);

                    // Seed column
                    ImGui::TableNextColumn();
                    ImGui::Text("%lld", foundSeeds[i]);

                    // Structure and Coordinates columns
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    if (i < structureNames.size() && i < positions.size()) {
                        std::string structureInfo = structureNames[i];
                        std::string coordsInfo;
                        
                        // Check if this is a multi-structure result
                        size_t newlinePos = structureInfo.find('\n');
                        if (newlinePos != std::string::npos) {
                            // Multiple structures
                            std::string baseStructure = structureInfo.substr(0, newlinePos);
                            std::string attachedStructures = structureInfo.substr(newlinePos + 1);
                            
                            // Display base structure in Structure column
                            ImGui::TableSetColumnIndex(2); // Go back to Structure column
                            ImGui::Text("%s", baseStructure.c_str());
                            
                            // Display coordinates with structure names
                            ImGui::TableSetColumnIndex(3); // Go to Coordinates column
                            
                            // First display base structure coordinates
                            size_t bracketStart = baseStructure.find('[');
                            size_t bracketEnd = baseStructure.find(']');
                            if (bracketStart != std::string::npos && bracketEnd != std::string::npos) {
                                std::string baseCoords = baseStructure.substr(bracketStart, bracketEnd - bracketStart + 1);
                                ImGui::Text("Base Structure: %s", baseCoords.c_str());
                            }
                            
                            // Then display attached structures' coordinates
                            std::istringstream attachedStream(attachedStructures);
                            std::string line;
                            while (std::getline(attachedStream, line)) {
                                if (!line.empty()) {
                                    size_t nameEnd = line.find('[');
                                    if (nameEnd != std::string::npos) {
                                        std::string structName = line.substr(2, nameEnd - 3); // Remove "+ " prefix
                                        std::string coords = line.substr(nameEnd);
                                        coords = coords.substr(0, coords.find(" (Distance")); // Remove distance info
                                        ImGui::Text("%s: %s", structName.c_str(), coords.c_str());
                                    }
                                }
                            }
                        } else {
                            // Single structure
                            ImGui::TableSetColumnIndex(2); // Go back to Structure column
                            ImGui::Text("%s", structureNames[i].c_str());
                            
                            ImGui::TableSetColumnIndex(3); // Go to Coordinates column
                            ImGui::Text("[X: %d, Z: %d]", positions[i].x, positions[i].z);
                        }
                    }

                    // Actions column
                    ImGui::TableNextColumn();
                    ImGui::PushID(static_cast<int>(i));
                    if (ImGui::Button("Copy")) {
                        char seedStr[32];
                        snprintf(seedStr, sizeof(seedStr), "%lld", foundSeeds[i]);
                        ImGui::SetClipboardText(seedStr);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Copy seed to clipboard");
                    }
                    ImGui::PopID();
                }
                
                ImGui::EndTable();
            }
        }
    }

    void renderSettingsTab() {
        if (ImGui::BeginTabItem("Settings")) {
            ImGui::Text("Performance Settings");
            ImGui::Separator();
            
            // Batch Size slider
            int batchSize = OPTIMAL_BATCH_SIZE;
            if (ImGui::SliderInt("Batch Size", &batchSize, 10000, 1000000, "%d", ImGuiSliderFlags_Logarithmic)) {
                OPTIMAL_BATCH_SIZE = batchSize;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Number of seeds to process in each batch\nHigher values = more memory usage but potentially faster");
            }
            
            // Generator Pool Size slider
            int poolSize = generatorPoolSize;
            if (ImGui::SliderInt("Generator Pool Size", &poolSize, 1, 128)) {
                generatorPoolSize = poolSize;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Number of parallel generators\nRecommended: Set to number of CPU threads");
            }
            
            // UI Color Settings
            ImGui::Separator();
            ImGui::Text("UI Colors");
            bool colorChanged = false;
            
            ImGui::TextColored(uiColors.text, "Current Colors:");
            colorChanged |= ImGui::ColorEdit3("Main Text", (float*)&uiColors.targetText);
            colorChanged |= ImGui::ColorEdit3("Background", (float*)&uiColors.targetBackground);
            colorChanged |= ImGui::ColorEdit3("Frame Background", (float*)&uiColors.targetFrame);
            colorChanged |= ImGui::ColorEdit3("Interactive Elements", (float*)&uiColors.targetAccent);
            colorChanged |= ImGui::ColorEdit3("Buttons & Tabs", (float*)&uiColors.targetButton);
            colorChanged |= ImGui::ColorEdit3("Headers", (float*)&uiColors.targetHeader);
            colorChanged |= ImGui::ColorEdit4("Border", (float*)&uiColors.targetBorder);

            // Add transition speed slider
            static float transitionSpeed = 0.1f;
            ImGui::SliderFloat("Transition Speed", &transitionSpeed, 0.01f, 0.5f, "%.2f");

            if (ImGui::Button("Reset Colors")) {
                UIColors defaultColors;
                uiColors.targetText = defaultColors.text;
                uiColors.targetBackground = defaultColors.background;
                uiColors.targetFrame = defaultColors.frame;
                uiColors.targetButton = defaultColors.button;
                uiColors.targetHeader = defaultColors.header;
                uiColors.targetBorder = defaultColors.border;
                uiColors.targetAccent = defaultColors.accent;
            }

            ImGui::Separator();
            ImGui::Text("Save/Load UI Colors");
            if (ImGui::Button("Save UI Colors")) {
                std::string filename = ShowSaveFileDialog();
                if (!filename.empty()) {
                    SaveUIColors(filename.c_str());
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Load UI Colors")) {
                std::string filename = ShowOpenFileDialog();
                if (!filename.empty()) {
                    LoadUIColors(filename.c_str());
                    colorChanged = true;
                }
            }

            if (colorChanged) {
                ApplyCustomColors();
            }

            ImGui::EndTabItem();
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

            // Settings Tab
            renderSettingsTab();

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

    bool isDeepOcean(int biomeId) {
        return biomeId == deep_ocean ||
               biomeId == deep_frozen_ocean ||
               biomeId == deep_cold_ocean ||
               biomeId == deep_lukewarm_ocean;
    }

    bool isShipwreckBiome(int biomeId) {
       return biomeId == beach ||
           biomeId == snowy_beach ||
           biomeId == ocean ||
           biomeId == frozen_ocean ||
           biomeId == deep_frozen_ocean ||
           biomeId == cold_ocean ||
           biomeId == deep_cold_ocean ||
           biomeId == lukewarm_ocean ||
           biomeId == deep_lukewarm_ocean ||
           biomeId == warm_ocean;
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

    void resetSearchMetrics() {
        seedsChecked = 0;
        foundSeeds.clear();
        searchStartTime = std::chrono::steady_clock::now();
        lastCalculatedSeedsPerSecond = 0.0;
    }

    double calculateSeedsPerSecond() {
        auto now = std::chrono::steady_clock::now();
        auto endTime = isSearching ? now : std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - searchStartTime).count();
        
        if (duration > 0) {
            lastCalculatedSeedsPerSecond = seedsChecked * 1000.0 / duration;
        }
        
        return lastCalculatedSeedsPerSecond;
    }

    bool isWithinRadius(const Pos& pos, int radius) {
        return (int)sqrt(pow(pos.x, 2) + pow(pos.z, 2)) <= radius;
    }

    bool checkSurroundingBiomes(int centerX, int centerZ, int biomeId, int radius, Generator& g) {
        int count = 0;
        int total = 0;
        const float threshold = 0.9f;
        const int step = 8; // Check every 8 blocks for speed
        const int radiusSquared = radius * radius;
        
        // Quick cardinal check first
        const int cardinals[4][2] = {{0,radius}, {radius,0}, {0,-radius}, {-radius,0}};
        for(int i = 0; i < 4; i++) {
            if(getBiomeAt(&g, 4, (centerX + cardinals[i][0]) >> 2, 319>>2, (centerZ + cardinals[i][1]) >> 2) != biomeId) {
                return false; // Fail fast if cardinal points don't match
            }
        }
        
        // Check in quadrants for better cache locality
        for(int quadrant = 0; quadrant < 4; quadrant++) {
            const int startX = (quadrant & 1) ? 0 : -radius;
            const int endX = (quadrant & 1) ? radius : 0;
            const int startZ = (quadrant & 2) ? 0 : -radius;
            const int endZ = (quadrant & 2) ? radius : 0;
            
            for(int x = startX; x <= endX; x += step) {
                for(int z = startZ; z <= endZ; z += step) {
                    int distSq = x*x + z*z;
                    if(distSq <= radiusSquared) {
                        total++;
                        if(getBiomeAt(&g, 4, (centerX + x) >> 2, 319>>2, (centerZ + z) >> 2) == biomeId) {
                            count++;
                            // Early success check
                            if((float)count/total >= threshold && total >= 10) {
                                return true;
                            }
                        } else {
                            // Early failure check
                            int remainingPoints = (radiusSquared / (step * step)) / 4; // Approximate remaining points
                            if((float)(count + remainingPoints)/(total + remainingPoints) < threshold) {
                                return false;
                            }
                        }
                    }
                }
            }
        }
        
        return total > 0 && (float)count/total >= threshold;
    }

    bool findStructure(int64_t seed, Pos* pos, int radius) {
        static thread_local Generator g;
        static thread_local bool initialized = false;
        
        if (!initialized) {
            setupGenerator(&g, MC_NEWEST, 0);
            initialized = true;
        }

        int32_t seed32 = (int32_t)(seed & 0xFFFFFFFF);
        int regionRadius = (radius / 512) + 1;
        const int regionStep = 1;

        for (int regionX = -regionRadius; regionX <= regionRadius; regionX += regionStep) {
            for (int regionZ = -regionRadius; regionZ <= regionRadius; regionZ += regionStep) {
                StructureConfig sconf;
                if (!getBedrockStructureConfig(selectedStructure, g.mc, &sconf)) {
                    continue;
                }

                Pos p;
                if (!getBedrockStructurePos(selectedStructure, g.mc, seed32, regionX, regionZ, &p)) {
                    continue;
                }

                if (!isWithinRadius(p, radius)) {
                    continue;
                }

                g.seed = seed;
                g.dim = DIM_OVERWORLD;
                applySeed(&g, DIM_OVERWORLD, seed);

                if (!isViableStructurePos(selectedStructure, &g, p.x, p.z, 0)) {
                    continue;
                }

                if (!isViableStructureTerrain(selectedStructure, &g, p.x, p.z)) {
                    continue;
                }

                int biomeId = getBiomeAt(&g, 4, p.x >> 2, 319>>2, p.z >> 2);
                if(biomeId == none) continue;

                bool validBiome = true;
                if (selectedStructure == Monument && !isDeepOcean(biomeId)) validBiome = false;
                else if (selectedStructure == Mansion && biomeId != dark_forest) validBiome = false;
                else if (selectedStructure == Shipwreck && !isShipwreckBiome(biomeId)) validBiome = false;
                else if (selectedStructure == Village && !isVillageBiome(biomeId)) validBiome = false;

                if (!validBiome) continue;

                // Additional biome checks
                if (selectedStructure == Monument || 
                    selectedStructure == Mansion || 
                    selectedStructure == Shipwreck || 
                    selectedStructure == Village) {
                    if (selectedStructure == Shipwreck) {
                        if (!isShipwreckBiome(biomeId)) continue;
                        if (!checkSurroundingBiomes(p.x, p.z, biomeId, 48, g)) continue;
                    } else if (selectedStructure == Monument || 
                               selectedStructure == Mansion || 
                               selectedStructure == Village) {
                        if (!checkSurroundingBiomes(p.x, p.z, biomeId, selectedStructure == Mansion ? 64 : 32, g)) {
                            continue;
                        }
                    }
                }

                *pos = p;
                return true;
            }
        }
        return false;
    }

    bool findMultipleStructures(int64_t seed, Pos* basePos) {
        try {
            // Use thread-local generator pool for thread safety
            static thread_local Generator g;  // Single generator per thread is enough
            static thread_local bool initialized = false;
            
            if (!initialized) {
                setupGenerator(&g, MC_NEWEST, 0);
                initialized = true;
            }

            // Get the lowest 32 bits of the seed
            int32_t seed32 = (int32_t)(seed & 0xFFFFFFFF);

            // Keep track of found positions to avoid duplicates
            std::vector<Pos> foundPositions;

            // First find the base structure
            selectedStructure = baseStructureType;  // Set to base structure type
            if (!findStructure(seed, basePos, searchRadius)) {
                return false;
            }
            foundPositions.push_back(*basePos);

            // Count enabled structures and reset found flags
            int enabledCount = 0;
            int foundCount = 0;
            for (auto& attached : attachedStructures) {
                if (attached.required) {
                    enabledCount++;
                    attached.found = false;
                }
            }

            // If no structures are enabled, consider it a success
            if (enabledCount == 0) {
                return true;
            }

            // Now check for each enabled attached structure
            for (auto& attached : attachedStructures) {
                if (!attached.required) continue;

                // Try to find the attached structure within its radius from the base structure
                bool found = false;

                // Search in regions around the base structure
                int regionRadius = (attached.maxDistance / 512) + 1;

                try {
                    for (int regionX = -regionRadius; regionX <= regionRadius && !found; ++regionX) {
                        for (int regionZ = -regionRadius; regionZ <= regionRadius && !found; ++regionZ) {
                            if (shouldStop) return false;  // Check for stop signal

                            Pos p;
                            // First check with lower 32 bits
                            if (!getBedrockStructurePos(attached.structureType, g.mc, seed32, regionX, regionZ, &p)) {
                                continue;
                            }

                            // Calculate distance from base structure
                            int dx = p.x - basePos->x;
                            int dz = p.z - basePos->z;
                            int distance = (int)sqrt(dx*dx + dz*dz);

                            if (distance > attached.maxDistance) {
                                continue;
                            }

                            // Check if this position is too close to any already found structure
                            bool tooClose = false;
                            for (const Pos& foundPos : foundPositions) {
                                int fdx = p.x - foundPos.x;
                                int fdz = p.z - foundPos.z;
                                int foundDistance = (int)sqrt(fdx*fdx + fdz*fdz);
                                // If it's the same structure type, ensure minimum distance of 128 blocks
                                if ((attached.structureType == baseStructureType || 
                                    std::any_of(attachedStructures.begin(), attachedStructures.end(), 
                                              [&](const AttachedStructure& other) { 
                                                  return other.found && other.structureType == attached.structureType; 
                                              })) && 
                                    foundDistance < 128) {
                                    tooClose = true;
                                    break;
                                }
                            }
                            if (tooClose) continue;

                            // Only now initialize the generator since we have a potential match
                            g.seed = seed;
                            g.dim = DIM_OVERWORLD;
                            applySeed(&g, DIM_OVERWORLD, seed);

                            // Basic validation first
                            if (!isViableStructurePos(attached.structureType, &g, p.x, p.z, 0)) {
                                continue;
                            }

                            // Skip terrain check for certain structures
                            bool skipTerrainCheck = (attached.structureType == Ancient_City || 
                                                  attached.structureType == Monument);
                            
                            if (!skipTerrainCheck && !isViableStructureTerrain(attached.structureType, &g, p.x, p.z)) {
                                continue;
                            }

                            // Additional biome checks based on structure type
                            int biomeId = getBiomeAt(&g, 4, p.x >> 2, 319>>2, p.z >> 2);
                            if(biomeId == none) continue;  // Skip invalid biomes
                            
                            bool validBiome = true;
                            if (attached.structureType == Monument) {
                                if (!isDeepOcean(biomeId)) {
                                    validBiome = false;
                                }
                            }
                            else if (attached.structureType == Mansion) {
                                if (biomeId != dark_forest) {
                                    validBiome = false;
                                }
                            }
                            else if (attached.structureType == Shipwreck) {
                                if (!isShipwreckBiome(biomeId)) {
                                    validBiome = false;
                                }
                            }
                            else if (attached.structureType == Village) {
                                if (!isVillageBiome(biomeId)) {
                                    validBiome = false;
                                }
                            }

                            if (!validBiome) continue;

                            // Structure found within radius and not too close to other structures
                            attached.foundPos = p;
                            attached.found = true;
                            foundCount++;
                            found = true;
                            foundPositions.push_back(p);
                        }
                    }
                } catch (const std::exception& e) {
                    // Log error but continue with next structure
                    currentStatus = "⚠️ Error checking structure: " + std::string(e.what());
                    continue;
                }
            }

            // Return true only if all enabled structures are found
            return foundCount == enabledCount;

        } catch (const std::exception& e) {
            currentStatus = "⚠️ Multiple structure search error: " + std::string(e.what());
            return false;
        }
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

    // Initialize custom colors
    ApplyCustomColors();

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

        // Update colors with smooth transitions
        UpdateColors();

        // Set the clear color to match our background color
        glClearColor(uiColors.background.x, uiColors.background.y, uiColors.background.z, uiColors.background.w);
        glClear(GL_COLOR_BUFFER_BIT);

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
