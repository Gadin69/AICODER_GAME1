#pragma once

#include "ElementTypes.h"
#include <SFML/Graphics.hpp>

// Phase change temperature thresholds (Celsius)
constexpr float SOLID_MELT_TEMP = 800.0f;    // Solid -> Liquid (rock melts)
constexpr float WATER_FREEZE_TEMP = 0.0f;    // Water -> Ice (solid)
constexpr float WATER_BOIL_TEMP = 100.0f;    // Water -> Steam (gas)
constexpr float LAVA_COOL_TEMP = 500.0f;     // Lava -> Rock (solid)

// Thermal properties (how much energy to change temperature)
constexpr float LAVA_HEAT_CAPACITY = 0.8f;    // Lava holds heat well
constexpr float WATER_HEAT_CAPACITY = 4.18f;  // Water has HIGH heat capacity
constexpr float GAS_HEAT_CAPACITY = 1.0f;     // Gas has low heat capacity
constexpr float SOLID_HEAT_CAPACITY = 2.0f;   // Solids have medium heat capacity
constexpr float AMBIENT_TEMP = 20.0f;         // Room temperature

struct Cell {
    ElementType elementType;
    float mass;            // kg - DYNAMIC mass (ONI-style: can be partial!)
    float temperature;
    float pressure;        // Pascals (for gas cells)
    float velocityX;
    float velocityY;
    bool updated;
    sf::Color color;
    
    // Phase transition tracking
    ElementType targetElementType = ElementType::Empty;  // What we're transitioning to
    float phaseTransitionProgress = 0.0f;  // 0.0 to 1.0 (0 = not started, 1 = complete)
    float phaseTransitionSpeed = 0.0f;  // How fast the transition happens per tick
    
    // Gas decay tracking
    float microMassDecayTime = 0.0f;  // Time spent at microscopic mass (for gas dissipation)

    Cell()
        : elementType(ElementType::Vacuum)  // Default is vacuum, not empty
        , mass(0.0f)                        // No mass in vacuum
        , temperature(-273.15f)             // Absolute zero in vacuum
        , pressure(0.0f)                    // No pressure in vacuum
        , velocityX(0.0f)
        , velocityY(0.0f)
        , updated(false)
        , color(sf::Color(10, 10, 15))      // Vacuum color
        , targetElementType(ElementType::Empty)
        , phaseTransitionProgress(0.0f)
        , phaseTransitionSpeed(0.0f)
        , microMassDecayTime(0.0f)
    {
    }

    void updateColor() {
        switch (elementType) {
            case ElementType::Empty:
                color = sf::Color::Transparent;
                break;
            case ElementType::Vacuum:
                color = sf::Color(10, 10, 15);  // Very dark, almost black
                break;
            case ElementType::Solid:
                color = sf::Color(128, 128, 128);
                break;
            case ElementType::Solid_Ice:
                color = sf::Color(200, 230, 255, 200);  // Light blue-white ice
                break;
            case ElementType::Solid_DryIce:
                color = sf::Color(240, 240, 255, 220);  // White/light blue dry ice
                break;
            case ElementType::Gas_O2:
                color = sf::Color(100, 150, 255, 100);
                break;
            case ElementType::Gas_Lava:
                color = sf::Color(255, 200, 50, 80);    // Bright orange-yellow gas
                break;
            case ElementType::Gas_CO2:
                color = sf::Color(100, 100, 100, 120);
                break;
            case ElementType::Liquid_Water:
                color = sf::Color(50, 100, 255, 180);
                break;
            case ElementType::Liquid_Lava:
                color = sf::Color(255, 100, 0, 200);
                break;
            case ElementType::ContaminatedWater:
                color = sf::Color(100, 150, 100, 180);
                break;
            case ElementType::Solid_ContaminatedWater:
                color = sf::Color(150, 180, 150, 200);  // Murky frozen green-gray
                break;
            default:
                color = sf::Color::Transparent;
                break;
        }
    }
};
