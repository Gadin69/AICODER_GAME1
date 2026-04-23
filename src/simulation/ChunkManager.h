#pragma once

#include <vector>
#include <algorithm>

// Simulation chunk for distance-based LOD updates
struct SimulationChunk {
    int chunkX, chunkY;  // Chunk coordinates
    int startX, startY;  // Grid cell range (inclusive)
    int endX, endY;      // Grid cell range (exclusive)
    int width, height;   // Chunk dimensions in cells
    
    float lastUpdateTime;   // Time of last full update
    float accumulatedTime;  // Time passed while out of view (for catch-up)
    
    // LOD level determines update frequency
    enum class LODLevel {
        High,    // Visible: update every tick
        Medium,  // Nearby: update every 2nd tick
        Low,     // Far: update every 4th tick + accumulate time
        None     // Too far: skip updates, accumulate time
    };
    
    LODLevel currentLOD = LODLevel::None;
    
    SimulationChunk() 
        : chunkX(0), chunkY(0), startX(0), startY(0), 
          endX(0), endY(0), width(0), height(0),
          lastUpdateTime(0.0f), accumulatedTime(0.0f) {}
};

// ChunkManager - manages distance-based LOD for simulation updates
class ChunkManager {
public:
    ChunkManager();
    
    // Initialize chunk grid
    void initialize(int gridWidth, int gridHeight, int chunkSize = 16);
    
    // Update camera position and view dimensions
    void updateCameraPosition(float cameraX, float cameraY, float viewWidth, float viewHeight);
    
    // Calculate LOD for each chunk based on camera distance
    void updateLODLevels();
    
    // Check if a cell should be updated based on its chunk's LOD
    bool shouldUpdateCell(int cellX, int cellY, float deltaTime);
    
    // Get chunk containing a cell
    const SimulationChunk* getChunkForCell(int cellX, int cellY) const;
    
    // Get all chunks (for debugging)
    const std::vector<SimulationChunk>& getChunks() const { return chunks; }
    
    // Getters
    int getChunkSize() const { return chunkSize; }
    int getGridWidth() const { return gridWidth; }
    int getGridHeight() const { return gridHeight; }
    
private:
    std::vector<SimulationChunk> chunks;
    float cameraX = 0, cameraY = 0;
    float viewWidth = 0, viewHeight = 0;
    int chunkSize = 16;  // cells per chunk
    int gridWidth = 0, gridHeight = 0;
    
    // LOD distance thresholds (in pixels, assuming 32px per cell)
    static constexpr float CELL_SIZE = 32.0f;
    static constexpr float MEDIUM_DISTANCE = 200.0f;  // pixels beyond view
    static constexpr float LOW_DISTANCE = 500.0f;     // pixels beyond view
    
    // Helper: calculate distance from chunk center to view bounds
    float getChunkDistanceToView(const SimulationChunk& chunk) const;
    
    // Helper: update time accumulation for a chunk
    void updateTimeAccumulation(SimulationChunk& chunk, float deltaTime);
};
