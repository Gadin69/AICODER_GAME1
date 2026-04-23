#include "FluidSim.h"
#include "ElementTypes.h"
#include <random>

FluidSim::FluidSim() 
    : SimulationSystem("FluidSim", 0.1f) {  // Update every 100ms (slower, more realistic)
}

bool FluidSim::update(float deltaTime) {
    if (!enabled || !grid) return false;
    
    if (!shouldUpdate(deltaTime)) return false;
    
    // NOTE: Worker thread already holds systemMutex, no need to lock again
    // Grid access is synchronized by the main thread's grid.lock()/unlock() calls
    
    int width = grid->getWidth();
    int height = grid->getHeight();
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            
            // LOD CHECK: Skip cells based on camera distance
            if (!shouldUpdateCell(x, y, deltaTime)) continue;
            
            Cell& cell = grid->getCell(x, y);
            
            // Only process liquids
            if (!isLiquidType(cell.elementType)) continue;
            
            // VISCOSITY-BASED FLOW: High viscosity = slower movement
            // ONI-STYLE: Mass also affects flow speed!
            // Less mass = flows faster, more mass = flows slower
            const Element& props = ElementTypes::getElement(cell.elementType);
            
            // Calculate flow probability based on viscosity AND mass
            // Low viscosity + low mass = high probability (flows fast)
            // High viscosity + high mass = low probability (flows slow)
            // TIME-SCALED: Adjust probability by deltaTime for frame-rate independence
            float massRatio = cell.mass / props.density;  // 0.0 to 1.0 (partial to full)
            float baseProbability = (1.0f / (1.0f + props.viscosity * 0.5f)) * massRatio;
            float moveProbability = baseProbability * (deltaTime / 0.1f);  // Normalize to 100ms base rate
            
            // Use random to determine if fluid moves this tick
            static thread_local std::mt19937 rng(42);
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            
            if (dist(rng) < moveProbability) {
                // Update liquid physics
                updateLiquid(x, y);
            }
        }
    }
    
    return true;
}

void FluidSim::reset() {
    updateTimer = 0.0f;
}

void FluidSim::updateLiquid(int x, int y) {
    // Try to move down first (gravity)
    moveLiquidDown(x, y);
    
    // If couldn't move down, try sideways
    moveLiquidSideways(x, y);
    
    // Apply viscosity effects
    applyViscosity(x, y);
}

void FluidSim::moveLiquidDown(int x, int y) {
    Cell& cell = grid->getCell(x, y);
    
    // Check cell below
    int belowY = y + 1;
    if (!grid->isValidPosition(x, belowY)) return;
    
    Cell& belowCell = grid->getCell(x, belowY);
    
    // Move into vacuum (nothing is ever "empty")
    if (belowCell.elementType == ElementType::Vacuum) {
        // Transfer entire mass to new cell
        belowCell.elementType = cell.elementType;
        belowCell.mass = cell.mass;  // ONI-STYLE: Transfer actual mass!
        belowCell.temperature = cell.temperature;
        belowCell.color = cell.color;
        
        // Source becomes vacuum with no mass
        cell.elementType = ElementType::Vacuum;
        cell.mass = 0.0f;
        cell.gasMass = 0.0f;
        cell.pressure = 0.0f;
        cell.updateColor();  // Fix: Update color to vacuum!
        return;
    }
    
    // Displace gas (liquids are denser)
    if (canDisplace(cell.elementType, belowCell.elementType)) {
        // Swap cells WITH MASS
        std::swap(cell.elementType, belowCell.elementType);
        std::swap(cell.mass, belowCell.mass);  // Transfer mass!
        std::swap(cell.temperature, belowCell.temperature);
    }
}

void FluidSim::moveLiquidSideways(int x, int y) {
    Cell& cell = grid->getCell(x, y);
    
    // Random direction for natural flow
    static thread_local std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1);
    int dir = dist(rng) == 0 ? -1 : 1;
    
    // Try both directions
    for (int attempt = 0; attempt < 2; ++attempt) {
        int nx = x + (attempt == 0 ? dir : -dir);
        int belowY = y + 1;
        
        if (!grid->isValidPosition(nx, y)) continue;
        if (!grid->isValidPosition(nx, belowY)) continue;
        
        Cell& neighbor = grid->getCell(nx, y);
        Cell& belowNeighbor = grid->getCell(nx, belowY);
        
        // Move diagonally down if possible
        if (belowNeighbor.elementType == ElementType::Vacuum) {
            belowNeighbor.elementType = cell.elementType;
            belowNeighbor.mass = cell.mass;  // Transfer mass!
            belowNeighbor.temperature = cell.temperature;
            belowNeighbor.color = cell.color;
            
            cell.elementType = ElementType::Vacuum;
            cell.mass = 0.0f;
            cell.gasMass = 0.0f;
            cell.pressure = 0.0f;
            cell.updateColor();  // Fix: Update color to vacuum!
            return;
        }
        
        // Move sideways into vacuum
        if (neighbor.elementType == ElementType::Vacuum) {
            neighbor.elementType = cell.elementType;
            neighbor.mass = cell.mass;  // Transfer mass!
            neighbor.temperature = cell.temperature;
            neighbor.color = cell.color;
            
            cell.elementType = ElementType::Vacuum;
            cell.mass = 0.0f;
            cell.gasMass = 0.0f;
            cell.pressure = 0.0f;
            cell.updateColor();  // Fix: Update color to vacuum!
            return;
        }
    }
}

void FluidSim::applyViscosity(int x, int y) {
    Cell& cell = grid->getCell(x, y);
    const Element& props = ElementTypes::getElement(cell.elementType);
    
    // Viscosity slows velocity
    if (props.viscosity > 0.0f) {
        cell.velocityX *= (1.0f - props.viscosity * 0.1f);
        cell.velocityY *= (1.0f - props.viscosity * 0.1f);
    }
}

bool FluidSim::isLiquidType(ElementType type) {
    return type == ElementType::Liquid_Water || 
           type == ElementType::Liquid_Lava || 
           type == ElementType::ContaminatedWater;
}

bool FluidSim::canDisplace(ElementType fluid, ElementType target) {
    // Liquids can displace gases
    if (isLiquidType(fluid) && 
        (target == ElementType::Gas_O2 || target == ElementType::Gas_CO2)) {
        return true;
    }
    return false;
}
