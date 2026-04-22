#include "Grid.h"

Grid::Grid()
    : width(0)
    , height(0)
{
}

void Grid::initialize(int w, int h) {
    width = w;
    height = h;
    cells.resize(height);
    writeBuffer.resize(height);
    
    for (int y = 0; y < height; ++y) {
        cells[y].resize(width);
        writeBuffer[y].resize(width);
    }
    
    currentBuffer = 0;
    
    // Fill entire grid with VACUUM (no empty space in universe!)
    fill(ElementType::Vacuum);
}

Cell& Grid::getCell(int x, int y) {
    return cells[y][x];
}

const Cell& Grid::getCell(int x, int y) const {
    return cells[y][x];
}

void Grid::setCell(int x, int y, const Cell& cell) {
    if (isValidPosition(x, y)) {
        cells[y][x] = cell;
    }
}

void Grid::setCellType(int x, int y, ElementType type) {
    if (isValidPosition(x, y)) {
        cells[y][x].elementType = type;
        cells[y][x].updateColor();
    }
}

int Grid::getWidth() const {
    return width;
}

int Grid::getHeight() const {
    return height;
}

bool Grid::isValidPosition(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
}

void Grid::clear() {
    // Clear to VACUUM, not empty
    fill(ElementType::Vacuum);
}

void Grid::fill(ElementType type) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            cells[y][x].elementType = type;
            cells[y][x].updateColor();
            writeBuffer[y][x].elementType = type;
            writeBuffer[y][x].updateColor();
        }
    }
}

void Grid::swapBuffers() {
    // Swap the buffers (atomic pointer swap)
    std::swap(cells, writeBuffer);
    currentBuffer = 1 - currentBuffer;
}

Grid& Grid::getWriteBuffer() {
    // Return the buffer that's NOT currently being read
    return (currentBuffer == 0) ? *this : *this;  // Simplified for now
}

const Grid& Grid::getReadBuffer() const {
    return *this;
}
