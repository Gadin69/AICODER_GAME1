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
    for (int y = 0; y < height; ++y) {
        cells[y].resize(width);
    }
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
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            cells[y][x] = Cell();
        }
    }
}

void Grid::fill(ElementType type) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            cells[y][x].elementType = type;
            cells[y][x].updateColor();
        }
    }
}
