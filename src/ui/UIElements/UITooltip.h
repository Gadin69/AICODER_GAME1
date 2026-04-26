#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <string>

class Renderer;  // Forward declaration

class UITooltip : public UIElement {
public:
    UITooltip();
    ~UITooltip() override;
    
    void initialize(const sf::Font& font);
    void render(Renderer& renderer) override;
    
    void show(const std::string& text, float x, float y);
    void hide();
    void setText(const std::string& text);
    void setPosition(float x, float y);
    void setFadeDuration(float duration);
    void update(float deltaTime);
    
    // Component-specific state (public for now)
    sf::RectangleShape background;
    sf::Text* tooltipText = nullptr;
    
    std::string text;
    
private:
    float fadeDuration = 0.2f;
    float fadeTime = 0.0f;
    float currentAlpha = 0.0f;
    bool isFadingIn = false;
    bool isFadingOut = false;
};
