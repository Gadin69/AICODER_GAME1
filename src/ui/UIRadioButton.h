#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <map>

class Renderer;  // Forward declaration

class UIRadioButton : public UIElement {
public:
    UIRadioButton();
    ~UIRadioButton() override;
    
    void initialize(float x, float y, const std::string& label, int groupId, const sf::Font& font);
    void render(Renderer& renderer) override;
    
    void handleMousePress(const sf::Vector2f& mousePos) override;
    
    void setSelected(bool selected);
    void setGroupId(int id);
    static void clearGroup(int groupId);
    static void setCallback(int groupId, std::function<void(int)> callback);
    
    // Component-specific state (public for now)
    sf::CircleShape outerCircle;
    sf::CircleShape innerCircle;
    sf::Text* labelText = nullptr;
    
    int groupId = 0;
    bool isSelected = false;
    
private:
    static std::map<int, std::vector<UIRadioButton*>> groups;
    static std::map<int, std::function<void(int)>> groupCallbacks;
    
    void updateVisuals();
    void notifyGroup();
};
