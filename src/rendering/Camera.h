#pragma once

#include <SFML/Graphics.hpp>

class Camera {
public:
    Camera();

    void setPosition(float x, float y);
    void move(float dx, float dy);
    void setZoom(float zoom);
    void zoomIn(float amount = 0.1f);
    void zoomOut(float amount = 0.1f);

    sf::Vector2f getPosition() const;
    float getZoom() const;

    sf::View getView() const;
    void applyTo(sf::RenderWindow& window);

    sf::Vector2f screenToWorld(float screenX, float screenY) const;
    sf::Vector2f worldToScreen(float worldX, float worldY, const sf::Vector2u& windowSize) const;
    
    // Smooth scrolling
    void update(float deltaTime);
    void setScrollSpeed(float speed);
    void setEdgeMargin(float margin);
    void handleArrowKeys(const sf::Keyboard::Scancode key, bool pressed);
    void handleMouseEdge(const sf::Vector2f& mousePos, const sf::Vector2u& windowSize);
    void setGridBounds(float minX, float minY, float maxX, float maxY);

private:
    sf::Vector2f position;
    float zoom;
    sf::View view;
    
    // Smooth scrolling state
    sf::Vector2f velocity;
    float acceleration = 2000.0f;  // pixels/s²
    float deceleration = 1500.0f;  // pixels/s²
    float maxSpeed = 800.0f;  // pixels/s
    bool arrowKeys[4];  // up, down, left, right
    float scrollSpeed = 500.0f;
    float edgeScrollMargin = 50.0f;
    bool mouseEdgeActive[4];  // up, down, left, right
    
    // Grid boundaries
    float gridMinX = 0.0f;
    float gridMinY = 0.0f;
    float gridMaxX = 0.0f;
    float gridMaxY = 0.0f;
    bool hasGridBounds = false;
};
