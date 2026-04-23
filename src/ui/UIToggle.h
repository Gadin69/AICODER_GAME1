#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

class Renderer;  // Forward declaration

class UIToggle {
public:
    UIToggle();
    ~UIToggle();
    
    void initialize(float x, float y, float width, float height, const std::string& label, const sf::Font& font, bool defaultState = false);
    void render(Renderer& renderer);
    
    void handleMousePress(const sf::Vector2f& mousePos);
    
    void toggle();
    void setOn(bool on);
    void setCallback(std::function<void(bool)> callback);
    
    sf::RectangleShape track;
    sf::CircleShape thumb;
    sf::Text* labelText = nullptr;
    sf::Font* fontPtr = nullptr;
    std::function<void(bool)> onToggle = nullptr;
    
    bool isOn = false;
    bool initialized = false;
    
private:
    sf::Color trackOffColor = sf::Color(80, 80, 80);
    sf::Color trackOnColor = sf::Color(100, 200, 100);
    sf::Color thumbColor = sf::Color(255, 255, 255);
    
    void updateVisuals();
};
