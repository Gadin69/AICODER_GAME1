#pragma once

#include "core/Window.h"
#include "core/Settings.h"
#include "rendering/Renderer.h"
#include "UIButton.h"
#include "UIBorder.h"
#include "../save/SaveManager.h"
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
    Back,
    Continue,
    SaveGame,
    LoadGame,
    DeleteSave
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
    void buildTitle();
    
    sf::Font font;
    sf::RenderWindow* window = nullptr;
    
    // Layout borders
    UIBorder mainBorder;           // Full-screen (0, 0, 100%, 100%)
    UIBorder centerButtonBorder;   // Centered button container
    
    UIButton playButton;
    UIButton loadButton;
    UIButton settingsButton;
    UIButton quitButton;
    UIButton* continueButton = nullptr;  // Dynamic, only if save exists
    
    MenuAction lastAction = MenuAction::None;
    bool initialized = false;
    bool hasRecentSave = false;
    
    sf::Vector2f mousePos;
    sf::Vector2u lastWindowSize;  // Track window size to detect changes
    float windowWidth = 0.0f;
    float windowHeight = 0.0f;
    
    void checkForRecentSave();
};
