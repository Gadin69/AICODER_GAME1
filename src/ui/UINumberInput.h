#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

class Renderer;  // Forward declaration
class UIButton;  // Forward declaration

class UINumberInput {
public:
    UINumberInput();
    ~UINumberInput();
    
    void initialize(float x, float y, float width, float height, float minVal, float maxVal, float defaultVal, const sf::Font& font);
    void render(Renderer& renderer);
    
    void handleMousePress(const sf::Vector2f& mousePos);
    void handleMouseMove(const sf::Vector2f& mousePos);
    void handleMouseRelease();
    void handleKeyPress(const sf::Event::KeyPressed& keyEvent);
    void handleTextEntered(const sf::Event::TextEntered& textEvent);
    
    void setValue(float value);
    void setStep(float step);
    void setCallback(std::function<void(float)> callback);
    
    sf::RectangleShape displayBox;
    sf::Text* valueText = nullptr;
    sf::Text* labelText = nullptr;
    sf::RectangleShape minusBtn;
    sf::RectangleShape plusBtn;
    sf::Text* minusText = nullptr;
    sf::Text* plusText = nullptr;
    sf::Font* fontPtr = nullptr;
    std::function<void(float)> onChange = nullptr;
    
    float value;
    float minValue;
    float maxValue;
    float stepSize = 1.0f;
    bool isFocused = false;
    bool initialized = false;
    
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
