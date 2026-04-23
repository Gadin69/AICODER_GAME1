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
    
    // Update heat for all cells
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            
            // LOD CHECK: Skip cells based on camera distance
            if (!shouldUpdateCell(x, y, deltaTime)) continue;
            
            const Cell& cell = grid->getCell(x, y);
            if (cell.elementType == ElementType::Empty) continue;
            
            // Skip vacuum - no heat transfer
            if (cell.elementType == ElementType::Vacuum) continue;
            
            // Transfer heat with neighbors
            transferHeat(x, y, deltaTime);
            
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

void HeatSim::transferHeat(int x, int y, float deltaTime) {
    Cell& cell = grid->getCell(x, y);
    const Element& cellProps = ElementTypes::getElement(cell.elementType);
    
    // ONI-STYLE: Use CELL MASS (dynamic, not fixed by density!)
    // A cell can be half-full, so mass varies
    // Thermal mass = actual mass × specific heat
    float cellThermalMass = cell.mass * cellProps.specificHeatCapacity;
    float cellEnergy = cellThermalMass * cell.temperature;  // Total energy in Joules
    
    // Check all 4 neighbors
    int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    for (auto& neighbor : neighbors) {
        int nx = x + neighbor[0];
        int ny = y + neighbor[1];
        
        if (!grid->isValidPosition(nx, ny)) continue;
        
        Cell& neighborCell = grid->getCell(nx, ny);
        
        // Skip vacuum cells (no heat transfer)
        if (neighborCell.elementType == ElementType::Vacuum) continue;
        
        const Element& neighborProps = ElementTypes::getElement(neighborCell.elementType);
        
        // Calculate temperature difference
        float tempDiff = cell.temperature - neighborCell.temperature;
        
        // Only transfer if meaningful difference
        if (std::abs(tempDiff) < 0.5f) continue;
        
        // NEIGHBOR THERMAL MASS (using actual cell mass!)
        float neighborThermalMass = neighborCell.mass * neighborProps.specificHeatCapacity;
        float neighborEnergy = neighborThermalMass * neighborCell.temperature;
        
        // ENERGY-BASED HEAT TRANSFER:
        // Heat flows from hot to cold until equilibrium
        // Equilibrium temperature = (E1 + E2) / (m1*cp1 + m2*cp2)
        float totalEnergy = cellEnergy + neighborEnergy;
        float totalThermalMass = cellThermalMass + neighborThermalMass;
        float equilibriumTemp = totalEnergy / totalThermalMass;
        
        // But transfer is limited by thermal conductivity!
        // Real heat transfer: q = k × A × ΔT / d (Fourier's Law)
        float avgConductivity = (cellProps.thermalConductivity + neighborProps.thermalConductivity) * 0.5f;
        
        // LAVA/WATER SPECIAL CASE: Film boiling increases transfer
        bool extremeInteraction = 
            (cell.elementType == ElementType::Liquid_Lava && neighborCell.elementType == ElementType::Liquid_Water) ||
            (cell.elementType == ElementType::Liquid_Water && neighborCell.elementType == ElementType::Liquid_Lava);
        
        float conductivityMultiplier = extremeInteraction ? 5.0f : 1.0f;
        
        // SLOW HEAT TRANSFER: Only move toward equilibrium gradually
        // Higher thermal mass = slower to change temperature
        // TIME-SCALED: Multiply by deltaTime for frame-rate independence
        float transferRate = avgConductivity * conductivityMultiplier * 0.01f * deltaTime;
        
        // Energy to transfer (limited by conductivity)
        float energyToTransfer = (equilibriumTemp - cell.temperature) * cellThermalMass * transferRate;
        
        // Apply energy transfer
        cell.temperature -= energyToTransfer / cellThermalMass;
        neighborCell.temperature += energyToTransfer / neighborThermalMass;
    }
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
