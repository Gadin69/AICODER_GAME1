#pragma once

#include "UIElement.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>

class Renderer;  // Forward declaration

class UIDropdown : public UIElement {
public:
    UIDropdown();
    ~UIDropdown() override;
    
    void initialize(float x, float y, float width, float itemHeight, const sf::Font& font);
    void render(Renderer& renderer) override;
    
    void handleMousePress(const sf::Vector2f& mousePos) override;
    void handleMouseMove(const sf::Vector2f& mousePos) override;
    
    void addOption(const std::string& option);
    void clearOptions();
    int getSelectedIndex() const;
    std::string getSelectedValue() const;
    void setSelectedIndex(int index);
    void setCallback(std::function<void(int, const std::string&)> callback);
    
    // Component-specific state (public for now)
    sf::RectangleShape dropdownBox;
    sf::Text* selectedText = nullptr;
    sf::RectangleShape arrow;
    std::vector<sf::RectangleShape> optionBoxes;
    std::vector<sf::Text*> optionTexts;
    std::function<void(int, const std::string&)> onSelect = nullptr;
    
    std::vector<std::string> options;
    int selectedIndex = -1;
    int hoveredIndex = -1;
    bool isOpen = false;
    
private:
    float itemHeight = 30.0f;
    sf::Color boxColor = sf::Color(50, 50, 50);
    sf::Color hoverColor = sf::Color(70, 70, 70);
    sf::Color selectedColor = sf::Color(60, 80, 120);
    
    void updateDisplay();
    void buildOptionList();
};
