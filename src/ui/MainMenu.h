#pragma once

#include "core/Window.h"
#include "core/Settings.h"
#include "rendering/Renderer.h"
#include "UIButton.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

enum class MenuAction {
    None,
    Play,
    Resume,
    Settings,
    Quit,
    QuitToMain,
    ApplySettings,
    Back
};

class MainMenu {
public:
    MainMenu();
    ~MainMenu();
    
    void initialize(sf::RenderWindow& window);
    bool isInitialized() const;
    
    void render(Renderer& renderer);
    MenuAction handleEvent(const sf::Event& event);

private:
    void buildMenu();
    
    sf::Font font;
    sf::RenderWindow* window = nullptr;
    
    UIButton playButton;
    UIButton settingsButton;
    UIButton quitButton;
    
    MenuAction lastAction = MenuAction::None;
    bool initialized = false;
    
    sf::Vector2f mousePos;
    sf::Vector2u lastWindowSize;  // Track window size to detect changes
};
