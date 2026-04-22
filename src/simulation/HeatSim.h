#pragma once

#include "SimulationSystem.h"

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
    float ambientTemp;  // Environmental temperature
    
    // Internal heat transfer functions
    void transferHeat(int x, int y);
    void applyEnvironmentalCooling(int x, int y);
    void checkPhaseChangeTriggers(int x, int y);
    void startPhaseTransition(Cell& cell, ElementType targetType, float transitionSpeed);
};
