#pragma once

#include "core/Window.h"
#include "core/Settings.h"
#include "rendering/Renderer.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

enum class MenuAction; // Forward declaration

class PauseMenu {
public:
    PauseMenu();
    ~PauseMenu();
    
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
    
    sf::Font font;
    sf::RenderWindow* window = nullptr;
    std::vector<Button> buttons;
    bool initialized = false;
    
    sf::Vector2f mousePos;
    sf::Vector2u lastWindowSize;  // Track window size to detect changes
};
