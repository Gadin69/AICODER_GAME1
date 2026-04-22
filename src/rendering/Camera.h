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

private:
    sf::Vector2f position;
    float zoom;
    sf::View view;
};
