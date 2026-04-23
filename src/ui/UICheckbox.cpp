#include "UICheckbox.h"
#include "rendering/Renderer.h"

UICheckbox::UICheckbox() {
}

UICheckbox::~UICheckbox() {
    delete checkmark;
    delete labelText;
}

void UICheckbox::initialize(float x, float y, float size, const std::string& label, const sf::Font& font) {
    fontPtr = const_cast<sf::Font*>(&font);
    
    // Checkbox box
    checkboxBox.setSize(sf::Vector2f(size, size));
    checkboxBox.setPosition(sf::Vector2f(x, y));
    checkboxBox.setFillColor(sf::Color(40, 40, 40));
    checkboxBox.setOutlineColor(sf::Color(150, 150, 150));
    checkboxBox.setOutlineThickness(2.0f);
    
    // Checkmark (✓ symbol)
    checkmark = new sf::Text(font, "", static_cast<unsigned int>(size * 0.8f));
    checkmark->setFillColor(sf::Color(100, 255, 100));
    float boxCenterX = x + size / 2.0f;
    float boxCenterY = y + size / 2.0f;
    sf::FloatRect checkBounds = checkmark->getLocalBounds();
    checkmark->setPosition(sf::Vector2f(
        boxCenterX - checkBounds.size.x / 2.0f,
        boxCenterY - checkBounds.size.y / 2.0f - checkBounds.position.y
    ));
    
    // Label
    labelText = new sf::Text(font, label, 16);
    labelText->setFillColor(sf::Color::White);
    labelText->setPosition(sf::Vector2f(x + size + 10.0f, y + size / 2.0f - 8.0f));
    
    initialized = true;
}

void UICheckbox::render(Renderer& renderer) {
    if (!initialized) return;
    
    renderer.drawRectangle(checkboxBox);
    if (isChecked) {
        renderer.drawText(*checkmark);
    }
    renderer.drawText(*labelText);
}

void UICheckbox::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    // Check if clicking on checkbox or label
    sf::FloatRect labelBounds = labelText->getGlobalBounds();
    float expandedRight = labelBounds.position.x + labelBounds.size.x + 10.0f;
    sf::FloatRect clickableArea(
        sf::Vector2f(checkboxBox.getPosition().x, checkboxBox.getPosition().y),
        sf::Vector2f(expandedRight - checkboxBox.getPosition().x, checkboxBox.getSize().y)
    );
    
    if (clickableArea.contains(mousePos)) {
        toggle();
    }
}

void UICheckbox::toggle() {
    isChecked = !isChecked;
    updateCheckmark();
    
    if (onToggle) {
        onToggle(isChecked);
    }
}

void UICheckbox::setChecked(bool checked) {
    if (isChecked != checked) {
        isChecked = checked;
        updateCheckmark();
    }
}

void UICheckbox::setCallback(std::function<void(bool)> callback) {
    onToggle = callback;
}

void UICheckbox::updateCheckmark() {
    if (!initialized || !checkmark) return;
    
    if (isChecked) {
        checkmark->setString("\u2713");  // ✓ checkmark
    } else {
        checkmark->setString("");
    }
}
