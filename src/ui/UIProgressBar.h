#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <string>

class Renderer;  // Forward declaration

class UIProgressBar : public UIElement {
public:
    UIProgressBar();
    ~UIProgressBar() override;
    
    void initialize(float x, float y, float width, float height, float minVal, float maxVal, float defaultVal, const sf::Font& font);
    void render(Renderer& renderer) override;
    
    void setValue(float value);
    void setMin(float min);
    void setMax(float max);
    void updateFill();
    void setShowText(bool show);
    void setColors(sf::Color background, sf::Color fill, sf::Color text);
    
    // Component-specific state (public for now)
    sf::RectangleShape background;
    sf::RectangleShape fillBar;
    sf::Text* valueText = nullptr;
    
    float currentValue;
    float minValue;
    float maxValue;
    bool showText = true;
    
private:
    sf::Color bgColor = sf::Color(40, 40, 40);
    sf::Color fillColor = sf::Color(100, 200, 100);
    sf::Color textColor = sf::Color::White;
};
