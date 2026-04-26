#include "SaveManager.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <ctime>

namespace fs = std::filesystem;

// Save file header structure
struct SaveFileHeader {
    char magic[8];           // "ONISAVE\0"
    uint32_t version;         // 1
    uint32_t gridWidth;
    uint32_t gridHeight;
    uint64_t timestamp;       // Unix timestamp
    uint32_t playTimeSeconds;
    uint32_t cellCount;       // Non-vacuum cells
    char saveName[64];        // User-provided or auto-generated
    char notes[256];          // Optional notes
};

// Singleton instance
SaveManager& SaveManager::getInstance() {
    static SaveManager instance;
    return instance;
}

void SaveManager::initialize() {
    // Get AppData/Roaming path
    const char* appData = getenv("APPDATA");
    if (!appData) {
        std::cerr << "[SaveManager] WARNING: APPDATA environment variable not found, using local saves directory" << std::endl;
        savesDir = "saves";
    } else {
        savesDir = std::string(appData) + "/ONIEngine/saves";
    }
    
    // Create directory if it doesn't exist
    try {
        fs::create_directories(savesDir);
        std::cout << "[SaveManager] Saves directory: " << savesDir << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] ERROR creating saves directory: " << e.what() << std::endl;
    }
}

std::string SaveManager::getSaveDirectory() const {
    return savesDir;
}

std::string SaveManager::generateSaveName() const {
    // Find the highest existing save number
    int maxNum = 0;
    try {
        if (fs::exists(savesDir)) {
            for (const auto& entry : fs::directory_iterator(savesDir)) {
                if (entry.path().extension() == ".dat") {
                    std::string filename = entry.path().stem().string();
                    if (filename.substr(0, 5) == "Save_") {
                        try {
                            int num = std::stoi(filename.substr(5));
                            if (num > maxNum) {
                                maxNum = num;
                            }
                        } catch (...) {
                            // Skip invalid filenames
                        }
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] ERROR scanning saves: " << e.what() << std::endl;
    }
    
    // Generate next save name
    char name[32];
    snprintf(name, sizeof(name), "Save_%03d", maxNum + 1);
    return std::string(name);
}

bool SaveManager::saveGame(const std::string& saveName, const std::string& notes) {
    if (savesDir.empty()) {
        std::cerr << "[SaveManager] ERROR: Save directory not initialized" << std::endl;
        return false;
    }
    
    // Generate filename
    std::string finalName = saveName.empty() ? generateSaveName() : saveName;
    std::string datPath = savesDir + "/" + finalName + ".dat";
    
    // SaveManager only creates the metadata file, Grid::saveToFile writes the actual data
    // This function is a placeholder for future thumbnail capture
    captureThumbnail(savesDir + "/" + finalName + "_thumb.png", 160, 90);
    
    std::cout << "[SaveManager] Preparing save: " << finalName << std::endl;
    return true;
}

bool SaveManager::loadGame(const std::string& filePath) {
    if (filePath.empty()) {
        std::cerr << "[SaveManager] ERROR: Empty file path" << std::endl;
        return false;
    }
    
    if (!fs::exists(filePath)) {
        std::cerr << "[SaveManager] ERROR: Save file not found: " << filePath << std::endl;
        return false;
    }
    
    // Open file for reading
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[SaveManager] ERROR: Cannot open file for reading: " << filePath << std::endl;
        return false;
    }
    
    // Read and validate header
    SaveFileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    if (std::string(header.magic) != "ONISAVE") {
        std::cerr << "[SaveManager] ERROR: Invalid save file magic number" << std::endl;
        return false;
    }
    
    if (header.version != 1) {
        std::cerr << "[SaveManager] ERROR: Unsupported save file version: " << header.version << std::endl;
        return false;
    }
    
    file.close();
    
    std::cout << "[SaveManager] Validated save file: " << header.saveName << std::endl;
    return true;
}

std::vector<SaveMetadata> SaveManager::getSaveList() const {
    std::vector<SaveMetadata> saveList;
    
    if (savesDir.empty() || !fs::exists(savesDir)) {
        return saveList;
    }
    
    try {
        for (const auto& entry : fs::directory_iterator(savesDir)) {
            if (entry.path().extension() == ".dat") {
                SaveMetadata meta;
                meta.filePath = entry.path().string();
                
                // Extract save name from filename
                std::string stem = entry.path().stem().string();
                meta.saveName = stem;
                
                // Check for thumbnail
                std::string thumbPath = entry.path().string();
                thumbPath.replace(thumbPath.find(".dat"), 4, "_thumb.png");
                if (fs::exists(thumbPath)) {
                    meta.thumbnailPath = thumbPath;
                }
                
                // Read header for metadata
                std::ifstream file(meta.filePath, std::ios::binary);
                if (file.is_open()) {
                    SaveFileHeader header;
                    file.read(reinterpret_cast<char*>(&header), sizeof(header));
                    
                    if (std::string(header.magic) == "ONISAVE") {
                        meta.notes = std::string(header.notes);
                        meta.timestamp = static_cast<time_t>(header.timestamp);
                        meta.playTimeSeconds = header.playTimeSeconds;
                        meta.gridWidth = header.gridWidth;
                        meta.gridHeight = header.gridHeight;
                        meta.cellCount = header.cellCount;
                        
                        // Use header save name if available
                        std::string headerName(header.saveName);
                        if (!headerName.empty() && headerName != stem) {
                            meta.saveName = headerName;
                        }
                    }
                    
                    file.close();
                }
                
                saveList.push_back(meta);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] ERROR reading save list: " << e.what() << std::endl;
    }
    
    // Sort by timestamp (newest first)
    std::sort(saveList.begin(), saveList.end(), [](const SaveMetadata& a, const SaveMetadata& b) {
        return a.timestamp > b.timestamp;
    });
    
    return saveList;
}

bool SaveManager::deleteSave(const std::string& filePath) {
    if (filePath.empty()) {
        return false;
    }
    
    try {
        // Delete .dat file
        if (fs::exists(filePath)) {
            fs::remove(filePath);
        }
        
        // Delete thumbnail if exists
        std::string thumbPath = filePath;
        if (thumbPath.size() > 4 && thumbPath.substr(thumbPath.size() - 4) == ".dat") {
            thumbPath.replace(thumbPath.size() - 4, 4, "_thumb.png");
            if (fs::exists(thumbPath)) {
                fs::remove(thumbPath);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] ERROR deleting save: " << e.what() << std::endl;
        return false;
    }
}

bool SaveManager::hasRecentSave() const {
    return !getSaveList().empty();
}

std::string SaveManager::getRecentSavePath() const {
    auto saves = getSaveList();
    if (saves.empty()) {
        return "";
    }
    return saves[0].filePath;  // First save is most recent (sorted by timestamp)
}

bool SaveManager::captureThumbnail(const std::string& outputPath, int width, int height) {
    // Placeholder: Thumbnail capture requires access to render window
    // This would be implemented with SFML's texture capture in a real implementation
    // For now, we skip thumbnails to avoid crashes
    return false;
}
