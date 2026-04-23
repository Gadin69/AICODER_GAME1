#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

class Renderer;  // Forward declaration

class UITextInput {
public:
    UITextInput();
    ~UITextInput();
    
    void initialize(float x, float y, float width, float height, const std::string& label, const sf::Font& font);
    void render(Renderer& renderer);
    
    void handleMousePress(const sf::Vector2f& mousePos);
    void handleKeyPress(const sf::Event::KeyPressed& keyEvent);
    void handleTextEntered(const sf::Event::TextEntered& textEvent);
    
    void setText(const std::string& text);
    std::string getText() const;
    void setMaxLength(int max);
    void setCallback(std::function<void(const std::string&)> callback);
    
    sf::RectangleShape inputBox;
    sf::Text* inputText = nullptr;
    sf::Text* labelText = nullptr;
    sf::RectangleShape cursor;
    sf::Font* fontPtr = nullptr;
    std::function<void(const std::string&)> onSubmit = nullptr;
    
    std::string text;
    bool isFocused = false;
    bool initialized = false;
    
private:
    int maxLength = 100;
    float cursorBlinkTime = 0.0f;
    bool cursorVisible = true;
    
    void updateCursorPosition();
    void updateCursorVisibility(float deltaTime);
};
