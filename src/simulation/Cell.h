#pragma once

#include "ElementTypes.h"
#include <SFML/Graphics.hpp>

struct Cell {
    ElementType elementType;
    float temperature;
    float pressure;
    float velocityX;
    float velocityY;
    bool updated;
    sf::Color color;

    Cell()
        : elementType(ElementType::Empty)
        , temperature(20.0f)
        , pressure(1.0f)
        , velocityX(0.0f)
        , velocityY(0.0f)
        , updated(false)
        , color(sf::Color::Transparent)
    {
    }

    void updateColor() {
        ElementProperties props = ElementTypes::getProperties(elementType);
        
        switch (elementType) {
            case ElementType::Empty:
                color = sf::Color::Transparent;
                break;
            case ElementType::Solid:
                color = sf::Color(128, 128, 128);
                break;
            case ElementType::Gas_O2:
                color = sf::Color(100, 150, 255, 100);
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
        }
    }
};
