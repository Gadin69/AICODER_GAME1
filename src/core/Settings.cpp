#include "Settings.h"
#include "Logger.h"
#include <fstream>
#include <sstream>

SettingsManager::SettingsManager()
    : settingsFilePath("settings.ini") {
}

SettingsManager& SettingsManager::getInstance() {
    static SettingsManager instance;
    return instance;
}

GameSettings& SettingsManager::getSettings() {
    return settings;
}

void SettingsManager::saveSettings() {
    std::ofstream file(settingsFilePath);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open settings file for writing: " + settingsFilePath);
        return;
    }
    
    // Write settings as key=value pairs
    file << "# Game Settings\n";
    file << "displayMode=" << static_cast<int>(settings.displayMode) << "\n";
    file << "screenWidth=" << settings.screenWidth << "\n";
    file << "screenHeight=" << settings.screenHeight << "\n";
    file << "vsync=" << (settings.vsync ? "1" : "0") << "\n";
    file << "gridWidth=" << settings.gridWidth << "\n";
    file << "gridHeight=" << settings.gridHeight << "\n";
    file << "cameraScrollSpeed=" << settings.cameraScrollSpeed << "\n";
    file << "cameraEdgeScrollMargin=" << settings.cameraEdgeScrollMargin << "\n";
    
    file.close();
    LOG_INFO("Settings saved to " + settingsFilePath);
}

void SettingsManager::loadSettings() {
    std::ifstream file(settingsFilePath);
    if (!file.is_open()) {
        LOG_INFO("No settings file found, using defaults");
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        // Parse key=value
        size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) continue;
        
        std::string key = line.substr(0, equalsPos);
        std::string value = line.substr(equalsPos + 1);
        
        if (key == "displayMode") {
            settings.displayMode = static_cast<DisplayMode>(std::stoi(value));
        } else if (key == "screenWidth") {
            settings.screenWidth = std::stoul(value);
        } else if (key == "screenHeight") {
            settings.screenHeight = std::stoul(value);
        } else if (key == "vsync") {
            settings.vsync = (value == "1");
        } else if (key == "gridWidth") {
            settings.gridWidth = std::stoul(value);
        } else if (key == "gridHeight") {
            settings.gridHeight = std::stoul(value);
        } else if (key == "cameraScrollSpeed") {
            settings.cameraScrollSpeed = std::stof(value);
        } else if (key == "cameraEdgeScrollMargin") {
            settings.cameraEdgeScrollMargin = std::stof(value);
        }
    }
    
    file.close();
    LOG_INFO("Settings loaded from " + settingsFilePath);
}
