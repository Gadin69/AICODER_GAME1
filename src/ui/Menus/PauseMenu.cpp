#include "PauseMenu.h"
#include "MainMenu.h" // For MenuAction
#include <iostream>

PauseMenu::PauseMenu() {
}

PauseMenu::~PauseMenu() {
}

void PauseMenu::initialize(sf::RenderWindow& window) {
    this->window = &window;
    
    if (!font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "ERROR: Failed to load font 'assets/fonts/arial.ttf'" << std::endl;
        return;
    }
    
    initialized = true;
    buildMenu();
}

bool PauseMenu::isInitialized() const {
    return initialized;
}

void PauseMenu::buildMenu() {
    if (!initialized) {
        std::cerr << "ERROR: buildMenu called but menu not initialized" << std::endl;
        return;
    }
    
    // Get actual window size
    sf::Vector2u windowSize = window->getSize();
    lastWindowSize = windowSize;
    windowWidth = static_cast<float>(windowSize.x);
    windowHeight = static_cast<float>(windowSize.y);
    
    // CRITICAL: Clear previous children before rebuilding
    mainBorder.clearChildren();
    
    // Initialize main border (full screen)
    mainBorder.initialize(0, 0, windowWidth, windowHeight);
    
    // Calculate button layout (centered on screen)
    int buttonCount = 6;  // Resume, Save, Load, Settings, Quit to Main, Quit
    float buttonWidth = windowWidth * 0.25f;
    float buttonHeight = windowHeight * 0.08f;
    float spacing = windowHeight * 0.03f;
    float startY = windowHeight / 2.0f - (buttonHeight * buttonCount + spacing * (buttonCount - 1)) / 2.0f;
    float centerX = (windowWidth - buttonWidth) / 2.0f;
    
    // Resume button
    resumeButton.initialize(0, 0, buttonWidth, buttonHeight, "Resume", font);
    resumeButton.setCallback([this]() {
        lastAction = MenuAction::Resume;
    });
    mainBorder.addChild(&resumeButton, centerX / windowWidth, startY / windowHeight,
                       buttonWidth / windowWidth, buttonHeight / windowHeight);
    startY += buttonHeight + spacing;
    
    // Save Game button
    saveButton.initialize(0, 0, buttonWidth, buttonHeight, "Save Game", font);
    saveButton.setCallback([this]() {
        lastAction = MenuAction::SaveGame;
    });
    mainBorder.addChild(&saveButton, centerX / windowWidth, startY / windowHeight,
                       buttonWidth / windowWidth, buttonHeight / windowHeight);
    startY += buttonHeight + spacing;
    
    // Load Game button
    loadButton.initialize(0, 0, buttonWidth, buttonHeight, "Load Game", font);
    loadButton.setCallback([this]() {
        lastAction = MenuAction::LoadGame;
    });
    mainBorder.addChild(&loadButton, centerX / windowWidth, startY / windowHeight,
                       buttonWidth / windowWidth, buttonHeight / windowHeight);
    startY += buttonHeight + spacing;
    
    // Settings button
    settingsButton.initialize(0, 0, buttonWidth, buttonHeight, "Settings", font);
    settingsButton.setCallback([this]() {
        lastAction = MenuAction::Settings;
    });
    mainBorder.addChild(&settingsButton, centerX / windowWidth, startY / windowHeight,
                       buttonWidth / windowWidth, buttonHeight / windowHeight);
    startY += buttonHeight + spacing;
    
    // Quit to Main button
    quitToMainBtn.initialize(0, 0, buttonWidth, buttonHeight, "Quit to Main", font);
    quitToMainBtn.setCallback([this]() {
        lastAction = MenuAction::QuitToMain;
    });
    mainBorder.addChild(&quitToMainBtn, centerX / windowWidth, startY / windowHeight,
                       buttonWidth / windowWidth, buttonHeight / windowHeight);
    startY += buttonHeight + spacing;
    
    // Quit button
    quitBtn.initialize(0, 0, buttonWidth, buttonHeight, "Quit", font);
    quitBtn.setCallback([this]() {
        lastAction = MenuAction::Quit;
    });
    mainBorder.addChild(&quitBtn, centerX / windowWidth, startY / windowHeight,
                       buttonWidth / windowWidth, buttonHeight / windowHeight);
}

void PauseMenu::render(Renderer& renderer) {
    if (!initialized) return;
    
    // Check if window size changed and rebuild menu if needed
    sf::Vector2u currentWindowSize = window->getSize();
    if (lastWindowSize.x != currentWindowSize.x || lastWindowSize.y != currentWindowSize.y) {
        buildMenu();
    }
    
    // Render main border (contains all buttons)
    mainBorder.render(renderer);
}

MenuAction PauseMenu::handleEvent(const sf::Event& event) {
    if (!initialized) return MenuAction::None;
    
    lastAction = MenuAction::None;
    
    if (event.is<sf::Event::MouseButtonPressed>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseButton && mouseButton->button == sf::Mouse::Button::Left) {
            sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);
            sf::Vector2f clickPos(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
            mainBorder.handleMousePress(clickPos);
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
