#include "UINumberInput.h"
#include "rendering/Renderer.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

UINumberInput::UINumberInput() : value(0), minValue(0), maxValue(100) {
}

UINumberInput::~UINumberInput() {
    delete valueText;
    delete labelText;
    delete minusText;
    delete plusText;
}

void UINumberInput::initialize(float x, float y, float width, float height, float minVal, float maxVal, float defaultVal, const sf::Font& font) {
    // Set base class properties
    position = sf::Vector2f(x, y);
    size = sf::Vector2f(width, height);
    fontPtr = &font;
    
    minValue = minVal;
    maxValue = maxVal;
    value = defaultVal;
    
    float btnWidth = 30.0f;
    float displayWidth = width - btnWidth * 2.0f - 4.0f;
    
    // Minus button
    minusBtn.setSize(sf::Vector2f(btnWidth, height));
    minusBtn.setPosition(sf::Vector2f(x, y));
    minusBtn.setFillColor(btnNormalColor);
    minusBtn.setOutlineColor(sf::Color(120, 120, 120));
    minusBtn.setOutlineThickness(1.0f);
    
    minusText = new sf::Text(font, "-", 20);
    minusText->setFillColor(sf::Color::White);
    sf::FloatRect minusBounds = minusText->getLocalBounds();
    minusText->setPosition(sf::Vector2f(
        x + (btnWidth - minusBounds.size.x) / 2.0f,
        y + (height - minusBounds.size.y) / 2.0f - minusBounds.position.y
    ));
    
    // Display box
    displayBox.setSize(sf::Vector2f(displayWidth, height));
    displayBox.setPosition(sf::Vector2f(x + btnWidth + 2.0f, y));
    displayBox.setFillColor(sf::Color(30, 30, 30));
    displayBox.setOutlineColor(sf::Color(100, 100, 100));
    displayBox.setOutlineThickness(2.0f);
    
    // Value text
    valueText = new sf::Text(font, "", 16);
    valueText->setFillColor(sf::Color::White);
    
    // Plus button
    plusBtn.setSize(sf::Vector2f(btnWidth, height));
    plusBtn.setPosition(sf::Vector2f(x + btnWidth + displayWidth + 4.0f, y));
    plusBtn.setFillColor(btnNormalColor);
    plusBtn.setOutlineColor(sf::Color(120, 120, 120));
    plusBtn.setOutlineThickness(1.0f);
    
    plusText = new sf::Text(font, "+", 20);
    plusText->setFillColor(sf::Color::White);
    sf::FloatRect plusBounds = plusText->getLocalBounds();
    plusText->setPosition(sf::Vector2f(
        x + btnWidth + displayWidth + 4.0f + (btnWidth - plusBounds.size.x) / 2.0f,
        y + (height - plusBounds.size.y) / 2.0f - plusBounds.position.y
    ));
    
    updateDisplay();
    initialized = true;
}

void UINumberInput::render(Renderer& renderer) {
    if (!initialized) return;
    
    renderer.drawRectangle(minusBtn);
    renderer.drawText(*minusText);
    renderer.drawRectangle(displayBox);
    renderer.drawText(*valueText);
    renderer.drawRectangle(plusBtn);
    renderer.drawText(*plusText);
}

void UINumberInput::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    // Minus button
    if (minusBtn.getGlobalBounds().contains(mousePos)) {
        isMinusPressed = true;
        updateButtonColors();
        value = std::max(minValue, value - stepSize);
        updateDisplay();
        if (onChange) onChange(value);
        return;
    }
    
    // Plus button
    if (plusBtn.getGlobalBounds().contains(mousePos)) {
        isPlusPressed = true;
        updateButtonColors();
        value = std::min(maxValue, value + stepSize);
        updateDisplay();
        if (onChange) onChange(value);
        return;
    }
    
    // Display box (focus for text input)
    if (displayBox.getGlobalBounds().contains(mousePos)) {
        isFocused = true;
        inputBuffer = std::to_string(static_cast<int>(value));
        return;
    }
    
    isFocused = false;
}

void UINumberInput::handleMouseMove(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    bool wasMinusHovered = isMinusHovered;
    bool wasPlusHovered = isPlusHovered;
    
    isMinusHovered = minusBtn.getGlobalBounds().contains(mousePos);
    isPlusHovered = plusBtn.getGlobalBounds().contains(mousePos);
    
    if (isMinusHovered != wasMinusHovered || isPlusHovered != wasPlusHovered) {
        updateButtonColors();
    }
}

void UINumberInput::handleMouseRelease() {
    if (!initialized) return;
    
    isMinusPressed = false;
    isPlusPressed = false;
    updateButtonColors();
}

void UINumberInput::handleKeyPress(const sf::Event::KeyPressed& keyEvent) {
    if (!initialized || !isFocused) return;
    
    if (keyEvent.scancode == sf::Keyboard::Scancode::Enter || keyEvent.scancode == sf::Keyboard::Scancode::NumpadEnter) {
        // Parse input
        try {
            float newValue = std::stof(inputBuffer);
            newValue = std::max(minValue, std::min(maxValue, newValue));
            value = newValue;
            updateDisplay();
            if (onChange) onChange(value);
        } catch (...) {
            // Invalid input, revert
            updateDisplay();
        }
        isFocused = false;
    } else if (keyEvent.scancode == sf::Keyboard::Scancode::Backspace) {
        if (!inputBuffer.empty()) {
            inputBuffer.pop_back();
        }
    }
}

void UINumberInput::handleTextEntered(const sf::Event::TextEntered& textEvent) {
    if (!initialized || !isFocused) return;
    
    if (textEvent.unicode < 32 || textEvent.unicode > 126) return;
    if (textEvent.unicode == '-') return;  // Allow negative sign
    
    char c = static_cast<char>(textEvent.unicode);
    if (isdigit(c) || c == '.') {
        inputBuffer += c;
    }
}

void UINumberInput::setValue(float newValue) {
    value = std::max(minValue, std::min(maxValue, newValue));
    updateDisplay();
}

void UINumberInput::setStep(float step) {
    stepSize = step;
}

void UINumberInput::setCallback(std::function<void(float)> callback) {
    onChange = callback;
}

void UINumberInput::updateDisplay() {
    if (!initialized || !valueText) return;
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value;
    valueText->setString(oss.str());
    
    // Center text in display box
    sf::FloatRect textBounds = valueText->getLocalBounds();
    float textX = displayBox.getPosition().x + (displayBox.getSize().x - textBounds.size.x) / 2.0f;
    float textY = displayBox.getPosition().y + (displayBox.getSize().y - textBounds.size.y) / 2.0f - textBounds.position.y;
    valueText->setPosition(sf::Vector2f(textX, textY));
}

void UINumberInput::updateButtonColors() {
    if (!initialized) return;
    
    if (isMinusPressed) {
        minusBtn.setFillColor(btnPressedColor);
    } else if (isMinusHovered) {
        minusBtn.setFillColor(btnHoverColor);
    } else {
        minusBtn.setFillColor(btnNormalColor);
    }
    
    if (isPlusPressed) {
        plusBtn.setFillColor(btnPressedColor);
    } else if (isPlusHovered) {
        plusBtn.setFillColor(btnHoverColor);
    } else {
        plusBtn.setFillColor(btnNormalColor);
    }
}
