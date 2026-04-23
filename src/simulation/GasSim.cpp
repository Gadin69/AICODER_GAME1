#include "GasSim.h"
#include "ElementTypes.h"
#include <cmath>

GasSim::GasSim() 
    : SimulationSystem("GasSim", 0.08f) {  // Update every 80ms (gas moves slower than you'd think)
}

bool GasSim::update(float deltaTime) {
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
            
            // Only process gas cells
            if (!isGasType(cell.elementType)) continue;
            
            // Calculate pressure for this cell
            calculatePressure(x, y);
            
            // Try to merge with neighbors if gas mass is small
            mergeGasWithNeighbors(x, y);
            
            // Flow gas based on pressure differences
            flowGasByPressure(x, y);
            
            // Prioritize filling vacuum spaces
            fillVacuumPreferentially(x, y);
        }
    }
    
    return true;
}

void GasSim::reset() {
    updateTimer = 0.0f;
}

void GasSim::calculatePressure(int x, int y) {
    Cell& cell = grid->getCell(x, y);
    if (!isGasType(cell.elementType)) return;
    
    const Element& props = ElementTypes::getElement(cell.elementType);
    
    // Ideal gas law: PV = nRT
    // P = (mass / molarMass) * R * T / V
    float moles = cell.gasMass / props.density;  // Simplified: using density as molar mass
    float tempKelvin = cell.temperature + 273.15f;
    float pressure = (moles * GAS_CONSTANT * tempKelvin) / CELL_VOLUME;
    
    // Clamp pressure
    cell.pressure = std::max(101325.0f, pressure);  // Minimum 1 atm
}

void GasSim::mergeGasWithNeighbors(int x, int y) {
    Cell& cell = grid->getCell(x, y);
    
    // Only merge if gas mass is below threshold
    if (cell.gasMass >= MIN_GAS_MASS * 2.0f) return;
    
    int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    for (auto& dir : neighbors) {
        int nx = x + dir[0];
        int ny = y + dir[1];
        
        if (!grid->isValidPosition(nx, ny)) continue;
        
        Cell& neighbor = grid->getCell(nx, ny);
        
        // Can merge with same gas type
        if (isSameGas(cell.elementType, neighbor.elementType)) {
            float combinedMass = cell.gasMass + neighbor.gasMass;
            
            // If combined mass fits in one cell, merge
            if (combinedMass <= MAX_GAS_MASS && neighbor.gasMass < MIN_GAS_MASS) {
                // Move neighbor's gas to this cell
                cell.gasMass += neighbor.gasMass;
                // Neighbor becomes vacuum - reset ALL data
                neighbor.gasMass = 0.0f;
                neighbor.elementType = ElementType::Vacuum;
                neighbor.pressure = 0.0f;
                neighbor.mass = 0.0f;
                neighbor.temperature = -273.15f;  // Absolute zero
                neighbor.velocityX = 0.0f;
                neighbor.velocityY = 0.0f;
                neighbor.updateColor();  // Update to vacuum color!
                break;
            }
        }
    }
}

void GasSim::flowGasByPressure(int x, int y) {
    Cell& cell = grid->getCell(x, y);
    if (!isGasType(cell.elementType)) return;
    
    int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    // Find neighbor with lowest pressure
    float lowestPressure = cell.pressure;
    int bestNx = x, bestNy = y;
    
    for (auto& dir : neighbors) {
        int nx = x + dir[0];
        int ny = y + dir[1];
        
        if (!grid->isValidPosition(nx, ny)) continue;
        
        Cell& neighbor = grid->getCell(nx, ny);
        
        // Gas flows to vacuum first
        if (neighbor.elementType == ElementType::Vacuum) {
            lowestPressure = 0.0f;
            bestNx = nx;
            bestNy = ny;
            break;
        }
        
        // Gas flows to lower pressure
        if (isGasType(neighbor.elementType) && neighbor.pressure < lowestPressure) {
            lowestPressure = neighbor.pressure;
            bestNx = nx;
            bestNy = ny;
        }
    }
    
    // Move gas if we found a better spot
    if (bestNx != x || bestNy != y) {
        Cell& target = grid->getCell(bestNx, bestNy);
        
        // Transfer some gas mass
        float transferAmount = cell.gasMass * 0.1f;  // 10% per tick
        
        if (target.elementType == ElementType::Vacuum) {
            // Move into empty space
            target.elementType = cell.elementType;
            target.gasMass = transferAmount;
            target.temperature = cell.temperature;
            target.pressure = cell.pressure;
            
            cell.gasMass -= transferAmount;
            
            // If cell has no gas left, it becomes vacuum - reset ALL data
            if (cell.gasMass < MIN_GAS_MASS) {
                cell.elementType = ElementType::Vacuum;
                cell.gasMass = 0.0f;
                cell.pressure = 0.0f;
                cell.mass = 0.0f;
                cell.temperature = -273.15f;  // Absolute zero
                cell.velocityX = 0.0f;
                cell.velocityY = 0.0f;
                cell.updateColor();  // Update to vacuum color!
            }
        }
    }
}

void GasSim::fillVacuumPreferentially(int x, int y) {
    Cell& cell = grid->getCell(x, y);
    if (!isGasType(cell.elementType)) return;
    
    // Check for adjacent vacuum cells
    int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    for (auto& dir : neighbors) {
        int nx = x + dir[0];
        int ny = y + dir[1];
        
        if (!grid->isValidPosition(nx, ny)) continue;
        
        Cell& neighbor = grid->getCell(nx, ny);
        
        // Expand into vacuum
        if (neighbor.elementType == ElementType::Vacuum) {
            // Gas expands to fill vacuum
            float expansionAmount = cell.gasMass * 0.2f;  // 20% expansion
            
            neighbor.elementType = cell.elementType;
            neighbor.gasMass = expansionAmount;
            neighbor.temperature = cell.temperature * 0.95f;  // Slight cooling on expansion
            neighbor.pressure = cell.pressure * 0.8f;  // Lower pressure in expanded space
            
            cell.gasMass -= expansionAmount;
            
            // Check if source cell should become vacuum - reset ALL data
            if (cell.gasMass < MIN_GAS_MASS) {
                cell.elementType = ElementType::Vacuum;
                cell.gasMass = 0.0f;
                cell.pressure = 0.0f;
                cell.mass = 0.0f;
                cell.temperature = -273.15f;  // Absolute zero
                cell.velocityX = 0.0f;
                cell.velocityY = 0.0f;
                cell.updateColor();  // Update to vacuum color!
            }
            
            break;  // Only expand to one vacuum per tick
        }
    }
}

bool GasSim::isGasType(ElementType type) {
    return type == ElementType::Gas_O2 || type == ElementType::Gas_CO2;
}

bool GasSim::isSameGas(ElementType type1, ElementType type2) {
    return type1 == type2;
}

float GasSim::getMolarMass(ElementType type) {
    switch (type) {
        case ElementType::Gas_O2: return 0.032f;   // O2: 32 g/mol
        case ElementType::Gas_CO2: return 0.044f;  // CO2: 44 g/mol
        default: return 0.029f;  // Air average
    }
}
