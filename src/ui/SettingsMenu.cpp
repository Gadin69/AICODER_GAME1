#include "SettingsMenu.h"
#include "MainMenu.h" // For MenuAction
#include <iostream>
#include <algorithm>

SettingsMenu::SettingsMenu() {
}

SettingsMenu::~SettingsMenu() {
}

void SettingsMenu::initialize(sf::RenderWindow& window) {
    this->window = &window;
    
    if (!font.openFromFile("assets/fonts/arial.ttf")) {
        std::cerr << "ERROR: Failed to load font 'assets/fonts/arial.ttf'" << std::endl;
        return;
    }
    
    initialized = true;
    buildMenu();
}

bool SettingsMenu::isInitialized() const {
    return initialized;
}

void SettingsMenu::buildMenu() {
    if (!initialized) {
        std::cerr << "ERROR: buildMenu called but menu not initialized" << std::endl;
        return;
    }
    
    auto& settings = SettingsManager::getInstance().getSettings();
    
    // Get actual window size
    sf::Vector2u windowSize = window->getSize();
    lastWindowSize = windowSize;  // Update tracked size
    float windowWidth = static_cast<float>(windowSize.x);
    float windowHeight = static_cast<float>(windowSize.y);
    
    // Scale UI elements
    float buttonWidth = windowWidth * 0.35f;
    float buttonHeight = windowHeight * 0.07f;
    float spacing = buttonHeight * 1.2f;
    float startY = windowHeight * 0.15f;
    unsigned int fontSize = static_cast<unsigned int>(windowHeight * 0.028f);
    
    // Display Mode Dropdown
    displayDropdown.initialize(
        (windowWidth - buttonWidth) / 2.0f, startY,
        buttonWidth, buttonHeight,
        font
    );
    displayDropdown.addOption("Windowed");
    displayDropdown.addOption("Fullscreen");
    displayDropdown.addOption("Borderless");
    displayDropdown.setSelectedIndex(static_cast<int>(settings.displayMode));
    displayDropdown.setCallback([this](int index, const std::string& value) {
        // Display mode will be applied when Apply is clicked
    });
    
    // Resolution Dropdown
    resolutionDropdown.initialize(
        (windowWidth - buttonWidth) / 2.0f, startY + spacing,
        buttonWidth, buttonHeight,
        font
    );
    for (const auto& res : resolutions) {
        resolutionDropdown.addOption(std::to_string(res.first) + "x" + std::to_string(res.second));
    }
    // Find current resolution
    for (size_t i = 0; i < resolutions.size(); ++i) {
        if (resolutions[i].first == settings.screenWidth && 
            resolutions[i].second == settings.screenHeight) {
            resolutionDropdown.setSelectedIndex(static_cast<int>(i));
            break;
        }
    }
    resolutionDropdown.setCallback([this](int index, const std::string& value) {
        // Resolution will be applied when Apply is clicked
    });
    
    // VSync Toggle
    vsyncToggle.initialize(
        (windowWidth - buttonWidth) / 2.0f, startY + spacing * 2,
        buttonWidth * 0.6f, buttonHeight * 0.6f,
        "VSync", font, settings.vsync
    );
    vsyncToggle.setCallback([this](bool isOn) {
        // VSync will be applied when Apply is clicked
    });
    
    // Grid Width Number Input
    gridWidthInput.initialize(
        (windowWidth - buttonWidth) / 2.0f, startY + spacing * 3,
        buttonWidth, buttonHeight,
        20.0f, 4000.0f, static_cast<float>(settings.gridWidth),
        font
    );
    gridWidthInput.setStep(20.0f);
    gridWidthInput.setCallback([this](float value) {
        // Grid width will be applied when Apply is clicked
    });
    
    // Grid Height Number Input
    gridHeightInput.initialize(
        (windowWidth - buttonWidth) / 2.0f, startY + spacing * 4,
        buttonWidth, buttonHeight,
        20.0f, 4000.0f, static_cast<float>(settings.gridHeight),
        font
    );
    gridHeightInput.setStep(20.0f);
    gridHeightInput.setCallback([this](float value) {
        // Grid height will be applied when Apply is clicked
    });
    
    // Apply Button
    float actionButtonWidth = buttonWidth * 0.5f;
    applyButton.initialize(
        windowWidth / 2.0f - actionButtonWidth - spacing * 0.5f,
        startY + spacing * 5 + buttonHeight * 0.3f,
        actionButtonWidth, buttonHeight,
        "Apply", font
    );
    applyButton.setColors(sf::Color(40, 120, 40), sf::Color(50, 150, 50), sf::Color(60, 180, 60));
    applyButton.setCallback([this]() {
        auto& settings = SettingsManager::getInstance().getSettings();
        settings.displayMode = static_cast<DisplayMode>(displayDropdown.getSelectedIndex());
        auto selectedRes = resolutions[resolutionDropdown.getSelectedIndex()];
        settings.screenWidth = selectedRes.first;
        settings.screenHeight = selectedRes.second;
        settings.vsync = vsyncToggle.isOn;
        settings.gridWidth = static_cast<int>(gridWidthInput.value);
        settings.gridHeight = static_cast<int>(gridHeightInput.value);
        
        // Apply camera settings if changed
        if (cameraSettingsChanged) {
            settings.cameraScrollSpeed = cameraSpeedSlider.currentValue;
            settings.cameraAcceleration = cameraAccelSlider.currentValue;
            settings.cameraMaxSpeed = cameraMaxSpeedSlider.currentValue;
        }
        
        lastAction = MenuAction::ApplySettings;
    });
    
    // Back Button
    backButton.initialize(
        windowWidth / 2.0f + spacing * 0.5f,
        startY + spacing * 5 + buttonHeight * 0.3f,
        actionButtonWidth, buttonHeight,
        "Back", font
    );
    backButton.setColors(sf::Color(120, 40, 40), sf::Color(150, 50, 50), sf::Color(180, 60, 60));
    backButton.setCallback([this]() {
        lastAction = MenuAction::Back;
    });
    
    // Initialize camera control sliders
    float sliderY = startY + spacing * 6.5f;
    float sliderWidth = windowWidth * 0.4f;
    
    // Camera Speed slider
    cameraSpeedSlider.initialize(
        (windowWidth - sliderWidth) / 2.0f, sliderY,
        sliderWidth,
        50.0f, 500.0f, settings.cameraScrollSpeed,
        "Camera Speed:", font
    );
    
    // Camera Acceleration slider
    cameraAccelSlider.initialize(
        (windowWidth - sliderWidth) / 2.0f, sliderY + spacing * 1.5f,
        sliderWidth,
        100.0f, 1000.0f, settings.cameraAcceleration,
        "Camera Acceleration:", font
    );
    
    // Camera Max Speed slider
    cameraMaxSpeedSlider.initialize(
        (windowWidth - sliderWidth) / 2.0f, sliderY + spacing * 3.0f,
        sliderWidth,
        200.0f, 1200.0f, settings.cameraMaxSpeed,
        "Camera Max Speed:", font
    );
    
    cameraSettingsChanged = false;
}

void SettingsMenu::render(Renderer& renderer) {
    if (!initialized) return;
    
    // Check if window size changed and rebuild menu if needed
    sf::Vector2u currentWindowSize = window->getSize();
    if (lastWindowSize.x != currentWindowSize.x || lastWindowSize.y != currentWindowSize.y) {
        lastWindowSize = currentWindowSize;
        buildMenu();  // Rebuild to recenter buttons
    }
    
    sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);
    mousePos = sf::Vector2f(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
    
    sf::Vector2u windowSize = window->getSize();
    float windowWidth = static_cast<float>(windowSize.x);
    float windowHeight = static_cast<float>(windowSize.y);
    
    sf::RectangleShape bgShape(sf::Vector2f(windowWidth, windowHeight));
    bgShape.setFillColor(sf::Color(0, 0, 0, 200));
    bgShape.setPosition(sf::Vector2f(0, 0));
    renderer.drawRectangle(bgShape);
    
    unsigned int titleFontSize = static_cast<unsigned int>(windowHeight * 0.07f);
    sf::Text title(font, "Settings", titleFontSize);
    title.setFillColor(sf::Color(200, 200, 200));
    auto titleBounds = title.getLocalBounds();
    title.setPosition(sf::Vector2f(
        (windowWidth - titleBounds.size.x) / 2.0f,
        windowHeight * 0.04f
    ));
    renderer.drawText(title);
    
    // Render all UI components
    displayDropdown.render(renderer);
    resolutionDropdown.render(renderer);
    vsyncToggle.render(renderer);
    gridWidthInput.render(renderer);
    gridHeightInput.render(renderer);
    applyButton.render(renderer);
    backButton.render(renderer);
    
    // Render camera control sliders
    cameraSpeedSlider.render(renderer);
    cameraAccelSlider.render(renderer);
    cameraMaxSpeedSlider.render(renderer);
}

MenuAction SettingsMenu::handleEvent(const sf::Event& event) {
    if (!initialized) return MenuAction::None;
    
    // Reset last action
    lastAction = MenuAction::None;
    
    sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);
    sf::Vector2f mousePosVec(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
    
    // Handle slider events
    if (event.is<sf::Event::MouseButtonPressed>()) {
        cameraSpeedSlider.handleMousePress(mousePosVec);
        cameraAccelSlider.handleMousePress(mousePosVec);
        cameraMaxSpeedSlider.handleMousePress(mousePosVec);
        if (cameraSpeedSlider.isDragging || cameraAccelSlider.isDragging || cameraMaxSpeedSlider.isDragging) {
            cameraSettingsChanged = true;
        }
        
        // Route to other components
        displayDropdown.handleMousePress(mousePosVec);
        resolutionDropdown.handleMousePress(mousePosVec);
        vsyncToggle.handleMousePress(mousePosVec);
        gridWidthInput.handleMousePress(mousePosVec);
        gridHeightInput.handleMousePress(mousePosVec);
        applyButton.handleMousePress(mousePosVec);
        backButton.handleMousePress(mousePosVec);
    } else if (event.is<sf::Event::MouseButtonReleased>()) {
        cameraSpeedSlider.handleMouseRelease();
        cameraAccelSlider.handleMouseRelease();
        cameraMaxSpeedSlider.handleMouseRelease();
        
        // Release triggers callbacks for buttons
        applyButton.handleMouseRelease();
        backButton.handleMouseRelease();
    } else if (event.is<sf::Event::MouseMoved>()) {
        auto mouseMove = event.getIf<sf::Event::MouseMoved>();
        if (mouseMove) {
            sf::Vector2f pos(static_cast<float>(mouseMove->position.x), static_cast<float>(mouseMove->position.y));
            cameraSpeedSlider.handleMouseMove(pos);
            cameraAccelSlider.handleMouseMove(pos);
            cameraMaxSpeedSlider.handleMouseMove(pos);
            
            displayDropdown.handleMouseMove(pos);
            resolutionDropdown.handleMouseMove(pos);
            gridWidthInput.handleMouseMove(pos);
            gridHeightInput.handleMouseMove(pos);
            applyButton.handleMouseMove(pos);
            backButton.handleMouseMove(pos);
        }
    }
    
    // Handle keyboard events for number inputs
    if (event.is<sf::Event::KeyPressed>()) {
        auto keyEvent = event.getIf<sf::Event::KeyPressed>();
        if (keyEvent) {
            gridWidthInput.handleKeyPress(*keyEvent);
            gridHeightInput.handleKeyPress(*keyEvent);
        }
    } else if (event.is<sf::Event::TextEntered>()) {
        auto textEvent = event.getIf<sf::Event::TextEntered>();
        if (textEvent) {
            gridWidthInput.handleTextEntered(*textEvent);
            gridHeightInput.handleTextEntered(*textEvent);
        }
    }
    
    return lastAction;
}

