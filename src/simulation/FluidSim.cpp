#include "FluidSim.h"
#include "ElementTypes.h"
#include <random>
#include <cmath>

FluidSim::FluidSim()
    : updateTimer(0.0f)
    , updateInterval(0.05f)
{
}

void FluidSim::initialize(Grid& grid) {
    updateTimer = 0.0f;
    cameraTileX = grid.getWidth() / 2;
    cameraTileY = grid.getHeight() / 2;
    lastUpdateTime = 0.0f;
    
    // Initialize heat map
    heatMap.initialize(grid.getWidth(), grid.getHeight());
    
    // Initialize lastSeenTime array
    lastSeenTime.resize(grid.getHeight());
    for (int y = 0; y < grid.getHeight(); ++y) {
        lastSeenTime[y].resize(grid.getWidth(), 0.0f);
    }
}

void FluidSim::setCameraPosition(int camX, int camY) {
    cameraTileX = camX;
    cameraTileY = camY;
}

void FluidSim::setLODConfig(float nearRange, float midRange, float farRange, float veryFarRange) {
    lodConfig.nearRange = nearRange;
    lodConfig.midRange = midRange;
    lodConfig.farRange = farRange;
    lodConfig.veryFarRange = veryFarRange;
}

void FluidSim::setCatchUpConfig(float catchUpThreshold, int maxCatchUpSteps) {
    catchUpConfig.timeThreshold = catchUpThreshold;
    catchUpConfig.maxSteps = maxCatchUpSteps;
}

int FluidSim::getUpdatePriority(int x, int y) const {
    // Calculate Manhattan distance from camera
    int distX = std::abs(x - cameraTileX);
    int distY = std::abs(y - cameraTileY);
    int distance = distX + distY;
    
    // LOD tiers (configurable):
    if (distance <= lodConfig.nearRange) return 1;
    if (distance <= lodConfig.midRange) return 2;
    if (distance <= lodConfig.farRange) return 4;
    if (distance <= lodConfig.veryFarRange) return 8;
    return 16;
}

void FluidSim::update(Grid& grid, float deltaTime) {
    updateTimer += deltaTime;
    lastUpdateTime += deltaTime;  // Track real time
    
    if (updateTimer < updateInterval) {
        return;
    }
    
    updateTimer = 0.0f;
    globalTick++;  // Increment global tick counter

    // Reset updated flags
    for (int y = 0; y < grid.getHeight(); ++y) {
        for (int x = 0; x < grid.getWidth(); ++x) {
            grid.getCell(x, y).updated = false;
        }
    }
    
    // Update heat map with current cell temperatures
    std::vector<std::vector<float>> cellTemps(grid.getHeight(), std::vector<float>(grid.getWidth()));
    for (int y = 0; y < grid.getHeight(); ++y) {
        for (int x = 0; x < grid.getWidth(); ++x) {
            cellTemps[y][x] = grid.getCell(x, y).temperature;
        }
    }
    heatMap.update(cellTemps, deltaTime);

    // Update from bottom to top for proper fluid behavior
    for (int y = grid.getHeight() - 1; y >= 0; --y) {
        // Alternate left-to-right and right-to-left for variation
        bool leftToRight = (y % 2 == 0);
        for (int i = 0; i < grid.getWidth(); ++i) {
            int x = leftToRight ? i : (grid.getWidth() - 1 - i);
            
            Cell& cell = grid.getCell(x, y);
            if (cell.updated || cell.elementType == ElementType::Empty) {
                continue;
            }
            
            // Check if this cell needs time-lapse catch-up
            if (needsTimeCatchUp(x, y, lastUpdateTime)) {
                float missedTime = lastUpdateTime - lastSeenTime[y][x];
                catchUpCell(grid, x, y, missedTime);
                lastSeenTime[y][x] = lastUpdateTime;  // Update last seen time
            }
            
            // LOD check: Skip update if this cell's priority says to wait
            int priority = getUpdatePriority(x, y);
            if (globalTick % priority != 0) {
                continue;  // Skip this cell, update next tick
            }
            
            // Update last seen time for cells in view
            lastSeenTime[y][x] = lastUpdateTime;

            // Check for element interactions first (can transform solids!)
            checkElementInteractions(grid, x, y);
            
            // Re-check cell type after interactions (may have changed)
            if (cell.updated) continue;
            
            ElementProperties props = ElementTypes::getProperties(cell.elementType);
            
            // Handle all element types - including solids that can melt!
            if (props.isGas) {
                updateGas(grid, x, y);
            } else if (props.isLiquid) {
                updateLiquid(grid, x, y);
            } else if (props.isSolid) {
                updateSolid(grid, x, y);  // Solids can melt/transform!
            }
            
            cell.updateColor();  // Update color after movement/interactions
        }
    }
}

void FluidSim::updateGas(Grid& grid, int x, int y) {
    Cell& cell = grid.getCell(x, y);
    
    // Update temperature FIRST (needed for phase change checks)
    updateTemperature(grid, x, y);
    
    // Altitude-based cooling: higher = cooler (simulates atmosphere)
    int height = grid.getHeight();
    float altitudeFactor = (float)(height - y) / (float)height;  // 0 at bottom, 1 at top
    cell.temperature -= altitudeFactor * 3.0f;  // More cooling at higher altitudes
    
    // Check for phase changes - start gradual transitions
    // Gas condenses into water when cold (1.5 seconds)
    if (cell.elementType == ElementType::Gas_O2 && cell.temperature <= WATER_BOIL_TEMP) {
        if (!isTransitioning(cell) || cell.targetElementType != ElementType::Liquid_Water) {
            startPhaseTransition(cell, ElementType::Liquid_Water, 1.5f);
            // Water condenses at boiling point
            cell.temperature = WATER_BOIL_TEMP - 5.0f;  // Just below boiling
        }
    }
    
    // Gas deposits into solid (frost) when very cold (2 seconds)
    if (cell.elementType == ElementType::Gas_O2 && cell.temperature <= WATER_FREEZE_TEMP) {
        if (!isTransitioning(cell) || cell.targetElementType != ElementType::Solid) {
            startPhaseTransition(cell, ElementType::Solid, 2.0f);
        }
    }
    
    // CO2 can freeze into dry ice (2.5 seconds)
    if (cell.elementType == ElementType::Gas_CO2 && cell.temperature <= -78.0f) {
        if (!isTransitioning(cell) || cell.targetElementType != ElementType::Solid) {
            startPhaseTransition(cell, ElementType::Solid, 2.5f);
        }
    }
    
    // Update any ongoing phase transitions
    updatePhaseTransitions(grid, x, y);
    
    // Skip movement if transitioning
    if (isTransitioning(cell)) {
        return;
    }
    
    // Priority 1: Rise straight up
    if (y > 0 && isEmpty(grid, x, y - 1)) {
        swapCells(grid, x, y, x, y - 1);
        return;
    }
    
    // Priority 2: Spread horizontally when blocked above
    // NO diagonal movement to prevent kitty-corner leaking
    bool blockedAbove = (y <= 0) || !isEmpty(grid, x, y - 1);
    
    if (blockedAbove) {
        int dir = (rand() % 2 == 0) ? -1 : 1;
        
        // Try spreading in random direction first
        if (grid.isValidPosition(x + dir, y) && isEmpty(grid, x + dir, y)) {
            swapCells(grid, x, y, x + dir, y);
            return;
        }
        
        // Try other direction
        if (grid.isValidPosition(x - dir, y) && isEmpty(grid, x - dir, y)) {
            swapCells(grid, x, y, x - dir, y);
        }
    }
}

void FluidSim::updateLiquid(Grid& grid, int x, int y) {
    Cell& cell = grid.getCell(x, y);
    
    // Update temperature FIRST (needed for phase change checks)
    updateTemperature(grid, x, y);
    
    // Check for phase changes - start gradual transitions
    // Water boils into steam (takes 2 seconds = 40 ticks at 0.05s interval)
    if (cell.elementType == ElementType::Liquid_Water && cell.temperature >= WATER_BOIL_TEMP) {
        if (!isTransitioning(cell) || cell.targetElementType != ElementType::Gas_O2) {
            startPhaseTransition(cell, ElementType::Gas_O2, 2.0f);  // 2 second transition
            // Steam should be at boiling point, not superheated
            cell.temperature = WATER_BOIL_TEMP + 10.0f;  // Just above boiling
        }
    }
    
    // Lava cools into solid rock (takes 3 seconds = 60 ticks)
    if (cell.elementType == ElementType::Liquid_Lava && cell.temperature <= LAVA_COOL_TEMP) {
        if (!isTransitioning(cell) || cell.targetElementType != ElementType::Solid) {
            startPhaseTransition(cell, ElementType::Solid, 3.0f);  // 3 second transition
        }
    }
    
    // Update any ongoing phase transitions
    updatePhaseTransitions(grid, x, y);
    
    // Skip movement if transitioning
    if (isTransitioning(cell)) {
        return;
    }
    
    // Priority 1: Fall straight down
    if (y < grid.getHeight() - 1 && isEmpty(grid, x, y + 1)) {
        swapCells(grid, x, y, x, y + 1);
        return;
    }
    
    // Priority 2: Spread horizontally (only when blocked below)
    // NO diagonal movement to prevent kitty-corner leaking
    bool blockedBelow = (y >= grid.getHeight() - 1) || !isEmpty(grid, x, y + 1);
    
    if (blockedBelow) {
        int dir = (rand() % 2 == 0) ? -1 : 1;
        
        // Try spreading in random direction first
        if (grid.isValidPosition(x + dir, y) && isEmpty(grid, x + dir, y)) {
            swapCells(grid, x, y, x + dir, y);
            return;
        }
        
        // Try other direction
        if (grid.isValidPosition(x - dir, y) && isEmpty(grid, x - dir, y)) {
            swapCells(grid, x, y, x - dir, y);
        }
    }
}

void FluidSim::updateSolid(Grid& grid, int x, int y) {
    Cell& cell = grid.getCell(x, y);
    
    // Update temperature FIRST (needed for phase change checks)
    updateTemperature(grid, x, y);
    
    // Check for phase changes - start gradual transitions
    // If solid gets hot enough, it melts into lava (3 seconds)
    if (cell.temperature >= SOLID_MELT_TEMP) {
        if (!isTransitioning(cell) || cell.targetElementType != ElementType::Liquid_Lava) {
            startPhaseTransition(cell, ElementType::Liquid_Lava, 3.0f);
        }
    }
    
    // Update any ongoing phase transitions
    updatePhaseTransitions(grid, x, y);
}

void FluidSim::updateTemperature(Grid& grid, int x, int y) {
    Cell& cell = grid.getCell(x, y);
    
    // Get thermal properties based on element type
    float heatCapacity = WATER_HEAT_CAPACITY;  // Default
    if (cell.elementType == ElementType::Liquid_Lava) {
        heatCapacity = LAVA_HEAT_CAPACITY;
    } else if (cell.elementType == ElementType::Liquid_Water) {
        heatCapacity = WATER_HEAT_CAPACITY;
    } else if (cell.elementType == ElementType::Solid) {
        heatCapacity = SOLID_HEAT_CAPACITY;
    } else if (ElementTypes::getProperties(cell.elementType).isGas) {
        heatCapacity = GAS_HEAT_CAPACITY;
    }
    
    // Temperature diffusion with neighbors - THERMODYNAMIC EXCHANGE
    // Each neighbor exchanges heat based on temperature difference AND thermal mass
    float totalHeatExchange = 0.0f;
    float totalWeight = 0.0f;
    
    // Check all 4 neighbors
    int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (auto& neighbor : neighbors) {
        int nx = x + neighbor[0];
        int ny = y + neighbor[1];
        
        if (grid.isValidPosition(nx, ny)) {
            const Cell& neighborCell = grid.getCell(nx, ny);
            
            // Calculate temperature difference
            float tempDiff = neighborCell.temperature - cell.temperature;
            
            // Get neighbor's heat capacity
            float neighborHeatCapacity = WATER_HEAT_CAPACITY;
            if (neighborCell.elementType == ElementType::Liquid_Lava) {
                neighborHeatCapacity = LAVA_HEAT_CAPACITY;
            } else if (neighborCell.elementType == ElementType::Liquid_Water) {
                neighborHeatCapacity = WATER_HEAT_CAPACITY;
            } else if (neighborCell.elementType == ElementType::Solid) {
                neighborHeatCapacity = SOLID_HEAT_CAPACITY;
            } else if (ElementTypes::getProperties(neighborCell.elementType).isGas) {
                neighborHeatCapacity = GAS_HEAT_CAPACITY;
            }
            
            // Heat transfer rate depends on both materials
            // High heat capacity = slower to change but holds more energy
            float transferRate = 1.0f / ((heatCapacity + neighborHeatCapacity) * 0.5f);
            
            // Weight by element interaction
            float weight = transferRate;
            if (neighborCell.elementType == ElementType::Liquid_Lava || 
                cell.elementType == ElementType::Liquid_Lava) {
                weight *= 2.5f;  // Lava exchanges heat faster (high conductivity)
            }
            if (neighborCell.elementType == ElementType::Liquid_Water || 
                cell.elementType == ElementType::Liquid_Water) {
                weight *= 1.2f;  // Water has good thermal contact
            }
            
            // Accumulate heat exchange (weighted by temperature difference)
            totalHeatExchange += tempDiff * weight;
            totalWeight += weight;
        }
    }
    
    // Apply net heat exchange
    if (totalWeight > 0.0f) {
        float avgHeatExchange = totalHeatExchange / totalWeight;
        cell.temperature += avgHeatExchange * 0.3f;  // 30% of exchange per tick
    }
    
    // Environmental cooling/heating (trends toward ambient temperature)
    float envEffect = (AMBIENT_TEMP - cell.temperature) * 0.01f;
    
    // Gases cool faster to environment
    if (ElementTypes::getProperties(cell.elementType).isGas) {
        envEffect *= 3.0f;
    }
    
    // Lava generates internal heat (exothermic reaction)
    if (cell.elementType == ElementType::Liquid_Lava) {
        cell.temperature += 1.0f;  // Slow heat generation
        envEffect *= 0.1f;  // Lava resists environmental cooling
    }
    
    cell.temperature += envEffect;
    
    // Clamp temperature to reasonable range
    cell.temperature = std::max(-100.0f, std::min(2000.0f, cell.temperature));
}

bool FluidSim::isEmpty(const Grid& grid, int x, int y) {
    return grid.getCell(x, y).elementType == ElementType::Empty;
}

bool FluidSim::canMoveTo(const Grid& grid, int x, int y, ElementType element) {
    if (!grid.isValidPosition(x, y)) {
        return false;
    }
    
    const Cell& target = grid.getCell(x, y);
    if (target.elementType == ElementType::Empty) {
        return true;
    }
    
    // Density-based displacement
    ElementProperties sourceProps = ElementTypes::getProperties(element);
    ElementProperties targetProps = ElementTypes::getProperties(target.elementType);
    
    if (sourceProps.isLiquid && targetProps.isGas) {
        return true;
    }
    
    return false;
}

void FluidSim::swapCells(Grid& grid, int x1, int y1, int x2, int y2) {
    Cell temp = grid.getCell(x1, y1);
    grid.setCell(x1, y1, grid.getCell(x2, y2));
    grid.setCell(x2, y2, temp);
    grid.getCell(x1, y1).updated = true;
    grid.getCell(x2, y2).updated = true;
}

void FluidSim::checkElementInteractions(Grid& grid, int x, int y) {
    Cell& cell = grid.getCell(x, y);
    
    // Check all 4 neighbors for interactions
    int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    for (auto& neighbor : neighbors) {
        int nx = x + neighbor[0];
        int ny = y + neighbor[1];
        
        if (!grid.isValidPosition(nx, ny)) continue;
        
        Cell& neighborCell = grid.getCell(nx, ny);
        
        // Lava + Water interaction - REAL THERMODYNAMICS
        if (cell.elementType == ElementType::Liquid_Lava && 
            neighborCell.elementType == ElementType::Liquid_Water) {
            
            ElementProperties lavaProps = ElementTypes::getProperties(ElementType::Liquid_Lava);
            ElementProperties waterProps = ElementTypes::getProperties(ElementType::Liquid_Water);
            
            // Calculate heat transfer based on temperature difference and thermal properties
            float tempDiff = cell.temperature - neighborCell.temperature;
            
            // Heat flow = thermal conductivity × temperature gradient
            float heatTransfer = (lavaProps.thermalConductivity + waterProps.thermalConductivity) * 0.5f * tempDiff;
            
            // Apply heat transfer (lava loses, water gains) - AGGRESSIVE for visible effect
            cell.temperature -= heatTransfer * 0.5f;  // Increased from 0.1 to 0.5
            neighborCell.temperature += heatTransfer * 0.5f;  // Increased from 0.1 to 0.5
            
            // If water is at boiling point, it vaporizes
            if (neighborCell.temperature >= waterProps.boilingPoint) {
                // Water flash-boils (happens instantly with 1000°C+ lava)
                if (!isTransitioning(neighborCell)) {
                    startPhaseTransition(neighborCell, ElementType::Gas_O2, 0.5f);  // Fast transition
                    neighborCell.temperature = 120.0f;  // Superheated steam
                }
            }
            
            // If lava loses enough heat, it solidifies
            if (cell.temperature <= lavaProps.meltingPoint) {
                if (!isTransitioning(cell)) {
                    startPhaseTransition(cell, ElementType::Solid, 2.0f);
                }
            }
            
            cell.updated = true;
            neighborCell.updated = true;
        }
        
        // O2 and CO2 can mix but don't react (density difference already handled)
        // High density liquids sink through low density gases
    }
}

bool FluidSim::needsTimeCatchUp(int x, int y, float currentTime) const {
    // Check if cell hasn't been seen for longer than threshold
    float timeSinceSeen = currentTime - lastSeenTime[y][x];
    return timeSinceSeen > catchUpConfig.timeThreshold;
}

void FluidSim::catchUpCell(Grid& grid, int x, int y, float missedTime) {
    // Calculate how many simulation steps were missed
    int stepsToSimulate = (int)(missedTime / updateInterval);
    
    // Cap catch-up to prevent massive lag spikes
    stepsToSimulate = std::min(stepsToSimulate, catchUpConfig.maxSteps);
    
    // Run simplified simulation for missed time
    for (int i = 0; i < stepsToSimulate; ++i) {
        simulateStep(grid, x, y);
    }
}

void FluidSim::simulateStep(Grid& grid, int x, int y) {
    // Simplified simulation step for catch-up (no interactions, just movement)
    Cell& cell = grid.getCell(x, y);
    
    if (cell.elementType == ElementType::Empty || 
        cell.elementType == ElementType::Solid) {
        return;
    }
    
    ElementProperties props = ElementTypes::getProperties(cell.elementType);
    
    if (props.isGas) {
        // Simple gas rise for catch-up
        if (y > 0 && isEmpty(grid, x, y - 1)) {
            swapCells(grid, x, y, x, y - 1);
        }
    } else if (props.isLiquid) {
        // Simple liquid fall for catch-up
        if (y < grid.getHeight() - 1 && isEmpty(grid, x, y + 1)) {
            swapCells(grid, x, y, x, y + 1);
        }
    }
}

void FluidSim::startPhaseTransition(Cell& cell, ElementType targetType, float transitionTime) {
    cell.targetElementType = targetType;
    cell.phaseTransitionProgress = 0.0f;
    // Speed = 1.0 / (transitionTime / updateInterval)
    // Example: 2 seconds / 0.05s per tick = 40 ticks, so speed = 1/40 = 0.025 per tick
    cell.phaseTransitionSpeed = 1.0f / (transitionTime / updateInterval);
}

void FluidSim::updatePhaseTransitions(Grid& grid, int x, int y) {
    Cell& cell = grid.getCell(x, y);
    
    if (!isTransitioning(cell)) {
        return;
    }
    
    // Update progress
    cell.phaseTransitionProgress += cell.phaseTransitionSpeed;
    
    // Check if transition is complete
    if (cell.phaseTransitionProgress >= 1.0f) {
        cell.elementType = cell.targetElementType;
        cell.targetElementType = ElementType::Empty;
        cell.phaseTransitionProgress = 0.0f;
        cell.phaseTransitionSpeed = 0.0f;
        cell.updated = true;
    }
}

bool FluidSim::isTransitioning(const Cell& cell) const {
    return cell.targetElementType != ElementType::Empty && cell.phaseTransitionProgress > 0.0f;
}
