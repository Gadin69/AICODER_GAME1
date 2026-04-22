#pragma once

#include "Grid.h"
#include "HeatMap.h"
#include <vector>

class FluidSim {
public:
    FluidSim();

    void initialize(Grid& grid);
    void update(Grid& grid, float deltaTime);
    
    // Set camera position for LOD simulation
    void setCameraPosition(int camX, int camY);
    
    // Configuration setters for tuning
    void setLODConfig(float nearRange, float midRange, float farRange, float veryFarRange);
    void setCatchUpConfig(float catchUpThreshold, int maxCatchUpSteps);

    // Simulation rules
    void updateGas(Grid& grid, int x, int y);
    void updateLiquid(Grid& grid, int x, int y);
    void updateSolid(Grid& grid, int x, int y);  // Solids can melt/transform!
    void updateTemperature(Grid& grid, int x, int y);
    void checkElementInteractions(Grid& grid, int x, int y);
    
    // Phase transition system
    void startPhaseTransition(Cell& cell, ElementType targetType, float transitionTime);
    void updatePhaseTransitions(Grid& grid, int x, int y);
    bool isTransitioning(const Cell& cell) const;

    // Helper functions
    bool isEmpty(const Grid& grid, int x, int y);
    bool canMoveTo(const Grid& grid, int x, int y, ElementType element);
    void swapCells(Grid& grid, int x1, int y1, int x2, int y2);

private:
    float updateTimer;
    float updateInterval;
    
    // Heat map system for grid-wide temperature simulation
    HeatMap heatMap;
    
    // LOD simulation system
    int cameraTileX = 0;
    int cameraTileY = 0;
    int globalTick = 0;
    
    // LOD configuration (configurable distances in tiles)
    struct LODConfig {
        float nearRange = 10.0f;      // 0-10: Update every frame
        float midRange = 20.0f;       // 11-20: Every 2 frames
        float farRange = 35.0f;       // 21-35: Every 4 frames
        float veryFarRange = 50.0f;   // 36-50: Every 8 frames
        // 51+: Every 16 frames
    } lodConfig;
    
    // Time-lapse catch-up system
    float lastUpdateTime = 0.0f;
    std::vector<std::vector<float>> lastSeenTime;
    
    // Catch-up configuration
    struct CatchUpConfig {
        float timeThreshold = 1.0f;   // Seconds before catch-up triggers
        int maxSteps = 100;           // Maximum catch-up steps to prevent lag
    } catchUpConfig;
    
    // Get simulation priority based on distance from camera
    int getUpdatePriority(int x, int y) const;
    
    // Check if cell needs time-lapse catch-up
    bool needsTimeCatchUp(int x, int y, float currentTime) const;
    
    // Perform catch-up simulation for a cell
    void catchUpCell(Grid& grid, int x, int y, float missedTime);
    
    // Run a single simplified simulation step for catch-up
    void simulateStep(Grid& grid, int x, int y);
};
