#pragma once

#include "UIBorder.h"
#include <SFML/Graphics.hpp>
#include <algorithm>

enum class ScrollbarSide {
    Left,
    Right
};

class UIScrollBorder : public UIBorder {
public:
    UIScrollBorder();
    ~UIScrollBorder() override;
    
    // Initialize with optional scrollbar configuration
    void initialize(float x, float y, float width, float height, 
                   ScrollbarSide side = ScrollbarSide::Right, 
                   float scrollbarWidth = 12.0f);
    
    // Override render to add scrollbar and viewport clipping
    void render(Renderer& renderer) override;
    
    // Override mouse handlers to support scrollbar interaction
    void handleMousePress(const sf::Vector2f& mousePos) override;
    void handleMouseMove(const sf::Vector2f& mousePos) override;
    void handleMouseRelease() override;
    
    // Scroll control
    void setScrollOffset(float offset);  // Set scroll position (0 to maxScroll)
    float getScrollOffset() const { return scrollOffset; }
    float getMaxScrollOffset() const;    // Calculate max scroll based on content
    void scrollToTop();
    void scrollToBottom();
    
    // Scrollbar configuration
    void setScrollbarSide(ScrollbarSide side);
    void setScrollbarWidth(float width);
    ScrollbarSide getScrollbarSide() const { return scrollbarSide; }
    
    // Scrollbar appearance
    void setScrollbarColor(const sf::Color& trackColor, const sf::Color& thumbColor);
    void setScrollbarHoverColor(const sf::Color& trackColor, const sf::Color& thumbColor);
    
    // Override updateChildPositions to account for scroll offset
    void updateChildPositions() override;

private:
    // Scrollbar geometry
    ScrollbarSide scrollbarSide = ScrollbarSide::Right;
    float scrollbarWidth = 12.0f;
    float scrollOffset = 0.0f;  // Current scroll position in pixels
    
    // Scrollbar visual elements
    sf::RectangleShape scrollbarTrack;
    sf::RectangleShape scrollbarThumb;
    sf::Color trackColor = sf::Color(40, 40, 50);
    sf::Color thumbColor = sf::Color(80, 80, 90);
    sf::Color trackHoverColor = sf::Color(50, 50, 60);
    sf::Color thumbHoverColor = sf::Color(100, 100, 110);
    
    // Scrollbar interaction state
    bool isDraggingThumb = false;
    bool isHoveringThumb = false;
    float dragStartY = 0.0f;
    float dragStartOffset = 0.0f;
    
    // Viewport clipping rectangle
    sf::RectangleShape viewportClip;
    
    // Helper methods
    void updateScrollbarGeometry();  // Recalculate scrollbar position/size
    void updateViewportClip();       // Update clipping rectangle
    bool isMouseOnScrollbar(const sf::Vector2f& mousePos) const;
    bool isMouseOnThumb(const sf::Vector2f& mousePos) const;
    void clampScrollOffset();        // Ensure scrollOffset stays in valid range
};
