#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

class Renderer;  // Forward declaration

class UICheckbox {
public:
    UICheckbox();
    ~UICheckbox();
    
    void initialize(float x, float y, float size, const std::string& label, const sf::Font& font);
    void render(Renderer& renderer);
    
    void handleMousePress(const sf::Vector2f& mousePos);
    
    void toggle();
    void setChecked(bool checked);
    void setCallback(std::function<void(bool)> callback);
    
    sf::RectangleShape checkboxBox;
    sf::Text* checkmark = nullptr;
    sf::Text* labelText = nullptr;
    sf::Font* fontPtr = nullptr;
    std::function<void(bool)> onToggle = nullptr;
    
    bool isChecked = false;
    bool initialized = false;
    
private:
    void updateCheckmark();
};
