#pragma once

#include "SimulationSystem.h"
#include <vector>

// Heat simulation system - handles thermodynamics
// Responsible for: heat transfer, temperature changes, phase change triggers
class HeatSim : public SimulationSystem {
public:
    HeatSim();
    
    // SimulationSystem interface
    bool update(float deltaTime) override;
    void reset() override;
    
    // Heat-specific methods
    void setAmbientTemperature(float temp);
    float getAmbientTemperature() const { return ambientTemp; }
    
private:
    float ambientTemp;  // Environmental temperature (kept for future use if needed)
    
    // Internal heat transfer functions
    void checkPhaseChangeTriggers(int x, int y, float deltaTime);
    void startPhaseTransition(Cell& cell, ElementType targetType, float transitionSpeed);
};
