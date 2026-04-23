#include "Camera.h"
#include <algorithm>

Camera::Camera()
    : position(0.0f, 0.0f)
    , zoom(1.0f)
    , velocity(0.0f, 0.0f)
{
    // Initialize arrow key states
    for (int i = 0; i < 4; ++i) {
        arrowKeys[i] = false;
        mouseEdgeActive[i] = false;
    }
    
    view = sf::View(sf::FloatRect(sf::Vector2f(0, 0), sf::Vector2f(800, 600)));
}

void Camera::setPosition(float x, float y) {
    position.x = x;
    position.y = y;
    view.setCenter(position);
}

void Camera::move(float dx, float dy) {
    position.x += dx;
    position.y += dy;
    view.setCenter(position);
}

void Camera::setZoom(float newZoom) {
    zoom = newZoom;
    view.setSize(sf::Vector2f(800.0f / zoom, 600.0f / zoom));
    view.setCenter(position);
}

void Camera::zoomIn(float amount) {
    setZoom(zoom + amount);
}

void Camera::zoomOut(float amount) {
    setZoom(zoom - amount);
}

sf::Vector2f Camera::getPosition() const {
    return position;
}

float Camera::getZoom() const {
    return zoom;
}

sf::View Camera::getView() const {
    return view;
}

void Camera::applyTo(sf::RenderWindow& window) {
    window.setView(view);
}

sf::Vector2f Camera::screenToWorld(float screenX, float screenY) const {
    sf::Vector2f screenPos(screenX, screenY);
    return view.getInverseTransform().transformPoint(screenPos);
}

sf::Vector2f Camera::worldToScreen(float worldX, float worldY, const sf::Vector2u& windowSize) const {
    sf::RenderWindow* window = const_cast<sf::RenderWindow*>(static_cast<const sf::RenderWindow*>(nullptr));
    sf::Vector2f worldPos(worldX, worldY);
    return worldPos;
}

// Smooth scrolling implementation
void Camera::update(float deltaTime) {
    // Calculate target velocity from inputs
    sf::Vector2f targetVelocity(0.0f, 0.0f);
    
    // Arrow key input
    if (arrowKeys[0]) targetVelocity.y -= scrollSpeed;  // Up
    if (arrowKeys[1]) targetVelocity.y += scrollSpeed;  // Down
    if (arrowKeys[2]) targetVelocity.x -= scrollSpeed;  // Left
    if (arrowKeys[3]) targetVelocity.x += scrollSpeed;  // Right
    
    // Mouse edge input
    if (mouseEdgeActive[0]) targetVelocity.y -= scrollSpeed;
    if (mouseEdgeActive[1]) targetVelocity.y += scrollSpeed;
    if (mouseEdgeActive[2]) targetVelocity.x -= scrollSpeed;
    if (mouseEdgeActive[3]) targetVelocity.x += scrollSpeed;
    
    // Smooth acceleration/deceleration
    if (targetVelocity.x != 0 || targetVelocity.y != 0) {
        // Accelerate towards target velocity
        float speed = std::sqrt(targetVelocity.x * targetVelocity.x + targetVelocity.y * targetVelocity.y);
        if (speed > maxSpeed) {
            targetVelocity = targetVelocity / speed * maxSpeed;
        }
        
        velocity.x += (targetVelocity.x - velocity.x) * acceleration * deltaTime;
        velocity.y += (targetVelocity.y - velocity.y) * acceleration * deltaTime;
    } else {
        // Decelerate when no input
        float decelFactor = deceleration * deltaTime;
        if (std::abs(velocity.x) < decelFactor) velocity.x = 0;
        else velocity.x -= (velocity.x > 0 ? decelFactor : -decelFactor);
        
        if (std::abs(velocity.y) < decelFactor) velocity.y = 0;
        else velocity.y -= (velocity.y > 0 ? decelFactor : -decelFactor);
    }
    
    // Apply velocity
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    
    // Clamp to extended bounds (grid + half view size for empty space viewing)
    // This allows camera to scroll past grid edges into empty space
    if (hasGridBounds) {
        sf::Vector2f viewSize = view.getSize();
        float extendedMinX = gridMinX - viewSize.x / 2.0f;
        float extendedMaxX = gridMaxX + viewSize.x / 2.0f;
        float extendedMinY = gridMinY - viewSize.y / 2.0f;
        float extendedMaxY = gridMaxY + viewSize.y / 2.0f;
        
        position.x = std::max(extendedMinX, std::min(extendedMaxX, position.x));
        position.y = std::max(extendedMinY, std::min(extendedMaxY, position.y));
    }
    
    // Update view
    view.setCenter(position);
}

void Camera::setScrollSpeed(float speed) {
    scrollSpeed = speed;
}

void Camera::setEdgeMargin(float margin) {
    edgeScrollMargin = margin;
}

void Camera::handleArrowKeys(const sf::Keyboard::Scancode key, bool pressed) {
    switch (key) {
        case sf::Keyboard::Scancode::Up:
        case sf::Keyboard::Scancode::W:
            arrowKeys[0] = pressed;  // Up
            break;
        case sf::Keyboard::Scancode::Down:
        case sf::Keyboard::Scancode::S:
            arrowKeys[1] = pressed;  // Down
            break;
        case sf::Keyboard::Scancode::Left:
        case sf::Keyboard::Scancode::A:
            arrowKeys[2] = pressed;  // Left
            break;
        case sf::Keyboard::Scancode::Right:
        case sf::Keyboard::Scancode::D:
            arrowKeys[3] = pressed;  // Right
            break;
        default:
            break;
    }
}

void Camera::handleMouseEdge(const sf::Vector2f& mousePos, const sf::Vector2u& windowSize) {
    // Check if mouse is near edges
    mouseEdgeActive[0] = mousePos.y < edgeScrollMargin;  // Top
    mouseEdgeActive[1] = mousePos.y > windowSize.y - edgeScrollMargin;  // Bottom
    mouseEdgeActive[2] = mousePos.x < edgeScrollMargin;  // Left
    mouseEdgeActive[3] = mousePos.x > windowSize.x - edgeScrollMargin;  // Right
}

void Camera::setGridBounds(float minX, float minY, float maxX, float maxY) {
    gridMinX = minX;
    gridMinY = minY;
    gridMaxX = maxX;
    gridMaxY = maxY;
    hasGridBounds = true;
}
