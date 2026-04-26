#pragma once

#include "../simulation/Grid.h"
#include "../simulation/SimulationManager.h"
#include "../rendering/TileMap.h"
#include "../rendering/Camera.h"
#include "../save/SaveManager.h"
#include "../ui/GameGUI.h"
#include <string>

/**
 * GameSessionManager handles complete save/load workflows.
 * 
 * Responsibilities:
 * - Loading: Clear grid → Resize → Load data → Reinit systems → Update visuals
 * - Saving: Capture thumbnail → Write metadata → Save cell data
 * - Session state tracking (current save file, modified flag, etc.)
 * 
 * This separates save/load concerns from main.cpp for better maintainability.
 */
class GameSessionManager {
public:
    GameSessionManager();
    ~GameSessionManager();
    
    /**
     * Load a game save file and properly initialize all systems.
     * 
     * Workflow:
     * 1. Load grid from file (clears and resizes if needed)
     * 2. Reinitialize simulation systems with new grid
     * 3. Update tile map to match grid size
     * 4. Update camera bounds
     * 5. Update all cell colors
     * 
     * @param filePath Path to the save file
     * @param grid Reference to the simulation grid
     * @param simManager Reference to the simulation manager
     * @param tileMap Reference to the tile map renderer
     * @param camera Reference to the camera
     * @return true if load was successful
     */
    bool loadGame(const std::string& filePath, 
                  Grid& grid, 
                  SimulationManager& simManager,
                  TileMap& tileMap,
                  Camera& camera,
                  GameGUI* gameGUI);
    
    /**
     * Save the current game state.
     * 
     * Workflow:
     * 1. Capture thumbnail screenshot
     * 2. Generate save metadata
     * 3. Write cell data to file
     * 
     * @param saveName Name for the save (used in filename)
     * @param notes Optional notes/description
     * @param grid Reference to the simulation grid
     * @return Path to saved file, or empty string on failure
     */
    std::string saveGame(const std::string& saveName, 
                         const std::string& notes,
                         Grid& grid);
    
    /**
     * Check if a game session is currently loaded.
     */
    bool hasActiveSession() const { return !currentSavePath.empty(); }
    
    /**
     * Get the path of the currently loaded save file.
     */
    std::string getCurrentSavePath() const { return currentSavePath; }
    
    /**
     * Clear the current session (no save loaded).
     */
    void clearSession() { currentSavePath = ""; }

private:
    std::string currentSavePath;  // Path to currently loaded save
};
