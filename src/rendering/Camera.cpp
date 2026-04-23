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
    // Manually calculate world position from screen coordinates
    sf::Vector2f viewSize = view.getSize();
    sf::Vector2f viewCenter = view.getCenter();
    
    // Get the viewport size (should match window size)
    sf::Vector2i viewportSize = sf::Vector2i(
        static_cast<int>(viewSize.x),
        static_cast<int>(viewSize.y)
    );
    
    // Calculate world position
    float worldX = viewCenter.x + (screenX - viewportSize.x / 2.0f);
    float worldY = viewCenter.y + (screenY - viewportSize.y / 2.0f);
    
    return sf::Vector2f(worldX, worldY);
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
        // Accelerate towards target velocity using additive acceleration (frame-rate independent)
        float speed = std::sqrt(targetVelocity.x * targetVelocity.x + targetVelocity.y * targetVelocity.y);
        if (speed > maxSpeed) {
            targetVelocity = targetVelocity / speed * maxSpeed;
        }
        
        // Calculate acceleration amount for this frame
        float accelAmount = acceleration * deltaTime;
        
        // Accelerate each axis independently towards target
        if (velocity.x < targetVelocity.x) {
            velocity.x = std::min(targetVelocity.x, velocity.x + accelAmount);
        } else if (velocity.x > targetVelocity.x) {
            velocity.x = std::max(targetVelocity.x, velocity.x - accelAmount);
        }
        
        if (velocity.y < targetVelocity.y) {
            velocity.y = std::min(targetVelocity.y, velocity.y + accelAmount);
        } else if (velocity.y > targetVelocity.y) {
            velocity.y = std::max(targetVelocity.y, velocity.y - accelAmount);
        }
    } else {
        // Decelerate when no input
        float decelFactor = deceleration * deltaTime;
        if (std::abs(velocity.x) < decelFactor) velocity.x = 0;
        else velocity.x -= (velocity.x > 0 ? decelFactor : -decelFactor);
        
        if (std::abs(velocity.y) < decelFactor) velocity.y = 0;
        else velocity.y -= (velocity.y > 0 ? decelFactor : -decelFactor);
    }
    
    // Hard clamp velocity to prevent runaway acceleration
    float currentSpeed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (currentSpeed > maxSpeed * 2.0f) {  // Allow temporary overshoot but prevent infinite
        velocity = velocity / currentSpeed * maxSpeed * 2.0f;
    }
    
    // Apply velocity
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;
    
    // Sanity check: if position jumped too far, reset velocity
    sf::Vector2f lastPos = position - sf::Vector2f(velocity.x * deltaTime, velocity.y * deltaTime);
    float jumpDistance = std::sqrt((position.x - lastPos.x) * (position.x - lastPos.x) + 
                                    (position.y - lastPos.y) * (position.y - lastPos.y));
    if (jumpDistance > maxSpeed * deltaTime * 3.0f) {
        velocity = sf::Vector2f(0, 0);  // Reset velocity on impossible jump
        position = lastPos;  // Revert to valid position
    }
    
    // Clamp to extended bounds (grid + 1/8 view size for limited empty space viewing)
    // This ensures 75% of screen still shows map area at full scroll
    if (hasGridBounds) {
        sf::Vector2f viewSize = view.getSize();
        float marginX = viewSize.x * 0.125f;  // 12.5% of view width (1/8)
        float marginY = viewSize.y * 0.125f;  // 12.5% of view height (1/8)
        
        float extendedMinX = gridMinX - marginX;
        float extendedMaxX = gridMaxX + marginX;
        float extendedMinY = gridMinY - marginY;
        float extendedMaxY = gridMaxY + marginY;
        
        position.x = std::max(extendedMinX, std::min(extendedMaxX, position.x));
        position.y = std::max(extendedMinY, std::min(extendedMaxY, position.y));
    }
    
    // Update view
    view.setCenter(position);
    
    // Clear input flags at END of update to prevent stale state in next frame
    // (handleMouseEdge will set them again before next update call)
    for (int i = 0; i < 4; ++i) {
        mouseEdgeActive[i] = false;
    }
}

void Camera::setScrollSpeed(float speed) {
    scrollSpeed = speed;
}

void Camera::setAcceleration(float accel) {
    acceleration = accel;
    deceleration = accel * 1.5f;  // Keep deceleration at 1.5x acceleration
}

void Camera::setMaxSpeed(float maxSpd) {
    maxSpeed = maxSpd;
}

void Camera::setEdgeMargin(float margin) {
    edgeScrollMargin = margin;
}

void Camera::setViewSize(float width, float height) {
    view.setSize(sf::Vector2f(width, height));
    view.setCenter(position);
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
    // Validate mouse position is within window bounds
    if (mousePos.x < 0 || mousePos.x > windowSize.x || 
        mousePos.y < 0 || mousePos.y > windowSize.y) {
        // Mouse outside window, disable all edge scrolling
        for (int i = 0; i < 4; ++i) mouseEdgeActive[i] = false;
        return;
    }
    
    // Only trigger on the very edge pixels (1-2 pixels from border)
    mouseEdgeActive[0] = mousePos.y < 2.0f;  // Top edge
    mouseEdgeActive[1] = mousePos.y > static_cast<float>(windowSize.y) - 2.0f;  // Bottom edge
    mouseEdgeActive[2] = mousePos.x < 2.0f;  // Left edge
    mouseEdgeActive[3] = mousePos.x > static_cast<float>(windowSize.x) - 2.0f;  // Right edge
}

void Camera::setGridBounds(float minX, float minY, float maxX, float maxY) {
    gridMinX = minX;
    gridMinY = minY;
    gridMaxX = maxX;
    gridMaxY = maxY;
    hasGridBounds = true;
}
