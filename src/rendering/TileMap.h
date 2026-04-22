#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct TileInfo {
    sf::Color color;
    bool solid;
    std::string name;
};

class TileMap {
public:
    TileMap();

    void initialize(int width, int height, float tileSize);
    void setTile(int x, int y, const TileInfo& tile);
    TileInfo getTile(int x, int y) const;

    void render(sf::RenderTarget& target, const sf::Vector2f& cameraPos, const sf::Vector2f& cameraSize);

    int getWidth() const;
    int getHeight() const;
    float getTileSize() const;

    sf::Vector2f getTilePosition(int x, int y) const;
    sf::Vector2i getTileAt(float worldX, float worldY) const;

private:
    int width;
    int height;
    float tileSize;
    std::vector<std::vector<TileInfo>> tiles;
    sf::VertexArray vertexArray;
    bool needsUpdate;

    void updateVertexArray();
};
