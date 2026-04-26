#include "UIToggle.h"
#include "../../rendering/Renderer.h"

UIToggle::UIToggle() {
}

UIToggle::~UIToggle() {
    delete labelText;
}

void UIToggle::initialize(float x, float y, float width, float height, const std::string& label, const sf::Font& font, bool defaultState) {
    // Set base class properties
    position = sf::Vector2f(x, y);
    size = sf::Vector2f(width, height);
    fontPtr = &font;
    
    isOn = defaultState;
    
    // Track (pill shape)
    track.setSize(sf::Vector2f(width, height));
    track.setPosition(sf::Vector2f(x, y));
    track.setFillColor(trackOffColor);
    track.setOutlineColor(sf::Color(120, 120, 120));
    track.setOutlineThickness(2.0f);
    
    // Thumb (circle)
    float thumbRadius = height / 2.0f - 2.0f;
    thumb.setRadius(thumbRadius);
    thumb.setFillColor(thumbColor);
    thumb.setOutlineColor(sf::Color(200, 200, 200));
    thumb.setOutlineThickness(1.0f);
    
    // Label
    labelText = new sf::Text(font, label, 16);
    labelText->setFillColor(sf::Color::White);
    labelText->setPosition(sf::Vector2f(x + width + 10.0f, y + height / 2.0f - 8.0f));
    
    updateVisuals();
    initialized = true;
}

void UIToggle::render(Renderer& renderer) {
    if (!initialized) return;
    
    renderer.drawRectangle(track);
    renderer.drawShape(thumb);
    renderer.drawText(*labelText);
}

void UIToggle::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    // Check if clicking on track or label
    sf::FloatRect labelBounds = labelText->getGlobalBounds();
    sf::FloatRect trackBounds = track.getGlobalBounds();
    
    // Create combined clickable area
    sf::FloatRect clickableArea(
        sf::Vector2f(trackBounds.position.x, trackBounds.position.y),
        sf::Vector2f(labelBounds.position.x + labelBounds.size.x - trackBounds.position.x,
                    std::max(trackBounds.size.y, labelBounds.size.y))
    );
    
    if (clickableArea.contains(mousePos)) {
        toggle();
    }
}

void UIToggle::toggle() {
    isOn = !isOn;
    updateVisuals();
    
    if (onToggle) {
        onToggle(isOn);
    }
}

void UIToggle::setOn(bool on) {
    if (isOn != on) {
        isOn = on;
        updateVisuals();
    }
}

void UIToggle::setCallback(std::function<void(bool)> callback) {
    onToggle = callback;
}

void UIToggle::updateVisuals() {
    if (!initialized) return;
    
    // Update track color
    track.setFillColor(isOn ? trackOnColor : trackOffColor);
    
    // Update thumb position
    float trackX = track.getPosition().x;
    float trackY = track.getPosition().y;
    float trackHeight = track.getSize().y;
    float thumbRadius = thumb.getRadius();
    
    if (isOn) {
        // Thumb on right
        float thumbX = trackX + track.getSize().x - thumbRadius * 2.0f - 2.0f;
        float thumbY = trackY + (trackHeight - thumbRadius * 2.0f) / 2.0f;
        thumb.setPosition(sf::Vector2f(thumbX, thumbY));
    } else {
        // Thumb on left
        float thumbX = trackX + 2.0f;
        float thumbY = trackY + (trackHeight - thumbRadius * 2.0f) / 2.0f;
        thumb.setPosition(sf::Vector2f(thumbX, thumbY));
    }
}
