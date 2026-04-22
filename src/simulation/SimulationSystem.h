#pragma once

#include "Grid.h"
#include <memory>
#include <mutex>
#include <string>

// Abstract base class for all simulation systems
// Provides OOP interface for parallel simulation systems
class SimulationSystem {
public:
    SimulationSystem(const std::string& name, float updateInterval)
        : systemName(name), updateInterval(updateInterval), updateTimer(0.0f), enabled(true) {}
    
    virtual ~SimulationSystem() = default;
    
    // Initialize system with grid reference
    virtual void initialize(Grid& grid) {
        this->grid = &grid;
    }
    
    // Update simulation by deltaTime
    // Returns true if system performed updates
    virtual bool update(float deltaTime) = 0;
    
    // Reset system to initial state
    virtual void reset() = 0;
    
    // System controls
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    bool isEnabled() const { return enabled; }
    
    // Getters
    const std::string& getName() const { return systemName; }
    float getUpdateInterval() const { return updateInterval; }
    float getUpdateTimer() const { return updateTimer; }
    
    // Thread safety
    std::mutex& getMutex() { return systemMutex; }
    
protected:
    Grid* grid = nullptr;
    std::string systemName;
    float updateInterval;  // How often to update (seconds)
    float updateTimer;     // Time since last update
    bool enabled;
    std::mutex systemMutex;  // For thread-safe grid access
    
    // Helper: check if system should update this frame
    bool shouldUpdate(float deltaTime) {
        updateTimer += deltaTime;
        if (updateTimer >= updateInterval) {
            updateTimer = 0.0f;
            return true;
        }
        return false;
    }
};
