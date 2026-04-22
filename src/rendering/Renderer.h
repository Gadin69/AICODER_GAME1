#pragma once

#include <SFML/Graphics.hpp>
#include "Camera.h"
#include "TileMap.h"
#include "SpriteBatch.h"

class Renderer {
public:
    Renderer();

    void initialize(sf::RenderWindow& window);
    void beginFrame();
    void endFrame();

    void setCamera(const Camera& camera);
    Camera& getCamera();

    void drawTileMap(TileMap& tileMap);
    void drawSprite(const sf::Sprite& sprite);
    void drawRectangle(const sf::RectangleShape& rectangle);
    void drawText(sf::Text& text);

    sf::RenderWindow& getRenderWindow();

private:
    sf::RenderWindow* renderWindow;
    Camera camera;
    SpriteBatch spriteBatch;
};
