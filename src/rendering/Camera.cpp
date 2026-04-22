#include "Camera.h"

Camera::Camera()
    : position(0.0f, 0.0f)
    , zoom(1.0f)
{
    view = sf::View(sf::FloatRect(0, 0, 800, 600));
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
    view.setSize(800.0f / zoom, 600.0f / zoom);
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
    return view.getInverse().transformPoint(screenX, screenY);
}

sf::Vector2f Camera::worldToScreen(float worldX, float worldY, const sf::Vector2u& windowSize) const {
    sf::Vector2f worldPos(worldX, worldY);
    sf::Vector2f screenPos = view.transformPoint(worldPos);
    return screenPos;
}
