#pragma once

#include "UIBorder.h"
#include "UISlider.h"
#include "UIElementSelector.h"
#include <SFML/Graphics.hpp>

class Renderer;  // Forward declaration

class GameGUI {
public:
    GameGUI();
    ~GameGUI();
    
    void initialize(const sf::Font& font);
    void handleResize(int windowWidth, int windowHeight);  // Call on window resize
    void render(Renderer& renderer);
    
    void handleMousePress(const sf::Vector2f& mousePos);
    void handleMouseRelease();
    void handleMouseMove(const sf::Vector2f& mousePos);
    
    // Check if mouse is over any UI element (for input blocking)
    bool isMouseOverUI(const sf::Vector2f& mousePos) const;
    
    // Access UI elements
    UISlider* getSimSpeedSlider() { return simSpeedSlider; }
    UIElementSelector* getElementSelector() { return elementSelector; }
    
    bool initialized = false;
    
private:
    const sf::Font* fontPtr = nullptr;
    int currentWindowWidth = 1920;
    int currentWindowHeight = 1080;
    
    // UIBorder containers
    UIBorder skillbarBorder;
    
    // UI Elements
    UISlider* simSpeedSlider = nullptr;
    UIElementSelector* elementSelector = nullptr;
    
    void buildSkillbarLayout();  // Creates skillbar border + children
    void buildElementSelectorLayout();  // Creates element selector widget
};
