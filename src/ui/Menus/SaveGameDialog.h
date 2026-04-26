#pragma once

#include "../UIElements/UITextInput.h"
#include "../UIElements/UIButton.h"
#include "../UIElements/UIBorder.h"
#include "MainMenu.h"
#include <SFML/Graphics.hpp>
#include <string>

class Renderer;

class SaveGameDialog {
public:
    SaveGameDialog();
    ~SaveGameDialog();
    
    void initialize(sf::RenderWindow& window);
    bool isInitialized() const;
    
    void render(Renderer& renderer);
    MenuAction handleEvent(const sf::Event& event);
    void handleKeyPress(const sf::Event::KeyPressed& keyEvent);
    void handleTextEntered(const sf::Event::TextEntered& textEvent);
    
    std::string getSaveName() const;
    std::string getNotes() const;
    
private:
    sf::Font font;
    sf::RenderWindow* window = nullptr;
    bool initialized = false;
    bool fontLoaded = false;
    
    // Layout borders
    UIBorder mainBorder;           // Full-screen overlay
    UIBorder dialogBorder;         // Centered dialog box
    
    UITextInput saveNameInput;
    UITextInput notesInput;
    UIButton saveButton;
    UIButton cancelButton;
    
    MenuAction lastAction = MenuAction::None;
    
    sf::Vector2u lastWindowSize;
    float windowWidth = 0.0f;
    float windowHeight = 0.0f;
};
