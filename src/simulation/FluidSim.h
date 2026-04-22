#pragma once

#include "Grid.h"
#include <vector>

class FluidSim {
public:
    FluidSim();

    void initialize(Grid& grid);
    void update(Grid& grid, float deltaTime);

    // Simulation rules
    void updateGas(Grid& grid, int x, int y);
    void updateLiquid(Grid& grid, int x, int y);
    void updateTemperature(Grid& grid, int x, int y);
    void checkElementInteractions(Grid& grid, int x, int y);

    // Helper functions
    bool isEmpty(const Grid& grid, int x, int y);
    bool canMoveTo(const Grid& grid, int x, int y, ElementType element);
    void swapCells(Grid& grid, int x1, int y1, int x2, int y2);

private:
    float updateTimer;
    float updateInterval;
};
