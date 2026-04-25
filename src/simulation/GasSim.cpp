#include "GasSim.h"
#include "ElementTypes.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

// Debug print helper macro
#define DEBUG_PRINT(msg) do { \
    extern bool g_debugPrintsEnabled; \
    if (g_debugPrintsEnabled) { \
        std::cout << msg << std::endl; \
    } \
} while(0)

// Utility: Calculate maximum mass for an element based on density
float GasSim::getMaxMassForElement(ElementType type) {
    const Element& props = ElementTypes::getElement(type);
    if (props.isGas) {
        return props.density * CELL_VOLUME * GAS_COMPRESSION_MULTIPLIER;
    } else if (props.isLiquid) {
        return props.density * CELL_VOLUME;  // No compression for liquids
    }
    return 0.0f;  // Solids don't have mass limits
}

GasSim::GasSim() 
    : SimulationSystem("GasSim", 0.04f) {  // Update every 40ms (faster spreading for visible movement)
    // Initialize gas types dynamically
    initializeGasTypes();
}

void GasSim::initializeGasTypes() {
    gasTypes.clear();
    
    // Iterate through all ElementType values and check if they're gases
    ElementType allTypes[] = {
        ElementType::Empty, ElementType::Vacuum, ElementType::Solid,
        ElementType::Solid_Ice, ElementType::Solid_DryIce, ElementType::Solid_Oil,
        ElementType::Solid_IndestructibleInsulator, ElementType::Gas_O2,
        ElementType::Gas_Lava, ElementType::Gas_CO2, ElementType::Gas_Oil,
        ElementType::Liquid_Water, ElementType::Liquid_Lava, ElementType::Liquid_Oil,
        ElementType::ContaminatedWater, ElementType::Solid_ContaminatedWater
    };
    
    for (ElementType type : allTypes) {
        const Element& props = ElementTypes::getElement(type);
        if (props.isGas) {
            gasTypes.insert(type);
            std::cout << "[GasSim] Registered gas type: " << props.name << std::endl;
        }
    }
    
    std::cout << "[GasSim] Total gas types: " << gasTypes.size() << std::endl;
}

bool GasSim::update(float deltaTime) {
    if (!enabled || !grid) {
        static bool warned = false;
        if (!warned) {
            std::cerr << "[GAS] WARNING: GasSim DISABLED or no grid! enabled=" << enabled << " grid=" << (grid != nullptr) << std::endl;
            warned = true;
        }
        return false;
    }
    
    // ALWAYS print first tick to confirm GasSim is running
    static bool firstTick = true;
    if (firstTick) {
        std::cerr << "[GAS] GasSim::update() CALLED! enabled=" << enabled << " grid=" << (grid != nullptr) << std::endl;
        firstTick = false;
    }
    
    // Track gas cells found
    static int totalGasCellsFound = 0;
    static int totalOvermassCellsFound = 0;
    
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
    
    // STEP 1: Snapshot current gas state (REQUIRED for STEP 4 flow)
    struct GasCellData {
        ElementType type = ElementType::Vacuum;
        float mass = 0.0f;
        float pressure = 0.0f;
        float temperature = -273.15f;
    };
    
    std::vector<std::vector<GasCellData>> gasSnapshot(height, std::vector<GasCellData>(width));
    int gasCellsFound = 0;
    int gasCellsWithMass = 0;
    int gasCellsWithoutMass = 0;
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            Cell& cell = grid->getCell(x, y);
            if (isGasType(cell.elementType)) {
                gasSnapshot[y][x].type = cell.elementType;
                gasSnapshot[y][x].mass = cell.mass;
                gasSnapshot[y][x].pressure = cell.pressure;
                gasSnapshot[y][x].temperature = cell.temperature;
                gasCellsFound++;
                
                if (cell.mass > 0.0f) {
                    gasCellsWithMass++;
                } else {
                    gasCellsWithoutMass++;
                }
            }
        }
    }
    
    // Print gas summary every 30 ticks
    static int summaryTick = 0;
    summaryTick++;
    if (summaryTick % 30 == 0) {
        std::cerr << "[GAS SUMMARY] Total=" << gasCellsFound 
                  << " WithMass=" << gasCellsWithMass
                  << " NoMass=" << gasCellsWithoutMass << std::endl;
    }
    
    // ========== OLD EXPANSION SYSTEM - DISABLED (replaced by new STEP 4 flow algorithm below) ==========
    #if 0
    // NOTE: gasSnapshot already declared above, reusing it here
    
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
    
    // DEBUG: Track loop execution
    static int loopDebugTick = 0;
    extern bool g_debugPrintsEnabled;
    
    if (g_debugPrintsEnabled && loopDebugTick < 5) {
        std::cout << "[GAS] ===== GasSim update tick " << loopDebugTick << " =====" << std::endl;
        loopDebugTick++;
    } else if (!g_debugPrintsEnabled) {
        loopDebugTick = 0;  // Reset when debug is off
    }
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!grid->isValidPosition(x, y)) continue;
            
            // GAS CELLS: ALWAYS spread to conserve mass (no LOD skipping!)
            // Mass conservation is critical - skipping spread causes mass loss
            // LOD is only applied in the FLOW step below
            
            // Only process gas cells from snapshot
            if (!isGasType(gasSnapshot[y][x].type)) continue;
            
            // DEBUG: Confirm we're processing gas cells
            static int processDebugCount = 0;
            if (processDebugCount < 5) {
                std::cerr << "[GAS LOOP] Processing gas cell at (" << x << "," << y << ") type=" 
                          << static_cast<int>(gasSnapshot[y][x].type) << " mass=" << currentMass << std::endl;
                processDebugCount++;
            }
            
            totalGasCellsFound++;
            
            float currentMass = gasSnapshot[y][x].mass;
            float currentPressure = gasSnapshot[y][x].pressure;
            
            float maxMass = getMaxMassForElement(gasSnapshot[y][x].type);
            if (currentMass > maxMass) {
                totalOvermassCellsFound++;
            }
            
            // DEBUG: Print gas cells when debug is enabled
            extern bool g_debugPrintsEnabled;
            if (g_debugPrintsEnabled) {
                static int printCount = 0;
                if (printCount < 50) {
                    printCount++;
                    Cell& liveCell = grid->getCell(x, y);
                    std::cout << "[GAS] Processing (" << x << "," << y << ") snap=" << currentMass 
                              << " live=" << liveCell.mass << " type=" << static_cast<int>(gasSnapshot[y][x].type) << std::endl;
                }
            }
            
            // CRITICAL FIX: Check LIVE grid for over-mass gas (may have been merged by FluidSim)
            Cell& liveCell = grid->getCell(x, y);
            bool hasLiveOvermass = isGasType(liveCell.elementType) && liveCell.mass > getMaxMassForElement(liveCell.elementType);
            bool hasSnapshotOvermass = currentMass > maxMass;
            
            if (isGasType(liveCell.elementType) && liveCell.mass > currentMass) {
                // FluidSim merged gas into this cell - use live mass instead
                currentMass = liveCell.mass;
            }
            
            // DEBUG: Show gas cells that are over-mass in EITHER snapshot or live
            extern bool g_debugPrintsEnabled;
            if (g_debugPrintsEnabled && (hasSnapshotOvermass || hasLiveOvermass)) {
                std::cout << "[GAS] Cell (" << x << "," << y << ") snap=" << gasSnapshot[y][x].mass 
                          << " live=" << liveCell.mass << " using=" << currentMass 
                          << " snap_over=" << (hasSnapshotOvermass ? "Y" : "N")
                          << " live_over=" << (hasLiveOvermass ? "Y" : "N") << std::endl;
            }
            
            // DEBUG: Track spawned gas cells
            if (g_debugPrintsEnabled) {
                static int spawnDebugCount = 0;
                bool isSpawnedDebug = (gasSnapshot[y][x].mass >= 0.19f && gasSnapshot[y][x].mass <= 0.21f);
                if (isSpawnedDebug) {
                    spawnDebugCount++;
                    if (spawnDebugCount <= 10) {  // First 10 only
                        std::cout << "[GAS DEBUG] SPAWNED gas at (" << x << "," << y 
                                  << ") mass=" << gasSnapshot[y][x].mass 
                                  << " pressure=" << gasSnapshot[y][x].pressure
                                  << " temp=" << gasSnapshot[y][x].temperature << std::endl;
                    }
                }
            }
            
            // PERFORMANCE OPTIMIZATION: Only expand (spread mass) if over max capacity
            // Gas cells at or below max mass skip expensive spreading calculation
            // They will still float and merge in STEP 4
            if (currentMass > maxMass) {
                // DEBUG: Track over-compressed gas
                static int overmassDebugCount = 0;
                overmassDebugCount++;
                if (overmassDebugCount <= 20) {
                    std::cout << "[GAS DEBUG] Overmass gas at (" << x << "," << y 
                              << ") mass=" << currentMass << std::endl;
                }
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
    #endif  // END OF OLD EXPANSION SYSTEM - DISABLED
    
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
            bool isOvermassExpansion;  // true = from STEP 5 over-mass expansion (allow exceeding MAX)
        };
        
        std::vector<FlowAction> flowActions;
        flowActions.reserve(width * height / 4);  // Reserve space for efficiency
        
        // Track which cells are already involved in merges or swaps to prevent conflicts
        std::vector<std::vector<bool>> isMergeSource(height, std::vector<bool>(width, false));
        std::vector<std::vector<bool>> isMergeTarget(height, std::vector<bool>(width, false));
        std::vector<std::vector<bool>> isSwapInvolved(height, std::vector<bool>(width, false));  // NEW: Track swaps
        
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
                
                // No minimum mass threshold - let cells fix themselves through natural simulation
                
                // Define upward directions: UP, UP-LEFT, UP-RIGHT
                int upDirections[3][2] = {{0, -1}, {-1, -1}, {1, -1}};
                int upDirCount = 3;
                
                // ============ STEP 1: MOVE upward - SWAP into vacuum ============
                // CRITICAL: Over-mass cells should NOT move upward - they need to expand downward
                bool movedUpward = false;
                
                // Only allow upward movement for non-over-mass cells
                float maxMass = getMaxMassForElement(cell.elementType);
                if (cellMass <= maxMass) {
                
                // Small chance to try sideways movement instead of upward (natural spreading)
                static unsigned int lateralMoveSeed = 77777;
                lateralMoveSeed = lateralMoveSeed * 1103515245 + 12345;
                int lateralRoll = (lateralMoveSeed / 65536) % 100;
                bool tryLateralMove = (lateralRoll < 15);  // 15% chance to try sideways
                
                if (tryLateralMove) {
                    // Try to move LEFT or RIGHT into vacuum
                    int sideDirections[2][2] = {{-1, 0}, {1, 0}};
                    int chosenSide = (simpleRand() % 2);  // Random: 0=left, 1=right
                    
                    int nx = x + sideDirections[chosenSide][0];
                    int ny = y + sideDirections[chosenSide][1];
                    
                    if (grid->isValidPosition(nx, ny)) {
                        Cell& neighbor = grid->getCell(nx, ny);
                        if (neighbor.elementType == ElementType::Vacuum) {
                            // Check if either cell is already involved in an action
                            if (isSwapInvolved[y][x] || isSwapInvolved[ny][nx] ||
                                isMergeSource[y][x] || isMergeTarget[y][x] ||
                                isMergeSource[ny][nx] || isMergeTarget[ny][nx]) {
                                continue;  // Skip, already reserved
                            }
                            
                            FlowAction action;
                            action.fromX = x; action.fromY = y;
                            action.toX = nx; action.toY = ny;
                            action.massToMove = cellMass;
                            action.temperature = cell.temperature;
                            action.type = cell.elementType;
                            action.isMerge = false;  // SWAP
                            action.isOvermassExpansion = false;
                            flowActions.push_back(action);
                            
                            // Mark as reserved
                            isSwapInvolved[y][x] = true;
                            isSwapInvolved[ny][nx] = true;
                            
                            movedUpward = true;  // Treat as moved (skip rest of steps)
                        }
                    }
                }
                
                // If didn't move sideways, try upward movement
                if (!movedUpward) {
                    for (int i = 0; i < upDirCount; i++) {
                        int nx = x + upDirections[i][0];
                        int ny = y + upDirections[i][1];
                        
                        if (!grid->isValidPosition(nx, ny)) continue;
                        
                        Cell& neighbor = grid->getCell(nx, ny);
                        if (neighbor.elementType == ElementType::Vacuum) {
                            // Check if either cell is already involved in an action
                            if (isSwapInvolved[y][x] || isSwapInvolved[ny][nx] ||
                                isMergeSource[y][x] || isMergeTarget[y][x] ||
                                isMergeSource[ny][nx] || isMergeTarget[ny][nx]) {
                                continue;  // Skip, already reserved
                            }
                            
                            FlowAction action;
                            action.fromX = x; action.fromY = y;
                            action.toX = nx; action.toY = ny;
                            action.massToMove = cellMass;
                            action.temperature = cell.temperature;
                            action.type = cell.elementType;
                            action.isMerge = false;  // SWAP
                            action.isOvermassExpansion = false;
                            flowActions.push_back(action);
                            
                            // Mark as reserved
                            isSwapInvolved[y][x] = true;
                            isSwapInvolved[ny][nx] = true;
                            
                            movedUpward = true;
                            break;  // Done with this cell
                        }
                        // GAS BUOYANCY: If neighbor is a heavier gas, swap with it
                        else if (isGasType(neighbor.elementType) && neighbor.elementType != cell.elementType) {
                            const Element& cellProps = ElementTypes::getElement(cell.elementType);
                            const Element& neighborProps = ElementTypes::getElement(neighbor.elementType);
                            
                            // If this gas is lighter than the neighbor above, swap (buoyancy)
                            if (cellProps.density < neighborProps.density) {
                                // Check if either cell is already involved
                                if (isSwapInvolved[y][x] || isSwapInvolved[ny][nx] ||
                                    isMergeSource[y][x] || isMergeTarget[y][x] ||
                                    isMergeSource[ny][nx] || isMergeTarget[ny][nx]) {
                                    continue;  // Skip, already reserved
                                }
                                
                                FlowAction action;
                                action.fromX = x; action.fromY = y;
                                action.toX = nx; action.toY = ny;
                                action.massToMove = cellMass;
                                action.temperature = cell.temperature;
                                action.type = cell.elementType;
                                action.isMerge = false;  // SWAP
                                action.isOvermassExpansion = false;
                                flowActions.push_back(action);
                                
                                // Mark as reserved
                                isSwapInvolved[y][x] = true;
                                isSwapInvolved[ny][nx] = true;
                                
                                movedUpward = true;
                                break;  // Done with this cell
                            }
                        }
                    }
                }  // End if (!movedUpward)
                }  // End if cellMass <= maxMass (STEP 1)
                
                if (movedUpward) continue;
                
                // ============ STEP 2: Wait 4 ticks before merging ============
                // Simulate settling time with randomness (20% chance to merge each tick)
                // This gives gas ~4 ticks on average to flow naturally before merging
                static unsigned int settleRandomSeed = 99999;
                settleRandomSeed = settleRandomSeed * static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()) + 12345;
                int settleRoll = (settleRandomSeed / 65536) % 100;
                bool shouldMerge = (settleRoll < 20);  // 20% chance = ~4 tick average
                
                if (!shouldMerge) {
                    // Not settling yet, skip merging this tick
                    continue;
                }
                
                // ============ STEP 3: MERGE upward ============
                // Transfer mass to same gas above until above gas is at max
                // CRITICAL: Over-mass cells should NOT merge upward - they need to expand downward instead
                bool mergedUpward = false;
                
                // Skip upward merge if cell is over-mass (let STEP 5 handle downward expansion)
                float maxMass2 = getMaxMassForElement(cell.elementType);
                if (cellMass <= maxMass2) {
                    for (int i = 0; i < upDirCount; i++) {
                        int nx = x + upDirections[i][0];
                        int ny = y + upDirections[i][1];
                        
                        if (!grid->isValidPosition(nx, ny)) continue;
                        
                        Cell& neighbor = grid->getCell(nx, ny);
                        
                        // Can only merge into same gas type
                        if (!isGasType(neighbor.elementType) || neighbor.elementType != cell.elementType) {
                            continue;
                        }
                        
                        // Merge if neighbor has space
                        float neighborMaxMass = getMaxMassForElement(neighbor.elementType);
                        if (neighbor.mass < neighborMaxMass) {
                            // RESERVE PARTNER FIRST
                            if (isMergeSource[y][x] || isMergeTarget[y][x] || 
                                isMergeSource[ny][nx] || isMergeTarget[ny][nx]) {
                                continue;
                            }
                            
                            // Transfer enough to fill target or empty source
                            float spaceAvailable = neighborMaxMass - neighbor.mass;
                            float transferAmount = std::min(cellMass, spaceAvailable);
                            
                            if (transferAmount > 0) {
                                // RESERVE IMMEDIATELY
                                isMergeSource[y][x] = true;
                                isMergeTarget[ny][nx] = true;
                                
                                FlowAction action;
                                action.fromX = x; action.fromY = y;
                                action.toX = nx; action.toY = ny;
                                action.massToMove = transferAmount;
                                action.temperature = cell.temperature;
                                action.type = cell.elementType;
                                action.isMerge = true;
                                action.isOvermassExpansion = false;
                                flowActions.push_back(action);
                                
                                mergedUpward = true;
                                break;  // Done with this cell
                            }
                        }
                    }  // End for loop
                }  // End if cellMass <= maxMass2
                
                if (mergedUpward) continue;
                
                // ============ STEP 4: MERGE sideways (low→high mass) ============
                int sideDirections[2][2] = {{-1, 0}, {1, 0}};
                bool mergedSideways = false;
                
                for (int i = 0; i < 2; i++) {
                    int nx = x + sideDirections[i][0];
                    int ny = y + sideDirections[i][1];
                    
                    if (!grid->isValidPosition(nx, ny)) continue;
                    
                    Cell& neighbor = grid->getCell(nx, ny);
                    
                    // Only merge with same gas type
                    if (!isGasType(neighbor.elementType) || neighbor.elementType != cell.elementType) {
                        continue;
                    }
                    
                    // Low-to-high merge: only if neighbor has MORE mass
                    float neighborMaxMass2 = getMaxMassForElement(neighbor.elementType);
                    if (neighbor.mass > cellMass && neighbor.mass < neighborMaxMass2) {
                        // RESERVE PARTNER
                        if (!isMergeSource[y][x] && !isMergeTarget[y][x] && 
                            !isMergeSource[ny][nx] && !isMergeTarget[ny][nx]) {
                            
                            float spaceAvailable = neighborMaxMass2 - neighbor.mass;
                            float transferAmount = std::min(cellMass, spaceAvailable);
                            
                            if (transferAmount > 0) {
                                // RESERVE IMMEDIATELY
                                isMergeSource[y][x] = true;
                                isMergeTarget[ny][nx] = true;
                                
                                FlowAction action;
                                action.fromX = x; action.fromY = y;
                                action.toX = nx; action.toY = ny;
                                action.massToMove = transferAmount;
                                action.temperature = cell.temperature;
                                action.type = cell.elementType;
                                action.isMerge = true;
                                action.isOvermassExpansion = false;
                                flowActions.push_back(action);
                                
                                mergedSideways = true;
                                break;  // Done with this cell
                            }
                        }
                    }
                }
                
                if (mergedSideways) continue;
                
                // ============ STEP 5: OVER-MASS GAS EXPANDS IN ALL 8 DIRECTIONS ============
                // When gas is compressed beyond its max capacity, distribute excess mass to ALL
                // available neighbors (vacuum or same gas), prioritizing downward directions.
                float maxMass3 = getMaxMassForElement(cell.elementType);
                if (cellMass > maxMass3) {
                    float excessMass = cellMass - maxMass3;
                    float remainingExcess = excessMass;
                    
                    // Define all 8 directions in priority order: DOWN(3), SIDES(2), UP(3)
                    int allDirections[8][2] = {
                        {0, 1}, {-1, 1}, {1, 1},   // DOWN, DOWN-LEFT, DOWN-RIGHT (priority 1)
                        {-1, 0}, {1, 0},            // LEFT, RIGHT (priority 2)
                        {0, -1}, {-1, -1}, {1, -1}  // UP, UP-LEFT, UP-RIGHT (priority 3)
                    };
                    
                    // Count available neighbors first
                    struct NeighborInfo {
                        int x, y;
                        bool isVacuum;
                        bool isSameGas;
                        float spaceAvailable;
                    };
                    std::vector<NeighborInfo> availableNeighbors;
                    
                    for (int i = 0; i < 8; i++) {
                        int nx = x + allDirections[i][0];
                        int ny = y + allDirections[i][1];
                        
                        if (!grid->isValidPosition(nx, ny)) continue;
                        
                        // Check if already involved in an action
                        if (isSwapInvolved[y][x] || isSwapInvolved[ny][nx] ||
                            isMergeSource[y][x] || isMergeTarget[y][x] ||
                            isMergeSource[ny][nx] || isMergeTarget[ny][nx]) {
                            continue;
                        }
                        
                        Cell& neighbor = grid->getCell(nx, ny);
                        
                        if (neighbor.elementType == ElementType::Vacuum) {
                            availableNeighbors.push_back({nx, ny, true, false, 999999.0f});
                        } else if (isGasType(neighbor.elementType) && neighbor.elementType == cell.elementType) {
                            // For same gas, calculate how much space (allow exceeding max for cascade)
                            float neighborMaxMass4 = getMaxMassForElement(neighbor.elementType);
                            float space = std::max(0.0f, neighbor.mass < neighborMaxMass4 ? 
                                                  (neighborMaxMass4 - neighbor.mass) : excessMass);
                            availableNeighbors.push_back({nx, ny, false, true, space});
                        }
                    }
                    
                    // Distribute excess mass across all available neighbors
                    if (!availableNeighbors.empty() && remainingExcess > 0.0f) {
                        float massPerNeighbor = remainingExcess / availableNeighbors.size();
                        
                        for (const auto& neighbor : availableNeighbors) {
                            if (remainingExcess <= 0.0f) break;
                            
                            float massToPush = std::min(massPerNeighbor, remainingExcess);
                            
                            FlowAction action;
                            action.fromX = x; action.fromY = y;
                            action.toX = neighbor.x; action.toY = neighbor.y;
                            action.massToMove = massToPush;
                            action.temperature = cell.temperature;
                            action.type = cell.elementType;
                            action.isMerge = true;
                            action.isOvermassExpansion = true;  // Mark as over-mass expansion
                            flowActions.push_back(action);
                            
                            isMergeSource[y][x] = true;
                            isMergeTarget[neighbor.y][neighbor.x] = true;
                            
                            remainingExcess -= massToPush;
                        }
                        
                        static int overmassDebugCount = 0;
                        if (overmassDebugCount < 10) {
                            std::cerr << "[GAS EXPAND] Over-mass (" << cellMass << "kg) distributing " 
                                      << excessMass << "kg to " << availableNeighbors.size() 
                                      << " neighbors, keeping " << maxMass3 << "kg" << std::endl;
                            overmassDebugCount++;
                        }
                    }
                }
                
                // ============ STEP 6: MOVE sideways - SWAP into vacuum ============
                // Only if blocked on both sides (can't merge)
                bool blockedLeft = false;
                bool blockedRight = false;
                
                if (grid->isValidPosition(x - 1, y)) {
                    Cell& leftNeighbor = grid->getCell(x - 1, y);
                    if (leftNeighbor.elementType != ElementType::Vacuum) {
                        blockedLeft = true;
                    }
                } else {
                    blockedLeft = true;  // Edge of grid
                }
                
                if (grid->isValidPosition(x + 1, y)) {
                    Cell& rightNeighbor = grid->getCell(x + 1, y);
                    if (rightNeighbor.elementType != ElementType::Vacuum) {
                        blockedRight = true;
                    }
                } else {
                    blockedRight = true;  // Edge of grid
                }
                
                // If blocked on BOTH sides, can't swap sideways
                if (blockedLeft && blockedRight) {
                    // Do nothing this tick
                    continue;
                }
                
                // Can swap sideways into vacuum
                int chosenDir = -1;
                if (!blockedLeft && !blockedRight) {
                    // Both sides open - random choice
                    chosenDir = (simpleRand() % 2 == 0) ? 0 : 1;
                } else if (!blockedLeft) {
                    chosenDir = 0;  // Left
                } else {
                    chosenDir = 1;  // Right
                }
                
                int nx = x + sideDirections[chosenDir][0];
                int ny = y + sideDirections[chosenDir][1];
                
                // Check if either cell is already involved in an action
                if (isSwapInvolved[y][x] || isSwapInvolved[ny][nx] ||
                    isMergeSource[y][x] || isMergeTarget[y][x] ||
                    isMergeSource[ny][nx] || isMergeTarget[ny][nx]) {
                    continue;  // Skip, already reserved
                }
                
                FlowAction action;
                action.fromX = x; action.fromY = y;
                action.toX = nx; action.toY = ny;
                action.massToMove = cellMass;
                action.temperature = cell.temperature;
                action.type = cell.elementType;
                action.isMerge = false;  // SWAP
                action.isOvermassExpansion = false;
                flowActions.push_back(action);
                
                // Mark as reserved
                isSwapInvolved[y][x] = true;
                isSwapInvolved[ny][nx] = true;
            }
        }
        
        // Apply all flow actions simultaneously
        static int applyDebugCount = 0;
        for (const auto& action : flowActions) {
            Cell& fromCell = grid->getCell(action.fromX, action.fromY);
            Cell& toCell = grid->getCell(action.toX, action.toY);
            
            // DEBUG: Log first few flow applications
            if (applyDebugCount < 10) {
                DEBUG_PRINT("[APPLY FLOW] " << (action.isMerge ? "MERGE" : "SWAP") 
                              << " (" << action.fromX << "," << action.fromY << ")->(" 
                              << action.toX << "," << action.toY << ") mass=" << action.massToMove);
                applyDebugCount++;
            }
            
            if (action.isMerge) {
                // MERGE: Push mass into target, leftover stays
                // CRITICAL: Allow exceeding MAX_GAS_MASS for over-mass expansion actions
                
                float massToMerge;
                float leftover;
                
                if (action.isOvermassExpansion) {
                    // Over-mass expansion: allow target to exceed MAX_GAS_MASS
                    massToMerge = action.massToMove;
                    leftover = 0.0f;
                } else {
                    // Regular merge: respect max mass limit
                    float targetMaxMass = getMaxMassForElement(toCell.elementType);
                    float spaceAvailable = targetMaxMass - toCell.mass;
                    massToMerge = std::min(action.massToMove, spaceAvailable);
                    leftover = action.massToMove - massToMerge;
                }
                
                // DEBUG: Log merge details for tracking
                DEBUG_PRINT("[MERGE APPLY] " << (action.isMerge ? "MERGE" : "SWAP")
                          << " (" << action.fromX << "," << action.fromY << ")->("
                          << action.toX << "," << action.toY << ") mass=" << action.massToMove 
                          << " targetMass=" << toCell.mass
                          << " overmass=" << (action.isOvermassExpansion ? "YES" : "NO")
                          << " merging=" << massToMerge << " leftover=" << leftover);
                
                // CRITICAL: If target is vacuum, convert it to gas type first
                if (toCell.elementType == ElementType::Vacuum && massToMerge > 0.0f) {
                    toCell.elementType = action.type;
                    toCell.temperature = action.temperature;  // Prevent vacuum's -273.15°C from contaminating merge
                }
                
                // Mass-weighted temperature average
                float totalMass = toCell.mass + massToMerge;
                float newTemp = (toCell.mass * toCell.temperature + massToMerge * action.temperature) / totalMass;
                
                toCell.mass += massToMerge;
                toCell.temperature = newTemp;
                toCell.updated = true;
                toCell.updateColor();
                calculatePressure(action.toX, action.toY);
                
                // Handle leftover in source cell
                if (action.isOvermassExpansion) {
                    // Over-mass expansion: source should keep its max mass
                    float sourceMaxMass = getMaxMassForElement(fromCell.elementType);
                    fromCell.mass = sourceMaxMass;
                    fromCell.updated = true;
                    fromCell.updateColor();
                    calculatePressure(action.fromX, action.fromY);
                } else if (leftover < 0.00001f) {
                    // DEBUG: Track when high-mass cells are converted to vacuum
                    DEBUG_PRINT("[VACUUM CONVERT] Cell (" << action.fromX << "," << action.fromY 
                              << ") converted to vacuum, original mass=" << action.massToMove 
                              << " leftover=" << leftover);
                    
                    // Cell is EMPTY - convert to vacuum with proper cleared data
                    fromCell.elementType = ElementType::Vacuum;
                    fromCell.mass = 0.0f;
                    fromCell.temperature = -273.15f;  // Absolute zero
                    fromCell.pressure = 0.0f;
                    fromCell.velocityX = 0.0f;
                    fromCell.velocityY = 0.0f;
                    fromCell.updateColor();  // Use proper vacuum color from updateColor()
                    fromCell.updated = true;
                } else {
                    // Cell still has mass - keep as gas
                    fromCell.mass = leftover;
                    fromCell.updated = true;
                    fromCell.updateColor();
                }
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
    /*
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
    */
    // Decay section commented out above
    
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
    
    // Print gas cell summary
    static int summaryCounter = 0;
    summaryCounter++;
    if (summaryCounter % 60 == 0) {  // Every 60 ticks (~1 second)
        std::cerr << "[GAS SUMMARY] Tick " << summaryCounter << ": Found " << totalGasCellsFound 
                  << " gas cells, " << totalOvermassCellsFound << " over-mass" << std::endl;
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
    return gasTypes.count(type) > 0;
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
