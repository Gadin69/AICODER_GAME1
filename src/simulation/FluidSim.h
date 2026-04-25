#pragma once

#include "SimulationSystem.h"
#include "ElementTypes.h"
#include <vector>
#include <unordered_set>

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
    // Initialize liquid types dynamically at startup
    void initializeLiquidTypes();
    
    // Internal fluid functions
    void updateLiquid(int x, int y);
    void moveLiquidDown(int x, int y);
    void moveLiquidSideways(int x, int y);
    void leakLiquidDiagonally(int x, int y, float deltaTime);
    void applyViscosity(int x, int y);
    
    bool isLiquidType(ElementType type);
    bool canDisplace(ElementType fluid, ElementType target);
    bool hasGasEscapeRoute(int x, int y, ElementType gasType);
    
    // Dynamic liquid type cache (populated at startup)
    std::unordered_set<ElementType> liquidTypes;
    
    // Gas displacement queue
    struct DisplacedGas {
        int x, y;
        float mass;
        float temperature;
        ElementType type;
    };
    std::vector<DisplacedGas> displacedGasQueue;
};
