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
    
    // Public utility functions
    bool isTrappedBetweenDenserLiquids(int x, int y);
    
private:
    // Flow action structure for liquid movement
    struct FlowAction {
        int fromX, fromY;
        int toX, toY;
        float massToMove;
        float temperature;
        ElementType type;
        bool isMerge;  // true = merge into target, false = swap
        bool overwriteGas = false;  // true = liquid overwriting gas (not a swap)
    };
    
    // Gas displacement queue
    struct DisplacedGas {
        int x, y;
        float mass;
        float temperature;
        ElementType type;
    };
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
    
    // Displacement function: tries to move target cell out of the way
    // Returns true if displacement succeeded, false if no escape route
    bool Displace(int fromX, int fromY, int targetX, int targetY,
                  std::vector<std::vector<bool>>& isMergeSource,
                  std::vector<std::vector<bool>>& isMergeTarget,
                  std::vector<std::vector<bool>>& isSwapInvolved,
                  std::vector<DisplacedGas>& displacedGasQueue,
                  std::vector<FlowAction>& flowActions);
    
    // Dynamic liquid type cache (populated at startup)
    std::unordered_set<ElementType> liquidTypes;
    std::vector<DisplacedGas> displacedGasQueue;
};
