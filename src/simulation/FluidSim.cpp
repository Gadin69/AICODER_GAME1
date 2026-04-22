#include "FluidSim.h"
#include "ElementTypes.h"
#include <random>

FluidSim::FluidSim()
    : updateTimer(0.0f)
    , updateInterval(0.05f)
{
}

void FluidSim::initialize(Grid& grid) {
    updateTimer = 0.0f;
}

void FluidSim::update(Grid& grid, float deltaTime) {
    updateTimer += deltaTime;
    
    if (updateTimer < updateInterval) {
        return;
    }
    
    updateTimer = 0.0f;

    // Reset updated flags
    for (int y = 0; y < grid.getHeight(); ++y) {
        for (int x = 0; x < grid.getWidth(); ++x) {
            grid.getCell(x, y).updated = false;
        }
    }

    // Update from bottom to top for proper fluid behavior
    for (int y = grid.getHeight() - 1; y >= 0; --y) {
        // Alternate left-to-right and right-to-left for variation
        bool leftToRight = (y % 2 == 0);
        for (int i = 0; i < grid.getWidth(); ++i) {
            int x = leftToRight ? i : (grid.getWidth() - 1 - i);
            
            Cell& cell = grid.getCell(x, y);
            if (cell.updated || cell.elementType == ElementType::Empty || 
                cell.elementType == ElementType::Solid) {
                continue;
            }

            ElementProperties props = ElementTypes::getProperties(cell.elementType);
            
            if (props.isGas) {
                updateGas(grid, x, y);
            } else if (props.isLiquid) {
                updateLiquid(grid, x, y);
            }
            
            updateTemperature(grid, x, y);
        }
    }
}

void FluidSim::updateGas(Grid& grid, int x, int y) {
    Cell& cell = grid.getCell(x, y);
    
    // Gas rises
    if (y > 0 && isEmpty(grid, x, y - 1)) {
        swapCells(grid, x, y, x, y - 1);
    } else {
        // Try to move diagonally up
        int dir = (rand() % 2 == 0) ? -1 : 1;
        if (y > 0 && grid.isValidPosition(x + dir, y - 1) && isEmpty(grid, x + dir, y - 1)) {
            swapCells(grid, x, y, x + dir, y - 1);
        }
    }
}

void FluidSim::updateLiquid(Grid& grid, int x, int y) {
    Cell& cell = grid.getCell(x, y);
    
    // Liquid falls
    if (y < grid.getHeight() - 1 && isEmpty(grid, x, y + 1)) {
        swapCells(grid, x, y, x, y + 1);
    } else {
        // Try to move diagonally down
        int dir = (rand() % 2 == 0) ? -1 : 1;
        if (y < grid.getHeight() - 1 && grid.isValidPosition(x + dir, y + 1) && isEmpty(grid, x + dir, y + 1)) {
            swapCells(grid, x, y, x + dir, y + 1);
        } else if (grid.isValidPosition(x - dir, y + 1) && isEmpty(grid, x - dir, y + 1)) {
            swapCells(grid, x, y, x - dir, y + 1);
        } else {
            // Try to spread horizontally
            int dir2 = (rand() % 2 == 0) ? -1 : 1;
            if (grid.isValidPosition(x + dir2, y) && isEmpty(grid, x + dir2, y)) {
                swapCells(grid, x, y, x + dir2, y);
            }
        }
    }
}

void FluidSim::updateTemperature(Grid& grid, int x, int y) {
    Cell& cell = grid.getCell(x, y);
    
    // Simple temperature diffusion
    float totalTemp = cell.temperature;
    int count = 1;
    
    // Check neighbors
    int neighbors[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (auto& neighbor : neighbors) {
        int nx = x + neighbor[0];
        int ny = y + neighbor[1];
        
        if (grid.isValidPosition(nx, ny)) {
            totalTemp += grid.getCell(nx, ny).temperature;
            count++;
        }
    }
    
    // Average temperature
    float avgTemp = totalTemp / count;
    cell.temperature += (avgTemp - cell.temperature) * 0.01f;
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
