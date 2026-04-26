#pragma once

#include "core/Window.h"
#include "core/Settings.h"
#include "rendering/Renderer.h"
#include "UIButton.h"
#include "UIBorder.h"
#include "MainMenu.h"  // For MenuAction enum
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class PauseMenu {
public:
    PauseMenu();
    ~PauseMenu();
    
    void initialize(sf::RenderWindow& window);
    bool isInitialized() const;
    
    void render(Renderer& renderer);
    MenuAction handleEvent(const sf::Event& event);

private:
    void buildMenu();
    
    sf::Font font;
    sf::RenderWindow* window = nullptr;
    
    // Layout borders
    UIBorder mainBorder;           // Full-screen container
    UIBorder centerButtonBorder;   // Centered button container
    
    // Buttons
    UIButton resumeButton;
    UIButton saveButton;
    UIButton loadButton;
    UIButton settingsButton;
    UIButton quitToMainBtn;
    UIButton quitBtn;
    
    bool initialized = false;
    MenuAction lastAction = MenuAction::None;
    
    sf::Vector2u lastWindowSize;  // Track window size to detect changes
    float windowWidth = 0.0f;
    float windowHeight = 0.0f;
};
