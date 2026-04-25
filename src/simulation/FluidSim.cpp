#include "FluidSim.h"
#include "ElementTypes.h"
#include "GasSim.h"  // For isGasType
#include <random>
#include <iostream>

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
        bool overwriteGas = false;  // true = liquid overwriting gas (not a swap)
    };
    
    std::vector<FlowAction> flowActions;
    flowActions.reserve(width * height / 4);
    
    // Track which cells are already involved in flow actions to prevent conflicts
    std::vector<std::vector<bool>> isMergeSource(height, std::vector<bool>(width, false));
    std::vector<std::vector<bool>> isMergeTarget(height, std::vector<bool>(width, false));
    std::vector<std::vector<bool>> isSwapInvolved(height, std::vector<bool>(width, false));
    
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
            
            // VISCOSITY-BASED FLOW RATE (only applies to MERGE operations)
            // Low viscosity (water=1.0) = fast merge, High viscosity (honey=100) = slow merge
            // Flow rate = portion of mass that can MERGE per tick
            float flowRate = 1.0f / (1.0f + props.viscosity * 0.1f);  // 0.0 to 1.0
            float massToMerge = cellMass * flowRate;  // Only for merge operations
            
            // ALL liquid cells flow regardless of mass - no minimum threshold
            
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
            // CRITICAL FIX: Track how much mass we've actually moved
            float massMoved = 0.0f;
            float remainingMergeMass = massToMerge;  // Only limits merge operations
            
            for (int i = 0; i < dirCount && (remainingMergeMass > 0.001f || massMoved == 0.0f); i++) {
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
                    // VACUUM: Move entire cell into vacuum
                    // Check if either cell is already involved
                    if (isSwapInvolved[y][x] || isSwapInvolved[ny][nx] ||
                        isMergeSource[y][x] || isMergeTarget[y][x]) {
                        continue;  // Skip, already reserved
                    }
                    
                    // CRITICAL FIX: Move ALL mass, not partial
                    action.massToMove = cellMass;  // Move entire mass
                    action.isMerge = false;  // Will swap/move
                    flowActions.push_back(action);
                    
                    // Mark as reserved
                    isSwapInvolved[y][x] = true;
                    isSwapInvolved[ny][nx] = true;
                    
                    massMoved = cellMass;
                    break;  // Cell fully moved, stop processing
                }
                else if (isSameLiquid) {
                    // SAME LIQUID: Merge based on viscosity
                    // Check if cells are available for merge
                    if (isMergeSource[y][x] || isMergeTarget[y][x] || 
                        isMergeSource[ny][nx] || isMergeTarget[ny][nx] ||
                        isSwapInvolved[y][x] || isSwapInvolved[ny][nx]) {
                        continue;  // Skip, already reserved
                    }
                    
                    float spaceAvailable = 2.0f - neighbor.mass;  // 2kg max
                    float mergeAmount = std::min(remainingMergeMass, spaceAvailable);
                    
                    if (mergeAmount >= 0.001f) {
                        action.massToMove = mergeAmount;
                        action.isMerge = true;
                        flowActions.push_back(action);
                        
                        // Mark as reserved
                        isMergeSource[y][x] = true;
                        isMergeTarget[ny][nx] = true;
                        
                        remainingMergeMass -= mergeAmount;
                        massMoved += mergeAmount;
                        // Don't break - can split mass to multiple neighbors
                    }
                }
                else if (isDifferentLiquid) {
                    // DIFFERENT LIQUID: Swap based on density
                    // Check if cells are available
                    if (isSwapInvolved[y][x] || isSwapInvolved[ny][nx] ||
                        isMergeSource[y][x] || isMergeTarget[y][x] ||
                        isMergeSource[ny][nx] || isMergeTarget[ny][nx]) {
                        continue;  // Skip, already reserved
                    }
                    
                    const Element& cellProps = ElementTypes::getElement(cell.elementType);
                    const Element& neighborProps = ElementTypes::getElement(neighborType);
                    
                    // Denser liquid sinks through lighter liquid
                    if (cellProps.density > neighborProps.density) {
                        // CRITICAL FIX: Swap entire mass
                        action.massToMove = cellMass;  // Swap all mass
                        action.isMerge = false;  // Swap
                        flowActions.push_back(action);
                        
                        // Mark as reserved
                        isSwapInvolved[y][x] = true;
                        isSwapInvolved[ny][nx] = true;
                        
                        massMoved = cellMass;
                        break;
                    }
                }
                else if (isDisplaceableGas) {
                    // GAS: Check if swap is possible or need displacement queue
                    bool isMovingDown = (directions[i][1] > 0);  // dy > 0 means moving down
                    bool gasAtBottomEdge = (ny >= height - 1);  // Can't swap if gas at bottom
                    bool canSwap = isMovingDown && !gasAtBottomEdge;
                    
                    if (canSwap) {
                        // MOVING DOWN and gas NOT at bottom: SWAP (bubble/droplet effect)
                        // Check if cells are available
                        if (isSwapInvolved[y][x] || isSwapInvolved[ny][nx] ||
                            isMergeSource[y][x] || isMergeTarget[y][x] ||
                            isMergeSource[ny][nx] || isMergeTarget[ny][nx]) {
                            continue;  // Skip, already reserved
                        }
                        
                        // SWAP - gas goes up, liquid goes down
                        action.massToMove = cellMass;
                        action.isMerge = false;
                        action.overwriteGas = false;  // This is a SWAP, not overwrite
                        flowActions.push_back(action);
                        
                        // Mark as reserved
                        isSwapInvolved[y][x] = true;
                        isSwapInvolved[ny][nx] = true;
                        
                        massMoved = cellMass;
                        break;
                    } else {
                        // Either moving sideways OR gas at bottom edge: Use displacement queue
                        static constexpr float MAX_GAS_MASS = 10.0f;
                        
                        if (neighbor.mass > MAX_GAS_MASS) {
                            // Pressurized gas - check if it has escape route
                            if (!hasGasEscapeRoute(nx, ny, neighbor.elementType)) {
                                // Air pocket - liquid cannot displace
                                continue;
                            }
                        }
                        
                        // Check if cells are available
                        if (isSwapInvolved[y][x] || isSwapInvolved[ny][nx] ||
                            isMergeSource[y][x] || isMergeTarget[y][x] ||
                            isMergeSource[ny][nx] || isMergeTarget[ny][nx]) {
                            continue;  // Skip, already reserved
                        }
                        
                        // VISCOSITY-BASED PARTIAL MASS TRANSFER
                        // High viscosity (lava) = moves slowly, transfers small amount
                        // Low viscosity (water) = moves quickly, transfers most mass
                        float flowRate = 1.0f / (1.0f + props.viscosity * 0.1f);
                        float viscosityMassTransfer = cellMass * flowRate;
                        
                        // Only transfer if there's meaningful mass to move
                        if (viscosityMassTransfer < 0.001f) {
                            continue;
                        }
                        
                        // Queue gas for displacement (will be processed at end of tick)
                        DisplacedGas displaced;
                        displaced.x = nx;
                        displaced.y = ny;
                        displaced.mass = neighbor.mass;
                        displaced.temperature = neighbor.temperature;
                        displaced.type = neighbor.elementType;
                        displacedGasQueue.push_back(displaced);
                        
                        // Liquid OVERWRITES gas with viscosity-based mass (not full swap)
                        action.massToMove = viscosityMassTransfer;
                        action.isMerge = false;
                        action.overwriteGas = true;  // Special flag for gas displacement
                        flowActions.push_back(action);
                        
                        // Mark as reserved
                        isSwapInvolved[y][x] = true;
                        isSwapInvolved[ny][nx] = true;
                        
                        massMoved = viscosityMassTransfer;
                        remainingMergeMass -= viscosityMassTransfer;
                        // Don't break - liquid can flow to multiple gas cells based on viscosity
                    }
                }
            }
            
            // CRITICAL FIX: Handle leftover mass
            // If cell moved >99% of mass, convert remainder to vacuum to prevent micro-cells
            float massRemaining = cellMass - massMoved;
            if (massMoved > 0.0f && massRemaining > 0.0f && massRemaining < cellMass * 0.01f) {
                // Cell moved almost all mass (>99%), convert leftover to vacuum
                FlowAction cleanupAction;
                cleanupAction.fromX = x;
                cleanupAction.fromY = y;
                cleanupAction.toX = x;  // Same cell - just mark for cleanup
                cleanupAction.toY = y;
                cleanupAction.massToMove = massRemaining;
                cleanupAction.isMerge = false;
                cleanupAction.type = ElementType::Vacuum;  // Special marker
                cleanupAction.temperature = -273.15f;
                flowActions.push_back(cleanupAction);
            }
        }
    }
    
    // Apply all flow actions simultaneously
    for (const auto& action : flowActions) {
        // CRITICAL FIX: Handle cleanup actions (leftover mass -> vacuum)
        if (action.fromX == action.toX && action.fromY == action.toY && action.type == ElementType::Vacuum) {
            // This is a cleanup action - convert cell to vacuum
            Cell& cell = grid->getCell(action.fromX, action.fromY);
            cell.elementType = ElementType::Vacuum;
            cell.mass = 0.0f;
            cell.temperature = -273.15f;
            cell.velocityX = 0.0f;
            cell.velocityY = 0.0f;
            cell.updated = false;
            cell.targetElementType = ElementType::Empty;
            cell.phaseTransitionProgress = 0.0f;
            cell.phaseTransitionSpeed = 0.0f;
            cell.microMassDecayTime = 0.0f;
            cell.color = sf::Color(10, 10, 15);
            continue;
        }
        
        Cell& fromCell = grid->getCell(action.fromX, action.fromY);
        Cell& toCell = grid->getCell(action.toX, action.toY);
        
        if (action.isMerge) {
            // MERGE: Add mass to target (WITH CAP CHECK to prevent overflow)
            float spaceAvailable = 2.0f - toCell.mass;  // 2kg max per cell
            float actualMergeAmount = std::min(action.massToMove, spaceAvailable);
            
            // If no space, skip this merge (mass stays in source)
            if (actualMergeAmount < 0.001f) {
                continue;
            }
            
            float totalMass = toCell.mass + actualMergeAmount;
            float newTemp = (toCell.mass * toCell.temperature + actualMergeAmount * action.temperature) / totalMass;
            
            toCell.mass = totalMass;
            toCell.temperature = newTemp;
            toCell.updated = true;
            toCell.updateColor();
            
            // Remove ONLY the actual merged amount from source
            fromCell.mass -= actualMergeAmount;
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
            if (action.overwriteGas) {
                // CRITICAL: Liquid displacing gas with viscosity-based partial transfer
                // The gas has been queued for displacement processing.
                // Only transfer the viscosity-calculated mass amount.
                
                // Store liquid data in a temporary marker (use targetElementType)
                toCell.targetElementType = fromCell.elementType;
                toCell.phaseTransitionProgress = action.massToMove;  // Store transferred mass
                toCell.phaseTransitionSpeed = fromCell.temperature;  // Store liquid temp
                toCell.updated = true;
                
                // Remove ONLY the transferred mass from source
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
                    fromCell.updateColor();
                }
            } else if (toCell.elementType == ElementType::Vacuum) {
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
            } else if (action.type == ElementType::Vacuum && action.fromX == action.toX && action.fromY == action.toY) {
                // Cleanup action: Convert cell to vacuum (leftover micro-mass)
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
    
    // PROCESS DISPLACED GAS QUEUE
    // Gas that was overwritten by liquid gets one chance to escape
    
    for (const auto& displaced : displacedGasQueue) {
        if (!grid->isValidPosition(displaced.x, displaced.y)) continue;
        
        Cell& gasCell = grid->getCell(displaced.x, displaced.y);
        
        // Check if cell is still gas (might have been processed already)
        auto isGasType = [](ElementType type) {
            return type == ElementType::Gas_O2 || type == ElementType::Gas_CO2 || type == ElementType::Gas_Lava;
        };
        
        bool gasStillPresent = isGasType(gasCell.elementType);
        
        // Check if this cell has a pending liquid overwrite
        bool hasPendingLiquid = (gasCell.targetElementType != ElementType::Empty && 
                                 isLiquidType(gasCell.targetElementType));
        
        if (!gasStillPresent && !hasPendingLiquid) {
            continue;
        }
        
        // PRIORITY 1: Merge into same-type gas neighbor (can exceed MAX_GAS_MASS)
        bool merged = false;
        int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};  // UP, DOWN, LEFT, RIGHT
        
        for (int i = 0; i < 4 && !merged; i++) {
            int nx = displaced.x + directions[i][0];
            int ny = displaced.y + directions[i][1];
            
            if (!grid->isValidPosition(nx, ny)) continue;
            
            Cell& neighbor = grid->getCell(nx, ny);
            
            // Same gas type - force merge (can exceed max)
            if (neighbor.elementType == displaced.type) {
                // Mass-weighted temperature average
                float totalMass = neighbor.mass + displaced.mass;
                float newTemp = (neighbor.mass * neighbor.temperature + displaced.mass * displaced.temperature) / totalMass;
                
                neighbor.mass += displaced.mass;
                neighbor.temperature = newTemp;
                neighbor.updated = true;
                neighbor.updateColor();
                
                merged = true;
            }
        }
        
        if (merged) {
            // Gas escaped! Now place the pending liquid
            if (hasPendingLiquid) {
                gasCell.elementType = gasCell.targetElementType;
                gasCell.mass = gasCell.phaseTransitionProgress;
                gasCell.temperature = gasCell.phaseTransitionSpeed;
                gasCell.pressure = 0.0f;
                gasCell.velocityX = 0.0f;
                gasCell.velocityY = 0.0f;
                gasCell.updated = true;
                gasCell.targetElementType = ElementType::Empty;
                gasCell.phaseTransitionProgress = 0.0f;
                gasCell.phaseTransitionSpeed = 0.0f;
                gasCell.microMassDecayTime = 0.0f;
                gasCell.updateColor();
            } else {
                // No liquid waiting, just clear gas
                gasCell.elementType = ElementType::Vacuum;
                gasCell.mass = 0.0f;
                gasCell.temperature = -273.15f;
                gasCell.updated = true;
            }
            continue;
        }
        
        // PRIORITY 2: Move into vacuum neighbor
        bool movedToVacuum = false;
        for (int i = 0; i < 4 && !movedToVacuum; i++) {
            int nx = displaced.x + directions[i][0];
            int ny = displaced.y + directions[i][1];
            
            if (!grid->isValidPosition(nx, ny)) continue;
            
            Cell& neighbor = grid->getCell(nx, ny);
            
            if (neighbor.elementType == ElementType::Vacuum) {
                neighbor.elementType = displaced.type;
                neighbor.mass = displaced.mass;
                neighbor.temperature = displaced.temperature;
                neighbor.updated = true;
                neighbor.updateColor();
                
                movedToVacuum = true;
            }
        }
        
        if (movedToVacuum) {
            // Gas escaped! Now place the pending liquid
            if (hasPendingLiquid) {
                gasCell.elementType = gasCell.targetElementType;
                gasCell.mass = gasCell.phaseTransitionProgress;
                gasCell.temperature = gasCell.phaseTransitionSpeed;
                gasCell.pressure = 0.0f;
                gasCell.velocityX = 0.0f;
                gasCell.velocityY = 0.0f;
                gasCell.updated = true;
                gasCell.targetElementType = ElementType::Empty;
                gasCell.phaseTransitionProgress = 0.0f;
                gasCell.phaseTransitionSpeed = 0.0f;
                gasCell.microMassDecayTime = 0.0f;
                gasCell.updateColor();
            } else {
                // No liquid waiting, just clear gas
                gasCell.elementType = ElementType::Vacuum;
                gasCell.mass = 0.0f;
                gasCell.temperature = -273.15f;
                gasCell.updated = true;
            }
            continue;
        }
        
        // PRIORITY 3: Gas is deleted (no escape route)
        // Place the liquid anyway
        if (hasPendingLiquid) {
            gasCell.elementType = gasCell.targetElementType;
            gasCell.mass = gasCell.phaseTransitionProgress;
            gasCell.temperature = gasCell.phaseTransitionSpeed;
            gasCell.pressure = 0.0f;
            gasCell.velocityX = 0.0f;
            gasCell.velocityY = 0.0f;
            gasCell.updated = true;
            gasCell.targetElementType = ElementType::Empty;
            gasCell.phaseTransitionProgress = 0.0f;
            gasCell.phaseTransitionSpeed = 0.0f;
            gasCell.microMassDecayTime = 0.0f;
            gasCell.updateColor();
        } else {
            gasCell.elementType = ElementType::Vacuum;
            gasCell.mass = 0.0f;
            gasCell.temperature = -273.15f;
            gasCell.updated = true;
        }
    }
    
    // Clear the queue for next tick
    displacedGasQueue.clear();
    
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
        (target == ElementType::Gas_O2 || target == ElementType::Gas_CO2 || target == ElementType::Gas_Lava)) {
        return true;
    }
    return false;
}

bool FluidSim::hasGasEscapeRoute(int x, int y, ElementType gasType) {
    // Check if gas has any neighbor it can escape to
    int directions[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};  // UP, DOWN, LEFT, RIGHT
    
    for (int i = 0; i < 4; i++) {
        int nx = x + directions[i][0];
        int ny = y + directions[i][1];
        
        if (!grid->isValidPosition(nx, ny)) continue;
        
        Cell& neighbor = grid->getCell(nx, ny);
        
        // Same gas type (can merge even if over max)
        if (neighbor.elementType == gasType) {
            return true;
        }
        
        // Vacuum (can move into)
        if (neighbor.elementType == ElementType::Vacuum) {
            return true;
        }
    }
    
    return false;  // No escape route - air pocket
}
