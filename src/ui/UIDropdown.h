#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>

class Renderer;  // Forward declaration

class UIDropdown {
public:
    UIDropdown();
    ~UIDropdown();
    
    void initialize(float x, float y, float width, float itemHeight, const sf::Font& font);
    void render(Renderer& renderer);
    
    void handleMousePress(const sf::Vector2f& mousePos);
    void handleMouseMove(const sf::Vector2f& mousePos);
    
    void addOption(const std::string& option);
    void clearOptions();
    int getSelectedIndex() const;
    std::string getSelectedValue() const;
    void setSelectedIndex(int index);
    void setCallback(std::function<void(int, const std::string&)> callback);
    
    sf::RectangleShape dropdownBox;
    sf::Text* selectedText = nullptr;
    sf::RectangleShape arrow;
    std::vector<sf::RectangleShape> optionBoxes;
    std::vector<sf::Text*> optionTexts;
    sf::Font* fontPtr = nullptr;
    std::function<void(int, const std::string&)> onSelect = nullptr;
    
    std::vector<std::string> options;
    int selectedIndex = -1;
    int hoveredIndex = -1;
    bool isOpen = false;
    bool initialized = false;
    
private:
    float itemHeight = 30.0f;
    sf::Color boxColor = sf::Color(50, 50, 50);
    sf::Color hoverColor = sf::Color(70, 70, 70);
    sf::Color selectedColor = sf::Color(60, 80, 120);
    
    void updateDisplay();
    void buildOptionList();
};
