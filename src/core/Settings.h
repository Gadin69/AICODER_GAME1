#pragma once

#include "Window.h"
#include <string>

struct GameSettings {
    DisplayMode displayMode = DisplayMode::Windowed;
    unsigned int screenWidth = 1280;
    unsigned int screenHeight = 720;
    bool vsync = true;
    unsigned int gridWidth = 80;   // Default larger grid
    unsigned int gridHeight = 60;
    float cameraScrollSpeed = 500.0f;  // pixels per second
    float cameraEdgeScrollMargin = 50.0f;  // pixels from edge
    float cameraAcceleration = 400.0f;  // pixels/s²
    float cameraMaxSpeed = 600.0f;  // pixels/s
};

class SettingsManager {
public:
    static SettingsManager& getInstance();
    
    GameSettings& getSettings();
    void saveSettings();  // Save to file
    void loadSettings();  // Load from file
    
private:
    SettingsManager();
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    
    GameSettings settings;
    std::string settingsFilePath;
};
