#pragma once

#include "Cell.h"
#include <vector>

class Grid {
public:
    Grid();

    void initialize(int width, int height);
    
    Cell& getCell(int x, int y);
    const Cell& getCell(int x, int y) const;
    
    void setCell(int x, int y, const Cell& cell);
    void setCellType(int x, int y, ElementType type);
    
    int getWidth() const;
    int getHeight() const;
    
    bool isValidPosition(int x, int y) const;
    
    void clear();
    void fill(ElementType type);

private:
    int width;
    int height;
    std::vector<std::vector<Cell>> cells;
};
