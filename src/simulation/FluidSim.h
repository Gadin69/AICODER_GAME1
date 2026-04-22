#pragma once

#include "SimulationSystem.h"

// Fluid simulation system - handles liquid physics
// Responsible for: liquid flow, viscosity, density-based movement
class FluidSim : public SimulationSystem {
public:
    FluidSim();
    
    // SimulationSystem interface
    bool update(float deltaTime) override;
    void reset() override;
    
    // Configure simulation speed
    void setUpdateInterval(float interval) { updateInterval = interval; }
    
private:
    // Internal fluid functions
    void updateLiquid(int x, int y);
    void moveLiquidDown(int x, int y);
    void moveLiquidSideways(int x, int y);
    void applyViscosity(int x, int y);
    
    bool isLiquidType(ElementType type);
    bool canDisplace(ElementType fluid, ElementType target);
};
