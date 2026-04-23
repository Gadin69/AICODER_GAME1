#include "SimulationManager.h"
#include <iostream>
#include <algorithm>
#include <chrono>

SimulationManager::SimulationManager() {
    // Create simulation systems
    heatSim = std::make_unique<HeatSim>();
    fluidSim = std::make_unique<FluidSim>();
    gasSim = std::make_unique<GasSim>();
    
    // Register systems
    registerSystem(std::move(heatSim));
    registerSystem(std::move(fluidSim));
    registerSystem(std::move(gasSim));
    
    // Multi-threading DISABLED by default - simulation systems share grid state
    // True parallel execution requires complete double-buffering isolation
    useThreading = false;
}

SimulationManager::~SimulationManager() {
    shutdown();
}

void SimulationManager::shutdown() {
    running = false;
    
    // Signal threads to stop
    cv.notify_all();
    
    // Wait for all threads to finish
    for (auto& thread : workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    workerThreads.clear();
}

void SimulationManager::initialize(Grid& grid) {
    this->grid = &grid;
    
    // Initialize all systems
    for (auto& system : systems) {
        system->initialize(grid);
        // Set chunk manager reference for LOD support
        system->setChunkManager(&chunkManager);
    }
    
    // Start persistent worker threads if threading is enabled
    if (useThreading) {
        startThreads();
    }
}

void SimulationManager::startThreads() {
    running = true;
    
    // Create one persistent thread per system
    for (auto& system : systems) {
        if (system->isEnabled()) {
            workerThreads.emplace_back(&SimulationManager::systemWorker, this, system.get());
        }
    }
    
    std::cout << "[SimulationManager] Started " << workerThreads.size() 
              << " persistent worker threads\n";
}

void SimulationManager::update(float deltaTime) {
    if (!grid) return;
    
    if (useThreading) {
        updateMultiThreaded(deltaTime);
    } else {
        updateSingleThreaded(deltaTime);
    }
}

void SimulationManager::reset() {
    for (auto& system : systems) {
        system->reset();
    }
}

void SimulationManager::enableThreading(bool enable) {
    if (enable == useThreading) return;  // No change
    
    useThreading = enable;
    
    if (!enable) {
        // Shutdown threads
        shutdown();
    } else if (grid) {
        // Start threads if grid is initialized
        startThreads();
    }
}

void SimulationManager::enableSystem(const std::string& name) {
    for (auto& system : systems) {
        if (system->getName() == name) {
            system->enable();
            break;
        }
    }
}

void SimulationManager::disableSystem(const std::string& name) {
    for (auto& system : systems) {
        if (system->getName() == name) {
            system->disable();
            break;
        }
    }
}

int SimulationManager::getActiveSystemCount() const {
    int count = 0;
    for (const auto& system : systems) {
        if (system->isEnabled()) {
            ++count;
        }
    }
    return count;
}

std::string SimulationManager::getSystemStatus() const {
    std::string status = "Simulation Systems:\n";
    for (const auto& system : systems) {
        status += "  " + system->getName() + ": " + 
                  (system->isEnabled() ? "ENABLED" : "DISABLED") + 
                  " (update: " + std::to_string(system->getUpdateInterval()) + "s)\n";
    }
    return status;
}

void SimulationManager::setCameraPosition(float x, float y, float viewWidth, float viewHeight) {
    chunkManager.updateCameraPosition(x, y, viewWidth, viewHeight);
    chunkManager.updateLODLevels();
}

bool SimulationManager::shouldUpdateCell(int x, int y, float deltaTime) {
    return chunkManager.shouldUpdateCell(x, y, deltaTime);
}

void SimulationManager::registerSystem(std::unique_ptr<SimulationSystem> system) {
    systems.push_back(std::move(system));
}

void SimulationManager::systemWorker(SimulationSystem* system) {
    while (running) {
        // Wait for frame signal
        std::unique_lock<std::mutex> lock(queueMutex);
        cv.wait(lock, [this] { return frameReady || !running; });
        
        if (!running) break;
        
        float dt = currentDeltaTime;
        lock.unlock();
        
        // Process this system - lock GRID mutex (not system mutex)
        {
            std::lock_guard<std::mutex> gridLock(grid->getMutex());
            system->update(dt);
        }
        
        // Signal completion
        {
            std::lock_guard<std::mutex> lock(completeMutex);
            completedCount++;
        }
        cvComplete.notify_one();
    }
}

void SimulationManager::updateSingleThreaded(float deltaTime) {
    // Update systems sequentially
    for (auto& system : systems) {
        if (system->isEnabled()) {
            system->update(deltaTime);
        }
    }
}

void SimulationManager::updateMultiThreaded(float deltaTime) {
    // Signal all persistent threads to process this frame
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        currentDeltaTime = deltaTime;
        frameReady = true;
    }
    cv.notify_all();  // Wake up all worker threads
    
    // Wait for all threads to complete this frame
    std::unique_lock<std::mutex> lock(completeMutex);
    cvComplete.wait(lock, [this] { return completedCount >= workerThreads.size(); });
    completedCount = 0;  // Reset for next frame
    frameReady = false;
    
    // Swap buffers after all systems finish (synchronization point)
    grid->swapBuffers();
}
