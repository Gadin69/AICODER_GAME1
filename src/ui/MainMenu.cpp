#include "MainMenu.h"
#include <iostream>
#include <algorithm>

MainMenu::MainMenu() {
}

MainMenu::~MainMenu() {
}

void MainMenu::initialize(sf::RenderWindow& window) {
    this->window = &window;
    
    if (!font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "ERROR: Failed to load font 'assets/fonts/arial.ttf'" << std::endl;
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
    
    // Get actual window size
    sf::Vector2u windowSize = window->getSize();
    lastWindowSize = windowSize;  // Update tracked size
    float windowWidth = static_cast<float>(windowSize.x);
    float windowHeight = static_cast<float>(windowSize.y);
    
    // Scale UI elements based on window size
    float buttonWidth = windowWidth * 0.25f;  // 25% of window width
    float buttonHeight = windowHeight * 0.08f; // 8% of window height
    float spacing = windowHeight * 0.03f;
    float startY = windowHeight / 2.0f - buttonHeight * 1.5f - spacing;
    
    // Scale font size based on window height
    unsigned int fontSize = static_cast<unsigned int>(windowHeight * 0.035f);
    
    // Play button
    playButton.initialize(
        (windowWidth - buttonWidth) / 2.0f,
        startY,
        buttonWidth, buttonHeight,
        "Play", font
    );
    playButton.setCallback([this]() {
        lastAction = MenuAction::Play;
    });
    
    // Settings button
    settingsButton.initialize(
        (windowWidth - buttonWidth) / 2.0f,
        startY + buttonHeight + spacing,
        buttonWidth, buttonHeight,
        "Settings", font
    );
    settingsButton.setCallback([this]() {
        lastAction = MenuAction::Settings;
    });
    
    // Quit button
    quitButton.initialize(
        (windowWidth - buttonWidth) / 2.0f,
        startY + (buttonHeight + spacing) * 2,
        buttonWidth, buttonHeight,
        "Quit", font
    );
    quitButton.setCallback([this]() {
        lastAction = MenuAction::Quit;
    });
}

void MainMenu::render(Renderer& renderer) {
    if (!initialized) return;
    
    // Check if window size changed and rebuild menu if needed
    sf::Vector2u currentWindowSize = window->getSize();
    if (lastWindowSize.x != currentWindowSize.x || lastWindowSize.y != currentWindowSize.y) {
        lastWindowSize = currentWindowSize;
        buildMenu();  // Rebuild to recenter buttons
    }
    
    sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);
    mousePos = sf::Vector2f(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
    
    // Update button hover states
    playButton.handleMouseMove(mousePos);
    settingsButton.handleMouseMove(mousePos);
    quitButton.handleMouseMove(mousePos);
    
    // Get actual window size
    sf::Vector2u windowSize = window->getSize();
    float windowWidth = static_cast<float>(windowSize.x);
    float windowHeight = static_cast<float>(windowSize.y);
    
    // Render semi-transparent background
    sf::RectangleShape bgShape(sf::Vector2f(windowWidth, windowHeight));
    bgShape.setFillColor(sf::Color(0, 0, 0, 180));
    bgShape.setPosition(sf::Vector2f(0, 0));
    renderer.drawRectangle(bgShape);
    
    // Render title - scale font based on window height
    unsigned int titleFontSize = static_cast<unsigned int>(windowHeight * 0.07f);
    sf::Text title(font, "ONI-like Simulation", titleFontSize);
    title.setFillColor(sf::Color(100, 200, 255));
    auto titleBounds = title.getLocalBounds();
    title.setPosition(sf::Vector2f(
        (windowWidth - titleBounds.size.x) / 2.0f,
        windowHeight * 0.2f
    ));
    renderer.drawText(title);
    
    // Render buttons
    playButton.render(renderer);
    settingsButton.render(renderer);
    quitButton.render(renderer);
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
            
            // Route mouse press to buttons
            playButton.handleMousePress(clickPos);
            settingsButton.handleMousePress(clickPos);
            quitButton.handleMousePress(clickPos);
        }
    } else if (event.is<sf::Event::MouseButtonReleased>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonReleased>();
        if (mouseButton && mouseButton->button == sf::Mouse::Button::Left) {
            sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);
            sf::Vector2f clickPos(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
            
            // Route mouse release to buttons (triggers callbacks)
            playButton.handleMouseRelease();
            settingsButton.handleMouseRelease();
            quitButton.handleMouseRelease();
        }
    }
    
    return lastAction;
}

