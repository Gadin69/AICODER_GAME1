#include "UIButton.h"
#include "rendering/Renderer.h"

UIButton::UIButton() {
}

UIButton::~UIButton() {
    delete buttonText;
}

void UIButton::initialize(float x, float y, float width, float height, const std::string& text, const sf::Font& font) {
    // Set base class properties
    position = sf::Vector2f(x, y);
    size = sf::Vector2f(width, height);
    fontPtr = &font;
    
    // Background
    background.setSize(sf::Vector2f(width, height));
    background.setPosition(sf::Vector2f(x, y));
    background.setFillColor(normalColor);
    background.setOutlineColor(sf::Color(150, 150, 150));
    background.setOutlineThickness(2.0f);
    
    // Text
    buttonText = new sf::Text(font, text, 18);
    
    // Center text in button
    sf::FloatRect textBounds = buttonText->getLocalBounds();
    float textX = x + (width - textBounds.size.x) / 2.0f;
    float textY = y + (height - textBounds.size.y) / 2.0f - textBounds.position.y;
    buttonText->setPosition(sf::Vector2f(textX, textY));
    buttonText->setFillColor(sf::Color::White);
    
    initialized = true;
}

void UIButton::render(Renderer& renderer) {
    if (!initialized) return;
    
    renderer.drawRectangle(background);
    renderer.drawText(*buttonText);
}

void UIButton::setPosition(float x, float y) {
    // Call base class to update position variable
    UIElement::setPosition(x, y);
    
    // Update SFML background shape position
    background.setPosition(sf::Vector2f(x, y));
    
    // Update text position (maintain centering)
    if (buttonText) {
        sf::FloatRect textBounds = buttonText->getLocalBounds();
        float textX = x + (getSize().x - textBounds.size.x) / 2.0f;
        float textY = y + (getSize().y - textBounds.size.y) / 2.0f - textBounds.position.y;
        buttonText->setPosition(sf::Vector2f(textX, textY));
    }
}

void UIButton::setSize(float width, float height) {
    // Call base class to update size variable
    UIElement::setSize(width, height);
    
    // Update SFML background shape size
    background.setSize(sf::Vector2f(width, height));
    
    // Update text position (re-center with new size)
    if (buttonText) {
        sf::FloatRect textBounds = buttonText->getLocalBounds();
        float textX = getPosition().x + (width - textBounds.size.x) / 2.0f;
        float textY = getPosition().y + (height - textBounds.size.y) / 2.0f - textBounds.position.y;
        buttonText->setPosition(sf::Vector2f(textX, textY));
    }
}

void UIButton::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    if (containsPoint(mousePos)) {
        isPressed = true;
        updateColor();
    }
}

void UIButton::handleMouseMove(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    bool wasHovered = isHovered;
    isHovered = containsPoint(mousePos);
    
    if (isHovered != wasHovered) {
        updateColor();
    }
}

void UIButton::handleMouseRelease() {
    if (!initialized) return;
    
    if (isPressed && isHovered && onClick) {
        onClick();
    }
    
    isPressed = false;
    updateColor();
}

void UIButton::setText(const std::string& text) {
    if (!initialized || !buttonText || !fontPtr) return;
    
    buttonText->setString(text);
    
    // Re-center text
    sf::FloatRect textBounds = buttonText->getLocalBounds();
    float buttonX = background.getPosition().x;
    float buttonY = background.getPosition().y;
    float width = background.getSize().x;
    float height = background.getSize().y;
    float textX = buttonX + (width - textBounds.size.x) / 2.0f;
    float textY = buttonY + (height - textBounds.size.y) / 2.0f - textBounds.position.y;
    buttonText->setPosition(sf::Vector2f(textX, textY));
}

void UIButton::setCallback(std::function<void()> callback) {
    onClick = callback;
}

void UIButton::setColors(sf::Color normal, sf::Color hover, sf::Color pressed) {
    normalColor = normal;
    hoverColor = hover;
    pressedColor = pressed;
    updateColor();
}

void UIButton::updateColor() {
    if (!initialized) return;
    
    if (isPressed) {
        background.setFillColor(pressedColor);
    } else if (isHovered) {
        background.setFillColor(hoverColor);
    } else {
        background.setFillColor(normalColor);
    }
}
