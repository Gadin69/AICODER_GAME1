#include "GameSessionManager.h"
#include <iostream>
#include <sstream>

GameSessionManager::GameSessionManager() {
}

GameSessionManager::~GameSessionManager() {
}

bool GameSessionManager::loadGame(const std::string& filePath, 
                                   Grid& grid, 
                                   SimulationManager& simManager,
                                   TileMap& tileMap,
                                   Camera& camera,
                                   GameGUI* gameGUI) {
    if (filePath.empty()) {
        std::cerr << "[GameSessionManager] ERROR: Empty file path" << std::endl;
        return false;
    }
    
    std::cout << "[GameSessionManager] Loading game from: " << filePath << std::endl;
    
    // Step 1: Load grid from file
    // This calls grid.initialize() internally if size changes, which clears all cells
    if (!grid.loadFromFile(filePath)) {
        std::cerr << "[GameSessionManager] ERROR: Failed to load grid from file" << std::endl;
        return false;
    }
    
    std::cout << "[GameSessionManager] Grid loaded, size: " << grid.getWidth() 
              << "x" << grid.getHeight() << std::endl;
    
    // Step 2: Reinitialize simulation systems with new grid
    // Systems need to know about the new grid dimensions
    simManager.initialize(grid);
    std::cout << "[GameSessionManager] Simulation systems reinitialized" << std::endl;
    
    // Step 3: Update tile map to match loaded grid size
    tileMap.initialize(grid.getWidth(), grid.getHeight(), 32.0f);
    std::cout << "[GameSessionManager] Tile map updated to " << grid.getWidth() 
              << "x" << grid.getHeight() << std::endl;
    
    // Step 4: Update camera bounds for new grid size
    camera.setGridBounds(0, 0, 
                         grid.getWidth() * 32.0f, 
                         grid.getHeight() * 32.0f);
    std::cout << "[GameSessionManager] Camera bounds updated" << std::endl;
    
    // Step 4.5: Center camera on the grid
    float gridCenterX = (grid.getWidth() * 32.0f) / 2.0f;
    float gridCenterY = (grid.getHeight() * 32.0f) / 2.0f;
    camera.setPosition(gridCenterX, gridCenterY);
    std::cout << "[GameSessionManager] Camera centered at (" << gridCenterX << ", " << gridCenterY << ")" << std::endl;
    
    // Step 4.6: Pause simulation by setting speed to 0
    // This lets player orient themselves before starting
    if (gameGUI && gameGUI->getSimSpeedSlider()) {
        UISlider* slider = gameGUI->getSimSpeedSlider();
        slider->currentValue = 0.0f;
        slider->updateThumbPosition();
        slider->updateValueText();
        std::cout << "[GameSessionManager] Simulation speed set to 0 (paused)" << std::endl;
    }
    
    // Step 5: Update all cell colors after loading
    // Cells were loaded with raw data, need to compute display colors
    for (int y = 0; y < grid.getHeight(); ++y) {
        for (int x = 0; x < grid.getWidth(); ++x) {
            Cell& cell = grid.getCell(x, y);
            cell.updateColor();
        }
    }
    std::cout << "[GameSessionManager] Cell colors updated" << std::endl;
    
    // Track current session
    currentSavePath = filePath;
    
    std::cout << "[GameSessionManager] Game loaded successfully" << std::endl;
    return true;
}

std::string GameSessionManager::saveGame(const std::string& saveName, 
                                          const std::string& notes,
                                          Grid& grid) {
    if (saveName.empty()) {
        std::cerr << "[GameSessionManager] ERROR: Empty save name" << std::endl;
        return "";
    }
    
    std::cout << "[GameSessionManager] Saving game: " << saveName << std::endl;
    
    // Use SaveManager for the actual save operation
    auto& saveManager = SaveManager::getInstance();
    
    // Generate full save path
    std::string savePath = saveManager.getSaveDirectory() + "/" + saveName + ".onisave";
    
    // Save the grid
    if (grid.saveToFile(savePath)) {
        std::cout << "[GameSessionManager] Game saved to: " << savePath << std::endl;
        currentSavePath = savePath;
        return savePath;
    } else {
        std::cerr << "[GameSessionManager] ERROR: Failed to save game" << std::endl;
        return "";
    }
}
