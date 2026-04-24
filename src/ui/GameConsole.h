#pragma once

#include "UITextInput.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>

class Renderer;

class GameConsole {
public:
    GameConsole();
    ~GameConsole();
    
    void initialize(float x, float y, float width, float height, const sf::Font& font);
    void render(Renderer& renderer);
    
    void handleToggle();  // Call when ` key is pressed
    void handleToggleKeyReleased();  // Call to clear any pending ` character
    void handleMousePress(const sf::Vector2f& mousePos);
    void handleKeyPress(const sf::Event::KeyPressed& keyEvent);
    void handleTextEntered(const sf::Event::TextEntered& textEvent);
    void update(float deltaTime);
    
    bool isVisible() const { return visible; }
    
    // Command registration
    void registerCommand(const std::string& name, std::function<void(const std::vector<std::string>&)> handler, const std::string& description = "");
    
private:
    bool visible = false;
    bool initialized = false;
    
    // UI Components
    sf::RectangleShape background;
    UITextInput inputField;
    
    // Output log
    struct ConsoleLine {
        std::string text;
        sf::Color color;
    };
    std::vector<ConsoleLine> outputLines;
    float scrollOffset = 0.0f;
    
    // Command system
    struct Command {
        std::string name;
        std::string description;
        std::function<void(const std::vector<std::string>&)> handler;
    };
    std::vector<Command> commands;
    
    const sf::Font* fontPtr = nullptr;
    
    // Internal methods
    void processCommand(const std::string& commandText);
    void addOutput(const std::string& text, sf::Color color = sf::Color::White);
    void renderOutput(Renderer& renderer);
};
