#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

class Renderer;  // Forward declaration

class UIButton : public UIElement {
public:
    UIButton();
    ~UIButton() override;
    
    void initialize(float x, float y, float width, float height, const std::string& text, const sf::Font& font);
    void render(Renderer& renderer) override;
    
    void handleMousePress(const sf::Vector2f& mousePos) override;
    void handleMouseMove(const sf::Vector2f& mousePos) override;
    void handleMouseRelease() override;
    
    void setText(const std::string& text);
    void setCallback(std::function<void()> callback);
    void setColors(sf::Color normal, sf::Color hover, sf::Color pressed);
    
    // Component-specific state (public for now, will be encapsulated when UIBorder uses polymorphism)
    sf::RectangleShape background;
    sf::Text* buttonText = nullptr;
    std::function<void()> onClick = nullptr;
    bool isHovered = false;
    bool isPressed = false;
    sf::Color normalColor = sf::Color(60, 60, 60);
    sf::Color hoverColor = sf::Color(80, 80, 80);
    sf::Color pressedColor = sf::Color(100, 100, 100);
    
    void updateColor();
};
