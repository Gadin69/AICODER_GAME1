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
            bool actedThisTick = false;
            
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
                    if (!isMergeSource[y][x] && !isMergeTarget[y][x] &&
                        !isMergeSource[downY][downX] && !isMergeTarget[downY][downX]) {
                        float spaceAvailable = 1000.0f - below.mass;
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
            
            // Only spread sideways if: didn't act this tick AND (blocked downward by solid)
            // Don't spread if downward is possible but blocked by viscosity (honey should fall, not spread)
            bool canSpreadSideways = !actedThisTick && cellMass >= MIN_SIDEWAY_MASS && !downwardPossible;
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
                    // SIDE into GAS - displace 100% (full swap)
                    else if (canDisplace(cell.elementType, sideType)) {
                        static constexpr float MAX_GAS_MASS = 10.0f;
                        
                        if (sideCell.mass > MAX_GAS_MASS) {
                            if (!hasGasEscapeRoute(sideX, sideY, sideType)) {
                                continue;  // Air pocket, skip
                            }
                        }
                        
                        if (!isSwapInvolved[y][x] && !isSwapInvolved[sideY][sideX] &&
                            !isMergeSource[y][x] && !isMergeTarget[y][x]) {
                            // Queue gas for displacement
                            DisplacedGas displaced;
                            displaced.x = sideX;
                            displaced.y = sideY;
                            displaced.mass = sideCell.mass;
                            displaced.temperature = sideCell.temperature;
                            displaced.type = sideType;
                            displacedGasQueue.push_back(displaced);
                            
                            action.massToMove = cellMass;
                            action.isMerge = false;
                            action.overwriteGas = true;
                            flowActions.push_back(action);
                            
                            isSwapInvolved[y][x] = true;
                            isSwapInvolved[sideY][sideX] = true;
                            actedThisTick = true;
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
                    
                    float spaceAvailable = 1000.0f - sideCell.mass;
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
                        
                        float spaceAvailable = 1000.0f - grid->getCell(bestX, bestY).mass;
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
            cell.updateColor();  // Use proper vacuum color
            continue;
        }
        
        Cell& fromCell = grid->getCell(action.fromX, action.fromY);
        Cell& toCell = grid->getCell(action.toX, action.toY);
        
        if (action.isMerge) {
            // MERGE: Add mass to target (WITH CAP CHECK to prevent overflow)
            float spaceAvailable = 1000.0f - toCell.mass;  // 1000kg max per cell (1 cubic meter of water)
            
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
                fromCell.updateColor();  // Use proper vacuum color
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
                    fromCell.updateColor();  // Use proper vacuum color
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
                fromCell.updateColor();  // Use proper vacuum color
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
                fromCell.updateColor();  // Use proper vacuum color
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
        cell.updateColor();  // Use proper vacuum color
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
            cell.updateColor();  // Use proper vacuum color
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
            cell.updateColor();  // Use proper vacuum color
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
                cell.updateColor();  // Use proper vacuum color
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
