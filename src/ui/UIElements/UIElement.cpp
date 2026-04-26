#include "UIElement.h"

UIElement::UIElement() : position(0.0f, 0.0f), size(0.0f, 0.0f) {
}

UIElement::~UIElement() {
}

void UIElement::setPosition(float x, float y) {
    position = sf::Vector2f(x, y);
}

void UIElement::setSize(float width, float height) {
    size = sf::Vector2f(width, height);
}

bool UIElement::containsPoint(const sf::Vector2f& point) const {
    return point.x >= position.x && point.x <= position.x + size.x &&
           point.y >= position.y && point.y <= position.y + size.y;
}
