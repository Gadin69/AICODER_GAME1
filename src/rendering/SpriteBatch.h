#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

class SpriteBatch {
public:
    SpriteBatch();

    void begin();
    void draw(const sf::Sprite& sprite);
    void draw(const sf::Sprite& sprite, const sf::Color& color);
    void end(sf::RenderTarget& target);

    void clear();

private:
    std::vector<sf::Sprite> sprites;
    bool isDrawing;
};
