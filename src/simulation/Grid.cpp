#include "Grid.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <filesystem>

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
        
        // If setting to Vacuum, reset ALL cell data to vacuum defaults
        if (type == ElementType::Vacuum) {
            cells[y][x].convertToVacuum();
        }
        
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
    return 
    x >= 0 && x < width && y >= 0 && y < height;
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

// Save file header structure (must match SaveManager.cpp)
struct SaveFileHeader {
    char magic[8];           // "ONISAVE\0"
    uint32_t version;         // 1
    uint32_t gridWidth;
    uint32_t gridHeight;
    uint64_t timestamp;       // Unix timestamp
    uint32_t playTimeSeconds;
    uint32_t cellCount;       // Non-vacuum cells
    char saveName[64];        // User-provided or auto-generated
    char notes[256];          // Optional notes
};

bool Grid::saveToFile(const std::string& filePath) const {
    if (width == 0 || height == 0) {
        std::cerr << "[Grid] ERROR: Cannot save uninitialized grid" << std::endl;
        return false;
    }
    
    // Count non-vacuum cells
    uint32_t cellCount = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (cells[y][x].elementType != ElementType::Vacuum) {
                cellCount++;
            }
        }
    }
    
    // Create header
    SaveFileHeader header;
    std::memset(&header, 0, sizeof(header));
    std::memcpy(header.magic, "ONISAVE", 8);
    header.version = 1;
    header.gridWidth = static_cast<uint32_t>(width);
    header.gridHeight = static_cast<uint32_t>(height);
    header.cellCount = cellCount;
    header.timestamp = static_cast<uint64_t>(std::time(nullptr));
    header.playTimeSeconds = 0;  // TODO: Track actual play time
    
    // Extract save name from filename
    std::string stem = filePath;
    size_t lastSlash = stem.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        stem = stem.substr(lastSlash + 1);
    }
    size_t dotPos = stem.find('.');
    if (dotPos != std::string::npos) {
        stem = stem.substr(0, dotPos);
    }
    std::strncpy(header.saveName, stem.c_str(), sizeof(header.saveName) - 1);
    
    // Open file for writing
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Grid] ERROR: Cannot open file for writing: " << filePath << std::endl;
        return false;
    }
    
    // Write header
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    // Write all cell data
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Cell& cell = cells[y][x];
            uint32_t elementType = static_cast<uint32_t>(cell.elementType);
            file.write(reinterpret_cast<const char*>(&elementType), sizeof(uint32_t));
            file.write(reinterpret_cast<const char*>(&cell.mass), sizeof(float));
            file.write(reinterpret_cast<const char*>(&cell.temperature), sizeof(float));
            file.write(reinterpret_cast<const char*>(&cell.pressure), sizeof(float));
            file.write(reinterpret_cast<const char*>(&cell.velocityX), sizeof(float));
            file.write(reinterpret_cast<const char*>(&cell.velocityY), sizeof(float));
        }
    }
    
    file.close();
    
    std::cout << "[Grid] Saved grid to: " << filePath << " (" << cellCount << " cells)" << std::endl;
    return true;
}

bool Grid::loadFromFile(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        std::cerr << "[Grid] ERROR: Save file not found: " << filePath << std::endl;
        return false;
    }
    
    // Open file for reading
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[Grid] ERROR: Cannot open file for reading: " << filePath << std::endl;
        return false;
    }
    
    // Read header
    SaveFileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    
    // Validate magic number
    if (std::string(header.magic) != "ONISAVE") {
        std::cerr << "[Grid] ERROR: Invalid save file format" << std::endl;
        return false;
    }
    
    // Validate version
    if (header.version != 1) {
        std::cerr << "[Grid] ERROR: Unsupported save file version: " << header.version << std::endl;
        return false;
    }
    
    // Resize grid if needed
    int newWidth = static_cast<int>(header.gridWidth);
    int newHeight = static_cast<int>(header.gridHeight);
    
    if (newWidth != width || newHeight != height) {
        std::cout << "[Grid] Resizing grid from " << width << "x" << height 
                  << " to " << newWidth << "x" << newHeight << std::endl;
        initialize(newWidth, newHeight);
    }
    
    // Read all cell data
    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            uint32_t elementType;
            file.read(reinterpret_cast<char*>(&elementType), sizeof(uint32_t));
            file.read(reinterpret_cast<char*>(&cells[y][x].mass), sizeof(float));
            file.read(reinterpret_cast<char*>(&cells[y][x].temperature), sizeof(float));
            file.read(reinterpret_cast<char*>(&cells[y][x].pressure), sizeof(float));
            file.read(reinterpret_cast<char*>(&cells[y][x].velocityX), sizeof(float));
            file.read(reinterpret_cast<char*>(&cells[y][x].velocityY), sizeof(float));
            
            cells[y][x].elementType = static_cast<ElementType>(elementType);
        }
    }
    
    file.close();
    
    std::cout << "[Grid] Loaded grid from: " << filePath << " (" << header.cellCount << " cells)" << std::endl;
    return true;
}
