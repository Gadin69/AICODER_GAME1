#include "UIThumbnailViewer.h"
#include "../../rendering/Renderer.h"
#include <iostream>

UIThumbnailViewer::UIThumbnailViewer() {
}

UIThumbnailViewer::~UIThumbnailViewer() {
    delete thumbnailTexture;
}

void UIThumbnailViewer::initialize(float x, float y, float width, float height) {
    position = sf::Vector2f(x, y);
    size = sf::Vector2f(width, height);
    viewerSize = sf::Vector2f(width, height);
    initialized = true;  // Mark as initialized
}

bool UIThumbnailViewer::loadThumbnail(const std::string& filePath) {
    if (filePath.empty()) {
        clearThumbnail();
        return false;
    }
    
    // Create texture if needed
    if (!thumbnailTexture) {
        thumbnailTexture = new sf::Texture();
    }
    
    if (!thumbnailTexture->loadFromFile(filePath)) {
        std::cerr << "[UIThumbnailViewer] Failed to load thumbnail: " << filePath << std::endl;
        clearThumbnail();
        return false;
    }
    
    hasThumbnailLoaded = true;
    
    // Calculate scale and position for sprite (will create sprite in render())
    sf::Vector2f textureSize(thumbnailTexture->getSize().x, thumbnailTexture->getSize().y);
    float scaleX = viewerSize.x / textureSize.x;
    float scaleY = viewerSize.y / textureSize.y;
    float scale = std::min(scaleX, scaleY);  // Use smaller scale to fit
    
    thumbnailScale = sf::Vector2f(scale, scale);
    
    // Calculate centered position
    float spriteWidth = textureSize.x * scale;
    float spriteHeight = textureSize.y * scale;
    thumbnailPosition = sf::Vector2f(
        position.x + (viewerSize.x - spriteWidth) / 2.0f,
        position.y + (viewerSize.y - spriteHeight) / 2.0f
    );
    
    std::cout << "[UIThumbnailViewer] Loaded thumbnail: " << filePath 
              << " (" << textureSize.x << "x" << textureSize.y << ")" << std::endl;
    
    return true;
}

void UIThumbnailViewer::clearThumbnail() {
    hasThumbnailLoaded = false;
    delete thumbnailTexture;
    thumbnailTexture = nullptr;
}

void UIThumbnailViewer::render(Renderer& renderer) {
    if (!initialized) {
        std::cout << "[UIThumbnailViewer] render: NOT initialized" << std::endl;
        return;
    }
    if (!hasThumbnailLoaded) {
        std::cout << "[UIThumbnailViewer] render: NO thumbnail loaded (hasThumbnailLoaded=false)" << std::endl;
        return;
    }
    if (!thumbnailTexture) {
        std::cout << "[UIThumbnailViewer] render: NULL texture pointer" << std::endl;
        return;
    }
    
    // Get current screen position
    sf::Vector2f currentPos = getPosition();
    
    // Calculate offset from initialize position to current position
    float offsetX = currentPos.x - position.x;
    float offsetY = currentPos.y - position.y;
    
    // Create sprite with current screen position
    sf::Sprite sprite(*thumbnailTexture);
    sprite.setPosition(sf::Vector2f(
        thumbnailPosition.x + offsetX,
        thumbnailPosition.y + offsetY
    ));
    sprite.setScale(thumbnailScale);
    
    // std::cout << "[UIThumbnailViewer] Drawing sprite at (" 
    //           << sprite.getPosition().x << ", " << sprite.getPosition().y 
    //           << ") scale=" << sprite.getScale().x << "x" << sprite.getScale().y 
    //           << " texture=" << thumbnailTexture->getSize().x << "x" << thumbnailTexture->getSize().y 
    //           << std::endl;
    
    // Draw directly to render window (avoid batching issues with local sprite)
    renderer.getRenderWindow().draw(sprite);
}
