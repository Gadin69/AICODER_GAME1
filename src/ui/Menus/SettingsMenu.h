#pragma once

#include "core/Window.h"
#include "core/Settings.h"
#include "rendering/Renderer.h"
#include "../UIElements/UISlider.h"
#include "../UIElements/UIButton.h"
#include "../UIElements/UICheckbox.h"
#include "../UIElements/UIToggle.h"
#include "../UIElements/UIDropdown.h"
#include "../UIElements/UINumberInput.h"
#include "../UIElements/UIBorder.h"
#include "MainMenu.h"  // For MenuAction enum
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class MenuAction; // Forward declaration

class SettingsMenu {
public:
    SettingsMenu();
    ~SettingsMenu();
    
    void initialize(sf::RenderWindow& window);
    bool isInitialized() const;
    
    void render(Renderer& renderer);
    MenuAction handleEvent(const sf::Event& event);

private:
    void buildMenu();
    
    // New layout methods
    void buildTitle();
    void buildSidebar();
    void buildContent();
    void buildButtons();
    void switchCategory(int category);
    void updateTabColors();
    
    sf::Font font;
    sf::RenderWindow* window = nullptr;
    
    // Layout borders
    UIBorder mainBorder;        // Entire settings window container
    UIBorder titleBorder;       // Top title area
    UIBorder sidebarBorder;     // Left category tabs
    UIBorder contentBorder;     // Right settings content
    UIBorder buttonBorder;      // Bottom buttons
    
    // Category tabs
    UIButton controlsTab;
    UIButton graphicsTab;
    UIButton audioTab;
    UIButton videoTab;
    UIButton gameplayTab;
    int activeCategory = 1;  // 0=Controls, 1=Graphics (default), 2=Audio, 3=Video, 4=Gameplay
    
    // Existing UI Components (reused in new layout)
    UIDropdown displayDropdown;
    UIDropdown resolutionDropdown;
    
    UIToggle vsyncToggle;
    UINumberInput gridWidthInput;
    UINumberInput gridHeightInput;
    
    UISlider cameraSpeedSlider;
    UISlider cameraAccelSlider;
    UISlider cameraMaxSpeedSlider;
    
    // Bottom buttons
    UIButton applyButton;
    UIButton backButton;
    UIButton resetButton;
    
    bool initialized = false;
    MenuAction lastAction = MenuAction::None;
    
    std::vector<std::pair<unsigned int, unsigned int>> resolutions = {
        {1280, 720},
        {1920, 1080},
        {2560, 1440},
        {3840, 2160}
    };
    
    sf::Vector2f mousePos;
    sf::Vector2u lastWindowSize;  // Track window size to detect changes
    bool cameraSettingsChanged = false;
    
    // Cached dimensions for layout
    float windowWidth = 0.0f;
    float windowHeight = 0.0f;
    float titleHeight = 0.0f;
    float buttonHeight = 0.0f;
};
