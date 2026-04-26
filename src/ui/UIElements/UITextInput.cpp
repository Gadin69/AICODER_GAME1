#include "UITextInput.h"
#include "../../rendering/Renderer.h"
#include <algorithm>

UITextInput::UITextInput() {
}

UITextInput::~UITextInput() {
    delete inputText;
    delete labelText;
}

void UITextInput::initialize(float x, float y, float width, float height, const std::string& label, const sf::Font& font) {
    // Set base class properties
    position = sf::Vector2f(x, y);
    size = sf::Vector2f(width, height);
    fontPtr = &font;
    
    // Input box
    inputBox.setSize(sf::Vector2f(width, height));
    inputBox.setPosition(sf::Vector2f(x, y));
    inputBox.setFillColor(sf::Color(30, 30, 30));
    inputBox.setOutlineColor(sf::Color(100, 100, 100));
    inputBox.setOutlineThickness(2.0f);
    
    // Label
    labelText = new sf::Text(font, label, 16);
    labelText->setFillColor(sf::Color(200, 200, 200));
    labelText->setPosition(sf::Vector2f(x, y - 22.0f));
    
    // Input text
    inputText = new sf::Text(font, "", 16);
    inputText->setFillColor(sf::Color::White);
    inputText->setPosition(sf::Vector2f(x + 5.0f, y + height / 2.0f - 8.0f));
    
    // Cursor
    cursor.setSize(sf::Vector2f(2.0f, height - 8.0f));
    cursor.setPosition(sf::Vector2f(x + 5.0f, y + 4.0f));
    cursor.setFillColor(sf::Color(255, 255, 255));
    
    initialized = true;
}

void UITextInput::render(Renderer& renderer) {
    if (!initialized) return;
    
    // CRITICAL: UITextInput elements use LOCAL coordinates (relative to component's top-left)
    // but SFML renders in SCREEN coordinates. We must offset all elements by the
    // component's position before rendering, then restore them.
    //
    // Common bug: Without this offset, text input renders at wrong screen position
    // (e.g., at initialize coordinates instead of UIBorder-calculated position).
    
    // Get current screen position
    sf::Vector2f currentPos = getPosition();
    
    // Store original positions
    sf::Vector2f originalBoxPos = inputBox.getPosition();
    sf::Vector2f originalLabelPos = labelText->getPosition();
    sf::Vector2f originalInputPos = inputText->getPosition();
    sf::Vector2f originalCursorPos = cursor.getPosition();
    
    // Offset all elements to screen coordinates
    // The elements were initialized with absolute positions, so we need to:
    // 1. Calculate the offset from initialize position to current position
    // 2. Apply that offset to all elements
    float offsetX = currentPos.x - originalBoxPos.x;
    float offsetY = currentPos.y - originalBoxPos.y;
    
    inputBox.setPosition(sf::Vector2f(originalBoxPos.x + offsetX, originalBoxPos.y + offsetY));
    labelText->setPosition(sf::Vector2f(originalLabelPos.x + offsetX, originalLabelPos.y + offsetY));
    inputText->setPosition(sf::Vector2f(originalInputPos.x + offsetX, originalInputPos.y + offsetY));
    cursor.setPosition(sf::Vector2f(originalCursorPos.x + offsetX, originalCursorPos.y + offsetY));
    
    // Render at screen coordinates
    renderer.drawRectangle(inputBox);
    renderer.drawText(*labelText);
    renderer.drawText(*inputText);
    
    // Draw cursor if focused and visible
    if (isFocused && cursorVisible) {
        renderer.drawRectangle(cursor);
    }
    
    // Restore original positions (so they remain consistent for next frame)
    inputBox.setPosition(originalBoxPos);
    labelText->setPosition(originalLabelPos);
    inputText->setPosition(originalInputPos);
    cursor.setPosition(originalCursorPos);
}

void UITextInput::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    // CRITICAL: Must use current screen position for hit detection, not initialize position
    sf::Vector2f currentPos = getPosition();
    sf::Vector2f currentSize = getSize();
    
    // Create bounds at current screen position
    sf::FloatRect bounds(currentPos, currentSize);
    
    if (bounds.contains(mousePos)) {
        isFocused = true;
        cursorBlinkTime = 0.0f;
        cursorVisible = true;
    } else {
        isFocused = false;
    }
}

void UITextInput::handleKeyPress(const sf::Event::KeyPressed& keyEvent) {
    if (!initialized || !isFocused) return;
    
    // Handle Enter key (submit)
    if (keyEvent.scancode == sf::Keyboard::Scancode::Enter || keyEvent.scancode == sf::Keyboard::Scancode::NumpadEnter) {
        if (onSubmit) {
            onSubmit(text);
        }
        isFocused = false;
        return;
    }
    
    // Handle Backspace
    if (keyEvent.scancode == sf::Keyboard::Scancode::Backspace) {
        if (!text.empty()) {
            text.pop_back();
            inputText->setString(text);
            updateCursorPosition();
        }
        return;
    }
}

void UITextInput::handleTextEntered(const sf::Event::TextEntered& textEvent) {
    if (!initialized || !isFocused) return;
    
    // Only handle printable characters
    if (textEvent.unicode < 32 || textEvent.unicode > 126) return;
    
    // Check max length
    if (static_cast<int>(text.length()) >= maxLength) return;
    
    // Add character
    char c = static_cast<char>(textEvent.unicode);
    text += c;
    inputText->setString(text);
    updateCursorPosition();
    
    // Reset cursor blink
    cursorBlinkTime = 0.0f;
    cursorVisible = true;
}

void UITextInput::setText(const std::string& newText) {
    text = newText;
    if (inputText) {
        inputText->setString(text);
    }
    updateCursorPosition();
}

std::string UITextInput::getText() const {
    return text;
}

void UITextInput::setMaxLength(int max) {
    maxLength = max;
}

void UITextInput::setCallback(std::function<void(const std::string&)> callback) {
    onSubmit = callback;
}

void UITextInput::updateCursorPosition() {
    if (!initialized || !inputText) return;
    
    // Position cursor at end of text
    sf::FloatRect textBounds = inputText->getLocalBounds();
    float cursorX = inputText->getPosition().x + textBounds.size.x + 2.0f;
    cursor.setPosition(sf::Vector2f(cursorX, cursor.getPosition().y));
}

void UITextInput::updateCursorVisibility(float deltaTime) {
    if (!isFocused) return;
    
    cursorBlinkTime += deltaTime;
    if (cursorBlinkTime > 0.5f) {
        cursorVisible = !cursorVisible;
        cursorBlinkTime = 0.0f;
    }
}
