#pragma once

#include "Cell.h"
#include <vector>
#include <mutex>
#include <string>

class Grid {
public:
    Grid();

    void initialize(int width, int height);
    
    // Thread-safe cell access
    Cell& getCell(int x, int y);
    const Cell& getCell(int x, int y) const;
    
    void setCell(int x, int y, const Cell& cell);
    void setCellType(int x, int y, ElementType type);
    
    int getWidth() const;
    int getHeight() const;
    
    bool isValidPosition(int x, int y) const;
    
    void clear();
    void fill(ElementType type);
    
    // DOUBLE-BUFFERING for thread-safe parallel simulation
    void swapBuffers();  // Swap read/write buffers
    Grid& getWriteBuffer();  // Get buffer for writing
    const Grid& getReadBuffer() const;  // Get buffer for reading
    
    // Mutex for thread synchronization
    std::mutex& getMutex() { return gridMutex; }
    
    // Thread-safe operations for main thread (mouse input, etc.)
    void lock() { gridMutex.lock(); }
    void unlock() { gridMutex.unlock(); }
    
    // Save/Load functionality
    bool saveToFile(const std::string& filePath) const;
    bool loadFromFile(const std::string& filePath);

private:
    int width;
    int height;
    std::vector<std::vector<Cell>> cells;
    
    // Double buffering
    std::vector<std::vector<Cell>> writeBuffer;  // Secondary buffer for parallel writes
    int currentBuffer;  // 0 = cells is read buffer, 1 = writeBuffer is read buffer
    std::mutex gridMutex;
};
