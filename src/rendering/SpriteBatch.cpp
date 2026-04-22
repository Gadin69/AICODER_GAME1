#include "SpriteBatch.h"

SpriteBatch::SpriteBatch()
    : isDrawing(false)
{
}

void SpriteBatch::begin() {
    isDrawing = true;
    sprites.clear();
}

void SpriteBatch::draw(const sf::Sprite& sprite) {
    if (isDrawing) {
        sprites.push_back(sprite);
    }
}

void SpriteBatch::draw(const sf::Sprite& sprite, const sf::Color& color) {
    if (isDrawing) {
        sf::Sprite coloredSprite = sprite;
        coloredSprite.setColor(color);
        sprites.push_back(coloredSprite);
    }
}

void SpriteBatch::end(sf::RenderTarget& target) {
    if (isDrawing) {
        for (const auto& sprite : sprites) {
            target.draw(sprite);
        }
        isDrawing = false;
    }
}

void SpriteBatch::clear() {
    sprites.clear();
    isDrawing = false;
}
