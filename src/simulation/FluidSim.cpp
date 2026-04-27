#include "FluidSim.h"
#include "ElementTypes.h"
#include "GasSim.h"  // For isGasType
#include <random>
#include <iostream>

FluidSim::FluidSim() 
    : SimulationSystem("FluidSim", 0.1f) {  // Update every 100ms (slower, more realistic)
    // Initialize liquid types dynamically
    initializeLiquidTypes();
}

void FluidSim::initializeLiquidTypes() {
    liquidTypes.clear();
    
    // Dynamically iterate through ALL ElementType enum values
    // This automatically includes any new elements added to the enum
    int enumCount = static_cast<int>(ElementType::Gas_Nitrogen) + 1;  // Last element + 1
    for (int i = 0; i < enumCount; i++) {
        ElementType type = static_cast<ElementType>(i);
        const Element& props = ElementTypes::getElement(type);
        if (props.isLiquid) {
            liquidTypes.insert(type);
            std::cout << "[FluidSim] Registered liquid type: " << props.name << std::endl;
        }
    }
    
    std::cout << "[FluidSim] Total liquid types: " << liquidTypes.size() << std::endl;
}

// Check if a liquid cell is trapped between denser liquids on LEFT or RIGHT side
// A liquid is NOT trapped if it has vacuum or gas adjacent (sides or down) - it has an escape route
// Returns true if the cell CANNOT receive mass from above (buoyancy restriction)
bool FluidSim::isTrappedBetweenDenserLiquids(int x, int y) {
    if (!grid->isValidPosition(x, y)) return false;
    
    Cell& cell = grid->getCell(x, y);
    if (!isLiquidType(cell.elementType)) return false;
    
    const Element& cellProps = ElementTypes::getElement(cell.elementType);
    
    // FIRST: Check if there's an escape route (vacuum or gas) on sides, down, and diagonal-up
    // If yes, the cell is NOT trapped even if denser liquid is adjacent
    int escapeDirs[7][2] = {
        {-1, 0},   // LEFT
        {1, 0},    // RIGHT
        {0, 1},    // DOWN
        {-1, -1},  // DIAGONAL-UP-LEFT
        {1, -1}    // DIAGONAL-UP-RIGHT
    };
    for (auto& dir : escapeDirs) {
        int ex = x + dir[0];
        int ey = y + dir[1];
        
        if (!grid->isValidPosition(ex, ey)) continue;
        
        Cell& escapeCell = grid->getCell(ex, ey);
        const Element& escapeProps = ElementTypes::getElement(escapeCell.elementType);
        
        // Vacuum or gas = escape route exists
        if (escapeCell.elementType == ElementType::Vacuum || escapeProps.isGas) {
            return false;  // NOT trapped - has escape route
        }
    }
    
    // NO escape route found - check if trapped between denser liquids on LEFT or RIGHT
    bool leftDenser = false;
    bool rightDenser = false;
    
    // Check LEFT neighbor
    int leftX = x - 1;
    if (grid->isValidPosition(leftX, y)) {
        Cell& leftCell = grid->getCell(leftX, y);
        if (isLiquidType(leftCell.elementType)) {
            const Element& leftProps = ElementTypes::getElement(leftCell.elementType);
            if (leftProps.density > cellProps.density) {
                leftDenser = true;
            }
        }
    }
    
    // Check RIGHT neighbor
    int rightX = x + 1;
    if (grid->isValidPosition(rightX, y)) {
        Cell& rightCell = grid->getCell(rightX, y);
        if (isLiquidType(rightCell.elementType)) {
            const Element& rightProps = ElementTypes::getElement(rightCell.elementType);
            if (rightProps.density > cellProps.density) {
                rightDenser = true;
            }
        }
    }
    
    // Trapped if denser liquid on LEFT OR RIGHT (and no escape route)
    return leftDenser || rightDenser;
}

// Displace function: tries to move target cell out of the way when liquid wants to enter it
// PRIORITY 1: Push into VACUUM
// PRIORITY 2: Push into SAME-TYPE (allow over-mass)
// PRIORITY 3: Swap as fallback
// Returns true if displacement succeeded, false if no escape route
bool FluidSim::Displace(int fromX, int fromY, int targetX, int targetY,
                        std::vector<std::vector<bool>>& isMergeSource,
                        std::vector<std::vector<bool>>& isMergeTarget,
                        std::vector<std::vector<bool>>& isSwapInvolved,
                        std::vector<DisplacedGas>& displacedGasQueue,
                        std::vector<FlowAction>& flowActions) {
    
    Cell& fromCell = grid->getCell(fromX, fromY);
    Cell& targetCell = grid->getCell(targetX, targetY);
    ElementType fromType = fromCell.elementType;
    ElementType targetType = targetCell.elementType;
    
    // Check reservations
    if (isSwapInvolved[fromY][fromX] || isSwapInvolved[targetY][targetX] ||
        isMergeSource[fromY][fromX] || isMergeTarget[fromY][fromX]) {
        return false;
    }
    
    bool displaced = false;
    int escapeDirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    // PRIORITY 1: Push target into VACUUM
    for (auto& dir : escapeDirs) {
        int ex = targetX + dir[0];
        int ey = targetY + dir[1];
        
        if (ex == fromX && ey == fromY) continue;  // Skip the source cell
        if (!grid->isValidPosition(ex, ey)) continue;
        
        Cell& escapeCell = grid->getCell(ex, ey);
        if (escapeCell.elementType == ElementType::Vacuum) {
            // Create flow actions to move target to vacuum, then source to target
            // Step 1: Move target mass to vacuum
            FlowAction moveTargetToVacuum;
            moveTargetToVacuum.fromX = targetX; moveTargetToVacuum.fromY = targetY;
            moveTargetToVacuum.toX = ex; moveTargetToVacuum.toY = ey;
            moveTargetToVacuum.massToMove = targetCell.mass;
            moveTargetToVacuum.temperature = targetCell.temperature;
            moveTargetToVacuum.type = targetType;
            moveTargetToVacuum.isMerge = true;  // Merge into vacuum (will convert it)
            flowActions.push_back(moveTargetToVacuum);
            
            // Step 2: Move source mass to target position
            FlowAction moveSourceToTarget;
            moveSourceToTarget.fromX = fromX; moveSourceToTarget.fromY = fromY;
            moveSourceToTarget.toX = targetX; moveSourceToTarget.toY = targetY;
            moveSourceToTarget.massToMove = fromCell.mass;
            moveSourceToTarget.temperature = fromCell.temperature;
            moveSourceToTarget.type = fromType;
            moveSourceToTarget.isMerge = true;
            flowActions.push_back(moveSourceToTarget);
            
            isMergeSource[targetY][targetX] = true;
            isMergeTarget[ey][ex] = true;
            isMergeSource[fromY][fromX] = true;
            isMergeTarget[targetY][targetX] = true;
            displaced = true;
            break;
        }
    }
    
    // PRIORITY 2: Push target into SAME-TYPE (allow over-mass)
    if (!displaced) {
        for (auto& dir : escapeDirs) {
            int ex = targetX + dir[0];
            int ey = targetY + dir[1];
            
            if (ex == fromX && ey == fromY) continue;  // Skip the source cell
            if (!grid->isValidPosition(ex, ey)) continue;
            
            Cell& escapeCell = grid->getCell(ex, ey);
            if (escapeCell.elementType == targetType) {
                // Create flow actions to merge target into same-type, then move source to target
                // Step 1: Merge target into same-type neighbor (allow over-mass)
                FlowAction mergeTargetToSame;
                mergeTargetToSame.fromX = targetX; mergeTargetToSame.fromY = targetY;
                mergeTargetToSame.toX = ex; mergeTargetToSame.toY = ey;
                mergeTargetToSame.massToMove = targetCell.mass;
                mergeTargetToSame.temperature = targetCell.temperature;
                mergeTargetToSame.type = targetType;
                mergeTargetToSame.isMerge = true;
                flowActions.push_back(mergeTargetToSame);
                
                // Step 2: Move source mass to target position
                FlowAction moveSourceToTarget;
                moveSourceToTarget.fromX = fromX; moveSourceToTarget.fromY = fromY;
                moveSourceToTarget.toX = targetX; moveSourceToTarget.toY = targetY;
                moveSourceToTarget.massToMove = fromCell.mass;
                moveSourceToTarget.temperature = fromCell.temperature;
                moveSourceToTarget.type = fromType;
                moveSourceToTarget.isMerge = true;
                flowActions.push_back(moveSourceToTarget);
                
                isMergeSource[targetY][targetX] = true;
                isMergeTarget[ey][ex] = true;
                isMergeSource[fromY][fromX] = true;
                isMergeTarget[targetY][targetX] = true;
                displaced = true;
                break;
            }
        }
    }
    
    // PRIORITY 3: Swap if no escape route (only for gas)
    if (!displaced && targetType != ElementType::Vacuum) {
        const Element& targetProps = ElementTypes::getElement(targetType);
        
        // For gas: check if we can swap
        if (targetProps.isGas) {
            float maxGasMass = GasSim::getMaxMassForElement(targetType);
            
            if (targetCell.mass > maxGasMass) {
                if (!hasGasEscapeRoute(targetX, targetY, targetType)) {
                    return false;  // Air pocket, can't displace
                }
            }
            
            // Queue gas for later displacement processing
            DisplacedGas displacedGas;
            displacedGas.x = targetX;
            displacedGas.y = targetY;
            displacedGas.mass = targetCell.mass;
            displacedGas.temperature = targetCell.temperature;
            displacedGas.type = targetType;
            displacedGasQueue.push_back(displacedGas);
            
            // Create flow action for liquid overwriting gas
            FlowAction action;
            action.fromX = fromX; action.fromY = fromY;
            action.toX = targetX; action.toY = targetY;
            action.massToMove = fromCell.mass;
            action.temperature = fromCell.temperature;
            action.type = fromType;
            action.isMerge = false;
            action.overwriteGas = true;
            flowActions.push_back(action);
            
            isSwapInvolved[fromY][fromX] = true;
            isSwapInvolved[targetY][targetX] = true;
            displaced = true;
        }
        // For liquid: just swap
        else if (targetProps.isLiquid) {
            FlowAction action;
            action.fromX = fromX; action.fromY = fromY;
            action.toX = targetX; action.toY = targetY;
            action.massToMove = fromCell.mass;
            action.temperature = fromCell.temperature;
            action.type = fromType;
            action.isMerge = false;
            flowActions.push_back(action);
            
            isSwapInvolved[fromY][fromX] = true;
            isSwapInvolved[targetY][targetX] = true;
            displaced = true;
        }
    }
    
    return displaced;
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
            
            bool actedThisTick = false;  // Track if cell acted this tick
            
            // PRIORITY 0: OVER-MASS OOZING (immediate, explosive pressure release)
            float maxMass = GasSim::getMaxMassForElement(cell.elementType);
            
            if (cellMass > maxMass) {
                float excessMass = cellMass - maxMass;
                
                // Check neighbors in order: DOWN, LEFT, RIGHT, UP
                int neighborDirs[4][2] = {{0, 1}, {-1, 0}, {1, 0}, {0, -1}};
                
                for (auto& dir : neighborDirs) {
                    int nx = x + dir[0];
                    int ny = y + dir[1];
                    
                    if (!grid->isValidPosition(nx, ny)) continue;
                    if (excessMass <= 0.0f) break;  // No more excess to transfer
                    
                    Cell& neighbor = grid->getCell(nx, ny);
                    ElementType neighborType = neighbor.elementType;
                    
                    // Skip solids
                    const Element& neighborProps = ElementTypes::getElement(neighborType);
                    if (!neighborProps.isGas && !neighborProps.isLiquid && neighborType != ElementType::Vacuum) {
                        continue;
                    }
                    
                    bool acted = false;
                    
                    // CASE 1: Vacuum - direct mass transfer
                    if (neighborType == ElementType::Vacuum) {
                        if (!isMergeSource[y][x] && !isMergeTarget[y][x] &&
                            !isMergeSource[ny][nx] && !isMergeTarget[ny][nx]) {
                            
                            // Convert vacuum to liquid with excess mass
                            neighbor.elementType = cell.elementType;
                            neighbor.temperature = cell.temperature;
                            
                            FlowAction action;
                            action.fromX = x; action.fromY = y;
                            action.toX = nx; action.toY = ny;
                            action.massToMove = excessMass;
                            action.temperature = cell.temperature;
                            action.type = cell.elementType;
                            action.isMerge = true;
                            flowActions.push_back(action);
                            
                            isMergeSource[y][x] = true;
                            isMergeTarget[ny][nx] = true;
                            acted = true;
                        }
                    }
                    // CASE 2: Gas - displace gas, transfer liquid
                    else if (neighborProps.isGas) {
                        if (!isSwapInvolved[y][x] && !isSwapInvolved[ny][nx] &&
                            !isMergeSource[y][x] && !isMergeTarget[y][x]) {
                            
                            // Queue gas for displacement
                            DisplacedGas displacedGas;
                            displacedGas.x = nx;
                            displacedGas.y = ny;
                            displacedGas.mass = neighbor.mass;
                            displacedGas.temperature = neighbor.temperature;
                            displacedGas.type = neighborType;
                            displacedGasQueue.push_back(displacedGas);
                            
                            // Create flow action for liquid overwriting gas
                            FlowAction action;
                            action.fromX = x; action.fromY = y;
                            action.toX = nx; action.toY = ny;
                            action.massToMove = excessMass;
                            action.temperature = cell.temperature;
                            action.type = cell.elementType;
                            action.isMerge = false;
                            action.overwriteGas = true;
                            flowActions.push_back(action);
                            
                            isSwapInvolved[y][x] = true;
                            isSwapInvolved[ny][nx] = true;
                            acted = true;
                        }
                    }
                    // CASE 3: Same liquid type - equalize (transfer until both equal or target full)
                    else if (neighborType == cell.elementType && isLiquidType(neighborType)) {
                        if (!isMergeSource[y][x] && !isMergeTarget[y][x] &&
                            !isMergeSource[ny][nx] && !isMergeTarget[ny][nx]) {
                            
                            float targetMaxMass = GasSim::getMaxMassForElement(neighborType);
                            float spaceInTarget = targetMaxMass - neighbor.mass;
                            
                            // Transfer min(excessMass, spaceInTarget)
                            float massToTransfer = std::min(excessMass, spaceInTarget);
                            
                            if (massToTransfer > 0.0f) {
                                FlowAction action;
                                action.fromX = x; action.fromY = y;
                                action.toX = nx; action.toY = ny;
                                action.massToMove = massToTransfer;
                                action.temperature = cell.temperature;
                                action.type = cell.elementType;
                                action.isMerge = true;
                                flowActions.push_back(action);
                                
                                isMergeSource[y][x] = true;
                                isMergeTarget[ny][nx] = true;
                                acted = true;
                            }
                        }
                    }
                    // CASE 4: Lighter liquid - displace using Displace() function
                    else if (isLiquidType(neighborType)) {
                        const Element& cellProps = ElementTypes::getElement(cell.elementType);
                        const Element& neighborProps2 = ElementTypes::getElement(neighborType);
                        
                        if (cellProps.density > neighborProps2.density) {
                            if (Displace(x, y, nx, ny, isMergeSource, isMergeTarget, isSwapInvolved, displacedGasQueue, flowActions)) {
                                acted = true;
                            }
                        }
                    }
                    // CASE 5: Denser liquid - skip
                    
                    if (acted) {
                        actedThisTick = true;
                        break;  // Stop at first valid neighbor
                    }
                }
            }
            
            // VISCOSITY-BASED LEAK PROBABILITY
            // High viscosity = low chance to leak, Low viscosity = high chance
            // Water (1.0) ~90% chance, Honey (50) ~20% chance, Lava (100) ~10% chance
            float leakProbability = 1.0f / (1.0f + props.viscosity * 0.1f);
            bool shouldLeak = (simpleRand() % 1000 < leakProbability * 1000);  // Random check
            
            // ORDERED PRIORITY FLOW (prevents mass duplication):
            // 1. MERGE DOWN into same liquid (fill from bottom)
            // 2. Fall into vacuum/gas below
            // 3. Spread sideways into vacuum/gas or equalize with same-liquid neighbors
            
            // PRIORITY 1: MERGE DOWN into same liquid (fill cell below to max)
            int downX = x;
            int downY = y + 1;
            
            if (grid->isValidPosition(downX, downY) && !actedThisTick) {
                Cell& below = grid->getCell(downX, downY);
                ElementType belowType = below.elementType;
                
                FlowAction action;
                action.fromX = x; action.fromY = y;
                action.toX = downX; action.toY = downY;
                action.massToMove = 0.0f;
                action.temperature = cell.temperature;
                action.type = cell.elementType;
                action.isMerge = false;
                
                // DOWN into SAME LIQUID - merge to fill up
                if (isLiquidType(belowType) && belowType == cell.elementType) {
                    // RESTRICTION 1: If the BELOW cell is trapped between denser liquids on either side,
                    // it CANNOT receive mass from above (only from below or sides)
                    if (isTrappedBetweenDenserLiquids(downX, downY)) {
                        // Skip - trapped cell cannot receive from above
                    }
                    // RESTRICTION 2: If the CURRENT cell is trapped between denser liquids,
                    // it should NOT merge downward - it needs to ooze upward instead (PRIORITY 1.5)
                    else if (isTrappedBetweenDenserLiquids(x, y)) {
                        // Skip - trapped cell should ooze upward, not merge down
                    }
                    else if (!isMergeSource[y][x] && !isMergeTarget[y][x] &&
                        !isMergeSource[downY][downX] && !isMergeTarget[downY][downX]) {
                        float maxMass = GasSim::getMaxMassForElement(belowType);
                        float spaceAvailable = maxMass - below.mass;
                        float mergeAmount = std::min(cellMass, spaceAvailable);
                        
                        if (mergeAmount >= 0.001f) {
                            action.massToMove = mergeAmount;
                            action.isMerge = true;
                            flowActions.push_back(action);
                            
                            isMergeSource[y][x] = true;
                            isMergeTarget[downY][downX] = true;
                            actedThisTick = true;
                        }
                    }
                }
                // DOWN into VACUUM - leak 10% of mass (natural falling)
                // Viscosity determines probability of leaking this tick
                else if (belowType == ElementType::Vacuum && shouldLeak) {
                    if (!isMergeSource[y][x] && !isMergeTarget[y][x] &&
                        !isMergeSource[downY][downX] && !isMergeTarget[downY][downX]) {
                        // Convert vacuum to liquid type with source temperature
                        below.elementType = cell.elementType;
                        below.temperature = cell.temperature;  // Prevent super-cooling
                        
                        action.massToMove = cellMass * 0.1f;
                        action.isMerge = true;
                        flowActions.push_back(action);
                        
                        isMergeSource[y][x] = true;
                        isMergeTarget[downY][downX] = true;
                        actedThisTick = true;
                    }
                }
                // DOWN into GAS - swap
                else if (canDisplace(cell.elementType, belowType)) {
                    if (!isSwapInvolved[y][x] && !isSwapInvolved[downY][downX] &&
                        !isMergeSource[y][x] && !isMergeTarget[y][x]) {
                        action.massToMove = cellMass;
                        action.isMerge = false;
                        action.overwriteGas = false;
                        flowActions.push_back(action);
                        
                        isSwapInvolved[y][x] = true;
                        isSwapInvolved[downY][downX] = true;
                        actedThisTick = true;
                    }
                }
                // DOWN into DIFFERENT LIQUID - swap if denser
                else if (isLiquidType(belowType) && belowType != cell.elementType) {
                    const Element& cellProps = ElementTypes::getElement(cell.elementType);
                    const Element& belowProps = ElementTypes::getElement(belowType);
                    
                    if (cellProps.density > belowProps.density) {
                        if (!isSwapInvolved[y][x] && !isSwapInvolved[downY][downX] &&
                            !isMergeSource[y][x] && !isMergeTarget[y][x]) {
                            action.massToMove = cellMass;
                            action.isMerge = false;
                            flowActions.push_back(action);
                            
                            isSwapInvolved[y][x] = true;
                            isSwapInvolved[downY][downX] = true;
                            actedThisTick = true;
                        }
                    }
                }
            }
            
            // PRIORITY 2: SIDES into VACUUM/GAS (only if blocked from falling OR viscosity prevented fall)
            static constexpr float MIN_SIDEWAY_MASS = 0.1f;  // Minimum mass to spread sideways into empty space
            static constexpr float CONSOLIDATE_THRESHOLD = 0.05f;  // Below this, consolidate (hysteresis gap: 0.05-0.1 prevents oscillation)
            
            // Check if downward was possible (vacuum/gas below) but blocked by viscosity
            bool downwardPossible = false;
            int checkDownX = x;
            int checkDownY = y + 1;
            if (grid->isValidPosition(checkDownX, checkDownY)) {
                Cell& checkBelow = grid->getCell(checkDownX, checkDownY);
                ElementType checkBelowType = checkBelow.elementType;
                const Element& checkBelowProps = ElementTypes::getElement(checkBelowType);
                downwardPossible = (checkBelowType == ElementType::Vacuum || 
                                   checkBelowProps.isGas ||
                                   (isLiquidType(checkBelowType) && checkBelowType == cell.elementType));
            }
            
            // PRIORITY 1.5: UPWARD OOZING - If lighter liquid is trapped between denser liquids on left OR right
            // It should ooze upward into gas/vacuum above (like viscosity-based leaking, but upward)
            if (!actedThisTick && isLiquidType(cell.elementType)) {
                int leftX = x - 1;
                int rightX = x + 1;
                int upY = y - 1;
                
                // Check if left or right neighbors are liquids DENSER than this cell
                bool leftDenser = false;
                bool rightDenser = false;
                
                if (grid->isValidPosition(leftX, y)) {
                    Cell& leftCell = grid->getCell(leftX, y);
                    if (isLiquidType(leftCell.elementType)) {
                        const Element& leftProps = ElementTypes::getElement(leftCell.elementType);
                        const Element& cellProps = ElementTypes::getElement(cell.elementType);
                        if (leftProps.density > cellProps.density) {
                            leftDenser = true;
                        }
                    }
                }
                
                if (grid->isValidPosition(rightX, y)) {
                    Cell& rightCell = grid->getCell(rightX, y);
                    if (isLiquidType(rightCell.elementType)) {
                        const Element& rightProps = ElementTypes::getElement(rightCell.elementType);
                        const Element& cellProps = ElementTypes::getElement(cell.elementType);
                        if (rightProps.density > cellProps.density) {
                            rightDenser = true;
                        }
                    }
                }
                
                // If liquid has denser liquid on EITHER side, try escape movements in priority order
                if (leftDenser || rightDenser) {
                    // PRIORITY 1: Merge sideways into SAME-TYPE liquid (can over-pressurize)
                    if (shouldLeak) {
                        int sideDirs[2] = {-1, 1};  // LEFT, RIGHT
                        for (int side = 0; side < 2 && !actedThisTick; side++) {
                            int sideX = x + sideDirs[side];
                            int sideY = y;
                            
                            if (!grid->isValidPosition(sideX, sideY)) continue;
                            
                            Cell& sideCell = grid->getCell(sideX, sideY);
                            if (sideCell.elementType == cell.elementType && isLiquidType(sideCell.elementType)) {
                                // Merge into same-type sideways (allow over-mass)
                                if (!isMergeSource[y][x] && !isMergeTarget[y][x] &&
                                    !isMergeSource[sideY][sideX] && !isMergeTarget[sideY][sideX]) {
                                    
                                    FlowAction action;
                                    action.fromX = x; action.fromY = y;
                                    action.toX = sideX; action.toY = sideY;
                                    action.massToMove = cellMass * 0.5f;
                                    action.temperature = cell.temperature;
                                    action.type = cell.elementType;
                                    action.isMerge = true;
                                    flowActions.push_back(action);
                                    
                                    isMergeSource[y][x] = true;
                                    isMergeTarget[sideY][sideX] = true;
                                    actedThisTick = true;
                                }
                            }
                        }
                    }
                    
                    // PRIORITY 2: Merge upward into SAME-TYPE liquid
                    if (!actedThisTick && shouldLeak && grid->isValidPosition(x, upY)) {
                        Cell& above = grid->getCell(x, upY);
                        if (above.elementType == cell.elementType && isLiquidType(above.elementType)) {
                            if (!isMergeSource[y][x] && !isMergeTarget[y][x] &&
                                !isMergeSource[upY][x] && !isMergeTarget[upY][x]) {
                                
                                float maxLiquidMass = GasSim::getMaxMassForElement(above.elementType);
                                float availableSpace = maxLiquidMass - above.mass;
                                float massToMove = std::min(cellMass * 0.5f, availableSpace);
                                
                                if (massToMove > 0.0f) {
                                    FlowAction action;
                                    action.fromX = x; action.fromY = y;
                                    action.toX = x; action.toY = upY;
                                    action.massToMove = massToMove;
                                    action.temperature = cell.temperature;
                                    action.type = cell.elementType;
                                    action.isMerge = true;
                                    flowActions.push_back(action);
                                    
                                    isMergeSource[y][x] = true;
                                    isMergeTarget[upY][x] = true;
                                    actedThisTick = true;
                                }
                            }
                        }
                    }
                    
                    // PRIORITY 3: Diagonal upward merge into SAME-TYPE
                    if (!actedThisTick && shouldLeak) {
                        int diagDirs[2][2] = {{-1, -1}, {1, -1}};  // Up-Left, Up-Right
                        
                        for (auto& dir : diagDirs) {
                            int diagX = x + dir[0];
                            int diagY = y + dir[1];
                            
                            if (!grid->isValidPosition(diagX, diagY)) continue;
                            
                            Cell& diagCell = grid->getCell(diagX, diagY);
                            if (diagCell.elementType == cell.elementType && isLiquidType(diagCell.elementType)) {
                                if (!isMergeSource[y][x] && !isMergeTarget[y][x] &&
                                    !isMergeSource[diagY][diagX] && !isMergeTarget[diagY][diagX]) {
                                    
                                    float maxLiquidMass = GasSim::getMaxMassForElement(diagCell.elementType);
                                    float availableSpace = maxLiquidMass - diagCell.mass;
                                    float massToMove = std::min(cellMass * 0.3f, availableSpace);
                                    
                                    if (massToMove > 0.0f) {
                                        FlowAction action;
                                        action.fromX = x; action.fromY = y;
                                        action.toX = diagX; action.toY = diagY;
                                        action.massToMove = massToMove;
                                        action.temperature = cell.temperature;
                                        action.type = cell.elementType;
                                        action.isMerge = true;
                                        flowActions.push_back(action);
                                        
                                        isMergeSource[y][x] = true;
                                        isMergeTarget[diagY][diagX] = true;
                                        actedThisTick = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    // PRIORITY 4: Diagonal upward swap with DENSER liquid only (last resort, immediate - no viscosity check)
                    if (!actedThisTick) {
                        int diagDirs[2][2] = {{-1, -1}, {1, -1}};  // Up-Left, Up-Right
                        
                        for (auto& dir : diagDirs) {
                            int diagX = x + dir[0];
                            int diagY = y + dir[1];
                            
                            if (!grid->isValidPosition(diagX, diagY)) continue;
                            
                            Cell& diagCell = grid->getCell(diagX, diagY);
                            
                            // Only swap with DENSER liquids
                            if (isLiquidType(diagCell.elementType)) {
                                const Element& diagProps = ElementTypes::getElement(diagCell.elementType);
                                const Element& cellProps = ElementTypes::getElement(cell.elementType);
                                
                                if (diagProps.density > cellProps.density) {
                                    if (!isSwapInvolved[y][x] && !isSwapInvolved[diagY][diagX] &&
                                        !isMergeSource[y][x] && !isMergeTarget[y][x]) {
                                        
                                        FlowAction action;
                                        action.fromX = x; action.fromY = y;
                                        action.toX = diagX; action.toY = diagY;
                                        action.massToMove = cellMass;
                                        action.temperature = cell.temperature;
                                        action.type = cell.elementType;
                                        action.isMerge = false;
                                        flowActions.push_back(action);
                                        
                                        isSwapInvolved[y][x] = true;
                                        isSwapInvolved[diagY][diagX] = true;
                                        actedThisTick = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Only spread sideways if: didn't act this tick AND (blocked downward by solid)
            // Don't spread if downward is possible but blocked by viscosity (honey should fall, not spread)
            bool canSpreadSideways = !actedThisTick && cellMass >= MIN_SIDEWAY_MASS && !downwardPossible;
            
            // DEBUG: Log why water isn't spreading
            if (cell.elementType == ElementType::Liquid_Water && cellMass > 1.0f) {
                static int debugCounter = 0;
                if (debugCounter++ % 60 == 0) {  // Log every 60 ticks
                    // DEBUG: Uncomment to log water simulation details
                    // printf("Water at (%d,%d): acted=%d, mass=%.1f, downwardPossible=%d, canSpread=%d\n",
                    //        x, y, actedThisTick, cellMass, downwardPossible, canSpreadSideways);
                }
            }
            
            if (canSpreadSideways) {
                int sideDirs[2] = {-1, 1};  // LEFT, RIGHT
                for (int side = 0; side < 2 && !actedThisTick; side++) {
                    int sideX = x + sideDirs[side];
                    int sideY = y;
                    
                    if (!grid->isValidPosition(sideX, sideY)) continue;
                    
                    Cell& sideCell = grid->getCell(sideX, sideY);
                    ElementType sideType = sideCell.elementType;
                    
                    FlowAction action;
                    action.fromX = x; action.fromY = y;
                    action.toX = sideX; action.toY = sideY;
                    action.massToMove = 0.0f;
                    action.temperature = cell.temperature;
                    action.type = cell.elementType;
                    action.isMerge = false;
                    
                    // SIDE into VACUUM - leak 10% of mass (natural spreading)
                    // Viscosity determines probability of leaking this tick
                    if (sideType == ElementType::Vacuum && shouldLeak) {
                        if (!isMergeSource[y][x] && !isMergeTarget[y][x] &&
                            !isMergeSource[sideY][sideX] && !isMergeTarget[sideY][sideX]) {
                            // Convert vacuum to liquid type with source temperature
                            sideCell.elementType = cell.elementType;
                            sideCell.temperature = cell.temperature;  // Prevent super-cooling
                            
                            action.massToMove = cellMass * 0.1f;
                            action.isMerge = true;
                            flowActions.push_back(action);
                            
                            isMergeSource[y][x] = true;
                            isMergeTarget[sideY][sideX] = true;
                            actedThisTick = true;
                        }
                    }
                    // SIDE into GAS - use Displace function
                    else if (canDisplace(cell.elementType, sideType)) {
                        if (Displace(x, y, sideX, sideY, isMergeSource, isMergeTarget, isSwapInvolved, displacedGasQueue, flowActions)) {
                            actedThisTick = true;
                        }
                    }
                    // SIDE into LIGHTER LIQUID - use Displace function
                    else if (isLiquidType(sideType) && sideType != cell.elementType) {
                        const Element& cellProps = ElementTypes::getElement(cell.elementType);
                        const Element& sideProps = ElementTypes::getElement(sideType);
                        
                        // DEBUG
                        if (cell.elementType == ElementType::Liquid_Water && sideType == ElementType::Liquid_Oil) {
                            static int dispDebug = 0;
                            if (dispDebug++ % 30 == 0) {
                                printf("Water->Oil displacement attempt: waterDensity=%.0f, oilDensity=%.0f, canDisplace=%d\n",
                                       cellProps.density, sideProps.density, cellProps.density > sideProps.density);
                            }
                        }
                        
                        // Only displace if current liquid is DENSER than side liquid
                        if (cellProps.density > sideProps.density) {
                            bool result = Displace(x, y, sideX, sideY, isMergeSource, isMergeTarget, isSwapInvolved, displacedGasQueue, flowActions);
                            
                            // DEBUG
                            if (cell.elementType == ElementType::Liquid_Water) {
                                static int dispResultDebug = 0;
                                if (dispResultDebug++ % 30 == 0) {
                                    printf("Displace() returned: %d\n", result);
                                }
                            }
                            
                            if (result) {
                                actedThisTick = true;
                            }
                        }
                    }
                }
            }
            
            // PRIORITY 3: EQUALIZE with same-liquid neighbors (only if blocked from falling/spreading)
            // Small droplets CAN merge into same liquid regardless of mass
            if (!actedThisTick) {
                int sideDirs[2] = {-1, 1};  // LEFT, RIGHT
                for (int side = 0; side < 2 && !actedThisTick; side++) {
                    int sideX = x + sideDirs[side];
                    int sideY = y;
                    
                    if (!grid->isValidPosition(sideX, sideY)) continue;
                    
                    Cell& sideCell = grid->getCell(sideX, sideY);
                    
                    // Only equalize with SAME liquid type
                    if (!isLiquidType(sideCell.elementType) || sideCell.elementType != cell.elementType) {
                        continue;
                    }
                    
                    // Don't equalize if BOTH cells are in hysteresis gap (prevents oscillation)
                    if (cellMass >= CONSOLIDATE_THRESHOLD && cellMass < MIN_SIDEWAY_MASS &&
                        sideCell.mass >= CONSOLIDATE_THRESHOLD && sideCell.mass < MIN_SIDEWAY_MASS) {
                        continue;
                    }
                    
                    // Only give mass to neighbor with LESS mass
                    if (sideCell.mass >= cell.mass) {
                        continue;
                    }
                    
                    // Check reservation
                    if (isMergeSource[y][x] || isMergeTarget[y][x] ||
                        isMergeSource[sideY][sideX] || isMergeTarget[sideY][sideX] ||
                        isSwapInvolved[y][x] || isSwapInvolved[sideY][sideX]) {
                        continue;
                    }
                    
                    float maxMass = GasSim::getMaxMassForElement(sideCell.elementType);
                    float spaceAvailable = maxMass - sideCell.mass;
                    float mergeAmount = std::min(cellMass * 0.1f, spaceAvailable);  // Give 10% or fill space
                    
                    if (mergeAmount >= 0.001f) {
                        FlowAction action;
                        action.fromX = x; action.fromY = y;
                        action.toX = sideX; action.toY = sideY;
                        action.massToMove = mergeAmount;
                        action.temperature = cell.temperature;
                        action.type = cell.elementType;
                        action.isMerge = true;
                        flowActions.push_back(action);
                        
                        isMergeSource[y][x] = true;
                        isMergeTarget[sideY][sideX] = true;
                        actedThisTick = true;
                    }
                }
            }
            
            // PRIORITY 4: CONSOLIDATE - Small droplets (< 0.05kg) move ALL mass into higher-mass neighbor
            // Hysteresis gap (0.05-0.1kg) prevents oscillation between spreading and consolidating
            if (!actedThisTick && cellMass < CONSOLIDATE_THRESHOLD && cellMass > 0.001f) {
                int sideDirs[2] = {-1, 1};  // LEFT, RIGHT
                
                // Find neighbor with HIGHEST mass to consolidate into (sideways only)
                // Downward consolidation already handled by Priority 1 (merge down)
                int bestX = -1, bestY = -1;
                float highestMass = cellMass;
                
                for (int side = 0; side < 2; side++) {
                    int sideX = x + sideDirs[side];
                    int sideY = y;
                    
                    if (!grid->isValidPosition(sideX, sideY)) continue;
                    
                    Cell& sideCell = grid->getCell(sideX, sideY);
                    
                    // Only consolidate into same liquid with MORE mass
                    if (isLiquidType(sideCell.elementType) && 
                        sideCell.elementType == cell.elementType && 
                        sideCell.mass > highestMass) {
                        highestMass = sideCell.mass;
                        bestX = sideX;
                        bestY = sideY;
                    }
                }
                
                // If found a higher-mass neighbor, move ALL mass into it
                if (bestX != -1 && bestY != -1) {
                    if (!isMergeSource[y][x] && !isMergeTarget[y][x] &&
                        !isMergeSource[bestY][bestX] && !isMergeTarget[bestY][bestX]) {
                        
                        float maxMass = GasSim::getMaxMassForElement(grid->getCell(bestX, bestY).elementType);
                        float spaceAvailable = maxMass - grid->getCell(bestX, bestY).mass;
                        float mergeAmount = std::min(cellMass, spaceAvailable);
                        
                        if (mergeAmount >= 0.001f) {
                            FlowAction action;
                            action.fromX = x; action.fromY = y;
                            action.toX = bestX; action.toY = bestY;
                            action.massToMove = mergeAmount;
                            action.temperature = cell.temperature;
                            action.type = cell.elementType;
                            action.isMerge = true;
                            flowActions.push_back(action);
                            
                            isMergeSource[y][x] = true;
                            isMergeTarget[bestY][bestX] = true;
                            actedThisTick = true;
                        }
                    }
                }
            }
        }
    }
    
    // ============ STEP 2: LIQUID LEVELING (Horizontal Spreading) ============
    // ONLY applies to liquid that couldn't move downward (blocked at bottom)
    // After ~1 second of being blocked, liquid starts spreading sideways
    // This makes liquid spread out to find lowest point on flat surfaces
    {
        // Track which cells moved downward in STEP 1
        // We need to re-check because flowActions were already applied
        // Instead, check if cell is at bottom or blocked below
        
        // First pass: identify leveling actions
        struct LevelAction {
            int fromX, fromY;
            int toX, toY;
            float massToMove;
        };
        std::vector<LevelAction> levelActions;
        
        // Track which cells are involved to prevent conflicts
        std::vector<std::vector<bool>> levelSource(height, std::vector<bool>(width, false));
        std::vector<std::vector<bool>> levelTarget(height, std::vector<bool>(width, false));
        
        // Random seed for chance-based leveling (spreads over time, not instantly)
        static unsigned int levelRandomSeed = 99999;
        auto levelRand = [&]() -> int {
            levelRandomSeed = levelRandomSeed * 1103515245 + 12345;
            return (levelRandomSeed / 65536) % 100;
        };
        
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                if (!grid->isValidPosition(x, y)) continue;
                
                Cell& cell = grid->getCell(x, y);
                if (!isLiquidType(cell.elementType)) continue;
                if (cell.mass < 0.01f) continue;  // Skip near-empty cells
                
                ElementType liquidType = cell.elementType;
                float cellMass = cell.mass;
                
                // CRITICAL: Only level if cell is blocked from moving downward
                // Check if below cell is NOT vacuum (blocked by liquid, solid, or gas)
                bool canMoveDown = false;
                if (grid->isValidPosition(x, y + 1)) {
                    Cell& below = grid->getCell(x, y + 1);
                    if (below.elementType == ElementType::Vacuum) {
                        canMoveDown = true;
                    }
                }
                
                // If can move down, skip horizontal spreading (let gravity handle it)
                if (canMoveDown) continue;
                
                // CRITICAL: Small droplets (< 0.1kg) only spread if resting on same liquid
                // - Small droplet on vacuum/solid/gas: completely still, just sits there
                // - Small droplet on same liquid: can spread (part of flowing body)
                // - Large amount (>= 0.1kg): always spreads when blocked
                bool isOnSameLiquid = false;
                if (grid->isValidPosition(x, y + 1)) {
                    Cell& below = grid->getCell(x, y + 1);
                    if (isLiquidType(below.elementType) && below.elementType == liquidType) {
                        isOnSameLiquid = true;
                    }
                }
                
                if (cellMass < 0.1f && !isOnSameLiquid) {
                    // Small droplet NOT on same liquid - completely still, just sits there
                    continue;
                }
                
                // Random chance to spread (~10% per tick = ~1 second at 10 ticks/sec)
                if (levelRand() >= 10) continue;
                
                // Try to spread LEFT and RIGHT
                int directions[2] = {-1, 1};  // LEFT, RIGHT
                
                for (int dir : directions) {
                    int nx = x + dir;
                    int ny = y;
                    
                    if (!grid->isValidPosition(nx, ny)) continue;
                    
                    // Check if already involved in a level action
                    if (levelSource[y][x] || levelTarget[ny][nx]) continue;
                    
                    Cell& neighbor = grid->getCell(nx, ny);
                    
                    // CASE 1: Spread into VACUUM
                    if (neighbor.elementType == ElementType::Vacuum) {
                        // Move portion of mass into vacuum (50% split to create spreading)
                        float massToMove = cellMass * 0.5f;
                        
                        if (massToMove > 0.01f) {
                            levelActions.push_back({x, y, nx, ny, massToMove});
                            levelSource[y][x] = true;
                            levelTarget[ny][nx] = true;
                        }
                    }
                    // CASE 2: Equalize with SAME LIQUID
                    else if (isLiquidType(neighbor.elementType) && neighbor.elementType == liquidType) {
                        float neighborMass = neighbor.mass;
                        float massDiff = cellMass - neighborMass;
                        
                        // If this cell has significantly more mass (>5%), transfer some
                        if (massDiff > 0.05f * cellMass && !levelSource[ny][nx]) {
                            // Transfer half the difference to equalize
                            float massToMove = massDiff * 0.5f;
                            
                            if (massToMove > 0.01f) {
                                levelActions.push_back({x, y, nx, ny, massToMove});
                                levelSource[y][x] = true;
                                levelTarget[ny][nx] = true;
                            }
                        }
                    }
                }
            }
        }
        
        // Second pass: apply leveling actions
        for (const auto& action : levelActions) {
            Cell& fromCell = grid->getCell(action.fromX, action.fromY);
            Cell& toCell = grid->getCell(action.toX, action.toY);
            
            // Verify cells are still in valid state
            if (!isLiquidType(fromCell.elementType)) continue;
            
            if (toCell.elementType == ElementType::Vacuum) {
                // Moving into vacuum - convert target to liquid
                toCell.elementType = fromCell.elementType;
                toCell.mass = action.massToMove;
                toCell.temperature = fromCell.temperature;
                toCell.updated = true;
                toCell.updateColor();
                
                // Remove mass from source
                fromCell.mass -= action.massToMove;
                fromCell.updated = true;
                fromCell.updateColor();
            } else if (isLiquidType(toCell.elementType) && toCell.elementType == fromCell.elementType) {
                // Merging with same liquid - mass-weighted temperature average
                float totalMass = toCell.mass + action.massToMove;
                float newTemp = (toCell.mass * toCell.temperature + action.massToMove * fromCell.temperature) / totalMass;
                
                toCell.mass += action.massToMove;
                toCell.temperature = newTemp;
                toCell.updated = true;
                toCell.updateColor();
                
                // Remove mass from source
                fromCell.mass -= action.massToMove;
                fromCell.updated = true;
                fromCell.updateColor();
            }
        }
    }
    
    // Apply all flow actions simultaneously
    for (const auto& action : flowActions) {
        // CRITICAL FIX: Handle cleanup actions (leftover mass -> vacuum)
        if (action.fromX == action.toX && action.fromY == action.toY && action.type == ElementType::Vacuum) {
            // This is a cleanup action - convert cell to vacuum
            Cell& cell = grid->getCell(action.fromX, action.fromY);
            cell.convertToVacuum();
            continue;
        }
        
        Cell& fromCell = grid->getCell(action.fromX, action.fromY);
        Cell& toCell = grid->getCell(action.toX, action.toY);
        
        if (action.isMerge) {
            // MERGE: Add mass to target (WITH CAP CHECK to prevent overflow)
            // Use the ACTUAL element type's max mass, not hardcoded 1000kg
            float maxMass = GasSim::getMaxMassForElement(toCell.elementType);
            float spaceAvailable = maxMass - toCell.mass;
            
            // CRITICAL: Cap by ACTUAL source mass at application time
            // (source may have already sent mass in previous actions this tick)
            float actualMergeAmount = std::min({action.massToMove, spaceAvailable, fromCell.mass});
            
            // If no space or no mass, skip this merge (mass stays in source)
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
                fromCell.convertToVacuum();
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
                    fromCell.convertToVacuum();
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
                fromCell.convertToVacuum();
            } else if (action.type == ElementType::Vacuum && action.fromX == action.toX && action.fromY == action.toY) {
                // Cleanup action: Convert cell to vacuum (leftover micro-mass)
                fromCell.convertToVacuum();
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
                gasCell.convertToVacuum();
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
                gasCell.convertToVacuum();
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
            gasCell.convertToVacuum();
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
        
        // Source becomes vacuum - use helper function to ensure proper cleanup
        cell.convertToVacuum();
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
            
            // Source becomes vacuum - use helper function to ensure proper cleanup
            cell.convertToVacuum();
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
            
            // Source becomes vacuum - use helper function to ensure proper cleanup
            cell.convertToVacuum();
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
            
            // If source has very little mass left, become vacuum - use helper function
            if (cell.mass < 0.01f) {
                cell.convertToVacuum();
            }
            
            break;  // Only leak to one diagonal per tick
        }
    }
}

bool FluidSim::isLiquidType(ElementType type) {
    return liquidTypes.count(type) > 0;
}

bool FluidSim::canDisplace(ElementType fluid, ElementType target) {
    // Liquids can displace gases
    if (isLiquidType(fluid)) {
        const Element& targetProps = ElementTypes::getElement(target);
        return targetProps.isGas;
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
