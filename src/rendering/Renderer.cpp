#include "Renderer.h"

Renderer::Renderer()
    : renderWindow(nullptr)
{
}

void Renderer::initialize(sf::RenderWindow& window) {
    renderWindow = &window;
}

void Renderer::beginFrame() {
    renderWindow->clear(sf::Color::Black);
    camera.applyTo(*renderWindow);
    spriteBatch.begin();
}

void Renderer::endFrame() {
    spriteBatch.end(*renderWindow);
}

void Renderer::setCamera(const Camera& cam) {
    camera = cam;
}

Camera& Renderer::getCamera() {
    return camera;
}

void Renderer::drawTileMap(TileMap& tileMap) {
    sf::Vector2u windowSize = renderWindow->getSize();
    sf::Vector2f cameraPos = camera.getPosition();
    sf::Vector2f cameraSize(windowSize.x / camera.getZoom(), windowSize.y / camera.getZoom());
    tileMap.render(*renderWindow, cameraPos, cameraSize);
}

void Renderer::drawSprite(const sf::Sprite& sprite) {
    spriteBatch.draw(sprite);
}

void Renderer::drawRectangle(const sf::RectangleShape& rectangle) {
    renderWindow->draw(rectangle);
}

void Renderer::drawText(sf::Text& text) {
    renderWindow->draw(text);
}

sf::RenderWindow& Renderer::getRenderWindow() {
    return *renderWindow;
}
