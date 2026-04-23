#include "GasSim.h"
#include "ElementTypes.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <chrono>

GasSim::GasSim() 
    : SimulationSystem("GasSim", 0.04f) {  // Update every 40ms (faster spreading for visible movement)
}

bool GasSim::update(float deltaTime) {
    if (!enabled || !grid) return false;
    
    static int debugDeltaTimeCount = 0;
    if (debugDeltaTimeCount < 30) {
        std::cout << "[GAS SIM] deltaTime=" << deltaTime << " updateTimer=" << updateTimer << std::endl;
        debugDeltaTimeCount++;
    }
    
    if (!shouldUpdate(deltaTime)) {
        static int skipDebugCount = 0;
        if (skipDebugCount++ % 50 == 0) {
            std::cout << "[GAS SIM] SKIPPED (shouldUpdate returned false)" << std::endl;
        }
        return false;
    }
    
    static int updateDebugCount = 0;
    if (updateDebugCount++ < 20) {
        std::cout << "[GAS SIM] UPDATE #" << updateDebugCount << " called!" << std::endl;
    }
    
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
            
            // GAS CELLS ALWAYS SIMULATE - no LOD skipping!
            // Gases are dynamic and must keep moving/spreading even off-screen
            // if (!shouldUpdateCell(x, y, deltaTime)) continue;  // REMOVED for gas
            
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
            if (currentMass < MIN_GAS_MASS) continue;
            
            // 4-directional neighbors (like heat transfer)
            int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
            
            // PRIORITIZE upward movement (buoyancy) - reorder neighbors
            // Up first, then sideways, then down last
            int priorityNeighbors[4][2] = {{0, -1}, {-1, 0}, {1, 0}, {0, 1}};
            
            // BUOYANCY: Directional spread weights
            // Equal distribution for natural, rounded expansion
            // Weights: UP=0.20, LEFT=0.20, RIGHT=0.20, DOWN=0.20 (total=0.80)
            float directionalWeights[4] = {0.20f, 0.20f, 0.20f, 0.20f};
            
            // Calculate how much mass to spread to each neighbor
            // Spread rate depends on pressure differential
            // BUOYANCY: Directional weights applied per-direction below
            // Total spread: 70% of mass per tick (35% up, 15% left, 15% right, 5% down)
            
            for (int i = 0; i < 4; i++) {
                auto& dir = priorityNeighbors[i];
                int nx = x + dir[0];
                int ny = y + dir[1];
                
                if (!grid->isValidPosition(nx, ny)) continue;
                
                // Can spread to: vacuum or gas cells ONLY (not liquids)
                ElementType neighborType = gasSnapshot[ny][nx].type;
                bool isVacuum = (neighborType == ElementType::Vacuum);
                bool isGas = isGasType(neighborType);
                
                if (!isVacuum && !isGas) continue;  // Skip liquids and solids
                
                // BUOYANCY: Apply directional weight
                float spreadAmount = currentMass * directionalWeights[i];
                
                // For gas neighbors, check pressure differential
                if (isGas) {
                    float neighborPressure = gasSnapshot[ny][nx].pressure;
                    
                    // Allow spreading even at near-equilibrium (thermal motion / Brownian motion)
                    // Only stop if neighbor pressure is SIGNIFICANTLY higher (20% threshold)
                    if (neighborPressure > currentPressure * 1.2f) continue;
                    
                    // Reduce spread amount based on pressure similarity
                    // More gradual reduction - gases should keep mixing even near equilibrium
                    float pressureRatio = neighborPressure / currentPressure;
                    float spreadModifier = 1.0f - (pressureRatio * 0.3f);  // 70% spread even at equal pressure
                    spreadAmount *= std::max(spreadModifier, 0.1f);  // Minimum 10% of directional weight
                }
                
                // Calculate transfer amount
                float transferAmount = spreadAmount;
                transferAmount = std::min(transferAmount, currentMass - MIN_GAS_MASS);
                
                // PREVENTION: Don't create micro-cells below threshold
                // If neighbor is vacuum, check if transfer would create cell below MIN_GAS_MASS
                if (isVacuum && transferAmount < MIN_GAS_MASS) {
                    // Skip this transfer - would create a micro-cell
                    if (isSpawnedDebug && spawnDebugCount <= 10) {
                        std::cout << "    -> SKIPPED (would create micro-cell: " << transferAmount << "kg < " << MIN_GAS_MASS << "kg)" << std::endl;
                    }
                    continue;
                }
                
                if (isSpawnedDebug && spawnDebugCount <= 10) {
                    std::cout << "  [" << (isVacuum ? "VAC" : "GAS") << "] (" << nx << "," << ny 
                              << ") pressure=" << (isGas ? gasSnapshot[ny][nx].pressure : 0.0f)
                              << " transfer=" << transferAmount << std::endl;
                }
                
                if (transferAmount > 0.00001f) {  // Allow microscopic transfers for continuous spreading
                    massOut[y][x] += transferAmount;
                    massIn[ny][nx].mass += transferAmount;
                    massIn[ny][nx].temperatureSum += gasSnapshot[y][x].temperature;
                    massIn[ny][nx].sourceCount++;
                    
                    if (isSpawnedDebug && spawnDebugCount <= 10) {
                        std::cout << "    -> TRANSFERRING " << transferAmount << "kg to (" << nx << "," << ny << ")" << std::endl;
                    }
                } else {
                    if (isSpawnedDebug && spawnDebugCount <= 10) {
                        std::cout << "    -> SKIPPED (transferAmount=" << transferAmount << " < 0.0001)" << std::endl;
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
                
                // DEBUG: Log temperature changes with timestamp
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
                
                float cellMass = cell.mass;
                
                // DEBUG: Log flow attempts for spawned gas
                static int flowDebugCount = 0;
                bool isSpawnedFlow = (cellMass >= 0.01f && cellMass <= 0.5f);  // Recently spawned gas
                if (isSpawnedFlow && flowDebugCount < 10) {
                    std::cout << "[FLOW] Cell(" << x << "," << y << ") mass=" << cellMass << " checking directions..." << std::endl;
                    flowDebugCount++;
                }
                
                // GAS FLOW: Try to move/merge in all directions EXCEPT downward
                // Gas can be PUSHED down by denser gas, but won't intentionally flow down
                // Gas expands upward via SPREADING (buoyancy), flows sideways/merges here
                
                // Priority order: UP, UP-DIAGONAL (random), SIDEWAYS (random)
                // NO intentional downward flow - gas only goes down if pushed
                // Total: 5 directions (up, up-left, up-right, left, right)
                int directions[5][2];
                int dirCount = 0;
                
                // UP first (buoyancy - gas wants to rise)
                directions[dirCount][0] = 0; directions[dirCount][1] = -1; dirCount++;
                
                // UP-DIAGONAL (randomize left/right)
                if (simpleRand() % 2 == 0) {
                    directions[dirCount][0] = -1; directions[dirCount][1] = -1; dirCount++;
                    directions[dirCount][0] = 1; directions[dirCount][1] = -1; dirCount++;
                } else {
                    directions[dirCount][0] = 1; directions[dirCount][1] = -1; dirCount++;
                    directions[dirCount][0] = -1; directions[dirCount][1] = -1; dirCount++;
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
                for (int i = 0; i < dirCount; i++) {
                    int nx = x + directions[i][0];
                    int ny = y + directions[i][1];
                    
                    if (!grid->isValidPosition(nx, ny)) continue;
                    
                    Cell& neighbor = grid->getCell(nx, ny);
                    ElementType neighborType = neighbor.elementType;
                    
                    bool isVacuum = (neighborType == ElementType::Vacuum);
                    bool isSameGas = isGasType(neighborType) && neighborType == cell.elementType;
                    bool isDifferentGas = isGasType(neighborType) && neighborType != cell.elementType;
                    
                    // DEBUG: Show what neighbor type was found
                    if (isSpawnedFlow && flowDebugCount <= 10) {
                        std::cout << "  Dir " << i << ": (" << nx << "," << ny << ") type=" << (int)neighborType 
                                  << " vacuum=" << isVacuum << " sameGas=" << isSameGas << std::endl;
                    }
                    
                    FlowAction action;
                    action.fromX = x; action.fromY = y;
                    action.toX = nx; action.toY = ny;
                    action.massToMove = cellMass;
                    action.temperature = cell.temperature;
                    action.type = cell.elementType;
                    action.isMerge = false;
                    
                    if (isVacuum) {
                        // VACUUM: Always swap (gas moves into vacuum)
                        action.isMerge = false;
                        flowActions.push_back(action);
                        break;  // Found a valid move
                    }
                    else if (isSameGas) {
                        // SAME GAS: Merge if room, otherwise try next direction
                        float spaceAvailable = 2.0f - neighbor.mass;  // 2kg max
                        
                        // Can only merge mass that leaves source above MIN_GAS_MASS
                        float maxMergeAmount = cellMass - MIN_GAS_MASS;
                        if (maxMergeAmount < MIN_GAS_MASS) {
                            // Not enough mass to merge without creating micro-cell
                            continue;
                        }
                        
                        float actualMergeAmount = std::min(cellMass - MIN_GAS_MASS, spaceAvailable);
                        
                        if (actualMergeAmount >= MIN_GAS_MASS && spaceAvailable > 0.001f) {
                            // Merge: push mass that fits and keeps source above threshold
                            action.massToMove = actualMergeAmount;
                            action.isMerge = true;
                            flowActions.push_back(action);
                            break;
                        } else {
                            // No room or can't merge meaningful amount - try next direction
                            continue;
                        }
                    }
                    else if (isDifferentGas) {
                        // DIFFERENT GAS: Swap based on density
                        const Element& cellProps = ElementTypes::getElement(cell.elementType);
                        const Element& neighborProps = ElementTypes::getElement(neighborType);
                        
                        // Lighter gas rises through heavier gas
                        if (cellProps.density < neighborProps.density) {
                            action.isMerge = false;  // Swap, not merge
                            flowActions.push_back(action);
                            break;
                        }
                        // Heavier gas - don't swap, try next direction
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
                float spaceAvailable = 2.0f - toCell.mass;
                float massToMerge = std::min(action.massToMove, spaceAvailable);
                float leftover = action.massToMove - massToMerge;
                
                // Mass-weighted temperature average
                float totalMass = toCell.mass + massToMerge;
                float newTemp = (toCell.mass * toCell.temperature + massToMerge * action.temperature) / totalMass;
                
                toCell.mass += massToMerge;
                toCell.temperature = newTemp;
                toCell.updated = true;
                calculatePressure(action.toX, action.toY);
                
                // Leftover stays in original cell (already guaranteed to be >= MIN_GAS_MASS)
                fromCell.mass = leftover;
                fromCell.updated = true;
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
    // Each cell gets a random decay time (60-120 seconds) when it drops below threshold
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
