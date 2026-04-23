#include "HeatSim.h"
#include "ElementTypes.h"
#include <cmath>

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
    std::vector<std::vector<float>> newTemps(height, std::vector<float>(width, 0.0f));
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            
            // LOD CHECK: Skip cells based on camera distance
            if (!shouldUpdateCell(x, y, deltaTime)) continue;
            
            const Cell& cell = grid->getCell(x, y);
            if (cell.elementType == ElementType::Empty) continue;
            
            // Skip vacuum - no heat transfer
            if (cell.elementType == ElementType::Vacuum) {
                newTemps[y][x] = tempSnapshot[y][x];  // Keep current temp
                continue;
            }
            
            // Calculate stable heat transfer using weighted averaging
            const Element& cellProps = ElementTypes::getElement(cell.elementType);
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
                if (neighborCell.elementType == ElementType::Vacuum) continue;
                if (neighborCell.elementType == ElementType::Empty) continue;
                
                const Element& neighborProps = ElementTypes::getElement(neighborCell.elementType);
                float neighborThermalMass = std::max(neighborCell.mass * neighborProps.specificHeatCapacity, 0.001f);
                
                // Calculate thermal conductance between cells
                float avgConductivity = (cellProps.thermalConductivity + neighborProps.thermalConductivity) * 0.5f;
                float avgThermalMass = (cellThermalMass + neighborThermalMass) * 0.5f;
                
                // Weight = conductivity * thermal mass (higher = more influence)
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
            if (cell.elementType == ElementType::Vacuum) continue;
            
            // Apply calculated temperature
            cell.temperature = newTemps[y][x];
            
            // Apply environmental cooling/heating
            applyEnvironmentalCooling(x, y, deltaTime);
            
            // Check if temperature triggers phase change
            checkPhaseChangeTriggers(x, y);
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



void HeatSim::applyEnvironmentalCooling(int x, int y, float deltaTime) {
    Cell& cell = grid->getCell(x, y);
    const Element& props = ElementTypes::getElement(cell.elementType);
    
    // ONI-STYLE: THERMAL MASS affects cooling rate
    // High mass = takes longer to cool/warm
    float thermalMass = cell.mass * props.specificHeatCapacity;
    
    // Trend toward ambient temperature
    // Cooling rate inversely proportional to thermal mass
    // TIME-SCALED: Multiply by deltaTime for frame-rate independence
    float coolingRate = 0.05f / (thermalMass * 0.001f) * deltaTime;  // Normalize
    float envEffect = (ambientTemp - cell.temperature) * coolingRate;
    
    // Gases cool faster (low thermal mass)
    if (props.isGas) {
        envEffect *= 2.0f;
    }
    
    cell.temperature += envEffect;
    
    // Prevent below absolute zero
    if (cell.temperature < -273.15f) {
        cell.temperature = -273.15f;
    }
}

void HeatSim::checkPhaseChangeTriggers(int x, int y) {
    Cell& cell = grid->getCell(x, y);
    const Element& props = ElementTypes::getElement(cell.elementType);
    
    // Skip if already transitioning
    if (cell.phaseTransitionProgress > 0.0f && cell.phaseTransitionProgress < 1.0f) {
        // Update transition progress
        cell.phaseTransitionProgress += cell.phaseTransitionSpeed;
        
        if (cell.phaseTransitionProgress >= 1.0f) {
            // Complete the phase change
            cell.elementType = cell.targetElementType;
            cell.phaseTransitionProgress = 0.0f;
            cell.targetElementType = ElementType::Empty;
            cell.phaseTransitionSpeed = 0.0f;
            
            // Update cell color
            cell.updateColor();
        }
        return;
    }
    
    // Check for phase changes based on temperature
    
    // WATER BOILING -> STEAM
    if (cell.elementType == ElementType::Liquid_Water && cell.temperature >= props.boilingPoint) {
        startPhaseTransition(cell, ElementType::Gas_O2, 100.0f);  // Steam at 100°C
    }
    
    // WATER FREEZING -> ICE (treat as solid for now)
    if (cell.elementType == ElementType::Liquid_Water && cell.temperature <= props.meltingPoint) {
        // startPhaseTransition(cell, ElementType::Solid, 30.0f);  // Disabled for now
    }
    
    // LAVA COOLING -> ROCK
    if (cell.elementType == ElementType::Liquid_Lava && cell.temperature <= props.meltingPoint) {
        startPhaseTransition(cell, ElementType::Solid, 20.0f);  // Rock at 700°C
    }
    
    // STEAM CONDENSING -> WATER
    if (cell.elementType == ElementType::Gas_O2 && cell.temperature <= 90.0f) {  // Below boiling point
        startPhaseTransition(cell, ElementType::Liquid_Water, 15.0f);  // Water
    }
    
    // Prevent below absolute zero
    if (cell.temperature < -273.15f) {
        cell.temperature = -273.15f;
    }
}

void HeatSim::startPhaseTransition(Cell& cell, ElementType targetType, float transitionSpeed) {
    // Only start if not already transitioning
    if (cell.phaseTransitionProgress > 0.0f && cell.phaseTransitionProgress < 1.0f) {
        return;
    }
    
    cell.targetElementType = targetType;
    cell.phaseTransitionProgress = 0.0f;
    cell.phaseTransitionSpeed = transitionSpeed;  // Progress per second
}
