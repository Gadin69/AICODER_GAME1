#include "UITooltip.h"
#include "rendering/Renderer.h"
#include <algorithm>

UITooltip::UITooltip() {
}

UITooltip::~UITooltip() {
    delete tooltipText;
}

void UITooltip::initialize(const sf::Font& font) {
    fontPtr = const_cast<sf::Font*>(&font);
    
    // Background
    background.setSize(sf::Vector2f(200.0f, 40.0f));
    background.setFillColor(sf::Color(20, 20, 20, 230));
    background.setOutlineColor(sf::Color(150, 150, 150, 230));
    background.setOutlineThickness(1.0f);
    
    // Tooltip text
    tooltipText = new sf::Text(font, "", 14);
    tooltipText->setFillColor(sf::Color::White);
    
    initialized = true;
}

void UITooltip::render(Renderer& renderer) {
    if (!initialized || !isVisible) return;
    
    // Apply fade
    sf::Color bgColor = background.getFillColor();
    bgColor.a = static_cast<uint8_t>(currentAlpha * 230);
    background.setFillColor(bgColor);
    
    sf::Color textColor = this->tooltipText->getFillColor();
    textColor.a = static_cast<uint8_t>(currentAlpha * 255);
    this->tooltipText->setFillColor(textColor);
    
    renderer.drawRectangle(background);
    renderer.drawText(*this->tooltipText);
    
    // Restore full alpha
    bgColor.a = 230;
    background.setFillColor(bgColor);
    textColor.a = 255;
    this->tooltipText->setFillColor(textColor);
}

void UITooltip::show(const std::string& tooltipTextParam, float x, float y) {
    if (!initialized) return;
    
    text = tooltipTextParam;
    this->tooltipText->setString(text);
    
    // Size background to fit text
    sf::FloatRect textBounds = this->tooltipText->getLocalBounds();
    float padding = 10.0f;
    background.setSize(sf::Vector2f(textBounds.size.x + padding * 2.0f, textBounds.size.y + padding * 2.0f));
    
    setPosition(x, y);
    
    // Start fade in
    isFadingIn = true;
    isFadingOut = false;
    fadeTime = 0.0f;
    isVisible = true;
}

void UITooltip::hide() {
    if (!isVisible) return;
    
    // Start fade out
    isFadingOut = true;
    isFadingIn = false;
    fadeTime = 0.0f;
}

void UITooltip::setText(const std::string& newText) {
    text = newText;
    if (tooltipText) {
        tooltipText->setString(text);
        
        // Resize background
        sf::FloatRect textBounds = this->tooltipText->getLocalBounds();
        float padding = 10.0f;
        background.setSize(sf::Vector2f(textBounds.size.x + padding * 2.0f, textBounds.size.y + padding * 2.0f));
    }
}

void UITooltip::setPosition(float x, float y) {
    if (!initialized) return;
    
    // Position tooltip above and to the right of cursor
    float tooltipX = x + 15.0f;
    float tooltipY = y - background.getSize().y - 10.0f;
    
    // Prevent going off-screen (assume 1920x1080 max)
    float maxWidth = 1920.0f;
    float maxHeight = 1080.0f;
    
    if (tooltipX + background.getSize().x > maxWidth) {
        tooltipX = x - background.getSize().x - 15.0f;
    }
    if (tooltipY < 0) {
        tooltipY = y + 15.0f;
    }
    
    background.setPosition(sf::Vector2f(tooltipX, tooltipY));
    
    // Center text in background
    sf::FloatRect textBounds = this->tooltipText->getLocalBounds();
    float textX = tooltipX + 10.0f;
    float textY = tooltipY + 10.0f - textBounds.position.y;
    this->tooltipText->setPosition(sf::Vector2f(textX, textY));
}

void UITooltip::setFadeDuration(float duration) {
    fadeDuration = duration;
}

void UITooltip::update(float deltaTime) {
    if (!initialized) return;
    
    if (isFadingIn) {
        fadeTime += deltaTime;
        currentAlpha = std::min(1.0f, fadeTime / fadeDuration);
        
        if (currentAlpha >= 1.0f) {
            isFadingIn = false;
            currentAlpha = 1.0f;
        }
    } else if (isFadingOut) {
        fadeTime += deltaTime;
        currentAlpha = std::max(0.0f, 1.0f - fadeTime / fadeDuration);
        
        if (currentAlpha <= 0.0f) {
            isFadingOut = false;
            isVisible = false;
            currentAlpha = 0.0f;
        }
    }
}
