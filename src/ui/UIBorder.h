#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <vector>

class Renderer;  // Forward declaration

class UIBorder {
public:
    UIBorder();
    ~UIBorder();
    
    void initialize(float x, float y, float width, float height);
    void setSize(float width, float height);  // Resizes and scales children
    void setPosition(float x, float y);
    
    // NEW: Add polymorphic UIElement child (preferred method)
    void addChild(UIElement* child, float relX, float relY, float relWidth, float relHeight);
    
    // OLD: Add SFML primitive child (backward compatibility)
    void addChild(sf::RectangleShape* child, float relX, float relY, float relWidth, float relHeight);
    void addChild(sf::Text* child, float relX, float relY);  // Text uses font scaling
    
    // Add popup/dropdown child (renders on top with highest z-order)
    void addPopupChild(sf::RectangleShape* child);
    void addPopupChild(sf::Text* child);
    
    void render(Renderer& renderer);
    
    // Getters for mouse collision detection
    sf::Vector2f getPosition() const;
    sf::Vector2f getSize() const;
    bool containsPoint(const sf::Vector2f& point) const;  // Hit testing
    
    bool initialized = false;
    
private:
    sf::RectangleShape border;
    
    // Polymorphic UIElement children
    struct UIElementChild {
        UIElement* element;
        float relX, relY, relWidth, relHeight;  // Percentages
    };
    std::vector<UIElementChild> uiElementChildren;
    
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
    
    void updateChildPositions();  // Recalculates absolute positions from percentages
};
