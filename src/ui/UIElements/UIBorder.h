#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <vector>

class Renderer;  // Forward declaration

class UIBorder : public UIElement {
public:
    UIBorder();
    ~UIBorder() override;
    
    void initialize(float x, float y, float width, float height);
    void setSize(float width, float height);  // Resizes and scales children
    void setPosition(float x, float y);
    void setBackgroundColor(const sf::Color& color);  // Set border background color
    void setBorderColor(const sf::Color& color);      // Set border outline color
    void setBorderThickness(float thickness);         // Set border outline thickness
    
    // NEW: Add polymorphic UIElement child (preferred method)
    // Add UIElement child with relative positioning
    void addChild(UIElement* child, float relX, float relY, float relWidth, float relHeight);
    
    // Add UIElement child with absolute positioning (no automatic position updates)
    void addChild(UIElement* child);
    
    // OLD: Add SFML primitive child (backward compatibility)
    void addChild(sf::RectangleShape* child, float relX, float relY, float relWidth, float relHeight);
    void addChild(sf::Text* child, float relX, float relY);  // Text uses font scaling
    
    // Add popup/dropdown child (renders on top with highest z-order)
    void addPopupChild(sf::RectangleShape* child);
    void addPopupChild(sf::Text* child);
    
    void render(Renderer& renderer) override;
    
    // Mouse event handling - propagates to all UIElement children
    void handleMousePress(const sf::Vector2f& mousePos);
    void handleMouseRelease();
    void handleMouseMove(const sf::Vector2f& mousePos);
    
    // Mouse through mode - if true, border doesn't block mouse events (for overlays)
    bool mouseThrough = false;
    
    // Getters for mouse collision detection (override to use border's values)
    sf::Vector2f getPosition() const override;
    sf::Vector2f getSize() const override;
    bool containsPoint(const sf::Vector2f& point) const;  // Hit testing
    void clearChildren();  // Remove all children (for dynamic content switching)
    size_t getChildCount() const { return uiElementChildren.size(); }  // Debug: count children
    
    // Protected members for derived classes (e.g., UIScrollBorder)
protected:
    bool initialized = false;
    
    struct UIElementChild {
        UIElement* element;
        float relX, relY, relWidth, relHeight;  // Percentages
        bool useAbsolutePosition = false;  // If true, don't update position in updateChildPositions()
        sf::Vector2f absolutePos = {0, 0};  // Stored absolute position for absolute children
        sf::Vector2f absoluteSize = {0, 0};  // Stored absolute size for absolute children
    };
    std::vector<UIElementChild> uiElementChildren;
    
private:
    sf::RectangleShape border;
    
protected:
    sf::RectangleShape& getBorder() { return border; }
    const sf::RectangleShape& getBorder() const { return border; }
    
    // SFML primitive children (backward compatibility)
    struct PrimitiveChild {
        enum Type { RECTANGLE, TEXT };
        Type type;
        union {
            sf::RectangleShape* rect;
            sf::Text* text;
        } element;
        float relX, relY, relWidth, relHeight;  // Percentages
        float originalFontSize = 0.0f;  // For text scaling
    };
    std::vector<PrimitiveChild> primitiveChildren;
    
    std::vector<sf::RectangleShape*> popupRectangles;  // Rendered on top (highest z-order)
    std::vector<sf::Text*> popupTexts;  // Rendered on top (highest z-order)
    
    virtual void updateChildPositions();  // Recalculates absolute positions from percentages (virtual for UIScrollBorder)
};
