#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h>
#include <Windows.h>
#include <filesystem>
#include <vector>
#include <fstream>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <random>
#include <set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include "Brng.h"

// Global variables
static int OPTIMAL_BATCH_SIZE = 200000;
static int generatorPoolSize = 64;
static float transitionSpeed = 0.1f;

// Forward declarations
bool LoadUIColors(const char* filename);

std::string GetExePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string::size_type pos = std::string(buffer).find_last_of("\\/");
    return std::string(buffer).substr(0, pos);
}

struct ThemeFile {
    std::string name;
    std::string path;
};

std::vector<ThemeFile> GetThemeFiles() {
    std::vector<ThemeFile> themes;
    std::string themesPath = GetExePath() + "\\themes";
    
    printf("Looking for themes in: %s\n", themesPath.c_str());
    
    if (!std::filesystem::exists(themesPath)) {
        printf("Themes directory not found! Creating it...\n");
        std::filesystem::create_directory(themesPath);
        return themes;
    }
    
    printf("Themes directory found, scanning for .txt files...\n");
    
    for (const auto& entry : std::filesystem::directory_iterator(themesPath)) {
        printf("Found file: %s\n", entry.path().string().c_str());
        if (entry.path().extension() == ".txt") {
            ThemeFile theme;
            theme.name = entry.path().filename().string();
            theme.path = entry.path().string();
            themes.push_back(theme);
            printf("Added theme: %s\n", theme.name.c_str());
        }
    }
    
    printf("Found %zu themes total\n", themes.size());
    return themes;
}

struct AppSettings {
    // Performance Settings
    int batchSize = 200000;
    int generatorPoolSize = 64;
    int threadCount = 1;
    
    // UI Settings
    float guiScale = 1.0f;
    std::string autoLoadColorFile = "";
    bool autoLoadColors = false;

    void saveToFile(const char* filename) {
        FILE* f = fopen(filename, "w");
        if (f) {
            fprintf(f, "[Performance]\n");
            fprintf(f, "batchSize=%d\n", batchSize);
            fprintf(f, "generatorPoolSize=%d\n", generatorPoolSize);
            fprintf(f, "threadCount=%d\n", threadCount);
            
            fprintf(f, "\n[UI]\n");
            fprintf(f, "guiScale=%f\n", guiScale);
            fprintf(f, "autoLoadColors=%d\n", autoLoadColors ? 1 : 0);
            fprintf(f, "autoLoadColorFile=%s\n", autoLoadColorFile.c_str());
            fclose(f);
        }
    }
    
    bool loadFromFile(const char* filename) {
        FILE* f = fopen(filename, "r");
        if (!f) {
            // Create default settings if file doesn't exist
            batchSize = 200000;
            generatorPoolSize = 64;
            threadCount = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
            guiScale = 1.0f;
            autoLoadColorFile = "";
            autoLoadColors = false;
            saveToFile(filename);
            return true;
        }

        char line[1024];
        char section[64] = "";

        while (fgets(line, sizeof(line), f)) {
            // Remove newline
            char* newline = strchr(line, '\n');
            if (newline) *newline = 0;

            // Skip empty lines
            if (line[0] == 0) continue;

            // Parse section
            if (line[0] == '[') {
                char* end = strchr(line, ']');
                if (end) {
                    *end = 0;
                    strcpy(section, line + 1);
                }
                continue;
            }

            // Parse key=value
            char* equals = strchr(line, '=');
            if (!equals) continue;
            *equals = 0;
            const char* key = line;
            const char* value = equals + 1;

            if (strcmp(section, "Performance") == 0) {
                if (strcmp(key, "batchSize") == 0) batchSize = atoi(value);
                else if (strcmp(key, "generatorPoolSize") == 0) generatorPoolSize = atoi(value);
                else if (strcmp(key, "threadCount") == 0) threadCount = atoi(value);
            }
            else if (strcmp(section, "UI") == 0) {
                if (strcmp(key, "guiScale") == 0) guiScale = (float)atof(value);
                else if (strcmp(key, "autoLoadColors") == 0) autoLoadColors = atoi(value) != 0;
                else if (strcmp(key, "autoLoadColorFile") == 0) autoLoadColorFile = value;
            }
        }

        fclose(f);
        return true;
    }
};

static AppSettings appSettings;

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

#include "cubiomes/generator.h"
#include "cubiomes/finders.h"
#include "Bfinders.h"

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
        file << "; Theme file for ChunkBiomesGUI\n";
        file << "; Format: name=r,g,b,a (values from 0.0 to 1.0)\n\n";
        
        file << "text=" << uiColors.text.x << "," << uiColors.text.y << "," << uiColors.text.z << "," << uiColors.text.w << "\n";
        file << "background=" << uiColors.background.x << "," << uiColors.background.y << "," << uiColors.background.z << "," << uiColors.background.w << "\n";
        file << "frame=" << uiColors.frame.x << "," << uiColors.frame.y << "," << uiColors.frame.z << "," << uiColors.frame.w << "\n";
        file << "button=" << uiColors.button.x << "," << uiColors.button.y << "," << uiColors.button.z << "," << uiColors.button.w << "\n";
        file << "header=" << uiColors.header.x << "," << uiColors.header.y << "," << uiColors.header.z << "," << uiColors.header.w << "\n";
        file << "border=" << uiColors.border.x << "," << uiColors.border.y << "," << uiColors.border.z << "," << uiColors.border.w << "\n";
        file << "accent=" << uiColors.accent.x << "," << uiColors.accent.y << "," << uiColors.accent.z << "," << uiColors.accent.w << "\n";
        file.close();
        printf("Theme saved successfully!\n");
    }
}

// Function to load UI colors from a file
bool LoadUIColors(const char* filename) {
    printf("Loading theme from: %s\n", filename);
    std::ifstream file(filename);
    if (!file.is_open()) {
        printf("Failed to open theme file!\n");
        return false;
    }

    UIColors newColors = uiColors;  // Create a temporary copy
    bool foundAnyColor = false;
    std::string line;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == ';') continue;
        
        // Parse key=value
        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;
        
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        
        float r, g, b, a;
        if (sscanf(value.c_str(), "%f,%f,%f,%f", &r, &g, &b, &a) == 4) {
            ImVec4 color(r, g, b, a);
            printf("Found color: %s = %.2f, %.2f, %.2f, %.2f\n", key.c_str(), r, g, b, a);
            foundAnyColor = true;
            
            if (key == "text") newColors.text = color;
            else if (key == "background") newColors.background = color;
            else if (key == "frame") newColors.frame = color;
            else if (key == "button") newColors.button = color;
            else if (key == "header") newColors.header = color;
            else if (key == "border") newColors.border = color;
            else if (key == "accent") newColors.accent = color;
            else {
                printf("Unknown color key: %s\n", key.c_str());
            }
        }
    }
    
    file.close();
    
    if (foundAnyColor) {
        // Update both current and target colors for smooth transition
        uiColors.targetText = newColors.text;
        uiColors.targetBackground = newColors.background;
        uiColors.targetFrame = newColors.frame;
        uiColors.targetButton = newColors.button;
        uiColors.targetHeader = newColors.header;
        uiColors.targetBorder = newColors.border;
        uiColors.targetAccent = newColors.accent;
        
        printf("Theme loaded successfully!\n");
        return true;
    }
    
    printf("No valid colors found in theme file!\n");
    return false;
}

bool showThemeSelector = false;
std::vector<ThemeFile> availableThemes;

void ShowThemeSelector() {
    if (!showThemeSelector) return;

    // Center the window
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    // Make it a popup modal instead of a regular window
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    bool open = true;
    if (ImGui::BeginPopupModal("Theme Selector", &open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1,1,0,1), "Available Themes: %zu", availableThemes.size());
        ImGui::Separator();

        static int selectedTheme = -1;
        ImGui::BeginChild("ThemesList", ImVec2(0, 200), true);
        for (int i = 0; i < availableThemes.size(); i++) {
            if (ImGui::Selectable(availableThemes[i].name.c_str(), selectedTheme == i)) {
                selectedTheme = i;
                printf("Selected theme: %s\n", availableThemes[i].name.c_str());
            }
        }
        ImGui::EndChild();

        ImGui::Separator();
        
        ImGui::BeginDisabled(selectedTheme == -1);
        if (ImGui::Button("Load Theme", ImVec2(120, 0))) {
            if (selectedTheme >= 0 && selectedTheme < availableThemes.size()) {
                printf("Loading theme: %s\n", availableThemes[selectedTheme].path.c_str());
                if (LoadUIColors(availableThemes[selectedTheme].path.c_str())) {
                    appSettings.autoLoadColorFile = availableThemes[selectedTheme].path;
                    appSettings.autoLoadColors = true;
                    appSettings.saveToFile("settings.ini");
                    printf("Theme loaded and saved to settings\n");
                    ImGui::CloseCurrentPopup();
                    showThemeSelector = false;
                } else {
                    printf("Failed to load theme!\n");
                }
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            showThemeSelector = false;
            printf("Theme selection cancelled\n");
        }

        ImGui::EndPopup();
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
    const int numThreads = appSettings.threadCount;  // Use appSettings.threadCount instead of hardware_concurrency

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
    int maxSearchRadius = 256;  // Changed from 2000 to 256
    int minSearchRadius = 0;    // Add min search radius

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
        int minDistance;  // Restore minDistance
        int maxDistance;
        bool required;
        bool found;
        Pos foundPos;

        AttachedStructure() : 
            structureType(Village), minDistance(0), maxDistance(256), required(false), found(false), foundPos({0, 0}) {}
        
        AttachedStructure(int type, int minDist, int maxDist, bool req) : 
            structureType(type), minDistance(minDist), maxDistance(maxDist), required(req), found(false), foundPos({0, 0}) {}
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
            std::vector<std::vector<int64_t>> threadSeeds(appSettings.threadCount);
            std::mt19937_64 globalGen(rd());
            std::uniform_int_distribution<int64_t> dist;
            
            if (useBedrockRange) {
                dist = std::uniform_int_distribution<int64_t>(INT32_MIN, INT32_MAX);
            } else {
                dist = std::uniform_int_distribution<int64_t>(INT64_MIN, INT64_MAX);
            }

            // Pre-generate seeds for each thread
            for (int i = 0; i < appSettings.threadCount; i++) {
                threadSeeds[i].reserve(OPTIMAL_BATCH_SIZE);
                for (int j = 0; j < OPTIMAL_BATCH_SIZE; j++) {
                    threadSeeds[i].push_back(dist(globalGen));
                }
            }
            
            // Create threads with pre-generated seeds
            for (int i = 0; i < appSettings.threadCount; i++) {
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
                                    found = findStructure(seedToCheck, &pos, maxSearchRadius);
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
        ImGui::Text("Version: BetaV4");
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
                outFile << "Search Radius: " << maxSearchRadius << "\n";
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
                // Initialize with empty vector
                attachedStructures.clear();
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

            // Table for attached structures with background color matching the UI
            ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);
            ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);
            ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);

            if (ImGui::BeginTable("AttachedStructures", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Enable", ImGuiTableColumnFlags_WidthFixed, 50.0f);
                ImGui::TableSetupColumn("Structure Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Min Distance", ImGuiTableColumnFlags_WidthFixed, 160.0f);
                ImGui::TableSetupColumn("Max Distance", ImGuiTableColumnFlags_WidthFixed, 160.0f);
                ImGui::TableHeadersRow();

                for (size_t i = 0; i < attachedStructures.size(); i++) {
                    ImGui::TableNextRow();
                    
                    // Enable checkbox
                    ImGui::TableNextColumn();
                    ImGui::Checkbox(("##required" + std::to_string(i)).c_str(), &attachedStructures[i].required);

                    // Structure type combo
                    ImGui::TableNextColumn();
                    int structIndex = getIndexFromStructureType(attachedStructures[i].structureType);
                    if (ImGui::Combo(("##type" + std::to_string(i)).c_str(), &structIndex, structures, IM_ARRAYSIZE(structures))) {
                        attachedStructures[i].structureType = getStructureTypeFromIndex(structIndex);
                    }

                    // Min distance column
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(60);
                    ImGui::DragInt(("##mindist" + std::to_string(i)).c_str(), &attachedStructures[i].minDistance, 1.0f, 0, attachedStructures[i].maxDistance);
                    ImGui::SameLine();
                    if (ImGui::Button(("-##mind" + std::to_string(i)).c_str(), ImVec2(20, 0))) {
                        attachedStructures[i].minDistance = std::max(0, attachedStructures[i].minDistance - 16);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(("+##mind" + std::to_string(i)).c_str(), ImVec2(20, 0))) {
                        attachedStructures[i].minDistance = std::min(attachedStructures[i].maxDistance, attachedStructures[i].minDistance + 16);
                    }

                    // Max distance column
                    ImGui::TableNextColumn();
                    ImGui::SetNextItemWidth(60);
                    ImGui::DragInt(("##maxdist" + std::to_string(i)).c_str(), &attachedStructures[i].maxDistance, 1.0f, attachedStructures[i].minDistance, 10000);
                    ImGui::SameLine();
                    if (ImGui::Button(("-##maxd" + std::to_string(i)).c_str(), ImVec2(20, 0))) {
                        attachedStructures[i].maxDistance = std::max(attachedStructures[i].minDistance, attachedStructures[i].maxDistance - 16);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(("+##maxd" + std::to_string(i)).c_str(), ImVec2(20, 0))) {
                        attachedStructures[i].maxDistance = std::min(10000, attachedStructures[i].maxDistance + 16);
                    }
                }
                ImGui::EndTable();
            }
            
            ImGui::PopStyleColor(3);

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
        ImGui::Text("Search Radius:");
        ImGui::SameLine();
        ImGui::BeginGroup();
        
        // Min radius controls
        ImGui::PushItemWidth(100);
        ImGui::Text("Min:"); 
        ImGui::SameLine();
        ImGui::DragInt("##minradius", &minSearchRadius, 1.0f, 0, maxSearchRadius);
        ImGui::SameLine();
        if (ImGui::Button("-##minr", ImVec2(20, 0))) { 
            minSearchRadius = std::max(0, minSearchRadius - 16); 
        }
        ImGui::SameLine();
        if (ImGui::Button("+##minr", ImVec2(20, 0))) { 
            minSearchRadius = std::min(maxSearchRadius, minSearchRadius + 16); 
        }
        
        // Max radius controls
        ImGui::Text("Max:"); 
        ImGui::SameLine();
        ImGui::DragInt("##maxradius", &maxSearchRadius, 1.0f, minSearchRadius, 10000);
        ImGui::SameLine();
        if (ImGui::Button("-##maxr", ImVec2(20, 0))) { 
            maxSearchRadius = std::max(minSearchRadius, maxSearchRadius - 16); 
        }
        ImGui::SameLine();
        if (ImGui::Button("+##maxr", ImVec2(20, 0))) { 
            maxSearchRadius = std::min(10000, maxSearchRadius + 16); 
        }
        ImGui::PopItemWidth();
        
        ImGui::EndGroup();
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
            ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);
            ImGui::PushStyleColor(ImGuiCol_TableRowBg, ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);
            ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImGui::GetStyle().Colors[ImGuiCol_WindowBg]);

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
            
            ImGui::PopStyleColor(3);
        }
    }

    void renderSettingsTab() {
        if (ImGui::BeginTabItem("Settings")) {
            // Performance Settings Category
            if (ImGui::CollapsingHeader("Performance Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                // Thread Count
                int maxThreads = std::max(1, (int)std::thread::hardware_concurrency());
                if (ImGui::SliderInt("Thread Count", &appSettings.threadCount, 1, maxThreads)) {
                    appSettings.saveToFile("settings.ini");
                }
                
                // Batch Size
                if (ImGui::SliderInt("Batch Size", &appSettings.batchSize, 10000, 1000000, "%d", ImGuiSliderFlags_Logarithmic)) {
                    OPTIMAL_BATCH_SIZE = appSettings.batchSize;
                    appSettings.saveToFile("settings.ini");
                }
                
                // Generator Pool Size
                if (ImGui::SliderInt("Generator Pool Size", &appSettings.generatorPoolSize, 1, 128)) {
                    generatorPoolSize = appSettings.generatorPoolSize;
                    appSettings.saveToFile("settings.ini");
                }
            }

            // UI Settings Category
            if (ImGui::CollapsingHeader("UI Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                // GUI Scale
                if (ImGui::SliderFloat("GUI Scale", &appSettings.guiScale, 0.5f, 2.0f, "%.2f")) {
                    ImGui::GetIO().FontGlobalScale = appSettings.guiScale;
                    appSettings.saveToFile("settings.ini");
                }
                
                ImGui::Separator();
                
                // Auto-load UI Colors
                if (ImGui::Checkbox("Auto-load UI Colors", &appSettings.autoLoadColors)) {
                    appSettings.saveToFile("settings.ini");
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Warning: A 'themes' folder must be created in the same directory as the executable.");
                    ImGui::Text("Make sure to place your .txt theme files inside this 'themes' folder for proper functionality.");
                    ImGui::EndTooltip();
                }
                
                // Theme selector button
                if (ImGui::Button("Theme Selector")) {
                    availableThemes = GetThemeFiles();  // Refresh themes list
                    if (availableThemes.empty()) {
                        printf("No themes found! Please add .txt files to the themes folder.\n");
                    } else {
                        showThemeSelector = true;
                    }
                }
                
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Select a theme from the themes folder");
                }
                
                ShowThemeSelector();  // Show the theme selector window if enabled
            }

            // UI Colors Category
            if (ImGui::CollapsingHeader("UI Colors", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool colorChanged = false;
                
                ImGui::TextColored(uiColors.text, "Current Colors:");
                colorChanged |= ImGui::ColorEdit3("Main Text", (float*)&uiColors.targetText);
                colorChanged |= ImGui::ColorEdit3("Background", (float*)&uiColors.targetBackground);
                colorChanged |= ImGui::ColorEdit3("Frame Background", (float*)&uiColors.targetFrame);
                colorChanged |= ImGui::ColorEdit3("Interactive Elements", (float*)&uiColors.targetAccent);
                colorChanged |= ImGui::ColorEdit3("Buttons & Tabs", (float*)&uiColors.targetButton);
                colorChanged |= ImGui::ColorEdit3("Headers", (float*)&uiColors.targetHeader);
                colorChanged |= ImGui::ColorEdit4("Border", (float*)&uiColors.targetBorder);

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
        try {
            static thread_local Generator g;
            static thread_local bool initialized = false;
            
            if (!initialized) {
                setupGenerator(&g, MC_NEWEST, 0);
                initialized = true;
            }

            int32_t seed32 = (int32_t)(seed & 0xFFFFFFFF);
            int regionRadius = (radius / 512) + 1;
            std::vector<Pos> validPositions;

            // Initialize generator once
            g.seed = seed;
            g.dim = DIM_OVERWORLD;
            applySeed(&g, DIM_OVERWORLD, seed);

            for (int regionX = -regionRadius; regionX <= regionRadius; ++regionX) {
                for (int regionZ = -regionRadius; regionZ <= regionRadius; ++regionZ) {
                    if (shouldStop) return false;

                    Pos p;
                    if (!getBedrockStructurePos(selectedStructure, g.mc, seed32, regionX, regionZ, &p)) {
                        continue;
                    }

                    // Calculate distance from origin
                    int distance = (int)sqrt(p.x*p.x + p.z*p.z);
                    if (distance < minSearchRadius || distance > radius) {
                        continue;
                    }

                    // Basic validation
                    if (!isViableStructurePos(selectedStructure, &g, p.x, p.z, 0)) {
                        continue;
                    }

                    bool skipTerrainCheck = (selectedStructure == Ancient_City || 
                                          selectedStructure == Monument);
                    
                    if (!skipTerrainCheck && !isViableStructureTerrain(selectedStructure, &g, p.x, p.z)) {
                        continue;
                    }

                    int biomeId = getBiomeAt(&g, 4, p.x >> 2, 319>>2, p.z >> 2);
                    if(biomeId == none) continue;
                    
                    bool validBiome = true;
                    if (selectedStructure == Monument) {
                        if (!isDeepOcean(biomeId)) {
                            validBiome = false;
                        }
                    }
                    else if (selectedStructure == Mansion) {
                        if (biomeId != dark_forest) {
                            validBiome = false;
                        }
                    }
                    else if (selectedStructure == Shipwreck) {
                        if (!isShipwreckBiome(biomeId)) {
                            validBiome = false;
                        }
                    }
                    else if (selectedStructure == Village) {
                        if (!isVillageBiome(biomeId)) {
                            validBiome = false;
                        }
                    }

                    if (!validBiome) continue;

                    // Add valid position
                    validPositions.push_back(p);
                }
            }

            // Find closest valid position
            if (!validPositions.empty()) {
                Pos bestPos = validPositions[0];
                int bestDistance = INT_MAX;

                for (const Pos& p : validPositions) {
                    int distance = (int)sqrt(p.x*p.x + p.z*p.z);
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        bestPos = p;
                    }
                }

                *pos = bestPos;
                return true;
            }

            return false;
        } catch (const std::exception& e) {
            return false;
        }
    }

    bool findMultipleStructures(int64_t seed, Pos* basePos) {
        try {
            static thread_local Generator g;
            static thread_local bool initialized = false;
            
            if (!initialized) {
                setupGenerator(&g, MC_NEWEST, 0);
                initialized = true;
            }

            int32_t seed32 = (int32_t)(seed & 0xFFFFFFFF);
            std::vector<std::pair<int, Pos>> allFoundStructures;

            // First find the base structure
            selectedStructure = baseStructureType;
            if (!findStructure(seed, basePos, maxSearchRadius)) {
                return false;
            }
            allFoundStructures.push_back({baseStructureType, *basePos});

            // Count enabled structures
            int enabledCount = 0;
            for (auto& attached : attachedStructures) {
                if (attached.required) {
                    enabledCount++;
                    attached.found = false;
                }
            }

            if (enabledCount == 0) return true;

            // Initialize generator once
            g.seed = seed;
            g.dim = DIM_OVERWORLD;
            applySeed(&g, DIM_OVERWORLD, seed);

            // For each required structure
            for (auto& attached : attachedStructures) {
                if (!attached.required) continue;

                int regionRadius = (attached.maxDistance / 512) + 1;
                std::vector<Pos> validPositions;

                // Search all regions for valid positions
                for (int regionX = -regionRadius; regionX <= regionRadius; ++regionX) {
                    for (int regionZ = -regionRadius; regionZ <= regionRadius; ++regionZ) {
                        if (shouldStop) return false;

                        Pos p;
                        if (!getBedrockStructurePos(attached.structureType, g.mc, seed32, regionX, regionZ, &p)) {
                            continue;
                        }

                        // Check distance from base structure
                        int dx = p.x - basePos->x;
                        int dz = p.z - basePos->z;
                        int distance = (int)sqrt(dx*dx + dz*dz);
                        
                        if (distance < attached.minDistance || distance > attached.maxDistance) {
                            continue;
                        }

                        // Check if this exact position was already used
                        bool positionUsed = false;
                        for (const auto& found : allFoundStructures) {
                            if (p.x == found.second.x && p.z == found.second.z) {
                                positionUsed = true;
                                break;
                            }
                        }
                        if (positionUsed) continue;

                        // Basic validation
                        if (!isViableStructurePos(attached.structureType, &g, p.x, p.z, 0)) {
                            continue;
                        }

                        bool skipTerrainCheck = (attached.structureType == Ancient_City || 
                                               attached.structureType == Monument);
                        
                        if (!skipTerrainCheck && !isViableStructureTerrain(attached.structureType, &g, p.x, p.z)) {
                            continue;
                        }

                        int biomeId = getBiomeAt(&g, 4, p.x >> 2, 319>>2, p.z >> 2);
                        if (biomeId == none) continue;

                        bool validBiome = true;
                        if (attached.structureType == Monument && !isDeepOcean(biomeId)) {
                            validBiome = false;
                        }
                        else if (attached.structureType == Mansion && biomeId != dark_forest) {
                            validBiome = false;
                        }
                        else if (attached.structureType == Shipwreck && !isShipwreckBiome(biomeId)) {
                            validBiome = false;
                        }
                        else if (attached.structureType == Village && !isVillageBiome(biomeId)) {
                            validBiome = false;
                        }

                        if (!validBiome) continue;

                        // Add to valid positions if it passed all checks
                        validPositions.push_back(p);
                    }
                }

                // If no valid positions found, fail
                if (validPositions.empty()) {
                    return false;
                }

                // Sort valid positions by distance from base
                std::sort(validPositions.begin(), validPositions.end(), 
                    [basePos](const Pos& a, const Pos& b) {
                        int dxa = a.x - basePos->x;
                        int dza = a.z - basePos->z;
                        int dxb = b.x - basePos->x;
                        int dzb = b.z - basePos->z;
                        return (dxa*dxa + dza*dza) < (dxb*dxb + dzb*dzb);
                    });

                // Use the first valid position (closest to base)
                attached.foundPos = validPositions[0];
                attached.found = true;
                allFoundStructures.push_back({attached.structureType, validPositions[0]});
            }

            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }
};

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char** argv)
#endif
{
    // Initialize GLFW
    if (!glfwInit()) {
        return 1;
    }

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Chunk Biomes GUI", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize settings (will create default settings if file doesn't exist)
    appSettings.loadFromFile("settings.ini");

    // Apply settings after ImGui is initialized
    OPTIMAL_BATCH_SIZE = appSettings.batchSize;
    generatorPoolSize = appSettings.generatorPoolSize;
    io.FontGlobalScale = appSettings.guiScale;

    // Check for themes
    printf("Checking for themes on startup...\n");
    availableThemes = GetThemeFiles();
    
    // If we have a saved theme and auto-load is enabled, load it
    if (appSettings.autoLoadColors && !appSettings.autoLoadColorFile.empty()) {
        printf("Auto-loading theme: %s\n", appSettings.autoLoadColorFile.c_str());
        if (LoadUIColors(appSettings.autoLoadColorFile.c_str())) {
            printf("Theme loaded successfully!\n");
        } else {
            printf("Failed to load theme!\n");
        }
    } else if (!availableThemes.empty()) {
        // If we have themes but none is selected, show the selector
        printf("Found %zu themes, showing selector...\n", availableThemes.size());
        showThemeSelector = true;
    }

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

        // Open theme selector if needed
        if (showThemeSelector) {
            ImGui::OpenPopup("Theme Selector");
        }
        ShowThemeSelector();

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

    // Save settings before exit
    appSettings.batchSize = OPTIMAL_BATCH_SIZE;
    appSettings.generatorPoolSize = generatorPoolSize;
    appSettings.saveToFile("settings.ini");

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
