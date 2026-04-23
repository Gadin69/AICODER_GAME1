#pragma once

#include "core/Window.h"
#include "core/Settings.h"
#include "rendering/Renderer.h"
#include "UISlider.h"
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
    struct Button {
        sf::RectangleShape background;
        sf::Text* text = nullptr;
        std::string label;
        bool isHovered = false;
        
        Button() = default;
        ~Button() { delete text; }
        
        Button(Button&& other) noexcept 
            : background(std::move(other.background)),
              text(other.text),
              label(std::move(other.label)),
              isHovered(other.isHovered) {
            other.text = nullptr;
        }
        
        Button& operator=(Button&& other) noexcept {
            if (this != &other) {
                delete text;
                background = std::move(other.background);
                text = other.text;
                label = std::move(other.label);
                isHovered = other.isHovered;
                other.text = nullptr;
            }
            return *this;
        }
        
        Button(const Button&) = delete;
        Button& operator=(const Button&) = delete;
    };
    
    void buildMenu();
    void renderButtons(Renderer& renderer);
    bool isMouseOverButton(const Button& btn, const sf::Vector2f& mousePos);
    void updateButtonHover(const sf::Vector2f& mousePos);
    void updateButtonText(Button& btn);
    
    sf::Font font;
    sf::RenderWindow* window = nullptr;
    std::vector<Button> buttons;
    bool initialized = false;
    
    // Settings state
    int selectedDisplayMode = 0;
    int selectedResolution = 0;
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
    sf::Vector2u lastWindowSize;  // Track window size to detect changes
    
    // Camera control sliders
    UISlider cameraSpeedSlider;
    UISlider cameraAccelSlider;
    UISlider cameraMaxSpeedSlider;
    bool cameraSettingsChanged = false;
};
