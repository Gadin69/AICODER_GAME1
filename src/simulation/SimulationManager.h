#pragma once

#include "SimulationSystem.h"
#include "HeatSim.h"
#include "FluidSim.h"
#include "GasSim.h"
#include <vector>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

// Simulation Manager - orchestrates all simulation systems
// Manages threading, synchronization, and system lifecycle
class SimulationManager {
public:
    SimulationManager();
    ~SimulationManager();
    
    // Initialize all systems with grid
    void initialize(Grid& grid);
    
    // Update all systems (called from main loop)
    void update(float deltaTime);
    
    // Reset all systems
    void reset();
    
    // System access
    HeatSim& getHeatSim() { return *heatSim; }
    FluidSim& getFluidSim() { return *fluidSim; }
    GasSim& getGasSim() { return *gasSim; }
    
    // Threading controls
    void enableThreading(bool enable);
    bool isThreadingEnabled() const { return useThreading; }
    
    // System controls
    void enableSystem(const std::string& name);
    void disableSystem(const std::string& name);
    
    // Performance stats
    int getActiveSystemCount() const;
    std::string getSystemStatus() const;
    
private:
    Grid* grid = nullptr;
    bool useThreading = false;  // DISABLED: True parallel sim requires full double-buffering isolation
    std::atomic<bool> running{false};
    
    // Simulation systems
    std::unique_ptr<HeatSim> heatSim;
    std::unique_ptr<FluidSim> fluidSim;
    std::unique_ptr<GasSim> gasSim;
    
    // Persistent worker threads
    std::vector<std::thread> workerThreads;
    
    // Synchronization primitives
    std::mutex queueMutex;         // Protects frame queue
    std::mutex completeMutex;      // Protects completion counter
    std::condition_variable cv;    // Signal workers to start
    std::condition_variable cvComplete;  // Signal main thread when done
    
    float currentDeltaTime = 0.0f;
    bool frameReady = false;
    int completedCount = 0;
    
    // Register a system
    void registerSystem(std::unique_ptr<SimulationSystem> system);
    
    // Start persistent worker threads
    void startThreads();
    
    // Shutdown threads cleanly
    void shutdown();
    
    // Thread worker function (persistent)
    void systemWorker(SimulationSystem* system);
    
    // Update systems in single-threaded mode
    void updateSingleThreaded(float deltaTime);
    
    // Update systems in multi-threaded mode (signals persistent threads)
    void updateMultiThreaded(float deltaTime);
    
    // All registered systems
    std::vector<std::unique_ptr<SimulationSystem>> systems;
};
