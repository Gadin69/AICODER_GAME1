#include "UIProgressBar.h"
#include "../../rendering/Renderer.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

UIProgressBar::UIProgressBar() : currentValue(0), minValue(0), maxValue(100) {
}

UIProgressBar::~UIProgressBar() {
    delete valueText;
}

void UIProgressBar::initialize(float x, float y, float width, float height, float minVal, float maxVal, float defaultVal, const sf::Font& font) {
    // Set base class properties
    position = sf::Vector2f(x, y);
    size = sf::Vector2f(width, height);
    fontPtr = &font;
    
    minValue = minVal;
    maxValue = maxVal;
    currentValue = defaultVal;
    
    // Background
    background.setSize(sf::Vector2f(width, height));
    background.setPosition(sf::Vector2f(x, y));
    background.setFillColor(bgColor);
    background.setOutlineColor(sf::Color(100, 100, 100));
    background.setOutlineThickness(2.0f);
    
    // Fill bar
    fillBar.setSize(sf::Vector2f(0, height - 4.0f));
    fillBar.setPosition(sf::Vector2f(x + 2.0f, y + 2.0f));
    fillBar.setFillColor(fillColor);
    
    // Value text
    valueText = new sf::Text(font, "", 16);
    valueText->setFillColor(textColor);
    
    // Center text over progress bar
    float textX = x + width / 2.0f;
    float textY = y - 22.0f;
    valueText->setPosition(sf::Vector2f(textX, textY));
    
    updateFill();
    initialized = true;
}

void UIProgressBar::render(Renderer& renderer) {
    if (!initialized) return;
    
    renderer.drawRectangle(background);
    renderer.drawRectangle(fillBar);
    
    if (showText && valueText) {
        renderer.drawText(*valueText);
    }
}

void UIProgressBar::setValue(float value) {
    currentValue = std::max(minValue, std::min(maxValue, value));
    updateFill();
}

void UIProgressBar::setMin(float min) {
    minValue = min;
    updateFill();
}

void UIProgressBar::setMax(float max) {
    maxValue = max;
    updateFill();
}

void UIProgressBar::updateFill() {
    if (!initialized) return;
    
    // Calculate fill percentage
    float range = maxValue - minValue;
    float percentage = (range > 0) ? (currentValue - minValue) / range : 0.0f;
    percentage = std::max(0.0f, std::min(1.0f, percentage));
    
    // Update fill bar width
    float maxWidth = background.getSize().x - 4.0f;
    float fillWidth = maxWidth * percentage;
    fillBar.setSize(sf::Vector2f(fillWidth, fillBar.getSize().y));
    
    // Update text
    if (valueText && showText) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << currentValue << " / " << maxValue;
        valueText->setString(oss.str());
        
        // Re-center text
        sf::FloatRect textBounds = valueText->getLocalBounds();
        float textX = background.getPosition().x + background.getSize().x / 2.0f - textBounds.size.x / 2.0f;
        float textY = background.getPosition().y - 22.0f;
        valueText->setPosition(sf::Vector2f(textX, textY));
    }
}

void UIProgressBar::setShowText(bool show) {
    showText = show;
}

void UIProgressBar::setColors(sf::Color bg, sf::Color fill, sf::Color text) {
    bgColor = bg;
    fillColor = fill;
    textColor = text;
    
    background.setFillColor(bgColor);
    fillBar.setFillColor(fillColor);
    if (valueText) {
        valueText->setFillColor(textColor);
    }
}
