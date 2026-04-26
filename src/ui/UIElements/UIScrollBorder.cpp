#include "UIScrollBorder.h"
#include "../../rendering/Renderer.h"
#include <iostream>

UIScrollBorder::UIScrollBorder() {}

UIScrollBorder::~UIScrollBorder() {}

void UIScrollBorder::initialize(float x, float y, float width, float height,
                                ScrollbarSide side, float scrollbarWidth) {
    // Call parent initialize
    UIBorder::initialize(x, y, width, height);
    
    this->scrollbarSide = side;
    this->scrollbarWidth = scrollbarWidth;
    this->scrollOffset = 0.0f;
    
    // Initialize scrollbar visuals
    updateScrollbarGeometry();
    updateViewportClip();
}

void UIScrollBorder::render(Renderer& renderer) {
    if (!initialized) return;
    
    sf::Vector2f borderPos = getPosition();
    sf::Vector2f borderSize = getSize();
    
    // Draw background
    renderer.drawRectangle(getBorder());
    
    // Draw placement area children (with scroll offset applied)
    // Note: UIBorder's primitive children are handled by parent's render()
    // We only handle UIElement children here with scroll offset
    for (size_t i = 0; i < uiElementChildren.size(); ++i) {
        auto& child = uiElementChildren[i];
        
        if (child.element->isVisible()) {
            // Get position - use stored absolute position if applicable
            sf::Vector2f childPos = child.useAbsolutePosition ? child.absolutePos : child.element->getPosition();
            sf::Vector2f childSize = child.useAbsolutePosition ? child.absoluteSize : child.element->getSize();
            
            // CRITICAL: Convert from LOCAL coordinates to SCREEN coordinates
            // childPos is relative to scroll border's top-left, need to add borderPos
            // Also apply scroll offset to Y coordinate
            //
            // Common bug: Using childPos directly renders at wrong screen position
            // Fix: screenX = borderPos.x + childPos.x, screenY = borderPos.y + scrolledY
            
            float scrolledY = childPos.y - scrollOffset;
            float screenX = borderPos.x + childPos.x;
            float screenY = borderPos.y + scrolledY;
            
            // Only render if child is within viewport (using screen coordinates)
            if (screenY + childSize.y > borderPos.y && 
                screenY < borderPos.y + borderSize.y) {
                
                // Temporarily adjust child position for rendering (screen coordinates)
                sf::Vector2f originalPos = child.element->getPosition();
                child.element->setPosition(screenX, screenY);
                child.element->render(renderer);
                child.element->setPosition(originalPos.x, originalPos.y);
            }
        }
    }
    
    // Draw scrollbar on top
    renderer.drawRectangle(scrollbarTrack);
    renderer.drawRectangle(scrollbarThumb);
}

void UIScrollBorder::handleMousePress(const sf::Vector2f& mousePos) {
    // Check if clicking on scrollbar thumb
    if (isMouseOnThumb(mousePos)) {
        isDraggingThumb = true;
        dragStartY = mousePos.y;
        dragStartOffset = scrollOffset;
    }
    // Otherwise, pass to parent (UIBorder child handling)
    else if (isMouseOnScrollbar(mousePos)) {
        // Click on track - scroll by page
        float pageScroll = getSize().y * 0.8f;  // 80% of viewport
        if (mousePos.y < scrollbarThumb.getPosition().y) {
            setScrollOffset(scrollOffset - pageScroll);
        } else {
            setScrollOffset(scrollOffset + pageScroll);
        }
    } else {
        // Pass to parent for child element handling
        UIBorder::handleMousePress(mousePos);
    }
}

void UIScrollBorder::handleMouseMove(const sf::Vector2f& mousePos) {
    // Update thumb hover state
    bool wasHovering = isHoveringThumb;
    isHoveringThumb = isMouseOnThumb(mousePos);
    
    if (wasHovering != isHoveringThumb) {
        updateScrollbarGeometry();  // Update thumb color
    }
    
    // Handle thumb dragging
    if (isDraggingThumb) {
        float deltaY = mousePos.y - dragStartY;
        float maxScroll = getMaxScrollOffset();
        float viewportHeight = getSize().y;
        float thumbHeight = scrollbarThumb.getSize().y;
        float scrollableArea = viewportHeight - thumbHeight;
        
        if (scrollableArea > 0) {
            float scrollDelta = (deltaY / scrollableArea) * maxScroll;
            setScrollOffset(dragStartOffset + scrollDelta);
        }
    }
    
    // Pass to parent for child element handling
    UIBorder::handleMouseMove(mousePos);
}

void UIScrollBorder::handleMouseRelease() {
    isDraggingThumb = false;
    UIBorder::handleMouseRelease();
}

void UIScrollBorder::setScrollOffset(float offset) {
    scrollOffset = offset;
    clampScrollOffset();
    updateChildPositions();  // Reposition children based on new scroll
}

float UIScrollBorder::getMaxScrollOffset() const {
    if (uiElementChildren.empty()) return 0.0f;
    
    // Find the bottom-most child
    float maxY = 0.0f;
    for (const auto& child : uiElementChildren) {
        float childBottom = child.element->getPosition().y + child.element->getSize().y;
        maxY = std::max(maxY, childBottom);
    }
    
    float viewportHeight = getSize().y;
    float contentHeight = maxY - getPosition().y;
    
    return std::max(0.0f, contentHeight - viewportHeight);
}

void UIScrollBorder::clampScrollOffset() {
    float maxScroll = getMaxScrollOffset();
    scrollOffset = std::max(0.0f, std::min(scrollOffset, maxScroll));
}

void UIScrollBorder::scrollToTop() {
    setScrollOffset(0.0f);
}

void UIScrollBorder::scrollToBottom() {
    setScrollOffset(getMaxScrollOffset());
}

void UIScrollBorder::setScrollbarSide(ScrollbarSide side) {
    scrollbarSide = side;
    updateScrollbarGeometry();
    updateViewportClip();
}

void UIScrollBorder::setScrollbarWidth(float width) {
    scrollbarWidth = width;
    updateScrollbarGeometry();
    updateViewportClip();
}

void UIScrollBorder::setScrollbarColor(const sf::Color& trackColor, const sf::Color& thumbColor) {
    this->trackColor = trackColor;
    this->thumbColor = thumbColor;
    updateScrollbarGeometry();
}

void UIScrollBorder::setScrollbarHoverColor(const sf::Color& trackColor, const sf::Color& thumbColor) {
    this->trackHoverColor = trackColor;
    this->thumbHoverColor = thumbColor;
}

void UIScrollBorder::updateScrollbarGeometry() {
    sf::Vector2f borderPos = getPosition();
    sf::Vector2f borderSize = getSize();
    
    // Track position
    float trackX = (scrollbarSide == ScrollbarSide::Right) 
        ? borderPos.x + borderSize.x - scrollbarWidth 
        : borderPos.x;
    
    scrollbarTrack.setSize(sf::Vector2f(scrollbarWidth, borderSize.y));
    scrollbarTrack.setPosition(sf::Vector2f(trackX, borderPos.y));
    scrollbarTrack.setFillColor(trackColor);
    
    // Thumb size proportional to visible content
    float maxScroll = getMaxScrollOffset();
    float viewportHeight = borderSize.y;
    float contentHeight = viewportHeight + maxScroll;
    float thumbHeight = (viewportHeight / contentHeight) * viewportHeight;
    thumbHeight = std::max(30.0f, thumbHeight);  // Minimum thumb size
    
    // Thumb position based on scroll offset
    float thumbY = borderPos.y;
    if (maxScroll > 0.0f) {
        thumbY = borderPos.y + (scrollOffset / maxScroll) * (viewportHeight - thumbHeight);
    }
    
    scrollbarThumb.setSize(sf::Vector2f(scrollbarWidth - 2, thumbHeight));  // 2px padding
    scrollbarThumb.setPosition(sf::Vector2f(trackX + 1, thumbY));
    scrollbarThumb.setFillColor(isHoveringThumb ? thumbHoverColor : thumbColor);
}

void UIScrollBorder::updateViewportClip() {
    sf::Vector2f borderPos = getPosition();
    float placementX = (scrollbarSide == ScrollbarSide::Right) 
        ? borderPos.x 
        : borderPos.x + scrollbarWidth;
    float placementWidth = getSize().x - scrollbarWidth;
    
    viewportClip.setSize(sf::Vector2f(placementWidth, getSize().y));
    viewportClip.setPosition(sf::Vector2f(placementX, borderPos.y));
}

bool UIScrollBorder::isMouseOnScrollbar(const sf::Vector2f& mousePos) const {
    return scrollbarTrack.getGlobalBounds().contains(mousePos);
}

bool UIScrollBorder::isMouseOnThumb(const sf::Vector2f& mousePos) const {
    return scrollbarThumb.getGlobalBounds().contains(mousePos);
}

void UIScrollBorder::updateChildPositions() {
    if (!initialized) return;
    
    sf::Vector2f borderPos = getPosition();
    sf::Vector2f borderSize = getSize();
    
    // Calculate placement area (excluding scrollbar)
    float placementX = (scrollbarSide == ScrollbarSide::Right) 
        ? borderPos.x 
        : borderPos.x + scrollbarWidth;
    float placementWidth = borderSize.x - scrollbarWidth;
    
    // Update UIElement children positions with scroll offset
    for (auto& child : uiElementChildren) {
        // Skip children that use absolute positioning - they manage their own position and size
        if (child.useAbsolutePosition) continue;
        
        float absX = placementX + (child.relX * placementWidth);
        float absY = borderPos.y + (child.relY * borderSize.y) - scrollOffset;
        float absWidth = child.relWidth * placementWidth;
        float absHeight = child.relHeight * borderSize.y;
        
        child.element->setPosition(absX, absY);
        child.element->setSize(absWidth, absHeight);
    }
    
    // Note: primitive children are handled by parent UIBorder
    // We only update UIElement children here
    
    // Update scrollbar geometry after repositioning
    updateScrollbarGeometry();
}
