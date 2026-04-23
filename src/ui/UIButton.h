#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

class Renderer;  // Forward declaration

class UIButton {
public:
    UIButton();
    ~UIButton();
    
    void initialize(float x, float y, float width, float height, const std::string& text, const sf::Font& font);
    void render(Renderer& renderer);
    
    void handleMousePress(const sf::Vector2f& mousePos);
    void handleMouseMove(const sf::Vector2f& mousePos);
    void handleMouseRelease();
    
    void setText(const std::string& text);
    void setCallback(std::function<void()> callback);
    void setColors(sf::Color normal, sf::Color hover, sf::Color pressed);
    
    sf::RectangleShape background;
    sf::Text* buttonText = nullptr;
    sf::Font* fontPtr = nullptr;
    std::function<void()> onClick = nullptr;
    
    bool isHovered = false;
    bool isPressed = false;
    bool initialized = false;
    
private:
    sf::Color normalColor = sf::Color(60, 60, 60);
    sf::Color hoverColor = sf::Color(80, 80, 80);
    sf::Color pressedColor = sf::Color(100, 100, 100);
    
    void updateColor();
};
