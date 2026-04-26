#include "MainMenu.h"
#include <iostream>
#include <algorithm>

MainMenu::MainMenu() {
}

MainMenu::~MainMenu() {
    if (continueButton) {
        delete continueButton;
        continueButton = nullptr;
    }
}

void MainMenu::initialize(sf::RenderWindow& window) {
    this->window = &window;
    
    if (!font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "ERROR: MainMenu failed to load font 'assets/fonts/arial.ttf'" << std::endl;
        std::cerr << "ERROR: Current working directory: " << std::filesystem::current_path() << std::endl;
        return;
    }
    
    initialized = true;
    buildMenu();
}

bool MainMenu::isInitialized() const {
    return initialized;
}

void MainMenu::buildMenu() {
    if (!initialized) {
        std::cerr << "ERROR: buildMenu called but menu not initialized" << std::endl;
        return;
    }
    
    checkForRecentSave();
    
    // Get actual window size
    sf::Vector2u windowSize = window->getSize();
    lastWindowSize = windowSize;  // Update tracked size
    windowWidth = static_cast<float>(windowSize.x);
    windowHeight = static_cast<float>(windowSize.y);
    
    // CRITICAL: Clear previous children before rebuilding
    mainBorder.clearChildren();
    
    // Initialize main border (full screen)
    mainBorder.initialize(0, 0, windowWidth, windowHeight);
    
    // Calculate button layout (centered on screen)
    int buttonCount = hasRecentSave ? 5 : 4;
    float buttonWidth = windowWidth * 0.25f;  // 25% of screen width
    float buttonHeight = windowHeight * 0.08f; // 8% of screen height
    float spacing = windowHeight * 0.03f;
    float startY = windowHeight / 2.0f - (buttonHeight * buttonCount + spacing * (buttonCount - 1)) / 2.0f;
    float centerX = (windowWidth - buttonWidth) / 2.0f;
    
    // Continue button (if save exists)
    if (hasRecentSave && continueButton) {
        continueButton->initialize(0, 0, buttonWidth, buttonHeight, "Continue", font);
        continueButton->setCallback([this]() {
            lastAction = MenuAction::Continue;
        });
        mainBorder.addChild(continueButton, centerX / windowWidth, startY / windowHeight, 
                           buttonWidth / windowWidth, buttonHeight / windowHeight);
        startY += buttonHeight + spacing;
    }
    
    // Play button
    playButton.initialize(0, 0, buttonWidth, buttonHeight, "Play", font);
    playButton.setCallback([this]() {
        lastAction = MenuAction::Play;
    });
    mainBorder.addChild(&playButton, centerX / windowWidth, startY / windowHeight,
                       buttonWidth / windowWidth, buttonHeight / windowHeight);
    startY += buttonHeight + spacing;
    
    // Load button
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
    
    // Quit button
    quitButton.initialize(0, 0, buttonWidth, buttonHeight, "Quit", font);
    quitButton.setCallback([this]() {
        lastAction = MenuAction::Quit;
    });
    mainBorder.addChild(&quitButton, centerX / windowWidth, startY / windowHeight,
                       buttonWidth / windowWidth, buttonHeight / windowHeight);
}

void MainMenu::render(Renderer& renderer) {
    if (!initialized) return;
    
    // Check if window size changed and rebuild menu if needed
    sf::Vector2u currentWindowSize = window->getSize();
    if (lastWindowSize.x != currentWindowSize.x || lastWindowSize.y != currentWindowSize.y) {
        buildMenu();
    }
    
    // Render main border (contains all buttons and background)
    mainBorder.render(renderer);
}

MenuAction MainMenu::handleEvent(const sf::Event& event) {
    if (!initialized) return MenuAction::None;
    
    // Reset last action
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

void MainMenu::checkForRecentSave() {
    hasRecentSave = SaveManager::getInstance().hasRecentSave();
    
    if (hasRecentSave && !continueButton) {
        continueButton = new UIButton();
    } else if (!hasRecentSave && continueButton) {
        delete continueButton;
        continueButton = nullptr;
    }
}

