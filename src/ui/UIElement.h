#pragma once

#include <SFML/Graphics.hpp>

class Renderer;  // Forward declaration

class UIElement {
public:
    UIElement();
    virtual ~UIElement();
    
    // Core lifecycle methods
    virtual void initialize(float x, float y, float width, float height) {}  // Default implementation
    virtual void render(Renderer& renderer) = 0;  // Pure virtual - must override
    
    // Mouse input handling
    virtual void handleMousePress(const sf::Vector2f& mousePos) = 0;
    virtual void handleMouseRelease() {}  // Optional override
    virtual void handleMouseMove(const sf::Vector2f& mousePos) {}  // Optional override
    
    // Keyboard input handling (optional override)
    virtual void handleKeyPress(const sf::Event::KeyPressed& keyEvent) {}
    virtual void handleTextEntered(const sf::Event::TextEntered& textEvent) {}
    
    // Position/size accessors
    sf::Vector2f getPosition() const { return position; }
    sf::Vector2f getSize() const { return size; }
    void setPosition(float x, float y);
    void setSize(float width, float height);
    
    // Visibility and state
    bool isVisible() const { return visible; }
    void setVisible(bool vis) { visible = vis; }
    bool isInitialized() const { return initialized; }
    
    // Hit testing
    virtual bool containsPoint(const sf::Vector2f& point) const;
    
protected:
    sf::Vector2f position;
    sf::Vector2f size;
    bool initialized = false;
    bool visible = true;
    const sf::Font* fontPtr = nullptr;
};
