#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <string>

class Renderer;  // Forward declaration

class UISlider : public UIElement {
public:
    UISlider();
    ~UISlider() override;
    
    void initialize(float x, float y, float width, float minVal, float maxVal, float defaultVal, 
                   const std::string& labelText, const sf::Font& font);
    void render(Renderer& renderer) override;
    
    void handleMousePress(const sf::Vector2f& mousePos) override;
    void handleMouseRelease() override;
    void handleMouseMove(const sf::Vector2f& mousePos) override;
    
    // Override to update internal component positions when moved
    void setPosition(float x, float y) override;
    void setSize(float width, float height) override;
    
    void updateThumbPosition();
    void updateValueText();
    
    // Component-specific state (public for now)
    sf::RectangleShape track;
    sf::RectangleShape thumb;
    sf::Text* label = nullptr;
    sf::Text* valueText = nullptr;
    float minValue;
    float maxValue;
    float currentValue;
    bool isDragging = false;
    
private:
    float thumbWidth = 20.0f;
    float thumbHeight = 30.0f;
};
