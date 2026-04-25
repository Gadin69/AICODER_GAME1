#pragma once

#include "SimulationSystem.h"

// Gas simulation system - handles gas physics
// Responsible for: pressure calculation, gas accumulation, pressure-based flow, vacuum filling
class GasSim : public SimulationSystem {
public:
    GasSim();
    
    // SimulationSystem interface
    bool update(float deltaTime) override;
    void reset() override;
    
    // Utility: Calculate maximum mass for an element based on density
    static float getMaxMassForElement(ElementType type);
    
private:
    // Gas physics constants
    static constexpr float GAS_CONSTANT = 8.314f;  // J/(mol·K)
    static constexpr float CELL_VOLUME = 0.001f;   // 1 liter per cell (m³)
    static constexpr float MIN_GAS_MASS = 0.001f;  // kg - minimum to keep gas cell
    static constexpr float GAS_COMPRESSION_MULTIPLIER = 10000.0f;  // Compression ratio for gases
    
    // Internal gas functions
    void calculatePressure(int x, int y);
    void mergeGasWithNeighbors(int x, int y);
    void swapWithLighterLiquidAbove(int x, int y);
    void flowGasByPressure(int x, int y);
    void fillVacuumPreferentially(int x, int y);
    void leakGasDiagonally(int x, int y, float deltaTime);
    
    bool isGasType(ElementType type);
    bool isSameGas(ElementType type1, ElementType type2);
    float getMolarMass(ElementType type);
};
