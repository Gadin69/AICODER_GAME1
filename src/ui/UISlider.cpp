#include "UISlider.h"
#include "rendering/Renderer.h"
#include <algorithm>
#include <iostream>

UISlider::UISlider() {
}

UISlider::~UISlider() {
    delete label;
    delete valueText;
}

void UISlider::initialize(float x, float y, float width, float minVal, float maxVal, float defaultVal, 
                         const std::string& labelText, const sf::Font& font) {
    // Set base class properties
    position = sf::Vector2f(x, y);
    size = sf::Vector2f(width, 30.0f);  // Track height
    fontPtr = &font;
    
    minValue = minVal;
    maxValue = maxVal;
    currentValue = defaultVal;
    
    // Track background
    track.setSize(sf::Vector2f(width, 20));
    track.setPosition(sf::Vector2f(x, y));
    track.setFillColor(sf::Color(60, 60, 60));
    track.setOutlineColor(sf::Color(100, 100, 100));
    track.setOutlineThickness(2.0f);
    
    // Thumb (draggable handle)
    thumb.setSize(sf::Vector2f(thumbWidth, thumbHeight));
    thumb.setFillColor(sf::Color(200, 200, 200));
    thumb.setOutlineColor(sf::Color(255, 255, 255));
    thumb.setOutlineThickness(1.0f);
    
    // Label
    label = new sf::Text(font, labelText, 18);
    label->setFillColor(sf::Color::White);
    label->setPosition(sf::Vector2f(x, y - 25.0f));
    
    // Value text
    valueText = new sf::Text(font, "", 18);
    valueText->setFillColor(sf::Color(255, 255, 100));
    valueText->setPosition(sf::Vector2f(x + width + 10.0f, y - 2.0f));
    
    // Mark as initialized BEFORE calling update functions
    initialized = true;
    
    // Now update thumb position and value text
    updateThumbPosition();
    updateValueText();
}

void UISlider::render(Renderer& renderer) {
    if (!initialized) return;
    
    renderer.drawRectangle(track);
    renderer.drawRectangle(thumb);
    renderer.drawText(*label);
    renderer.drawText(*valueText);
}

void UISlider::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    // Check if clicking on thumb
    if (thumb.getGlobalBounds().contains(mousePos)) {
        isDragging = true;
    }
    // Check if clicking on track (move thumb to that position)
    else if (track.getGlobalBounds().contains(mousePos)) {
        float trackLeft = track.getPosition().x;
        float trackRight = trackLeft + track.getSize().x;
        float t = (mousePos.x - trackLeft) / (trackRight - trackLeft);
        t = std::max(0.0f, std::min(1.0f, t));
        currentValue = minValue + t * (maxValue - minValue);
        updateThumbPosition();
        updateValueText();
        isDragging = true;
    }
}

void UISlider::handleMouseRelease() {
    isDragging = false;
}

void UISlider::handleMouseMove(const sf::Vector2f& mousePos) {
    if (!isDragging || !initialized) return;
    
    float trackLeft = track.getPosition().x;
    float trackRight = trackLeft + track.getSize().x;
    float t = (mousePos.x - trackLeft) / (trackRight - trackLeft);
    t = std::max(0.0f, std::min(1.0f, t));
    
    currentValue = minValue + t * (maxValue - minValue);
    updateThumbPosition();
    updateValueText();
}

void UISlider::updateThumbPosition() {
    if (!initialized) return;
    
    float t = (currentValue - minValue) / (maxValue - minValue);
    float trackLeft = track.getPosition().x;
    float trackWidth = track.getSize().x;
    
    float thumbX = trackLeft + t * (trackWidth - thumbWidth) - thumbWidth / 2.0f;
    float thumbY = track.getPosition().y - (thumbHeight - track.getSize().y) / 2.0f;
    
    thumb.setPosition(sf::Vector2f(thumbX, thumbY));
}

void UISlider::updateValueText() {
    if (!initialized || !valueText || !fontPtr) return;
    
    // Format value with 1 decimal place
    std::string value = std::to_string(static_cast<int>(currentValue));
    valueText->setString(value);
}
