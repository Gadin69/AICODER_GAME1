#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <ctime>

struct SaveMetadata {
    std::string filePath;
    std::string thumbnailPath;
    std::string saveName;
    std::string notes;
    time_t timestamp;
    uint32_t playTimeSeconds;
    uint32_t gridWidth;
    uint32_t gridHeight;
    uint32_t cellCount;
};

class SaveManager {
public:
    static SaveManager& getInstance();
    
    void initialize();  // Create saves directory if needed
    std::string getSaveDirectory() const;
    
    // Save/Load operations
    bool saveGame(const std::string& saveName, const std::string& notes = "");
    bool loadGame(const std::string& filePath);
    
    // Save file management
    std::vector<SaveMetadata> getSaveList() const;
    bool deleteSave(const std::string& filePath);
    bool hasRecentSave() const;
    std::string getRecentSavePath() const;
    
    // Screenshot capture
    bool captureThumbnail(const std::string& outputPath, int width = 160, int height = 90);
    std::string generateSaveName() const;
    
private:
    SaveManager() = default;
    std::string savesDir;
};
