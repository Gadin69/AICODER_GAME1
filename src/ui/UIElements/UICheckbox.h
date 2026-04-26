#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

class Renderer;  // Forward declaration

class UICheckbox : public UIElement {
public:
    UICheckbox();
    ~UICheckbox() override;
    
    void initialize(float x, float y, float size, const std::string& label, const sf::Font& font);
    void render(Renderer& renderer) override;
    
    void handleMousePress(const sf::Vector2f& mousePos) override;
    
    void toggle();
    void setChecked(bool checked);
    void setCallback(std::function<void(bool)> callback);
    
    // Component-specific state (public for now)
    sf::RectangleShape checkboxBox;
    sf::Text* checkmark = nullptr;
    sf::Text* labelText = nullptr;
    std::function<void(bool)> onToggle = nullptr;
    bool isChecked = false;
    
private:
    void updateCheckmark();
};
