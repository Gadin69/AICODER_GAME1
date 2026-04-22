#include "HeatMap.h"
#include <algorithm>

HeatMap::HeatMap()
    : width(0)
    , height(0)
{
}

void HeatMap::initialize(int width, int height) {
    this->width = width;
    this->height = height;
    
    // Initialize temperature field to ambient temperature (20°C)
    temperatureField.resize(height, std::vector<float>(width, 20.0f));
    
    // Initialize heat flow vectors
    heatFlowField.resize(height, std::vector<HeatVector>(width));
}

void HeatMap::update(const std::vector<std::vector<float>>& cellTemperatures, float deltaTime) {
    // Sync temperature field with cell temperatures
    for (int y = 0; y < height && y < (int)cellTemperatures.size(); ++y) {
        for (int x = 0; x < width && x < (int)cellTemperatures[y].size(); ++x) {
            temperatureField[y][x] = cellTemperatures[y][x];
        }
    }
    
    // Apply heat sources and sinks
    applySourcesAndSinks();
    
    // Diffuse heat (thermal conduction)
    diffuse(0.1f);  // 10% diffusion per update
    
    // Calculate heat flow vectors
    calculateHeatFlow();
}

float HeatMap::getTemperature(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 20.0f;  // Ambient temperature out of bounds
    }
    return temperatureField[y][x];
}

HeatVector HeatMap::getHeatFlow(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return HeatVector();
    }
    return heatFlowField[y][x];
}

void HeatMap::addHeatSource(int x, int y, float temperature, float intensity) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        HeatSource source;
        source.x = x;
        source.y = y;
        source.temperature = temperature;
        source.intensity = intensity;
        heatSources.push_back(source);
    }
}

void HeatMap::addHeatSink(int x, int y, float temperature, float intensity) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        HeatSource sink;
        sink.x = x;
        sink.y = y;
        sink.temperature = temperature;
        sink.intensity = intensity;
        heatSinks.push_back(sink);
    }
}

HeatVector HeatMap::getTemperatureGradient(int x, int y) const {
    HeatVector gradient;
    
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return gradient;
    }
    
    // Calculate gradient using finite differences
    float left = (x > 0) ? temperatureField[y][x-1] : temperatureField[y][x];
    float right = (x < width-1) ? temperatureField[y][x+1] : temperatureField[y][x];
    float up = (y > 0) ? temperatureField[y-1][x] : temperatureField[y][x];
    float down = (y < height-1) ? temperatureField[y+1][x] : temperatureField[y][x];
    
    // Gradient points in direction of increasing temperature
    gradient.dx = (right - left) * 0.5f;
    gradient.dy = (up - down) * 0.5f;
    gradient.magnitude = std::sqrt(gradient.dx * gradient.dx + gradient.dy * gradient.dy);
    
    return gradient;
}

void HeatMap::diffuse(float diffusionRate) {
    // Create temporary buffer for diffusion
    std::vector<std::vector<float>> newField = temperatureField;
    
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            // Average with neighbors (Laplacian diffusion)
            float avg = (temperatureField[y-1][x] + temperatureField[y+1][x] +
                        temperatureField[y][x-1] + temperatureField[y][x+1]) * 0.25f;
            
            // Blend current temperature with average
            newField[y][x] = temperatureField[y][x] + (avg - temperatureField[y][x]) * diffusionRate;
        }
    }
    
    temperatureField = newField;
}

void HeatMap::calculateHeatFlow() {
    // Heat flows from hot to cold (opposite of temperature gradient)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            HeatVector gradient = getTemperatureGradient(x, y);
            
            // Heat flow is opposite to gradient (hot → cold)
            heatFlowField[y][x].dx = -gradient.dx;
            heatFlowField[y][x].dy = -gradient.dy;
            heatFlowField[y][x].magnitude = gradient.magnitude;
        }
    }
}

void HeatMap::applySourcesAndSinks() {
    // Apply heat sources
    for (const auto& source : heatSources) {
        if (source.x >= 0 && source.x < width && source.y >= 0 && source.y < height) {
            // Blend source temperature into field based on intensity
            float& cellTemp = temperatureField[source.y][source.x];
            cellTemp += (source.temperature - cellTemp) * source.intensity;
        }
    }
    
    // Apply heat sinks
    for (const auto& sink : heatSinks) {
        if (sink.x >= 0 && sink.x < width && sink.y >= 0 && sink.y < height) {
            float& cellTemp = temperatureField[sink.y][sink.x];
            cellTemp += (sink.temperature - cellTemp) * sink.intensity;
        }
    }
    
    // Clear sources/sinks after applying (they need to be re-added each frame)
    heatSources.clear();
    heatSinks.clear();
}

void HeatMap::clear() {
    heatSources.clear();
    heatSinks.clear();
    
    // Reset to ambient temperature
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            temperatureField[y][x] = 20.0f;
        }
    }
}
