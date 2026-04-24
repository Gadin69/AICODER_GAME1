#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

class Renderer;  // Forward declaration
class UIButton;  // Forward declaration

class UINumberInput : public UIElement {
public:
    UINumberInput();
    ~UINumberInput() override;
    
    void initialize(float x, float y, float width, float height, float minVal, float maxVal, float defaultVal, const sf::Font& font);
    void render(Renderer& renderer) override;
    
    void handleMousePress(const sf::Vector2f& mousePos) override;
    void handleMouseMove(const sf::Vector2f& mousePos) override;
    void handleMouseRelease() override;
    void handleKeyPress(const sf::Event::KeyPressed& keyEvent) override;
    void handleTextEntered(const sf::Event::TextEntered& textEvent) override;
    
    void setValue(float value);
    void setStep(float step);
    void setCallback(std::function<void(float)> callback);
    
    // Component-specific state (public for now)
    sf::RectangleShape displayBox;
    sf::Text* valueText = nullptr;
    sf::Text* labelText = nullptr;
    sf::RectangleShape minusBtn;
    sf::RectangleShape plusBtn;
    sf::Text* minusText = nullptr;
    sf::Text* plusText = nullptr;
    std::function<void(float)> onChange = nullptr;
    
    float value;
    float minValue;
    float maxValue;
    float stepSize = 1.0f;
    bool isFocused = false;
    
private:
    std::string inputBuffer;
    sf::Color btnNormalColor = sf::Color(50, 50, 50);
    sf::Color btnHoverColor = sf::Color(70, 70, 70);
    sf::Color btnPressedColor = sf::Color(90, 90, 90);
    
    bool isMinusHovered = false;
    bool isPlusHovered = false;
    bool isMinusPressed = false;
    bool isPlusPressed = false;
    
    void updateDisplay();
    void updateButtonColors();
};
