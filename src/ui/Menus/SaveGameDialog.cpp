#include "SaveGameDialog.h"
#include "../../save/SaveManager.h"
#include "../../rendering/Renderer.h"
#include <iostream>

SaveGameDialog::SaveGameDialog() {
}

SaveGameDialog::~SaveGameDialog() {
}

void SaveGameDialog::initialize(sf::RenderWindow& window) {
    this->window = &window;
    
    // Load font only once
    if (!fontLoaded) {
        if (!font.openFromFile("assets/fonts/arial.ttf")) {
            std::cerr << "ERROR: Failed to load font for SaveGameDialog" << std::endl;
            initialized = false;
            return;
        }
        fontLoaded = true;
        std::cout << "[SaveGameDialog] Font loaded successfully" << std::endl;
    }
    
    sf::Vector2u windowSize = window.getSize();
    lastWindowSize = windowSize;
    windowWidth = static_cast<float>(windowSize.x);
    windowHeight = static_cast<float>(windowSize.y);
    
    // CRITICAL: Clear previous children before rebuilding
    mainBorder.clearChildren();
    dialogBorder.clearChildren();
    
    // Main border (full screen overlay)
    mainBorder.initialize(0, 0, windowWidth, windowHeight);
    
    // Dialog border (centered, 50% width, 40% height)
    float dialogWidth = windowWidth * 0.50f;
    float dialogHeight = windowHeight * 0.40f;
    float dialogX = (windowWidth - dialogWidth) / 2.0f;
    float dialogY = (windowHeight - dialogHeight) / 2.0f;
    dialogBorder.initialize(dialogX, dialogY, dialogWidth, dialogHeight);
    
    // Initialize UITextInput for save name (relative to dialogBorder)
    saveNameInput.initialize(0, 0, dialogWidth - 80, 40, "Save Name:", font);
    saveNameInput.setText(SaveManager::getInstance().generateSaveName());
    saveNameInput.setMaxLength(50);
    mainBorder.addChild(&saveNameInput, 
                       (dialogX + 40) / windowWidth, 
                       (dialogY + 60) / windowHeight,
                       (dialogWidth - 80) / windowWidth, 
                       40 / windowHeight);
    
    // Initialize UITextInput for notes
    notesInput.initialize(0, 0, dialogWidth - 80, 80, "Notes (optional):", font);
    notesInput.setMaxLength(200);
    mainBorder.addChild(&notesInput,
                       (dialogX + 40) / windowWidth,
                       (dialogY + 140) / windowHeight,
                       (dialogWidth - 80) / windowWidth,
                       80 / windowHeight);
    
    // Initialize Save button
    float buttonWidth = 120.0f;
    float buttonHeight = 40.0f;
    float buttonY = dialogY + dialogHeight - 60;
    
    saveButton.initialize(0, 0, buttonWidth, buttonHeight, "Save", font);
    saveButton.setCallback([this]() {
        lastAction = MenuAction::ApplySettings;  // Reuse ApplySettings as Save action
    });
    mainBorder.addChild(&saveButton,
                       (dialogX + dialogWidth / 2.0f - buttonWidth - 10) / windowWidth,
                       buttonY / windowHeight,
                       buttonWidth / windowWidth,
                       buttonHeight / windowHeight);
    
    // Initialize Cancel button
    cancelButton.initialize(0, 0, buttonWidth, buttonHeight, "Cancel", font);
    cancelButton.setCallback([this]() {
        lastAction = MenuAction::Back;
    });
    mainBorder.addChild(&cancelButton,
                       (dialogX + dialogWidth / 2.0f + 10) / windowWidth,
                       buttonY / windowHeight,
                       buttonWidth / windowWidth,
                       buttonHeight / windowHeight);
    
    initialized = true;
    std::cout << "[SaveGameDialog] Save dialog initialized" << std::endl;
}

bool SaveGameDialog::isInitialized() const {
    return initialized;
}

std::string SaveGameDialog::getSaveName() const {
    return saveNameInput.getText();
}

std::string SaveGameDialog::getNotes() const {
    return notesInput.getText();
}

MenuAction SaveGameDialog::handleEvent(const sf::Event& event) {
    if (!initialized) return MenuAction::None;
    
    lastAction = MenuAction::None;
    
    if (event.is<sf::Event::MouseButtonPressed>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseButton && mouseButton->button == sf::Mouse::Button::Left) {
            sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);
            sf::Vector2f mousePos(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
            
            // Route to border (handles all children: inputs + buttons)
            mainBorder.handleMousePress(mousePos);
        }
    } else if (event.is<sf::Event::MouseButtonReleased>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonReleased>();
        if (mouseButton && mouseButton->button == sf::Mouse::Button::Left) {
            mainBorder.handleMouseRelease();
        }
    } else if (event.is<sf::Event::MouseMoved>()) {
        auto mouseMove = event.getIf<sf::Event::MouseMoved>();
        if (mouseMove) {
            sf::Vector2f pos(static_cast<float>(mouseMove->position.x), static_cast<float>(mouseMove->position.y));
            mainBorder.handleMouseMove(pos);
        }
    }
    
    return lastAction;
}

void SaveGameDialog::handleKeyPress(const sf::Event::KeyPressed& keyEvent) {
    if (!initialized) return;
    
    // Route keyboard events to UITextInput components
    saveNameInput.handleKeyPress(keyEvent);
    notesInput.handleKeyPress(keyEvent);
}

void SaveGameDialog::handleTextEntered(const sf::Event::TextEntered& textEvent) {
    if (!initialized) return;
    
    // Route text input events to UITextInput components
    saveNameInput.handleTextEntered(textEvent);
    notesInput.handleTextEntered(textEvent);
}

void SaveGameDialog::render(Renderer& renderer) {
    if (!initialized) return;
    
    // Check if window size changed
    sf::Vector2u currentWindowSize = window->getSize();
    if (lastWindowSize.x != currentWindowSize.x || lastWindowSize.y != currentWindowSize.y) {
        initialize(*window);
    }
    
    // Render main border (contains overlay, dialog, inputs, and buttons)
    mainBorder.render(renderer);
}
