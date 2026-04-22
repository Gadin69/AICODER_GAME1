#pragma once

#include "core/Window.h"
#include "core/Settings.h"
#include "rendering/Renderer.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

enum class MenuState {
    Main,
    Settings,
    Paused
};

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
    
    void initialize(sf::Font& font);
    MenuState getState() const;
    void setState(MenuState state);
    
    MenuAction render(Renderer& renderer, Window& window);
    void handleEvent(const sf::Event& event);
    
    void setPaused(bool paused);
    bool isInitialized() const;

private:
    struct Button {
        sf::RectangleShape background;
        sf::Text* text = nullptr;  // Use pointer for SFML 3
        std::string label;
        bool isHovered = false;
        bool isPressed = false;
        
        ~Button() {
            delete text;
        }
    };
    
    struct Slider {
        sf::RectangleShape track;
        sf::RectangleShape thumb;
        sf::Text label;
        sf::Text valueText;
        float minValue;
        float maxValue;
        float currentValue;
        bool isDragging = false;
        std::string labelText;
    };
    
    void buildMainMenu();
    void buildSettingsMenu();
    void buildPausedMenu();
    
    void renderButtons(Renderer& renderer);
    void renderSettingsUI(Renderer& renderer);
    
    bool isMouseOverButton(const Button& button, const sf::Vector2f& mousePos) const;
    void updateButtonHover(const sf::Vector2f& mousePos);
    
    MenuState currentState = MenuState::Main;
    bool initialized = false;
    
    sf::Font* font = nullptr;
    std::vector<Button> buttons;
    std::vector<Slider> sliders;
    
    // Settings state
    int selectedDisplayMode = 0;  // 0=Windowed, 1=Fullscreen, 2=Borderless
    int selectedResolution = 0;   // Index into resolutions array
    bool vsyncEnabled = true;
    int gridWidthInput = 80;
    int gridHeightInput = 60;
    
    std::vector<std::pair<unsigned int, unsigned int>> resolutions = {
        {1280, 720},
        {1920, 1080},
        {2560, 1440},
        {3840, 2160}
    };
    
    sf::Vector2f mousePos;
};
