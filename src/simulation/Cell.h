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
        , temperature(20.0f)                // Room temp (neutral, prevents corruption)
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

    // Convert this cell to vacuum with proper defaults
    // Use this instead of manually setting fields to prevent mistakes
    void convertToVacuum() {
        elementType = ElementType::Vacuum;
        mass = 0.0f;
        temperature = 20.0f;  // Room temp (neutral, prevents corruption)
        pressure = 0.0f;
        velocityX = 0.0f;
        velocityY = 0.0f;
        updated = false;
        targetElementType = ElementType::Empty;
        phaseTransitionProgress = 0.0f;
        phaseTransitionSpeed = 0.0f;
        microMassDecayTime = 0.0f;
        updateColor();
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
            case ElementType::Solid_Oil:
                color = sf::Color(60, 50, 40, 220);  // Dark brown frozen oil
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
            case ElementType::Gas_Oil:
                color = sf::Color(180, 150, 100, 100);  // Brownish oil vapor
                break;
            case ElementType::Liquid_Water:
                color = sf::Color(50, 100, 255, 180);
                break;
            case ElementType::Liquid_Lava:
                color = sf::Color(255, 100, 0, 200);
                break;
            case ElementType::Liquid_Oil:
                color = sf::Color(120, 90, 50, 200);  // Dark golden/brown oil
                break;
            case ElementType::ContaminatedWater:
                color = sf::Color(100, 150, 100, 180);
                break;
            case ElementType::Solid_ContaminatedWater:
                color = sf::Color(150, 180, 150, 200);  // Murky frozen green-gray
                break;
            
            // NEW GASES
            case ElementType::Gas_Hydrogen:
                color = sf::Color(200, 220, 255, 80);  // Very light blue, almost transparent
                break;
            case ElementType::Gas_Methane:
                color = sf::Color(180, 200, 180, 90);  // Pale greenish-gray
                break;
            case ElementType::Gas_Ammonia:
                color = sf::Color(220, 240, 200, 100);  // Pale yellow-green
                break;
            case ElementType::Gas_Chlorine:
                color = sf::Color(150, 255, 100, 120);  // Yellow-green (characteristic chlorine color)
                break;
            case ElementType::Gas_SulfurDioxide:
                color = sf::Color(200, 200, 150, 110);  // Pale yellow-gray
                break;
            case ElementType::Gas_Propane:
                color = sf::Color(190, 190, 190, 85);  // Light gray, slightly transparent
                break;
            
            // NEW LIQUIDS
            case ElementType::Liquid_Mercury:
                color = sf::Color(180, 180, 190, 240);  // Silvery-gray metallic
                break;
            case ElementType::Liquid_Ethanol:
                color = sf::Color(200, 220, 255, 160);  // Clear blue-tinted
                break;
            case ElementType::Liquid_Acetone:
                color = sf::Color(210, 230, 255, 150);  // Very pale blue, clear
                break;
            case ElementType::Liquid_Glycerol:
                color = sf::Color(230, 220, 200, 200);  // Viscous honey-like, pale yellow
                break;
            case ElementType::Liquid_SulfuricAcid:
                color = sf::Color(220, 210, 180, 190);  // Oily yellow-brown
                break;
            case ElementType::Liquid_Nitrogen:
                color = sf::Color(180, 200, 255, 170);  // Cryogenic pale blue
                break;
            case ElementType::Liquid_Oxygen:
                color = sf::Color(150, 180, 255, 180);  // Pale blue (liquid O2 is paramagnetic blue)
                break;
            case ElementType::Liquid_Bromine:
                color = sf::Color(180, 60, 30, 210);  // Deep reddish-brown
                break;
            
            // NEW SOLIDS
            case ElementType::Solid_Iron:
                color = sf::Color(120, 120, 125, 255);  // Dark gray metallic
                break;
            case ElementType::Solid_Copper:
                color = sf::Color(184, 115, 51, 255);  // Characteristic copper orange-brown
                break;
            case ElementType::Solid_Aluminum:
                color = sf::Color(200, 200, 210, 255);  // Light silvery-gray
                break;
            case ElementType::Solid_Silver:
                color = sf::Color(210, 210, 220, 255);  // Bright silvery-white
                break;
            case ElementType::Solid_Gold:
                color = sf::Color(255, 215, 0, 255);  // Rich golden yellow
                break;
            case ElementType::Solid_Lead:
                color = sf::Color(100, 100, 110, 255);  // Dark gray-blue
                break;
            case ElementType::Solid_Zinc:
                color = sf::Color(160, 160, 165, 255);  // Medium gray metallic
                break;
            
            // LIQUID METALS
            case ElementType::Liquid_Iron:
                color = sf::Color(255, 140, 50, 230);  // Glowing orange-red molten
                break;
            case ElementType::Liquid_Copper:
                color = sf::Color(255, 120, 40, 230);  // Bright orange molten copper
                break;
            case ElementType::Liquid_Aluminum:
                color = sf::Color(220, 220, 230, 220);  // Bright silvery liquid
                break;
            case ElementType::Liquid_Silver:
                color = sf::Color(230, 230, 240, 230);  // Bright liquid silver
                break;
            case ElementType::Liquid_Gold:
                color = sf::Color(255, 200, 50, 240);  // Glowing liquid gold
                break;
            case ElementType::Liquid_Lead:
                color = sf::Color(150, 150, 160, 230);  // Dark gray liquid
                break;
            case ElementType::Liquid_Zinc:
                color = sf::Color(200, 200, 205, 220);  // Bluish-white liquid
                break;
            
            // LIQUID GAS FORMS
            case ElementType::Liquid_Methane:
                color = sf::Color(160, 180, 160, 190);  // Pale greenish cryogenic liquid
                break;
            case ElementType::Liquid_Ammonia:
                color = sf::Color(200, 220, 180, 180);  // Pale yellow-green liquid
                break;
            case ElementType::Liquid_Chlorine:
                color = sf::Color(130, 230, 80, 190);  // Yellow-green liquid
                break;
            case ElementType::Liquid_SulfurDioxide:
                color = sf::Color(180, 180, 130, 190);  // Pale yellow-gray liquid
                break;
            case ElementType::Liquid_Propane:
                color = sf::Color(170, 170, 170, 180);  // Clear colorless liquid
                break;
            
            // GAS LIQUID FORMS (vapors)
            case ElementType::Gas_Mercury:
                color = sf::Color(160, 160, 170, 100);  // Faint silvery vapor
                break;
            case ElementType::Gas_Ethanol:
                color = sf::Color(180, 200, 230, 90);  // Pale blue-tinted vapor
                break;
            case ElementType::Gas_Acetone:
                color = sf::Color(190, 210, 230, 85);  // Very pale blue vapor
                break;
            case ElementType::Gas_Glycerol:
                color = sf::Color(210, 200, 180, 95);  // Faint yellowish vapor
                break;
            case ElementType::Gas_SulfuricAcid:
                color = sf::Color(200, 190, 160, 100);  // Faint yellow-brown corrosive vapor
                break;
            case ElementType::Gas_Oxygen:
                color = sf::Color(130, 160, 230, 95);  // Pale blue oxygen gas
                break;
            case ElementType::Gas_Bromine:
                color = sf::Color(160, 50, 20, 110);  // Reddish-brown toxic vapor
                break;
            
            // CRYOGENIC ELEMENTS
            case ElementType::Liquid_Hydrogen:
                color = sf::Color(170, 190, 255, 160);  // Very pale blue cryogenic liquid
                break;
            case ElementType::Gas_Nitrogen:
                color = sf::Color(150, 170, 220, 80);  // Pale blue-gray nitrogen gas
                break;
            default:
                color = sf::Color::Transparent;
                break;
        }
    }
};
