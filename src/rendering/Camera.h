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

    sf::Vector2f screenToWorld(float screenX, float screenY, const sf::Vector2u& windowSize) const;
    sf::Vector2f worldToScreen(float worldX, float worldY, const sf::Vector2u& windowSize) const;
    
    // Smooth scrolling
    void update(float deltaTime);
    void setScrollSpeed(float speed);
    void setAcceleration(float accel);
    void setMaxSpeed(float maxSpd);
    void setEdgeMargin(float margin);
    void setViewSize(float width, float height);
    void handleArrowKeys(const sf::Keyboard::Scancode key, bool pressed);
    void handleMouseEdge(const sf::Vector2f& mousePos, const sf::Vector2u& windowSize);
    void setGridBounds(float minX, float minY, float maxX, float maxY);

private:
    sf::Vector2f position;
    float zoom;
    sf::View view;
    
    // Smooth scrolling state
    sf::Vector2f velocity;
    float acceleration = 400.0f;  // pixels/s² (faster acceleration)
    float deceleration = 600.0f;  // pixels/s² (faster deceleration)
    float maxSpeed = 600.0f;  // pixels/s (1.5x top speed)
    bool arrowKeys[4];  // up, down, left, right
    float scrollSpeed = 225.0f;  // pixels/s (1.5x base speed)
    float edgeScrollMargin = 25.0f; // pixels (percent of window size, 25% or 1/4)
    bool mouseEdgeActive[4];  // up, down, left, right
    
    // Grid boundaries
    float gridMinX = 0.0f;
    float gridMinY = 0.0f;
    float gridMaxX = 0.0f;
    float gridMaxY = 0.0f;
    bool hasGridBounds = false;
};
