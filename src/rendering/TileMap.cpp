#include "TileMap.h"

TileMap::TileMap()
    : width(0)
    , height(0)
    , tileSize(32.0f)
    , needsUpdate(false)
{
}

void TileMap::initialize(int w, int h, float tileSz) {
    width = w;
    height = h;
    tileSize = tileSz;

    tiles.resize(height);
    for (int y = 0; y < height; ++y) {
        tiles[y].resize(width);
        for (int x = 0; x < width; ++x) {
            tiles[y][x] = {sf::Color::Transparent, false, "empty"};
        }
    }

    vertexArray.setPrimitiveType(sf::Quads);
    vertexArray.resize(width * height * 4);
    needsUpdate = true;
}

void TileMap::setTile(int x, int y, const TileInfo& tile) {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        tiles[y][x] = tile;
        needsUpdate = true;
    }
}

TileInfo TileMap::getTile(int x, int y) const {
    if (x >= 0 && x < width && y >= 0 && y < height) {
        return tiles[y][x];
    }
    return {sf::Color::Transparent, false, "empty"};
}

void TileMap::render(sf::RenderTarget& target, const sf::Vector2f& cameraPos, const sf::Vector2f& cameraSize) {
    if (needsUpdate) {
        updateVertexArray();
    }

    // Calculate visible tile range
    int startX = std::max(0, static_cast<int>((cameraPos.x - cameraSize.x / 2) / tileSize));
    int startY = std::max(0, static_cast<int>((cameraPos.y - cameraSize.y / 2) / tileSize));
    int endX = std::min(width, static_cast<int>((cameraPos.x + cameraSize.x / 2) / tileSize) + 1);
    int endY = std::min(height, static_cast<int>((cameraPos.y + cameraSize.y / 2) / tileSize) + 1);

    // Only draw visible tiles
    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            const TileInfo& tile = tiles[y][x];
            if (tile.color.a > 0) {
                sf::RectangleShape tileRect(sf::Vector2f(tileSize, tileSize));
                tileRect.setPosition(x * tileSize, y * tileSize);
                tileRect.setFillColor(tile.color);
                target.draw(tileRect);
            }
        }
    }
}

int TileMap::getWidth() const {
    return width;
}

int TileMap::getHeight() const {
    return height;
}

float TileMap::getTileSize() const {
    return tileSize;
}

sf::Vector2f TileMap::getTilePosition(int x, int y) const {
    return sf::Vector2f(x * tileSize, y * tileSize);
}

sf::Vector2i TileMap::getTileAt(float worldX, float worldY) const {
    int tileX = static_cast<int>(worldX / tileSize);
    int tileY = static_cast<int>(worldY / tileSize);
    return sf::Vector2i(tileX, tileY);
}

void TileMap::updateVertexArray() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 4;
            const TileInfo& tile = tiles[y][x];

            vertexArray[index + 0].position = sf::Vector2f(x * tileSize, y * tileSize);
            vertexArray[index + 1].position = sf::Vector2f((x + 1) * tileSize, y * tileSize);
            vertexArray[index + 2].position = sf::Vector2f((x + 1) * tileSize, (y + 1) * tileSize);
            vertexArray[index + 3].position = sf::Vector2f(x * tileSize, (y + 1) * tileSize);

            vertexArray[index + 0].color = tile.color;
            vertexArray[index + 1].color = tile.color;
            vertexArray[index + 2].color = tile.color;
            vertexArray[index + 3].color = tile.color;
        }
    }
    needsUpdate = false;
}
