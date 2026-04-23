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
    
    // LIQUID FLOW SYSTEM: Mass-transfer based flow (like gas, but inverted for gravity)
    // Priority: DOWN, DOWN-DIAGONALS, SIDEWAYS
    // Mass transfer rate based on viscosity
    
    struct FlowAction {
        int fromX, fromY;
        int toX, toY;
        float massToMove;
        float temperature;
        ElementType type;
        bool isMerge;  // true = merge into target, false = swap
    };
    
    std::vector<FlowAction> flowActions;
    flowActions.reserve(width * height / 4);
    
    // Random seed for direction choice
    static unsigned int fluidRandomSeed = 42;
    auto simpleRand = []() -> int {
        fluidRandomSeed = fluidRandomSeed * 1103515245 + 12345;
        return (fluidRandomSeed / 65536) % 32768;
    };
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            
            Cell& cell = grid->getCell(x, y);
            if (!isLiquidType(cell.elementType)) continue;
            
            float cellMass = cell.mass;
            const Element& props = ElementTypes::getElement(cell.elementType);
            
            // VISCOSITY-BASED FLOW RATE
            // Low viscosity (water=1.0) = fast flow, High viscosity (honey=100) = slow flow
            // Flow rate = portion of mass that can move this tick
            float flowRate = 1.0f / (1.0f + props.viscosity * 0.1f);  // 0.0 to 1.0
            float massToFlow = cellMass * flowRate;
            
            // Minimum mass to flow (prevent micro-transfers)
            if (massToFlow < 0.001f) continue;
            
            // PRIORITY ORDER: DOWN, DOWN-DIAGONAL (random), SIDEWAYS (random)
            // Total: 5 directions
            int directions[5][2];
            int dirCount = 0;
            
            // DOWN first (gravity - liquids MUST fall)
            directions[dirCount][0] = 0; directions[dirCount][1] = 1; dirCount++;
            
            // DOWN-DIAGONAL (randomize left/right)
            if (simpleRand() % 2 == 0) {
                directions[dirCount][0] = -1; directions[dirCount][1] = 1; dirCount++;
                directions[dirCount][0] = 1; directions[dirCount][1] = 1; dirCount++;
            } else {
                directions[dirCount][0] = 1; directions[dirCount][1] = 1; dirCount++;
                directions[dirCount][0] = -1; directions[dirCount][1] = 1; dirCount++;
            }
            
            // SIDEWAYS (randomize left/right)
            if (simpleRand() % 2 == 0) {
                directions[dirCount][0] = -1; directions[dirCount][1] = 0; dirCount++;
                directions[dirCount][0] = 1; directions[dirCount][1] = 0; dirCount++;
            } else {
                directions[dirCount][0] = 1; directions[dirCount][1] = 0; dirCount++;
                directions[dirCount][0] = -1; directions[dirCount][1] = 0; dirCount++;
            }
            
            // Try each direction in priority order
            float remainingMass = massToFlow;
            for (int i = 0; i < dirCount && remainingMass > 0.001f; i++) {
                int nx = x + directions[i][0];
                int ny = y + directions[i][1];
                
                if (!grid->isValidPosition(nx, ny)) continue;
                
                Cell& neighbor = grid->getCell(nx, ny);
                ElementType neighborType = neighbor.elementType;
                
                bool isVacuum = (neighborType == ElementType::Vacuum);
                bool isSameLiquid = isLiquidType(neighborType) && neighborType == cell.elementType;
                bool isDifferentLiquid = isLiquidType(neighborType) && neighborType != cell.elementType;
                bool isDisplaceableGas = canDisplace(cell.elementType, neighborType);
                
                FlowAction action;
                action.fromX = x; action.fromY = y;
                action.toX = nx; action.toY = ny;
                action.massToMove = 0.0f;
                action.temperature = cell.temperature;
                action.type = cell.elementType;
                action.isMerge = false;
                
                if (isVacuum) {
                    // VACUUM: Move mass into vacuum
                    action.massToMove = remainingMass;
                    action.isMerge = false;  // Will swap/move
                    flowActions.push_back(action);
                    remainingMass = 0.0f;
                    break;
                }
                else if (isSameLiquid) {
                    // SAME LIQUID: Merge based on viscosity
                    float spaceAvailable = 2.0f - neighbor.mass;  // 2kg max
                    float mergeAmount = std::min(remainingMass, spaceAvailable);
                    
                    if (mergeAmount >= 0.001f) {
                        action.massToMove = mergeAmount;
                        action.isMerge = true;
                        flowActions.push_back(action);
                        remainingMass -= mergeAmount;
                        // Don't break - can split mass to multiple neighbors
                    }
                }
                else if (isDifferentLiquid) {
                    // DIFFERENT LIQUID: Swap based on density
                    const Element& cellProps = ElementTypes::getElement(cell.elementType);
                    const Element& neighborProps = ElementTypes::getElement(neighborType);
                    
                    // Denser liquid sinks through lighter liquid
                    if (cellProps.density > neighborProps.density) {
                        action.massToMove = remainingMass;
                        action.isMerge = false;  // Swap
                        flowActions.push_back(action);
                        remainingMass = 0.0f;
                        break;
                    }
                }
                else if (isDisplaceableGas) {
                    // GAS: Displace it (liquids are denser)
                    action.massToMove = remainingMass;
                    action.isMerge = false;  // Swap
                    flowActions.push_back(action);
                    remainingMass = 0.0f;
                    break;
                }
            }
        }
    }
    
    // Apply all flow actions simultaneously
    for (const auto& action : flowActions) {
        Cell& fromCell = grid->getCell(action.fromX, action.fromY);
        Cell& toCell = grid->getCell(action.toX, action.toY);
        
        if (action.isMerge) {
            // MERGE: Add mass to target
            float totalMass = toCell.mass + action.massToMove;
            float newTemp = (toCell.mass * toCell.temperature + action.massToMove * action.temperature) / totalMass;
            
            toCell.mass = totalMass;
            toCell.temperature = newTemp;
            toCell.updated = true;
            toCell.updateColor();
            
            // Remove mass from source
            fromCell.mass -= action.massToMove;
            if (fromCell.mass < 0.001f) {
                // Source became micro-cell - convert to vacuum
                fromCell.elementType = ElementType::Vacuum;
                fromCell.mass = 0.0f;
                fromCell.temperature = -273.15f;
                fromCell.velocityX = 0.0f;
                fromCell.velocityY = 0.0f;
                fromCell.updated = false;
                fromCell.targetElementType = ElementType::Empty;
                fromCell.phaseTransitionProgress = 0.0f;
                fromCell.phaseTransitionSpeed = 0.0f;
                fromCell.microMassDecayTime = 0.0f;
                fromCell.color = sf::Color(10, 10, 15);
            } else {
                fromCell.updated = true;
            }
        } else {
            // SWAP/MOVE: Exchange cells
            if (toCell.elementType == ElementType::Vacuum) {
                // Moving into vacuum - transfer all data
                toCell.elementType = fromCell.elementType;
                toCell.mass = action.massToMove;
                toCell.temperature = fromCell.temperature;
                toCell.pressure = 0.0f;
                toCell.velocityX = 0.0f;
                toCell.velocityY = 0.0f;
                toCell.updated = true;
                toCell.targetElementType = ElementType::Empty;
                toCell.phaseTransitionProgress = 0.0f;
                toCell.phaseTransitionSpeed = 0.0f;
                toCell.microMassDecayTime = 0.0f;
                toCell.color = fromCell.color;
                
                // Source becomes vacuum
                fromCell.elementType = ElementType::Vacuum;
                fromCell.mass = 0.0f;
                fromCell.temperature = -273.15f;
                fromCell.velocityX = 0.0f;
                fromCell.velocityY = 0.0f;
                fromCell.updated = false;
                fromCell.targetElementType = ElementType::Empty;
                fromCell.phaseTransitionProgress = 0.0f;
                fromCell.phaseTransitionSpeed = 0.0f;
                fromCell.microMassDecayTime = 0.0f;
                fromCell.color = sf::Color(10, 10, 15);
            } else {
                // Swapping with gas or different liquid
                ElementType tempType = fromCell.elementType;
                float tempMass = fromCell.mass;
                float tempTemp = fromCell.temperature;
                float tempVelX = fromCell.velocityX;
                float tempVelY = fromCell.velocityY;
                sf::Color tempColor = fromCell.color;
                
                fromCell.elementType = toCell.elementType;
                fromCell.mass = toCell.mass;
                fromCell.temperature = toCell.temperature;
                fromCell.velocityX = toCell.velocityX;
                fromCell.velocityY = toCell.velocityY;
                fromCell.color = toCell.color;
                fromCell.updated = true;
                
                toCell.elementType = tempType;
                toCell.mass = tempMass;
                toCell.temperature = tempTemp;
                toCell.velocityX = tempVelX;
                toCell.velocityY = tempVelY;
                toCell.color = tempColor;
                toCell.updated = true;
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
    
    // Corner leaking: seep through diagonal gaps (last resort)
    // Only attempt if we still have mass (didn't fully move)
    Cell& cell = grid->getCell(x, y);
    if (isLiquidType(cell.elementType) && cell.mass > 0.01f) {
        leakLiquidDiagonally(x, y, 0.1f);  // Use base update interval
    }
}

void FluidSim::moveLiquidDown(int x, int y) {
    Cell& cell = grid->getCell(x, y);
    
    // Check cell below
    int belowY = y + 1;
    if (!grid->isValidPosition(x, belowY)) return;
    
    Cell& belowCell = grid->getCell(x, belowY);
    
    // Move into vacuum (nothing is ever "empty")
    if (belowCell.elementType == ElementType::Vacuum) {
        // Transfer entire mass to new cell - initialize ALL fields
        belowCell.elementType = cell.elementType;
        belowCell.mass = cell.mass;
        belowCell.temperature = cell.temperature;
        belowCell.pressure = 0.0f;  // Liquids don't have pressure
        belowCell.velocityX = 0.0f;  // Reset velocity
        belowCell.velocityY = 0.0f;
        belowCell.updated = true;
        belowCell.targetElementType = ElementType::Empty;
        belowCell.phaseTransitionProgress = 0.0f;
        belowCell.phaseTransitionSpeed = 0.0f;
        belowCell.microMassDecayTime = 0.0f;
        belowCell.color = cell.color;
        
        // Source becomes vacuum - reset ALL data to vacuum defaults (COMPLETE cleanup)
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
        cell.color = sf::Color(10, 10, 15);
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
            
            // Source becomes vacuum - reset ALL data (COMPLETE cleanup)
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
            cell.color = sf::Color(10, 10, 15);
            return;
        }
        
        // Move sideways into vacuum
        if (neighbor.elementType == ElementType::Vacuum) {
            // Transfer mass - initialize ALL fields
            neighbor.elementType = cell.elementType;
            neighbor.mass = cell.mass;
            neighbor.temperature = cell.temperature;
            neighbor.pressure = 0.0f;
            neighbor.velocityX = 0.0f;
            neighbor.velocityY = 0.0f;
            neighbor.updated = true;
            neighbor.targetElementType = ElementType::Empty;
            neighbor.phaseTransitionProgress = 0.0f;
            neighbor.phaseTransitionSpeed = 0.0f;
            neighbor.microMassDecayTime = 0.0f;
            neighbor.color = cell.color;
            
            // Source becomes vacuum - reset ALL data (COMPLETE cleanup)
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
            cell.color = sf::Color(10, 10, 15);
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

void FluidSim::leakLiquidDiagonally(int x, int y, float deltaTime) {
    Cell& cell = grid->getCell(x, y);
    const Element& props = ElementTypes::getElement(cell.elementType);
    
    // 2 diagonal directions (DOWNWARD corner-leaking only)
    // Liquids can only leak diagonally DOWN, never UP
    int diagonals[2][2] = {
        {-1, 1}, {1, 1}  // Down-left, Down-right
    };
    
    for (auto& dir : diagonals) {
        int nx = x + dir[0];
        int ny = y + dir[1];
        
        if (!grid->isValidPosition(nx, ny)) continue;
        
        Cell& target = grid->getCell(nx, ny);
        
        // Can only leak into vacuum or lighter gases (Darcy's Law inspired)
        if (target.elementType != ElementType::Vacuum && 
            !canDisplace(cell.elementType, target.elementType)) {
            continue;
        }
        
        // Calculate leak probability based on:
        // - Viscosity: lower viscosity = faster leak
        // - Mass ratio: more mass = higher pressure = faster leak
        // - DeltaTime: frame-rate independent
        float leakProbability = (1.0f / (1.0f + props.viscosity * 2.0f)) * 
                                (cell.mass / props.density) * 
                                (deltaTime / 0.1f) * 
                                0.3f;  // Base leak rate is 30% of normal flow
        
        static thread_local std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        if (dist(rng) < leakProbability) {
            // Leak small portion of mass through corner (simulates seepage)
            float leakAmount = cell.mass * 0.15f;  // 15% of mass
            
            target.elementType = cell.elementType;
            target.mass = leakAmount;
            target.temperature = cell.temperature;
            target.pressure = 0.0f;
            target.velocityX = 0.0f;
            target.velocityY = 0.0f;
            target.updated = true;
            target.targetElementType = ElementType::Empty;
            target.phaseTransitionProgress = 0.0f;
            target.phaseTransitionSpeed = 0.0f;
            target.microMassDecayTime = 0.0f;
            target.updateColor();
            
            cell.mass -= leakAmount;
            
            // If source has very little mass left, become vacuum (COMPLETE cleanup)
            if (cell.mass < 0.01f) {
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
                cell.color = sf::Color(10, 10, 15);
            }
            
            break;  // Only leak to one diagonal per tick
        }
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
