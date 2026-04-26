#include "HeatSim.h"
#include "GasSim.h"
#include "ElementTypes.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <chrono>

HeatSim::HeatSim() 
    : SimulationSystem("HeatSim", 0.1f),  // Update every 100ms (heat transfer takes time)
      ambientTemp(20.0f) {
}

bool HeatSim::update(float deltaTime) {
    if (!enabled || !grid) return false;
    
    if (!shouldUpdate(deltaTime)) return false;
    
    // NOTE: Worker thread already holds systemMutex, no need to lock again
    // Grid access is synchronized by the main thread's grid.lock()/unlock() calls
    
    int width = grid->getWidth();
    int height = grid->getHeight();
    
    // STABLE FTCS HEAT TRANSFER using weighted averaging method:
    // Step 1: Create snapshot of all temperatures
    // Step 2: Calculate new temperatures using weighted average
    // Step 3: Apply all updates simultaneously
    
    // STEP 1: Temperature snapshot buffer (read all current temps)
    std::vector<std::vector<float>> tempSnapshot(height, std::vector<float>(width));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (grid->isValidPosition(x, y)) {
                tempSnapshot[y][x] = grid->getCell(x, y).temperature;
            }
        }
    }
    
    // STEP 2: Calculate new temperatures
    // Initialize with current temperatures (LOD-skipped cells keep their temp)
    std::vector<std::vector<float>> newTemps(height, std::vector<float>(width));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (grid->isValidPosition(x, y)) {
                newTemps[y][x] = tempSnapshot[y][x];  // Default: keep current temp
            }
        }
    }
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            
            // LOD CHECK: Skip cells based on camera distance
            if (!shouldUpdateCell(x, y, deltaTime)) continue;
            
            const Cell& cell = grid->getCell(x, y);
            if (cell.elementType == ElementType::Empty) continue;
            
            // Use polymorphic check for heat transfer capability
            const Element& cellProps = ElementTypes::getElement(cell.elementType);
            if (!cellProps.canTransferHeat()) {
                newTemps[y][x] = tempSnapshot[y][x];  // Keep current temp
                continue;
            }
            
            // Calculate stable heat transfer using weighted averaging
            // Use mass for all element types (gases and liquids both have mass)
            float cellThermalMass = std::max(cell.mass * cellProps.specificHeatCapacity, 0.001f);
            
            // Weighted sum of temperatures
            float weightedSum = 0.0f;
            float totalWeight = 0.0f;
            
            // Center cell contribution (weighted by thermal mass)
            weightedSum += tempSnapshot[y][x] * cellThermalMass;
            totalWeight += cellThermalMass;
            
            // Check all 4 neighbors
            int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            
            for (auto& neighbor : neighbors) {
                int nx = x + neighbor[0];
                int ny = y + neighbor[1];
                
                if (!grid->isValidPosition(nx, ny)) continue;
                
                const Cell& neighborCell = grid->getCell(nx, ny);
                if (neighborCell.elementType == ElementType::Empty) continue;
                
                const Element& neighborProps = ElementTypes::getElement(neighborCell.elementType);
                if (!neighborProps.canTransferHeat()) continue;
                // Use mass for all element types
                float neighborThermalMass = std::max(neighborCell.mass * neighborProps.specificHeatCapacity, 0.001f);
                
                // Calculate thermal conductance between cells
                // Gases have low thermal conductivity (0.01-0.1 W/mK) vs solids (1-400 W/mK)
                // The conductivity values are already in the Element properties, so this is naturally handled
                float avgConductivity = (cellProps.thermalConductivity + neighborProps.thermalConductivity) * 0.5f;
                float avgThermalMass = (cellThermalMass + neighborThermalMass) * 0.5f;
                
                // Weight = conductivity * thermal mass
                float conductance = avgConductivity * avgThermalMass;
                
                weightedSum += tempSnapshot[ny][nx] * conductance;
                totalWeight += conductance;
            }
            
            // Calculate weighted average temperature
            float equilibriumTemp = weightedSum / totalWeight;
            
            // LAVA/WATER SPECIAL CASE: Film boiling increases transfer rate
            bool hasExtremeInteraction = false;
            for (auto& neighbor : neighbors) {
                int nx = x + neighbor[0];
                int ny = y + neighbor[1];
                if (grid->isValidPosition(nx, ny)) {
                    const Cell& n = grid->getCell(nx, ny);
                    if ((cell.elementType == ElementType::Liquid_Lava && n.elementType == ElementType::Liquid_Water) ||
                        (cell.elementType == ElementType::Liquid_Water && n.elementType == ElementType::Liquid_Lava)) {
                        hasExtremeInteraction = true;
                        break;
                    }
                }
            }
            
            // BLEND old and new temperature (prevents oscillation)
            // Blend factor determines how quickly we approach equilibrium
            float baseBlend = cellProps.thermalConductivity * deltaTime * 0.5f;
            float blendFactor = hasExtremeInteraction ? std::min(baseBlend * 5.0f, 0.5f) : std::min(baseBlend, 0.5f);
            
            // Apply blending (guaranteed stable when blend <= 0.5)
            newTemps[y][x] = tempSnapshot[y][x] * (1.0f - blendFactor) + equilibriumTemp * blendFactor;
            
            // Clamp to absolute zero
            newTemps[y][x] = std::max(newTemps[y][x], -273.15f);
        }
    }
    
    // STEP 3: Apply all temperature updates simultaneously
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            
            Cell& cell = grid->getCell(x, y);
            if (cell.elementType == ElementType::Empty) continue;
            
            // Use polymorphic check for heat transfer capability
            const Element& cellPropsCheck = ElementTypes::getElement(cell.elementType);
            if (!cellPropsCheck.canTransferHeat()) continue;
            
            // LOD CHECK: Only update cells that were processed in STEP 2
            if (!shouldUpdateCell(x, y, deltaTime)) {
                // LOD-skipped cell: DON'T update temperature or check phase change
                continue;
            }
            
            // Apply calculated temperature
            // All elements (including gases) participate in heat transfer
            cell.temperature = newTemps[y][x];
            
            // Environmental cooling removed - vacuum is a perfect insulator
            // Cells only exchange heat with neighboring cells, not environment
            
            // Check if temperature triggers phase change
            checkPhaseChangeTriggers(x, y, deltaTime);
        }
    }
    
    return true;
}

void HeatSim::reset() {
    updateTimer = 0.0f;
    ambientTemp = 20.0f;
}

void HeatSim::setAmbientTemperature(float temp) {
    ambientTemp = temp;
}




void HeatSim::checkPhaseChangeTriggers(int x, int y, float /*deltaTime*/) {
    Cell& cell = grid->getCell(x, y);
    const Element& props = ElementTypes::getElement(cell.elementType);
    
    // Skip vacuum and empty cells (or any element that can't transfer heat)
    if (!props.canTransferHeat() || cell.elementType == ElementType::Empty) {
        return;
    }
    
    // Query element for phase change based on temperature
    ElementType newType = props.getPhaseAtTemperature(cell.temperature);
    
    // No phase change needed
    if (newType == ElementType::Empty) {
        return;
    }
    
    // Debug logging
    static int phaseExecCount = 0;
    static auto phaseStartTime = std::chrono::steady_clock::now();
    if (phaseExecCount++ % 50 == 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - phaseStartTime).count();
        std::cout << "[" << elapsed << "ms] [PHASE] "
                  << ElementTypes::getTypeName(cell.elementType)
                  << " -> " << ElementTypes::getTypeName(newType)
                  << " at " << cell.temperature << "°C" << std::endl;
    }
    
    float oldMass = cell.mass;
    
    // Apply phase change using element's polymorphic method
    props.applyPhaseChange(cell, newType, oldMass);
    
    // SPECIAL CASE: Steam condensation collection
    // (This stays here because it involves neighbor interactions)
    if (props.isGas && newType == ElementType::Liquid_Water) {
        collectCondensedWater(cell, x, y, oldMass);
    }
}

void HeatSim::collectCondensedWater(Cell& cell, int x, int y, float originalMass) {
    int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    float remainingMass = cell.mass;
    
    for (auto& dir : neighbors) {
        int nx = x + dir[0];
        int ny = y + dir[1];
        
        if (!grid->isValidPosition(nx, ny)) continue;
        
        Cell& neighbor = grid->getCell(nx, ny);
        
        // If neighbor is water and has space, merge into it
        if (neighbor.elementType == ElementType::Liquid_Water) {
            float maxWaterMass = GasSim::getMaxMassForElement(ElementType::Liquid_Water);
            if (neighbor.mass < maxWaterMass) {
                float spaceAvailable = maxWaterMass - neighbor.mass;
                float mergeAmount = std::min(remainingMass, spaceAvailable);
            
                neighbor.mass += mergeAmount;
                neighbor.updated = true;  // Mark neighbor as updated so it flows next tick
                neighbor.updateColor();  // Update color for new mass
                remainingMass -= mergeAmount;
                
                if (remainingMass < 0.001f) break;
            }
        }
    }
    
    // Update this cell with remaining mass
    cell.mass = remainingMass;
    if (remainingMass < 0.0001f) {
        cell.convertToVacuum();
    }
}
