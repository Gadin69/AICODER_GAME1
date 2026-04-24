#include "UIRadioButton.h"
#include "rendering/Renderer.h"
#include <algorithm>

// Static member initialization
std::map<int, std::vector<UIRadioButton*>> UIRadioButton::groups;
std::map<int, std::function<void(int)>> UIRadioButton::groupCallbacks;

UIRadioButton::UIRadioButton() {
}

UIRadioButton::~UIRadioButton() {
    delete labelText;
    
    // Remove from group
    if (groupId > 0) {
        auto& group = groups[groupId];
        group.erase(std::remove(group.begin(), group.end(), this), group.end());
    }
}

void UIRadioButton::initialize(float x, float y, const std::string& label, int grpId, const sf::Font& font) {
    // Set base class properties
    position = sf::Vector2f(x, y);
    size = sf::Vector2f(20.0f, 20.0f);  // Circle diameter
    fontPtr = &font;
    
    groupId = grpId;
    
    // Outer circle
    outerCircle.setRadius(10.0f);
    outerCircle.setPosition(sf::Vector2f(x, y));
    outerCircle.setFillColor(sf::Color(40, 40, 40));
    outerCircle.setOutlineColor(sf::Color(150, 150, 150));
    outerCircle.setOutlineThickness(2.0f);
    
    // Inner circle (dot)
    innerCircle.setRadius(5.0f);
    innerCircle.setPosition(sf::Vector2f(x + 5.0f, y + 5.0f));
    innerCircle.setFillColor(sf::Color(100, 180, 255));
    
    // Label
    labelText = new sf::Text(font, label, 16);
    labelText->setFillColor(sf::Color::White);
    labelText->setPosition(sf::Vector2f(x + 30.0f, y + 2.0f));
    
    // Register with group
    groups[groupId].push_back(this);
    
    updateVisuals();
    initialized = true;
}

void UIRadioButton::render(Renderer& renderer) {
    if (!initialized) return;
    
    renderer.drawShape(outerCircle);
    if (isSelected) {
        renderer.drawShape(innerCircle);
    }
    renderer.drawText(*labelText);
}

void UIRadioButton::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    // Check if clicking on radio button or label
    sf::FloatRect labelBounds = labelText->getGlobalBounds();
    sf::FloatRect radioBounds(
        sf::Vector2f(outerCircle.getPosition().x, outerCircle.getPosition().y),
        sf::Vector2f(labelBounds.position.x + labelBounds.size.x - outerCircle.getPosition().x,
                    outerCircle.getRadius() * 2.0f)
    );
    
    if (radioBounds.contains(mousePos)) {
        if (!isSelected) {
            // Deselect others in group
            clearGroup(groupId);
            setSelected(true);
            notifyGroup();
        }
    }
}

void UIRadioButton::setSelected(bool selected) {
    if (isSelected != selected) {
        isSelected = selected;
        updateVisuals();
    }
}

void UIRadioButton::setGroupId(int id) {
    // Remove from old group
    if (groupId > 0) {
        auto& oldGroup = groups[groupId];
        oldGroup.erase(std::remove(oldGroup.begin(), oldGroup.end(), this), oldGroup.end());
    }
    
    groupId = id;
    groups[groupId].push_back(this);
}

void UIRadioButton::clearGroup(int grpId) {
    if (groups.find(grpId) != groups.end()) {
        for (auto radio : groups[grpId]) {
            radio->setSelected(false);
        }
    }
}

void UIRadioButton::setCallback(int grpId, std::function<void(int)> callback) {
    groupCallbacks[grpId] = callback;
}

void UIRadioButton::updateVisuals() {
    if (!initialized) return;
    
    if (isSelected) {
        outerCircle.setOutlineColor(sf::Color(100, 180, 255));
        innerCircle.setFillColor(sf::Color(100, 180, 255));
    } else {
        outerCircle.setOutlineColor(sf::Color(150, 150, 150));
    }
}

void UIRadioButton::notifyGroup() {
    if (groupCallbacks.find(groupId) != groupCallbacks.end()) {
        groupCallbacks[groupId](groupId);
    }
}
