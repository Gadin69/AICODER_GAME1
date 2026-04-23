#include "ChunkManager.h"
#include <cmath>
#include <algorithm>

ChunkManager::ChunkManager() {
}

void ChunkManager::initialize(int gridWidth, int gridHeight, int chunkSize) {
    this->gridWidth = gridWidth;
    this->gridHeight = gridHeight;
    this->chunkSize = chunkSize;
    
    // Clear existing chunks
    chunks.clear();
    
    // Calculate number of chunks in each dimension
    int chunksX = (gridWidth + chunkSize - 1) / chunkSize;  // Round up
    int chunksY = (gridHeight + chunkSize - 1) / chunkSize;
    
    // Create chunks
    for (int cy = 0; cy < chunksY; ++cy) {
        for (int cx = 0; cx < chunksX; ++cx) {
            SimulationChunk chunk;
            chunk.chunkX = cx;
            chunk.chunkY = cy;
            
            // Calculate cell range for this chunk
            chunk.startX = cx * chunkSize;
            chunk.startY = cy * chunkSize;
            chunk.endX = std::min((cx + 1) * chunkSize, gridWidth);
            chunk.endY = std::min((cy + 1) * chunkSize, gridHeight);
            
            chunk.width = chunk.endX - chunk.startX;
            chunk.height = chunk.endY - chunk.startY;
            
            chunk.lastUpdateTime = 0.0f;
            chunk.accumulatedTime = 0.0f;
            chunk.currentLOD = SimulationChunk::LODLevel::None;
            
            chunks.push_back(chunk);
        }
    }
}

void ChunkManager::updateCameraPosition(float cameraX, float cameraY, float viewWidth, float viewHeight) {
    this->cameraX = cameraX;
    this->cameraY = cameraY;
    this->viewWidth = viewWidth;
    this->viewHeight = viewHeight;
}

void ChunkManager::updateLODLevels() {
    for (auto& chunk : chunks) {
        float distance = getChunkDistanceToView(chunk);
        
        // Determine LOD level based on distance
        if (distance <= 0.0f) {
            // Chunk is within view bounds
            chunk.currentLOD = SimulationChunk::LODLevel::High;
        } else if (distance <= MEDIUM_DISTANCE) {
            // Chunk is nearby
            chunk.currentLOD = SimulationChunk::LODLevel::Medium;
        } else if (distance <= LOW_DISTANCE) {
            // Chunk is far
            chunk.currentLOD = SimulationChunk::LODLevel::Low;
        } else {
            // Chunk is too far
            chunk.currentLOD = SimulationChunk::LODLevel::None;
        }
    }
}

bool ChunkManager::shouldUpdateCell(int cellX, int cellY, float deltaTime) {
    // Find chunk for this cell
    SimulationChunk* chunk = const_cast<SimulationChunk*>(getChunkForCell(cellX, cellY));
    if (!chunk) return false;  // Cell out of bounds
    
    // Determine if cell should update based on chunk's LOD
    switch (chunk->currentLOD) {
        case SimulationChunk::LODLevel::High:
            return true;  // Always update visible cells
            
        case SimulationChunk::LODLevel::Medium:
            // Update every 2nd tick - use simple counter
            chunk->accumulatedTime += deltaTime;
            if (chunk->accumulatedTime >= 0.02f * 2.0f) {  // Every 2 frames at 50fps
                chunk->accumulatedTime = 0.0f;
                return true;
            }
            return false;
            
        case SimulationChunk::LODLevel::Low:
            // Update every 4th tick
            chunk->accumulatedTime += deltaTime;
            if (chunk->accumulatedTime >= 0.02f * 4.0f) {  // Every 4 frames at 50fps
                chunk->accumulatedTime = 0.0f;
                return true;
            }
            return false;
            
        case SimulationChunk::LODLevel::None:
            return false;  // Don't update far cells
            
        default:
            return false;
    }
}

const SimulationChunk* ChunkManager::getChunkForCell(int cellX, int cellY) const {
    // Check bounds
    if (cellX < 0 || cellX >= gridWidth || cellY < 0 || cellY >= gridHeight) {
        return nullptr;
    }
    
    // Calculate chunk coordinates
    int chunkX = cellX / chunkSize;
    int chunkY = cellY / chunkSize;
    
    // Find chunk in vector
    // Chunks are stored in row-major order: chunk index = chunkY * chunksX + chunkX
    int chunksX = (gridWidth + chunkSize - 1) / chunkSize;
    int chunkIndex = chunkY * chunksX + chunkX;
    
    if (chunkIndex >= 0 && chunkIndex < static_cast<int>(chunks.size())) {
        return &chunks[chunkIndex];
    }
    
    return nullptr;
}

float ChunkManager::getChunkDistanceToView(const SimulationChunk& chunk) const {
    // Calculate chunk center in pixel coordinates
    float chunkCenterX = (chunk.startX + chunk.endX) / 2.0f * CELL_SIZE;
    float chunkCenterY = (chunk.startY + chunk.endY) / 2.0f * CELL_SIZE;
    
    // Calculate chunk dimensions in pixels
    float chunkHalfWidth = (chunk.width * CELL_SIZE) / 2.0f;
    float chunkHalfHeight = (chunk.height * CELL_SIZE) / 2.0f;
    
    // Calculate view bounds
    float viewLeft = cameraX - viewWidth / 2.0f;
    float viewRight = cameraX + viewWidth / 2.0f;
    float viewTop = cameraY - viewHeight / 2.0f;
    float viewBottom = cameraY + viewHeight / 2.0f;
    
    // Calculate chunk bounds
    float chunkLeft = chunkCenterX - chunkHalfWidth;
    float chunkRight = chunkCenterX + chunkHalfWidth;
    float chunkTop = chunkCenterY - chunkHalfHeight;
    float chunkBottom = chunkCenterY + chunkHalfHeight;
    
    // Calculate distance from chunk to view bounds
    float dx = 0.0f;
    if (chunkRight < viewLeft) {
        dx = viewLeft - chunkRight;
    } else if (chunkLeft > viewRight) {
        dx = chunkLeft - viewRight;
    }
    
    float dy = 0.0f;
    if (chunkBottom < viewTop) {
        dy = viewTop - chunkBottom;
    } else if (chunkTop > viewBottom) {
        dy = chunkTop - viewBottom;
    }
    
    // Return distance (0 if overlapping)
    return std::sqrt(dx * dx + dy * dy);
}

void ChunkManager::updateTimeAccumulation(SimulationChunk& chunk, float deltaTime) {
    switch (chunk.currentLOD) {
        case SimulationChunk::LODLevel::High:
            // High LOD: reset accumulated time, update normally
            chunk.accumulatedTime = 0.0f;
            chunk.lastUpdateTime += deltaTime;
            break;
            
        case SimulationChunk::LODLevel::Medium:
            // Medium LOD: accumulate time slowly
            chunk.accumulatedTime += deltaTime * 0.5f;
            if (chunk.accumulatedTime >= 1.0f) {
                chunk.accumulatedTime = 0.0f;  // Reset after update
                chunk.lastUpdateTime += deltaTime;
            }
            break;
            
        case SimulationChunk::LODLevel::Low:
            // Low LOD: accumulate time faster
            chunk.accumulatedTime += deltaTime * 0.75f;
            if (chunk.accumulatedTime >= 1.0f) {
                chunk.accumulatedTime = 0.0f;  // Reset after update
                chunk.lastUpdateTime += deltaTime;
            }
            break;
            
        case SimulationChunk::LODLevel::None:
            // None LOD: accumulate all time (for catch-up later)
            chunk.accumulatedTime += deltaTime;
            break;
    }
}
