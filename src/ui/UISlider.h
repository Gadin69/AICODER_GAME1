#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class Renderer;  // Forward declaration

class UISlider {
public:
    UISlider();
    ~UISlider();
    
    void initialize(float x, float y, float width, float minVal, float maxVal, float defaultVal, 
                   const std::string& labelText, const sf::Font& font);
    void render(Renderer& renderer);
    
    void handleMousePress(const sf::Vector2f& mousePos);
    void handleMouseRelease();
    void handleMouseMove(const sf::Vector2f& mousePos);
    
    void updateThumbPosition();
    void updateValueText();
    
    sf::RectangleShape track;
    sf::RectangleShape thumb;
    sf::Text* label = nullptr;
    sf::Text* valueText = nullptr;
    sf::Font* fontPtr = nullptr;
    float minValue;
    float maxValue;
    float currentValue;
    bool isDragging = false;
    bool initialized = false;
    
private:
    float thumbWidth = 20.0f;
    float thumbHeight = 30.0f;
};
