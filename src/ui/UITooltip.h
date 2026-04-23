#pragma once

#include <SFML/Graphics.hpp>
#include <string>

class Renderer;  // Forward declaration

class UITooltip {
public:
    UITooltip();
    ~UITooltip();
    
    void initialize(const sf::Font& font);
    void render(Renderer& renderer);
    
    void show(const std::string& text, float x, float y);
    void hide();
    void setText(const std::string& text);
    void setPosition(float x, float y);
    void setFadeDuration(float duration);
    void update(float deltaTime);
    
    sf::RectangleShape background;
    sf::Text* tooltipText = nullptr;
    sf::Font* fontPtr = nullptr;
    
    std::string text;
    bool isVisible = false;
    bool initialized = false;
    
private:
    float fadeDuration = 0.2f;
    float fadeTime = 0.0f;
    float currentAlpha = 0.0f;
    bool isFadingIn = false;
    bool isFadingOut = false;
};
