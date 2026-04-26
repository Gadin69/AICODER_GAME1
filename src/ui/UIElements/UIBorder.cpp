#include "UIBorder.h"
#include "../../rendering/Renderer.h"

UIBorder::UIBorder() {}

UIBorder::~UIBorder() {}

void UIBorder::initialize(float x, float y, float width, float height) {
    border.setSize(sf::Vector2f(width, height));
    border.setPosition(sf::Vector2f(x, y));
    border.setFillColor(sf::Color(30, 30, 40, 200));  // Semi-transparent dark
    border.setOutlineThickness(2.0f);
    border.setOutlineColor(sf::Color(100, 100, 150));
    
    initialized = true;
}

void UIBorder::setSize(float width, float height) {
    border.setSize(sf::Vector2f(width, height));
    updateChildPositions();  // Scale all children
}

void UIBorder::setPosition(float x, float y) {
    border.setPosition(sf::Vector2f(x, y));
    updateChildPositions();  // Reposition all children
}

void UIBorder::setBackgroundColor(const sf::Color& color) {
    border.setFillColor(color);
}

void UIBorder::setBorderColor(const sf::Color& color) {
    border.setOutlineColor(color);
}

void UIBorder::setBorderThickness(float thickness) {
    border.setOutlineThickness(thickness);
}

// NEW: Add polymorphic UIElement child
void UIBorder::addChild(UIElement* child, float relX, float relY, float relWidth, float relHeight) {
    if (!child) return;
    
    UIElementChild elemChild;
    elemChild.element = child;
    elemChild.relX = relX;
    elemChild.relY = relY;
    elemChild.relWidth = relWidth;
    elemChild.relHeight = relHeight;
    elemChild.useAbsolutePosition = false;
    
    uiElementChildren.push_back(elemChild);
}

// Add UIElement child with absolute positioning (child manages its own position)
void UIBorder::addChild(UIElement* child) {
    if (!child) return;
    
    UIElementChild elemChild;
    elemChild.element = child;
    elemChild.relX = 0.0f;
    elemChild.relY = 0.0f;
    elemChild.relWidth = 1.0f;
    elemChild.relHeight = 1.0f;
    elemChild.useAbsolutePosition = true;
    elemChild.absolutePos = child->getPosition();  // Store current position
    elemChild.absoluteSize = child->getSize();     // Store current size
    
    uiElementChildren.push_back(elemChild);
}

// OLD: Add SFML RectangleShape child (backward compatibility)
void UIBorder::addChild(sf::RectangleShape* child, float relX, float relY, float relWidth, float relHeight) {
    if (!child) return;  // Null check
    
    PrimitiveChild childElem;
    childElem.type = PrimitiveChild::RECTANGLE;
    childElem.element.rect = child;
    childElem.relX = relX;
    childElem.relY = relY;
    childElem.relWidth = relWidth;
    childElem.relHeight = relHeight;
    childElem.originalFontSize = 0.0f;
    
    primitiveChildren.push_back(childElem);
    // Don't call updateChildPositions() here - let render() handle it
}

void UIBorder::addChild(sf::Text* child, float relX, float relY) {
    if (!child) return;  // Null check
    
    PrimitiveChild childElem;
    childElem.type = PrimitiveChild::TEXT;
    childElem.element.text = child;
    childElem.relX = relX;
    childElem.relY = relY;
    childElem.relWidth = 0.0f;  // Not used for text
    childElem.relHeight = 0.0f;  // Not used for text
    childElem.originalFontSize = static_cast<float>(child->getCharacterSize());
    
    primitiveChildren.push_back(childElem);
    // Don't call updateChildPositions() here - let render() handle it
}

void UIBorder::addPopupChild(sf::RectangleShape* child) {
    popupRectangles.push_back(child);
}

void UIBorder::addPopupChild(sf::Text* child) {
    popupTexts.push_back(child);
}

void UIBorder::render(Renderer& renderer) {
    if (!initialized) return;
    
    // Update all child positions before rendering
    updateChildPositions();
    
    // Draw border background
    renderer.drawRectangle(border);
    
    // Render all UIElement children (polymorphic)
    for (const auto& child : uiElementChildren) {
        if (child.element->isVisible()) {
            child.element->render(renderer);
        }
    }
    
    // Draw all primitive SFML children (backward compatibility)
    for (const auto& child : primitiveChildren) {
        if (child.type == PrimitiveChild::RECTANGLE) {
            renderer.drawRectangle(*child.element.rect);
        } else if (child.type == PrimitiveChild::TEXT) {
            renderer.drawText(*child.element.text);
        }
    }
    
    // Draw popup/dropdown children LAST (highest z-order - on top of everything)
    for (auto* rect : popupRectangles) {
        renderer.drawRectangle(*rect);
    }
    for (auto* text : popupTexts) {
        renderer.drawText(*text);
    }
}

sf::Vector2f UIBorder::getPosition() const {
    return border.getPosition();
}

sf::Vector2f UIBorder::getSize() const {
    return border.getSize();
}

bool UIBorder::containsPoint(const sf::Vector2f& point) const {
    if (!initialized) return false;
    
    sf::Vector2f pos = border.getPosition();
    sf::Vector2f size = border.getSize();
    
    return point.x >= pos.x && point.x <= pos.x + size.x &&
           point.y >= pos.y && point.y <= pos.y + size.y;
}

void UIBorder::clearChildren() {
    uiElementChildren.clear();
    primitiveChildren.clear();
    // Note: Does NOT delete the SFML objects, just removes from border
    // The UI components own their SFML objects
}

void UIBorder::updateChildPositions() {
    if (!initialized) return;
    
    float borderX = border.getPosition().x;
    float borderY = border.getPosition().y;
    float borderWidth = border.getSize().x;
    float borderHeight = border.getSize().y;
    
    // Update UIElement children positions and sizes
    for (auto& child : uiElementChildren) {
        // Skip children that use absolute positioning
        if (child.useAbsolutePosition) continue;
        
        float absX = borderX + (child.relX * borderWidth);
        float absY = borderY + (child.relY * borderHeight);
        float absWidth = child.relWidth * borderWidth;
        float absHeight = child.relHeight * borderHeight;
        
        child.element->setPosition(absX, absY);
        child.element->setSize(absWidth, absHeight);
    }
    
    // Update primitive SFML children positions and sizes
    for (auto& child : primitiveChildren) {
        if (child.type == PrimitiveChild::RECTANGLE) {
            // Calculate absolute position and size
            float absX = borderX + (child.relX * borderWidth);
            float absY = borderY + (child.relY * borderHeight);
            float absWidth = child.relWidth * borderWidth;
            float absHeight = child.relHeight * borderHeight;
            
            child.element.rect->setPosition(sf::Vector2f(absX, absY));
            child.element.rect->setSize(sf::Vector2f(absWidth, absHeight));
            
        } else if (child.type == PrimitiveChild::TEXT) {
            // Calculate absolute position
            float absX = borderX + (child.relX * borderWidth);
            float absY = borderY + (child.relY * borderHeight);
            
            child.element.text->setPosition(sf::Vector2f(absX, absY));
            
            // Scale font size proportionally to border height
            if (child.originalFontSize > 0.0f) {
                // Assume original design was for 1080p height
                float scaleFactor = borderHeight / 1080.0f;
                unsigned int newFontSize = static_cast<unsigned int>(child.originalFontSize * scaleFactor);
                newFontSize = std::max(8u, std::min(newFontSize, 48u));  // Clamp between 8-48
                child.element.text->setCharacterSize(newFontSize);
            }
        }
    }
}

void UIBorder::handleMousePress(const sf::Vector2f& mousePos) {
    // If mouseThrough is enabled, don't process mouse events (for overlays)
    if (mouseThrough) return;
    
    // Propagate mouse press to all UIElement children
    for (auto& child : uiElementChildren) {
        if (child.element->isVisible()) {
            child.element->handleMousePress(mousePos);
        }
    }
}

void UIBorder::handleMouseRelease() {
    // If mouseThrough is enabled, don't process mouse events (for overlays)
    if (mouseThrough) return;
    
    // Propagate mouse release to all UIElement children
    for (auto& child : uiElementChildren) {
        if (child.element->isVisible()) {
            child.element->handleMouseRelease();
        }
    }
}

void UIBorder::handleMouseMove(const sf::Vector2f& mousePos) {
    // If mouseThrough is enabled, don't process mouse events (for overlays)
    if (mouseThrough) return;
    
    // Propagate mouse move to all UIElement children
    for (auto& child : uiElementChildren) {
        if (child.element->isVisible()) {
            child.element->handleMouseMove(mousePos);
        }
    }
}
