#include "GasSim.h"
#include "ElementTypes.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

GasSim::GasSim() 
    : SimulationSystem("GasSim", 0.04f) {  // Update every 40ms (faster spreading for visible movement)
}

bool GasSim::update(float deltaTime) {
    if (!enabled || !grid) return false;
    
    // DEBUG: Log delta timing (COMMENTED OUT - too spammy)
    /*
    static int debugDeltaTimeCount = 0;
    if (debugDeltaTimeCount < 30) {
        std::cout << "[GAS SIM] deltaTime=" << deltaTime << " updateTimer=" << updateTimer << std::endl;
        debugDeltaTimeCount++;
    }
    */
    
    if (!shouldUpdate(deltaTime)) {
        // Don't spam logs - LOD skipping is normal
        return false;
    }
    
    // DEBUG: Track update calls (COMMENTED OUT - too spammy)
    /*
    static int updateDebugCount = 0;
    if (updateDebugCount++ < 20) {
        std::cout << "[GAS SIM] UPDATE #" << updateDebugCount << " called!" << std::endl;
    }
    */
    
    int width = grid->getWidth();
    int height = grid->getHeight();
    
    // DOUBLE-BUFFERED GAS SIMULATION (same as HeatSim pattern):
    // Step 1: Snapshot all gas cell data
    // Step 2: Calculate gas movements in separate buffers
    // Step 3: Apply all changes simultaneously
    
    // STEP 1: Snapshot current gas state
    struct GasCellData {
        ElementType type = ElementType::Vacuum;
        float mass = 0.0f;
        float pressure = 0.0f;
        float temperature = -273.15f;
    };
    
    std::vector<std::vector<GasCellData>> gasSnapshot(height, std::vector<GasCellData>(width));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (grid->isValidPosition(x, y)) {
                Cell& cell = grid->getCell(x, y);
                if (isGasType(cell.elementType)) {
                    gasSnapshot[y][x].type = cell.elementType;
                    gasSnapshot[y][x].mass = cell.mass;
                    gasSnapshot[y][x].pressure = cell.pressure;
                    gasSnapshot[y][x].temperature = cell.temperature;
                }
            }
        }
    }
    
    // STEP 2: Calculate gas transfers (HEAT-LIKE DIFFUSION)
    // Gases spread to ALL adjacent cells (vacuum, gas, even liquids via buoyancy)
    // Mass splits based on pressure differential - stops at equilibrium
    
    // Track how much mass leaves each cell
    std::vector<std::vector<float>> massOut(height, std::vector<float>(width, 0.0f));
    // Track how much mass enters each cell
    struct MassIn {
        float mass = 0.0f;
        float temperatureSum = 0.0f;  // Start at 0, not -273.15!
        int sourceCount = 0;
    };
    std::vector<std::vector<MassIn>> massIn(height, std::vector<MassIn>(width));
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            
            // GAS CELLS: ALWAYS spread to conserve mass (no LOD skipping!)
            // Mass conservation is critical - skipping spread causes mass loss
            // LOD is only applied in the FLOW step below
            
            // Only process gas cells from snapshot
            if (!isGasType(gasSnapshot[y][x].type)) continue;
            
            // DEBUG: Track spawned gas cells
            static int spawnDebugCount = 0;
            bool isSpawnedDebug = (gasSnapshot[y][x].mass >= 0.19f && gasSnapshot[y][x].mass <= 0.21f);
            if (isSpawnedDebug) {
                spawnDebugCount++;
                if (spawnDebugCount <= 10) {  // First 10 ticks only
                    std::cout << "[GAS DEBUG] SPAWNED gas at (" << x << "," << y 
                              << ") mass=" << gasSnapshot[y][x].mass 
                              << " pressure=" << gasSnapshot[y][x].pressure
                              << " temp=" << gasSnapshot[y][x].temperature << std::endl;
                }
            }
            
            float currentMass = gasSnapshot[y][x].mass;
            float currentPressure = gasSnapshot[y][x].pressure;
            
            // PERFORMANCE OPTIMIZATION: Only expand (spread mass) if over max capacity
            // Gas cells at or below max mass skip expensive spreading calculation
            // They will still float and merge in STEP 4
            if (currentMass > MAX_GAS_MASS) {
                // OVER CAPACITY: Perform expansion/spreading to reduce mass
                // Continue to STEP 2 spreading logic below
            } else {
                // AT OR BELOW MAX: Skip spreading, will float/merge in STEP 4
                continue;
            }
            
            // 4-directional neighbors (like heat transfer)
            int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            
            // PRIORITIZE upward movement (buoyancy) - reorder neighbors
            // Up first, then sideways, then down last
            int priorityNeighbors[4][2] = {{0, -1}, {-1, 0}, {1, 0}, {0, 1}};
            
            // BUOYANCY: Directional spread weights
            // Equal distribution for natural, rounded expansion
            // Weights: UP=0.20, LEFT=0.20, RIGHT=0.20, DOWN=0.20 (total=0.80)
            float directionalWeights[4] = {0.20f, 0.20f, 0.20f, 0.20f};
            
            // Calculate how much mass to spread
            // CRITICAL FIX: Split mass correctly to avoid mass loss!
            // Total spread rate = 80% of mass per tick
            float totalSpreadRate = 0.80f;  // 80% of mass spreads per tick
            float massToSpread = currentMass * totalSpreadRate;
            
            // STEP 1: Count valid spread directions
            int validDirCount = 0;
            bool validDirections[4] = {false, false, false, false};
            
            for (int i = 0; i < 4; i++) {
                auto& dir = priorityNeighbors[i];
                int nx = x + dir[0];
                int ny = y + dir[1];
                
                if (!grid->isValidPosition(nx, ny)) continue;
                
                ElementType neighborType = gasSnapshot[ny][nx].type;
                bool isVacuum = (neighborType == ElementType::Vacuum);
                bool isGas = isGasType(neighborType);
                
                if (isVacuum || isGas) {
                    // Check pressure for gas neighbors
                    if (isGas) {
                        float neighborPressure = gasSnapshot[ny][nx].pressure;
                        if (neighborPressure <= currentPressure * 1.2f) {
                            validDirections[i] = true;
                            validDirCount++;
                        }
                    } else {
                        // Vacuum is always valid
                        validDirections[i] = true;
                        validDirCount++;
                    }
                }
            }
            
            // STEP 2: Split mass among valid directions and apply transfers
            if (validDirCount > 0 && massToSpread > 0.00001f) {
                float massPerDirection = massToSpread / validDirCount;
                
                for (int i = 0; i < 4; i++) {
                    if (!validDirections[i]) continue;
                    
                    auto& dir = priorityNeighbors[i];
                    int nx = x + dir[0];
                    int ny = y + dir[1];
                    
                    ElementType neighborType = gasSnapshot[ny][nx].type;
                    bool isVacuum = (neighborType == ElementType::Vacuum);
                    bool isGas = isGasType(neighborType);
                    
                    // Calculate transfer amount
                    float transferAmount = massPerDirection;
                    
                    // For gas neighbors, apply pressure differential
                    if (isGas) {
                        float neighborPressure = gasSnapshot[ny][nx].pressure;
                        float pressureRatio = neighborPressure / currentPressure;
                        float spreadModifier = 1.0f - (pressureRatio * 0.3f);
                        transferAmount *= std::max(spreadModifier, 0.1f);
                    }
                    
                    if (transferAmount > 0.00001f) {
                        massOut[y][x] += transferAmount;
                        massIn[ny][nx].mass += transferAmount;
                        massIn[ny][nx].temperatureSum += gasSnapshot[y][x].temperature;
                        massIn[ny][nx].sourceCount++;
                    }
                }
            }
        }
    }
    
    // STEP 3: Apply all gas transfers simultaneously
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            
            Cell& cell = grid->getCell(x, y);
            
            // Skip non-gas cells and cells with no transfers
            if (!isGasType(cell.elementType) && cell.elementType != ElementType::Vacuum) continue;
            if (massOut[y][x] == 0.0f && massIn[y][x].mass == 0.0f) continue;
            
            // Calculate new mass
            float snapshotMass = isGasType(gasSnapshot[y][x].type) ? gasSnapshot[y][x].mass : 0.0f;
            float newMass = snapshotMass - massOut[y][x] + massIn[y][x].mass;
            
            // If cell was vacuum and received gas, it becomes gas
            if (cell.elementType == ElementType::Vacuum && newMass > MIN_GAS_MASS) {
                // Determine gas type from incoming mass sources
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx;
                        int ny = y + dy;
                        if (grid->isValidPosition(nx, ny) && isGasType(gasSnapshot[ny][nx].type)) {
                            cell.elementType = gasSnapshot[ny][nx].type;
                            break;
                        }
                    }
                    if (isGasType(cell.elementType)) break;
                }
                // NEW GAS CELL - mark as updated so it simulates next frame
                cell.updated = true;
                
                // DEBUG: Log new gas cell creation
                static int newCellDebugCount = 0;
                if (newCellDebugCount++ < 20) {
                    std::cout << "[GAS CREATE] Vacuum -> Gas at (" << x << "," << y 
                              << ") type=" << (int)cell.elementType
                              << " mass=" << newMass << std::endl;
                }
            }
            
            // Update cell state
            cell.mass = newMass;
            
            // Update color for gas cells
            cell.updateColor();
            
            // Mark cell as updated so it gets simulated next frame
            cell.updated = true;
                
            // Update temperature ONLY if cell received mass from neighbors
            // If cell only sent mass out (no incoming), keep its current temperature
            if (massIn[y][x].sourceCount > 0 && massIn[y][x].mass > 0.0f) {
                float oldTemp = cell.temperature;
                
                // If cell was vacuum (temp ~-273°C), it's becoming a new gas cell
                // Use the DEFAULT temperature for this gas type, not averaged temperature
                if (oldTemp <= -273.0f) {
                    const Element& gasProps = ElementTypes::getElement(cell.elementType);
                    cell.temperature = gasProps.defaultTemperature;  // Use default (e.g., 100°C for steam)
                } else {
                    // EXISTING gas cell - blend existing temperature with incoming temperature
                    // Weight by mass to prevent temperature oscillation
                    float existingMass = snapshotMass - massOut[y][x];  // Mass remaining after sending out
                    float incomingMass = massIn[y][x].mass;  // Mass received from neighbors
                    float totalMass = existingMass + incomingMass;
                    
                    if (totalMass > 0.001f && massIn[y][x].sourceCount > 0) {
                        // Calculate average incoming temperature (temperatureSum / sourceCount)
                        float avgIncomingTemp = massIn[y][x].temperatureSum / massIn[y][x].sourceCount;
                        
                        // Mass-weighted temperature blend
                        float existingTempContribution = existingMass * oldTemp;
                        float incomingTempContribution = incomingMass * avgIncomingTemp;
                        cell.temperature = (existingTempContribution + incomingTempContribution) / totalMass;
                    }
                }
                
                // ADIABATIC COOLING: Gas loses temperature as it expands/spreads
                // Expansion ratio = mass lost / original mass
                float massLost = massOut[y][x];
                float expansionRatio = massLost / snapshotMass;
                if (expansionRatio > 0.01f && snapshotMass > 0.001f) {  // Only cool if significant expansion
                    // Very small temperature drop - real adiabatic cooling is minimal for small expansions
                    // Typical: ~0.1-0.5°C per 10% expansion for steam at atmospheric pressure
                    float coolingFactor = 0.005f;  // 0.5% of expansion ratio as temperature drop
                    float tempDrop = expansionRatio * coolingFactor * std::abs(cell.temperature);
                    cell.temperature -= tempDrop;
                }
                
                // DEBUG: Log temperature changes with timestamp (COMMENTED OUT - too spammy)
                /*
                static int tempDebugCount = 0;
                static auto startTime = std::chrono::steady_clock::now();
                if (tempDebugCount++ % 50 == 0) {
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
                    std::cout << "[" << elapsed << "ms] [GAS TEMP] Cell (" << x << "," << y << ") "
                              << oldTemp << "°C -> " << cell.temperature << "°C "
                              << "(sources: " << massIn[y][x].sourceCount 
                              << ", massIn: " << massIn[y][x].mass << "kg)" << std::endl;
                }
                */
            }
            // If no incoming mass, cell keeps whatever temperature it already has
            
            // Recalculate pressure
            calculatePressure(x, y);
        }
    }
    
    // STEP 4: GAS FLOW - Move/swap gas cells based on buoyancy and density
    // This happens AFTER expansion to give gas continuous movement
    {
        // Create flow snapshot (reuse gasSnapshot from STEP 1)
        // For each gas cell, determine if it should move/swap
        
        struct FlowAction {
            int fromX, fromY;
            int toX, toY;
            float massToMove;
            float temperature;
            ElementType type;
            bool isMerge;  // true = merge into target, false = swap positions
        };
        
        std::vector<FlowAction> flowActions;
        flowActions.reserve(width * height / 4);  // Reserve space for efficiency
        
        // Track which cells are already involved in merges to prevent conflicts
        std::vector<std::vector<bool>> isMergeSource(height, std::vector<bool>(width, false));
        std::vector<std::vector<bool>> isMergeTarget(height, std::vector<bool>(width, false));
        
        // Random seed for direction choice
        static unsigned int flowRandomSeed = 42;
        auto simpleRand = []() -> int {
            flowRandomSeed = flowRandomSeed * 1103515245 + 12345;
            return (flowRandomSeed / 65536) % 32768;
        };
        
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                if (!grid->isValidPosition(x, y)) continue;
                
                Cell& cell = grid->getCell(x, y);
                if (!isGasType(cell.elementType)) continue;
                
                // GAS MUST ALWAYS FLOAT - remove LOD check for gas flow
                // Gas buoyancy is critical for realistic behavior
                // LOD was causing gas to hang mid-air when far from camera
                // if (!shouldUpdateCell(x, y, deltaTime)) continue;  // REMOVED
                
                float cellMass = cell.mass;
                
                // Allow very small gas cells to float and merge
                // Lower threshold for flow than for cell existence
                // This prevents tiny gas pockets from getting stuck
                static constexpr float FLOW_MIN_MASS = 0.0001f;  // 0.1g threshold for flow
                if (cellMass < FLOW_MIN_MASS) {
                    continue;
                }
                
                // DEBUG: Log flow attempts for spawned gas (COMMENTED OUT - too spammy)
                /*
                static int flowDebugCount = 0;
                bool isSpawnedFlow = (cellMass >= 0.01f && cellMass <= 0.5f);  // Recently spawned gas
                if (isSpawnedFlow && flowDebugCount < 10) {
                    std::cout << "[FLOW] Cell(" << x << "," << y << ") mass=" << cellMass << " checking directions..." << std::endl;
                    flowDebugCount++;
                }
                */
                
                // GAS FLOW: Prioritize UPWARD movement, expand sideways only if blocked
                
                // Define directions with priority: UP, UP-DIAGONALS, SIDEWAYS, DOWN
                int directions[7][2];
                int dirCount = 0;
                
                // UP first (buoyancy - highest priority)
                directions[dirCount][0] = 0; directions[dirCount][1] = -1; dirCount++;
                
                // UP-DIAGONAL (randomize)
                if (simpleRand() % 2 == 0) {
                    directions[dirCount][0] = -1; directions[dirCount][1] = -1; dirCount++;
                    directions[dirCount][0] = 1; directions[dirCount][1] = -1; dirCount++;
                } else {
                    directions[dirCount][0] = 1; directions[dirCount][1] = -1; dirCount++;
                    directions[dirCount][0] = -1; directions[dirCount][1] = -1; dirCount++;
                }
                
                // SIDEWAYS (randomize)
                if (simpleRand() % 2 == 0) {
                    directions[dirCount][0] = -1; directions[dirCount][1] = 0; dirCount++;
                    directions[dirCount][0] = 1; directions[dirCount][1] = 0; dirCount++;
                } else {
                    directions[dirCount][0] = 1; directions[dirCount][1] = 0; dirCount++;
                    directions[dirCount][0] = -1; directions[dirCount][1] = 0; dirCount++;
                }
                
                // DOWN last (lowest priority - gas only flows down if pushed)
                directions[dirCount][0] = 0; directions[dirCount][1] = 1; dirCount++;
                
                // Scan for upward vacuum first (float upward)
                int upVacuumDir = -1;
                for (int i = 0; i < 3; i++) {  // Only check UP directions (0, 1, 2)
                    int nx = x + directions[i][0];
                    int ny = y + directions[i][1];
                    
                    if (!grid->isValidPosition(nx, ny)) continue;
                    
                    Cell& neighbor = grid->getCell(nx, ny);
                    if (neighbor.elementType == ElementType::Vacuum) {
                        upVacuumDir = i;
                        break;
                    }
                }
                
                // If upward vacuum found, SWAP (float upward)
                if (upVacuumDir != -1) {
                    int nx = x + directions[upVacuumDir][0];
                    int ny = y + directions[upVacuumDir][1];
                    
                    FlowAction action;
                    action.fromX = x; action.fromY = y;
                    action.toX = nx; action.toY = ny;
                    action.massToMove = cellMass;
                    action.temperature = cell.temperature;
                    action.type = cell.elementType;
                    action.isMerge = false;  // SWAP action
                    flowActions.push_back(action);
                    continue;  // Done with this cell
                }
                
                // UPWARD BLOCKED: Try to merge upward into same gas
                int upMergeDir = -1;
                for (int i = 0; i < 3; i++) {  // Only check UP directions
                    int nx = x + directions[i][0];
                    int ny = y + directions[i][1];
                    
                    if (!grid->isValidPosition(nx, ny)) continue;
                    
                    Cell& neighbor = grid->getCell(nx, ny);
                    bool isSameGas = isGasType(neighbor.elementType) && neighbor.elementType == cell.elementType;
                    
                    if (isSameGas && upMergeDir == -1) {
                        upMergeDir = i;
                        break;
                    }
                }
                
                // If upward merge possible, do it
                if (upMergeDir != -1) {
                    int nx = x + directions[upMergeDir][0];
                    int ny = y + directions[upMergeDir][1];
                    Cell& neighbor = grid->getCell(nx, ny);
                    
                    // Check if already involved in merge
                    if (!isMergeSource[y][x] && !isMergeTarget[y][x] && 
                        !isMergeSource[ny][nx] && !isMergeTarget[ny][nx]) {
                        
                        float spaceAvailable = MAX_GAS_MASS - neighbor.mass;
                        if (spaceAvailable >= 0.001f) {
                            float transferAmount = std::min(cellMass, spaceAvailable);
                            
                            if (transferAmount >= 0.0001f) {
                                FlowAction action;
                                action.fromX = x; action.fromY = y;
                                action.toX = nx; action.toY = ny;
                                action.massToMove = transferAmount;
                                action.temperature = cell.temperature;
                                action.type = cell.elementType;
                                action.isMerge = true;
                                flowActions.push_back(action);
                                
                                isMergeSource[y][x] = true;
                                isMergeTarget[ny][nx] = true;
                                continue;  // Done with this cell
                            }
                        }
                    }
                }
                
                // UPWARD COMPLETELY BLOCKED: Expand sideways into vacuum (spread mass, not swap)
                int sideVacuumDir = -1;
                for (int i = 3; i < 5; i++) {  // Check SIDEWAY directions (3, 4)
                    int nx = x + directions[i][0];
                    int ny = y + directions[i][1];
                    
                    if (!grid->isValidPosition(nx, ny)) continue;
                    
                    Cell& neighbor = grid->getCell(nx, ny);
                    if (neighbor.elementType == ElementType::Vacuum) {
                        sideVacuumDir = i;
                        break;
                    }
                }
                
                // If sideways vacuum found, SPREAD mass (partial transfer, not full swap)
                if (sideVacuumDir != -1) {
                    int nx = x + directions[sideVacuumDir][0];
                    int ny = y + directions[sideVacuumDir][1];
                    
                    // Spread 50% of mass sideways (expansion, not movement)
                    float spreadAmount = cellMass * 0.5f;
                    
                    if (spreadAmount > 0.0001f) {
                        FlowAction action;
                        action.fromX = x; action.fromY = y;
                        action.toX = nx; action.toY = ny;
                        action.massToMove = spreadAmount;
                        action.temperature = cell.temperature;
                        action.type = cell.elementType;
                        action.isMerge = true;  // Treat as merge (partial transfer)
                        flowActions.push_back(action);
                    }
                }
            }
        }
        
        // Apply all flow actions simultaneously
        static int applyDebugCount = 0;
        for (const auto& action : flowActions) {
            Cell& fromCell = grid->getCell(action.fromX, action.fromY);
            Cell& toCell = grid->getCell(action.toX, action.toY);
            
            // DEBUG: Log first few flow applications
            if (applyDebugCount < 10) {
                std::cout << "[APPLY FLOW] " << (action.isMerge ? "MERGE" : "SWAP") 
                          << " (" << action.fromX << "," << action.fromY << ")->(" 
                          << action.toX << "," << action.toY << ") mass=" << action.massToMove << std::endl;
                applyDebugCount++;
            }
            
            if (action.isMerge) {
                // MERGE: Push mass into target, leftover stays
                float spaceAvailable = MAX_GAS_MASS - toCell.mass;
                float massToMerge = std::min(action.massToMove, spaceAvailable);
                float leftover = action.massToMove - massToMerge;
                
                // Mass-weighted temperature average
                float totalMass = toCell.mass + massToMerge;
                float newTemp = (toCell.mass * toCell.temperature + massToMerge * action.temperature) / totalMass;
                
                toCell.mass += massToMerge;
                toCell.temperature = newTemp;
                toCell.updated = true;
                toCell.updateColor();
                calculatePressure(action.toX, action.toY);
                
                // Handle leftover in source cell
                fromCell.mass = leftover;
                fromCell.updated = true;
                fromCell.updateColor();
                calculatePressure(action.fromX, action.fromY);
            } else {
                // SWAP: Exchange ALL cell data
                ElementType tempType = fromCell.elementType;
                float tempMass = fromCell.mass;
                float tempTemp = fromCell.temperature;
                float tempPressure = fromCell.pressure;
                float tempVelX = fromCell.velocityX;
                float tempVelY = fromCell.velocityY;
                sf::Color tempColor = fromCell.color;
                
                fromCell.elementType = toCell.elementType;
                fromCell.mass = toCell.mass;
                fromCell.temperature = toCell.temperature;
                fromCell.pressure = toCell.pressure;
                fromCell.velocityX = toCell.velocityX;
                fromCell.velocityY = toCell.velocityY;
                fromCell.color = toCell.color;
                fromCell.updated = true;
                
                toCell.elementType = tempType;
                toCell.mass = tempMass;
                toCell.temperature = tempTemp;
                toCell.pressure = tempPressure;
                toCell.velocityX = tempVelX;
                toCell.velocityY = tempVelY;
                toCell.color = tempColor;
                toCell.updated = true;
                
                calculatePressure(action.fromX, action.fromY);
                calculatePressure(action.toX, action.toY);
            }
        }
    }
    
    // Gas decay: time-based removal of microscopic gas cells
    // CRITICAL FIX: Only decay isolated micro-cells, not active gas clouds
    static constexpr float MICRO_MASS_THRESHOLD = 0.001f;
    static constexpr float MIN_DECAY_TIME = 60.0f;   // Minimum 60 seconds
    static constexpr float MAX_DECAY_TIME = 120.0f;  // Maximum 120 seconds
    
    // Random seed for decay time generation
    static unsigned int decayRandomSeed = 12345;
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            
            Cell& cell = grid->getCell(x, y);
            
            if (!isGasType(cell.elementType)) continue;
            
            // Only decay microscopic gas cells
            if (cell.mass < MICRO_MASS_THRESHOLD) {
                // CHECK: Is this cell isolated? (no gas neighbors)
                bool hasGasNeighbor = false;
                int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
                
                for (int i = 0; i < 4; i++) {
                    int nx = x + neighbors[i][0];
                    int ny = y + neighbors[i][1];
                    
                    if (grid->isValidPosition(nx, ny)) {
                        const Cell& neighbor = grid->getCell(nx, ny);
                        if (isGasType(neighbor.elementType) && neighbor.mass >= MICRO_MASS_THRESHOLD) {
                            hasGasNeighbor = true;
                            break;
                        }
                    }
                }
                
                // SKIP decay if cell has a "healthy" gas neighbor (part of active cloud)
                if (hasGasNeighbor) {
                    cell.microMassDecayTime = 0.0f;  // Reset timer
                    continue;
                }
                
                // Initialize decay timer if not already set (negative = time remaining)
                if (cell.microMassDecayTime >= 0.0f) {
                    // Generate random decay time between 60-120 seconds
                    decayRandomSeed = decayRandomSeed * 1103515245 + 12345;
                    float randomFraction = (float)(decayRandomSeed % 100000) / 100000.0f;
                    float decayTime = MIN_DECAY_TIME + randomFraction * (MAX_DECAY_TIME - MIN_DECAY_TIME);
                    cell.microMassDecayTime = -decayTime;  // Negative = counting up to zero
                }
                
                // Increment timer by actual time since last update
                cell.microMassDecayTime += deltaTime;
                
                // Check if decay time has elapsed
                if (cell.microMassDecayTime >= 0.0f) {
                    // COMPLETELY clear the cell - reset ALL fields
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
                    
                    // Force color update to vacuum (dark)
                    cell.color = sf::Color(10, 10, 15);
                }
            } else {
                // Cell is above threshold - reset decay timer
                cell.microMassDecayTime = 0.0f;
            }
        }
    }
    
    // MASS AUDIT: Track total mass across ALL phases to detect conservation issues
    static int auditTickCounter = 0;
    static float lastTotalMass = 0.0f;
    static int auditInterval = 100;  // Check every 100 ticks
    static std::ofstream auditFile;  // File output for mass audit logs
    
    auditTickCounter++;
    if (auditTickCounter >= auditInterval) {
        auditTickCounter = 0;
        
        // Open file on first run
        if (!auditFile.is_open()) {
            auditFile.open("mass_audit.log", std::ios::out | std::ios::trunc);
        }
        
        // Sum all mass in the grid by type
        float totalGasMass = 0.0f;
        int gasCellCount = 0;
        float totalLiquidMass = 0.0f;
        int liquidCellCount = 0;
        float totalSolidMass = 0.0f;
        int solidCellCount = 0;
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (!grid->isValidPosition(x, y)) continue;
                
                const Cell& cell = grid->getCell(x, y);
                const Element& props = ElementTypes::getElement(cell.elementType);
                
                if (props.isGas) {
                    totalGasMass += cell.mass;
                    gasCellCount++;
                } else if (props.isLiquid) {
                    totalLiquidMass += cell.mass;
                    liquidCellCount++;
                } else if (cell.elementType != ElementType::Vacuum && cell.elementType != ElementType::Empty) {
                    totalSolidMass += cell.mass;
                    solidCellCount++;
                }
            }
        }
        
        float totalMass = totalGasMass + totalLiquidMass + totalSolidMass;
        
        // ALWAYS log to console for visibility
        std::cout << "[MASS AUDIT] Gas: " << totalGasMass << "kg (" << gasCellCount 
                  << ") | Liquid: " << totalLiquidMass << "kg (" << liquidCellCount
                  << ") | Solid: " << totalSolidMass << "kg (" << solidCellCount
                  << ") | TOTAL: " << totalMass << "kg";
        
        // Log to file and console if mass changed significantly (> 0.01kg difference)
        if (lastTotalMass > 0.0f && std::abs(totalMass - lastTotalMass) > 0.01f) {
            float massDiff = totalMass - lastTotalMass;
            float percentChange = (massDiff / lastTotalMass) * 100.0f;
            
            std::cout << " | Change: " << massDiff << "kg (" << percentChange << "%)";
            
            // Write to file with timestamp
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            auditFile << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
                     << " | Gas: " << totalGasMass << "kg | Liquid: " << totalLiquidMass << "kg"
                     << " | Solid: " << totalSolidMass << "kg | TOTAL: " << totalMass << "kg"
                     << " | Change: " << massDiff << "kg (" << percentChange << "%)" << std::endl;
        } else if (lastTotalMass == 0.0f && totalMass > 0.0f) {
            // Initial measurement
            std::cout << " | INITIAL";
            auditFile << "[INITIAL] Gas: " << totalGasMass << "kg | Liquid: " << totalLiquidMass 
                     << "kg | Solid: " << totalSolidMass << "kg | TOTAL: " << totalMass << "kg" << std::endl;
        }
        
        std::cout << std::endl;
        
        // Always log to file for tracking
        if (totalMass > 0.0f) {
            auditFile << std::fixed << std::setprecision(3) 
                     << "Gas: " << totalGasMass << "kg | Liquid: " << totalLiquidMass 
                     << "kg | Solid: " << totalSolidMass << "kg | TOTAL: " << totalMass << "kg" << std::endl;
        }
        
        lastTotalMass = totalMass;
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
    // Use actual molar mass based on gas type (O2=32g/mol, CO2=44g/mol, etc.)
    float molarMass = getMolarMass(cell.elementType) / 1000.0f;  // Convert g/mol to kg/mol
    float moles = cell.mass / molarMass;
    float tempKelvin = cell.temperature + 273.15f;
    float pressure = (moles * GAS_CONSTANT * tempKelvin) / CELL_VOLUME;
    
    // NO minimum clamp - low mass should = low pressure for proper gradient
    cell.pressure = std::max(0.0f, pressure);  // Just prevent negative
}

bool GasSim::isGasType(ElementType type) {
    return type == ElementType::Gas_O2 || type == ElementType::Gas_CO2 || type == ElementType::Gas_Lava;
}

bool GasSim::isSameGas(ElementType type1, ElementType type2) {
    return type1 == type2;
}

float GasSim::getMolarMass(ElementType type) {
    switch (type) {
        case ElementType::Gas_O2: return 32.0f;
        case ElementType::Gas_CO2: return 44.0f;
        default: return 28.0f;
    }
}
