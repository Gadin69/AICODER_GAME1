#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <string>

class Renderer;  // Forward declaration

/**
 * UIThumbnailViewer - Displays a save game thumbnail image.
 * 
 * Loads and displays a thumbnail image from a file path.
 * Automatically scales to fit within the element's bounds while maintaining aspect ratio.
 */
class UIThumbnailViewer : public UIElement {
public:
    UIThumbnailViewer();
    ~UIThumbnailViewer() override;
    
    // Delete copy operations
    UIThumbnailViewer(const UIThumbnailViewer&) = delete;
    UIThumbnailViewer& operator=(const UIThumbnailViewer&) = delete;
    
    /**
     * Initialize the thumbnail viewer.
     * @param x X position
     * @param y Y position  
     * @param width Width of the viewer
     * @param height Height of the viewer
     */
    void initialize(float x, float y, float width, float height);
    
    /**
     * Load and display a thumbnail from file.
     * @param filePath Path to the thumbnail image file
     * @return true if loaded successfully
     */
    bool loadThumbnail(const std::string& filePath);
    
    /**
     * Clear the current thumbnail.
     */
    void clearThumbnail();
    
    /**
     * Check if a thumbnail is loaded.
     */
    bool hasThumbnail() const { return hasThumbnailLoaded; }
    
    void render(Renderer& renderer) override;
    
    // UIElement interface
    void handleMousePress(const sf::Vector2f& mousePos) override { }  // Thumbnails are non-interactive

private:
    sf::Texture* thumbnailTexture = nullptr;
    sf::Vector2f thumbnailPosition;  // Cached sprite position
    sf::Vector2f thumbnailScale;     // Cached sprite scale
    bool hasThumbnailLoaded = false;
    
    sf::Vector2f viewerSize;  // Size of the viewer area
};
