#include "HeatSim.h"
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
            // GASES: Skip temperature updates - gas temperature is handled by GasSim only
            // But still allow phase changes (handled later in this function)
            if (!cellPropsCheck.isGas) {
                cell.temperature = newTemps[y][x];
            }
            
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
    
    // Check if temperature triggers a phase change (instant, no gradual transition)
    ElementType targetType = ElementType::Empty;
    
    // ==========================================
    // WATER SYSTEM (5 transitions)
    // ==========================================
    
    // WATER FREEZING -> ICE
    if (cell.elementType == ElementType::Liquid_Water && 
        cell.temperature <= props.freezingPoint) {
        targetType = ElementType::Solid_Ice;
    }
    // ICE MELTING -> WATER
    else if (cell.elementType == ElementType::Solid_Ice && 
        cell.temperature > props.meltingPoint) {
        targetType = ElementType::Liquid_Water;
    }
    // WATER BOILING -> STEAM
    else if (cell.elementType == ElementType::Liquid_Water && 
        cell.temperature >= props.boilingPoint) {
        // DEBUG: Track why water might not boil
        static int boilDebugCount = 0;
        if (boilDebugCount++ % 200 == 0) {
            std::cout << "[BOIL DEBUG] Water at " << cell.temperature 
                      << "°C, boilingPoint=" << props.boilingPoint
                      << ", mass=" << cell.mass
                      << ", isLiquid=" << props.isLiquid << std::endl;
        }
        targetType = ElementType::Gas_O2;
    }
    // STEAM PHASE CHANGE - Choose correct phase based on temperature!
    else if (cell.elementType == ElementType::Gas_O2) {
        if (cell.temperature <= props.freezingPoint) {
            // Below 0°C: Steam becomes ICE (deposition)
            targetType = ElementType::Solid_Ice;
        } else if (cell.temperature < props.condensationPoint - 5.0f) {
            // 0°C to 95°C: Steam condenses to WATER
            targetType = ElementType::Liquid_Water;
        }
        // Above 95°C: Stay as steam (no phase change)
    }
    // ICE SUBLIMATING -> STEAM (skip liquid phase at high temps)
    else if (cell.elementType == ElementType::Solid_Ice && 
        cell.temperature >= 100.0f) {
        targetType = ElementType::Gas_O2;
    }
    
    // ==========================================
    // LAVA SYSTEM (4 transitions)
    // ==========================================
    
    // LAVA FREEZING -> ROCK
    else if (cell.elementType == ElementType::Liquid_Lava && 
        cell.temperature <= props.freezingPoint) {
        targetType = ElementType::Solid;
    }
    // ROCK MELTING -> LAVA
    else if (cell.elementType == ElementType::Solid && 
        cell.temperature > props.meltingPoint) {
        targetType = ElementType::Liquid_Lava;
    }
    // LAVA VAPORIZING -> GAS LAVA
    else if (cell.elementType == ElementType::Liquid_Lava && 
        cell.temperature >= props.boilingPoint) {
        targetType = ElementType::Gas_Lava;
    }
    // GAS LAVA CONDENSING -> LAVA (with hysteresis)
    else if (cell.elementType == ElementType::Gas_Lava && 
        cell.temperature < props.condensationPoint - 1.0f) {
        targetType = ElementType::Liquid_Lava;
    }
    
    // ==========================================
    // CONTAMINATED WATER SYSTEM (4 transitions)
    // ==========================================
    
    // CONTAMINATED WATER FREEZING -> SOLID
    else if (cell.elementType == ElementType::ContaminatedWater && 
        cell.temperature <= props.freezingPoint) {
        targetType = ElementType::Solid_ContaminatedWater;
    }
    // FROZEN CONTAMINATED WATER MELTING -> LIQUID
    else if (cell.elementType == ElementType::Solid_ContaminatedWater && 
        cell.temperature > props.meltingPoint) {
        targetType = ElementType::ContaminatedWater;
    }
    // CONTAMINATED WATER BOILING -> STEAM (purification)
    else if (cell.elementType == ElementType::ContaminatedWater && 
        cell.temperature >= props.boilingPoint) {
        targetType = ElementType::Gas_O2;
    }
    // STEAM CONDENSING -> WATER (handled by element's condensationPoint property)
    // Removed hardcoded check - was creating water mid-air when steam cooled below 100°C
    
    // ==========================================
    // CO2 SYSTEM (2 transitions - deposition/sublimation)
    // ==========================================
    
    // CO2 DEPOSITION -> DRY ICE (gas directly to solid)
    else if (cell.elementType == ElementType::Gas_CO2 && 
        cell.temperature <= props.sublimationPoint) {
        targetType = ElementType::Solid_DryIce;
    }
    // DRY ICE SUBLIMATION -> CO2 GAS (solid directly to gas)
    else if (cell.elementType == ElementType::Solid_DryIce && 
        cell.temperature > props.sublimationPoint) {
        targetType = ElementType::Gas_CO2;
    }
    
    // If no phase change triggered, return
    if (targetType == ElementType::Empty) {
        return;
    }
    
    // DEBUG: Log when phase change executes with timestamp
    static int phaseExecCount = 0;
    static auto phaseStartTime = std::chrono::steady_clock::now();
    if (phaseExecCount++ % 50 == 0) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - phaseStartTime).count();
        std::cout << "[" << elapsed << "ms] [PHASE] "
                  << ElementTypes::getTypeName(cell.elementType)
                  << " -> " << ElementTypes::getTypeName(targetType)
                  << " at " << cell.temperature << "°C" << std::endl;
    }
    
    // ==========================================
    // EXECUTE PHASE CHANGE (like cell placement)
    // ==========================================
    
    const Element& newProps = ElementTypes::getElement(targetType);
    
    // Preserve temperature (phase change happens at current temp)
    float preservedTemp = cell.temperature;
    
    // Convert mass based on phase change type
    float newMass = cell.mass;
    
    // Liquid -> Gas: Convert ALL liquid mass to gas mass
    // Gas has lower density, so all the mass converts
    if (props.isLiquid && newProps.isGas) {
        newMass = cell.mass;  // All mass converts to gas
    }
    // Gas -> Liquid: Gas mass becomes liquid mass (PRESERVE MASS!)
    else if (props.isGas && newProps.isLiquid) {
        newMass = cell.mass;  // Preserve exact mass - NO MAGIC MASS!
    }
    // Solid <-> Liquid: Keep mass as-is
    // Solid <-> Gas: Keep mass as-is
    
    // DIRECTLY replace the cell (like placement does)
    cell.elementType = targetType;
    cell.mass = newMass;
    cell.temperature = preservedTemp;
    
    // Update pressure for gases
    if (newProps.isGas) {
        cell.pressure = (newMass * 8.314f * (cell.temperature + 273.15f)) / 0.001f;  // Ideal gas law
    } else {
        cell.pressure = 0.0f;
    }
    
    // Reset velocity (but give liquids gravity to fall immediately)
    cell.velocityX = 0.0f;
    if (newProps.isLiquid) {
        cell.velocityY = 2.0f;  // Give downward velocity so it falls immediately
    } else {
        cell.velocityY = 0.0f;
    }
    
    // Update color to new element
    cell.updateColor();
    
    // Mark cell as updated so it gets simulated next frame
    cell.updated = true;
    
    // Prevent below absolute zero
    if (cell.temperature < -273.15f) {
        cell.temperature = -273.15f;
    }
    
    // STEAM COLLECTION: If steam condensed to water, check if it should merge with adjacent water
    if (props.isGas && newProps.isLiquid && newProps.name == "Water") {
        // Check all 4 neighbors for water cells to merge into
        int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        
        for (auto& dir : neighbors) {
            int nx = x + dir[0];
            int ny = y + dir[1];
            
            if (!grid->isValidPosition(nx, ny)) continue;
            
            Cell& neighbor = grid->getCell(nx, ny);
            
            // If neighbor is water and has space (< 1.0 kg), merge into it
            if (neighbor.elementType == ElementType::Liquid_Water && neighbor.mass < 1.0f) {
                float spaceAvailable = 1.0f - neighbor.mass;
                float mergeAmount = std::min(newMass, spaceAvailable);
                
                neighbor.mass += mergeAmount;
                neighbor.updated = true;  // Mark neighbor as updated so it flows next tick
                neighbor.updateColor();  // Update color for new mass
                newMass -= mergeAmount;
                
                // If remaining mass is too small, cap it at 0.001kg to prevent micro-cells
                if (newMass < 0.001f && newMass > 0.0f) {
                    newMass = 0.001f;  // Cap at minimum, don't convert to vacuum
                }
                
                // If all mass merged (zero remaining), this cell becomes vacuum
                if (newMass < 0.0001f) {
                    cell.elementType = ElementType::Vacuum;
                    cell.mass = 0.0f;
                    cell.pressure = 0.0f;
                    cell.temperature = -273.15f;
                    cell.velocityX = 0.0f;
                    cell.velocityY = 0.0f;
                    cell.updated = false;
                    cell.targetElementType = ElementType::Empty;
                    cell.phaseTransitionProgress = 0.0f;
                    cell.phaseTransitionSpeed = 0.0f;
                    cell.microMassDecayTime = 0.0f;
                    cell.updateColor();  // Use proper vacuum color
                    break;
                }
            }
        }
        
        // Update remaining mass
        cell.mass = newMass;
    }
}
