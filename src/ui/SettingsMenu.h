#pragma once

#include "core/Window.h"
#include "core/Settings.h"
#include "rendering/Renderer.h"
#include "UISlider.h"
#include "UIButton.h"
#include "UICheckbox.h"
#include "UIToggle.h"
#include "UIDropdown.h"
#include "UINumberInput.h"
#include "UIBorder.h"
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
    
    sf::Font font;
    sf::RenderWindow* window = nullptr;
    
    // UI Components
    UIBorder displayBorder;  // Display mode + resolution dropdowns
    UIDropdown displayDropdown;
    UIDropdown resolutionDropdown;
    
    UIBorder gridBorder;  // Grid size inputs
    UIToggle vsyncToggle;
    UINumberInput gridWidthInput;
    UINumberInput gridHeightInput;
    
    UIBorder cameraBorder;  // Camera control sliders
    UISlider cameraSpeedSlider;
    UISlider cameraAccelSlider;
    UISlider cameraMaxSpeedSlider;
    
    UIBorder actionBorder;  // Apply/Back buttons
    UIButton applyButton;
    UIButton backButton;
    
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
};
