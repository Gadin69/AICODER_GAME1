#include "SaveManager.h"
#include "../simulation/Grid.h"  // For SaveFileHeader
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <ctime>

namespace fs = std::filesystem;

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
                        
                        // Use header thumbnail path if available
                        std::string headerThumbPath(header.thumbnailPath);
                        if (!headerThumbPath.empty() && fs::exists(headerThumbPath)) {
                            meta.thumbnailPath = headerThumbPath;
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
    std::cout << "[SaveManager] WARNING: Thumbnail capture not implemented yet" << std::endl;
    return false;
}

bool SaveManager::captureThumbnailFromWindow(const std::string& outputPath, 
                                              sf::RenderWindow& window,
                                              int width, int height) {
    try {
        std::cout << "[SaveManager] Capturing thumbnail..." << std::endl;
        std::cout << "[SaveManager] Window size: " << window.getSize().x << "x" << window.getSize().y << std::endl;
        
        // SFML 3: Texture must be created with size in constructor
        sf::Texture windowTexture(window.getSize());
        
        // Copy the current frame from the window
        // NOTE: This captures the back buffer, which may not have the latest frame yet
        windowTexture.update(window);
        
        std::cout << "[SaveManager] Window texture created: " 
                  << windowTexture.getSize().x << "x" << windowTexture.getSize().y << std::endl;
        
        // Create render texture for thumbnail
        sf::RenderTexture renderTexture(sf::Vector2u(width, height));
        renderTexture.clear(sf::Color::Black);
        
        // Create sprite with window texture
        sf::Sprite windowSprite(windowTexture);
        
        // Scale to fit thumbnail size
        float scaleX = static_cast<float>(width) / window.getSize().x;
        float scaleY = static_cast<float>(height) / window.getSize().y;
        float scale = std::min(scaleX, scaleY);
        
        windowSprite.setScale(sf::Vector2f(scale, scale));
        
        // Center in thumbnail
        sf::FloatRect bounds = windowSprite.getLocalBounds();
        float centerX = (width - bounds.size.x * scale) / 2.0f;
        float centerY = (height - bounds.size.y * scale) / 2.0f;
        windowSprite.setPosition(sf::Vector2f(centerX, centerY));
        
        renderTexture.draw(windowSprite);
        renderTexture.display();
        
        // Save to file
        sf::Image image = renderTexture.getTexture().copyToImage();
        bool success = image.saveToFile(outputPath);
        
        std::cout << "[SaveManager] Thumbnail capture " << (success ? "SUCCESS" : "FAILED") 
                  << " - " << outputPath << std::endl;
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "[SaveManager] ERROR capturing thumbnail: " << e.what() << std::endl;
        return false;
    }
}
