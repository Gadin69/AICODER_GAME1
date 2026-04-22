#pragma once

#include <vector>
#include <cmath>

// Heat flow direction and magnitude
struct HeatVector {
    float dx = 0.0f;  // Horizontal heat flow
    float dy = 0.0f;  // Vertical heat flow
    float magnitude = 0.0f;  // Total heat flow intensity
};

// Heat map for grid-wide temperature simulation
class HeatMap {
public:
    HeatMap();
    
    // Initialize heat map for grid size
    void initialize(int width, int height);
    
    // Update heat map based on cell temperatures
    void update(const std::vector<std::vector<float>>& cellTemperatures, float deltaTime);
    
    // Get temperature at position (interpolated)
    float getTemperature(int x, int y) const;
    
    // Get heat flow vector at position
    HeatVector getHeatFlow(int x, int y) const;
    
    // Apply heat source at position
    void addHeatSource(int x, int y, float temperature, float intensity);
    
    // Apply heat sink at position (cooling)
    void addHeatSink(int x, int y, float temperature, float intensity);
    
    // Get temperature gradient (direction of steepest temperature change)
    HeatVector getTemperatureGradient(int x, int y) const;
    
    // Smooth the heat map (heat diffusion)
    void diffuse(float diffusionRate);
    
    // Clear all heat sources/sinks
    void clear();
    
private:
    int width;
    int height;
    
    // Temperature field
    std::vector<std::vector<float>> temperatureField;
    
    // Heat flow vectors
    std::vector<std::vector<HeatVector>> heatFlowField;
    
    // Heat sources (position, temperature, intensity)
    struct HeatSource {
        int x, y;
        float temperature;
        float intensity;
    };
    std::vector<HeatSource> heatSources;
    
    // Heat sinks
    std::vector<HeatSource> heatSinks;
    
    // Calculate heat flow vectors from temperature gradients
    void calculateHeatFlow();
    
    // Apply heat sources and sinks
    void applySourcesAndSinks();
};
