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
    
    std::cout << "[SETTINGS] Building menu..." << std::endl;
    
    auto& settings = SettingsManager::getInstance().getSettings();
    
    // Get actual window size
    sf::Vector2u windowSize = window->getSize();
    lastWindowSize = windowSize;  // Update tracked size
    float windowWidth = static_cast<float>(windowSize.x);
    float windowHeight = static_cast<float>(windowSize.y);
    
    std::cout << "[SETTINGS] Window size: " << windowWidth << "x" << windowHeight << std::endl;
    
    // Scale UI elements
    float buttonWidth = windowWidth * 0.35f;
    float buttonHeight = windowHeight * 0.07f;
    float spacing = buttonHeight * 1.2f;
    float startY = windowHeight * 0.15f;
    unsigned int fontSize = static_cast<unsigned int>(windowHeight * 0.028f);
    
    std::cout << "[SETTINGS] Creating display border..." << std::endl;
    
    // === DISPLAY SETTINGS BORDER ===
    // Border large enough for dropdowns when open (3x dropdown height)
    float displayBorderWidth = buttonWidth;
    float displayBorderHeight = buttonHeight * 5.0f;  // Space for 2 dropdowns + labels + open state
    float displayBorderX = (windowWidth - displayBorderWidth) / 2.0f;
    float displayBorderY = startY;
    
    displayBorder.initialize(displayBorderX, displayBorderY, displayBorderWidth, displayBorderHeight);
    
    std::cout << "[SETTINGS] Initializing display dropdown..." << std::endl;
    
    // Display Mode Dropdown
    displayDropdown.initialize(
        displayBorderX, displayBorderY + displayBorderHeight * 0.05f,  // Initial position
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
    
    std::cout << "[SETTINGS] Adding display dropdown to border..." << std::endl;
    
    // Add to border with relative positioning
    displayBorder.addChild(&displayDropdown.dropdownBox, 0.0f, 0.05f, 1.0f, 0.20f);
    
    if (displayDropdown.selectedText) {
        displayBorder.addChild(displayDropdown.selectedText, 0.05f, 0.05f);
    }
    
    displayBorder.addChild(&displayDropdown.arrow, 0.85f, 0.08f, 0.10f, 0.15f);
    
    // Resolution Dropdown
    resolutionDropdown.initialize(
        displayBorderX, displayBorderY + displayBorderHeight * 0.35f,  // Initial position
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
    
    // Add to border
    displayBorder.addChild(&resolutionDropdown.dropdownBox, 0.0f, 0.35f, 1.0f, 0.20f);
    displayBorder.addChild(resolutionDropdown.selectedText, 0.05f, 0.35f);
    displayBorder.addChild(&resolutionDropdown.arrow, 0.85f, 0.38f, 0.10f, 0.15f);
    
    // === GRID SETTINGS BORDER ===
    float gridBorderWidth = buttonWidth;
    float gridBorderHeight = buttonHeight * 4.5f;  // VSync + 2 number inputs + spacing
    float gridBorderX = (windowWidth - gridBorderWidth) / 2.0f;
    float gridBorderY = startY + spacing * 2.2f;
    
    gridBorder.initialize(gridBorderX, gridBorderY, gridBorderWidth, gridBorderHeight);
    
    // VSync Toggle
    vsyncToggle.initialize(
        gridBorderX + gridBorderWidth * 0.20f, gridBorderY + gridBorderHeight * 0.05f,  // Initial position
        buttonWidth * 0.6f, buttonHeight * 0.6f,
        "VSync", font, settings.vsync
    );
    vsyncToggle.setCallback([this](bool isOn) {
        // VSync will be applied when Apply is clicked
    });
    gridBorder.addChild(&vsyncToggle.track, 0.20f, 0.05f, 0.15f, 0.20f);
    gridBorder.addChild(vsyncToggle.labelText, 0.40f, 0.05f);
    
    // Grid Width Number Input
    gridWidthInput.initialize(
        gridBorderX, gridBorderY + gridBorderHeight * 0.30f,  // Initial position
        buttonWidth, buttonHeight,
        20.0f, 4000.0f, static_cast<float>(settings.gridWidth),
        font
    );
    gridWidthInput.setStep(20.0f);
    gridWidthInput.setCallback([this](float value) {
        // Grid width will be applied when Apply is clicked
    });
    gridBorder.addChild(&gridWidthInput.displayBox, 0.0f, 0.35f, 1.0f, 0.25f);
    gridBorder.addChild(gridWidthInput.labelText, 0.05f, 0.30f);
    gridBorder.addChild(gridWidthInput.valueText, 0.05f, 0.40f);
    
    // Grid Height Number Input
    gridHeightInput.initialize(
        gridBorderX, gridBorderY + gridBorderHeight * 0.63f,  // Initial position
        buttonWidth, buttonHeight,
        20.0f, 4000.0f, static_cast<float>(settings.gridHeight),
        font
    );
    gridHeightInput.setStep(20.0f);
    gridHeightInput.setCallback([this](float value) {
        // Grid height will be applied when Apply is clicked
    });
    gridBorder.addChild(&gridHeightInput.displayBox, 0.0f, 0.68f, 1.0f, 0.25f);
    gridBorder.addChild(gridHeightInput.labelText, 0.05f, 0.63f);
    gridBorder.addChild(gridHeightInput.valueText, 0.05f, 0.73f);
    
    // === ACTION BUTTONS BORDER ===
    float actionBorderWidth = buttonWidth * 1.2f;
    float actionBorderHeight = buttonHeight * 1.5f;
    float actionBorderX = (windowWidth - actionBorderWidth) / 2.0f;
    float actionBorderY = gridBorderY + gridBorderHeight + spacing * 0.5f;
    
    actionBorder.initialize(actionBorderX, actionBorderY, actionBorderWidth, actionBorderHeight);
    
    // Apply Button
    float actionButtonWidth = buttonWidth * 0.5f;
    applyButton.initialize(
        actionBorderX + actionBorderWidth * 0.05f, actionBorderY + actionBorderHeight * 0.15f,  // Initial position
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
    actionBorder.addChild(&applyButton.background, 0.05f, 0.15f, 0.45f, 0.70f);
    actionBorder.addChild(applyButton.buttonText, 0.15f, 0.20f);
    
    // Back Button
    backButton.initialize(
        actionBorderX + actionBorderWidth * 0.50f, actionBorderY + actionBorderHeight * 0.15f,  // Initial position
        actionButtonWidth, buttonHeight,
        "Back", font
    );
    backButton.setColors(sf::Color(120, 40, 40), sf::Color(150, 50, 50), sf::Color(180, 60, 60));
    backButton.setCallback([this]() {
        lastAction = MenuAction::Back;
    });
    actionBorder.addChild(&backButton.background, 0.50f, 0.15f, 0.45f, 0.70f);
    actionBorder.addChild(backButton.buttonText, 0.62f, 0.20f);
    
    // === CAMERA SETTINGS BORDER ===
    float cameraBorderWidth = buttonWidth * 1.2f;
    float cameraBorderHeight = buttonHeight * 4.5f;  // 3 sliders with labels
    float cameraBorderX = (windowWidth - cameraBorderWidth) / 2.0f;
    float cameraBorderY = actionBorderY + actionBorderHeight + spacing * 0.5f;
    
    cameraBorder.initialize(cameraBorderX, cameraBorderY, cameraBorderWidth, cameraBorderHeight);
    
    // Initialize camera control sliders
    float sliderWidth = cameraBorderWidth * 0.85f;
    
    // Camera Speed slider
    cameraSpeedSlider.initialize(
        cameraBorderX + cameraBorderWidth * 0.08f, cameraBorderY + cameraBorderHeight * 0.02f,  // Initial position
        sliderWidth,
        50.0f, 500.0f, settings.cameraScrollSpeed,
        "Camera Speed:", font
    );
    cameraBorder.addChild(&cameraSpeedSlider.track, 0.08f, 0.08f, 0.85f, 0.08f);
    cameraBorder.addChild(cameraSpeedSlider.label, 0.08f, 0.02f);
    cameraBorder.addChild(cameraSpeedSlider.valueText, 0.75f, 0.08f);
    cameraBorder.addChild(&cameraSpeedSlider.thumb, 0.08f, 0.06f, 0.05f, 0.12f);
    
    // Camera Acceleration slider
    cameraAccelSlider.initialize(
        cameraBorderX + cameraBorderWidth * 0.08f, cameraBorderY + cameraBorderHeight * 0.32f,  // Initial position
        sliderWidth,
        100.0f, 1000.0f, settings.cameraAcceleration,
        "Camera Acceleration:", font
    );
    cameraBorder.addChild(&cameraAccelSlider.track, 0.08f, 0.38f, 0.85f, 0.08f);
    cameraBorder.addChild(cameraAccelSlider.label, 0.08f, 0.32f);
    cameraBorder.addChild(cameraAccelSlider.valueText, 0.75f, 0.38f);
    cameraBorder.addChild(&cameraAccelSlider.thumb, 0.08f, 0.36f, 0.05f, 0.12f);
    
    // Camera Max Speed slider
    cameraMaxSpeedSlider.initialize(
        cameraBorderX + cameraBorderWidth * 0.08f, cameraBorderY + cameraBorderHeight * 0.62f,  // Initial position
        sliderWidth,
        200.0f, 1200.0f, settings.cameraMaxSpeed,
        "Camera Max Speed:", font
    );
    cameraBorder.addChild(&cameraMaxSpeedSlider.track, 0.08f, 0.68f, 0.85f, 0.08f);
    cameraBorder.addChild(cameraMaxSpeedSlider.label, 0.08f, 0.62f);
    cameraBorder.addChild(cameraMaxSpeedSlider.valueText, 0.75f, 0.68f);
    cameraBorder.addChild(&cameraMaxSpeedSlider.thumb, 0.08f, 0.66f, 0.05f, 0.12f);
    
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
    
    // Render all UI borders (which contain and render their children)
    displayBorder.render(renderer);
    gridBorder.render(renderer);
    actionBorder.render(renderer);
    cameraBorder.render(renderer);
    
    // Render dropdown popups AFTER borders (highest z-order)
    // Dropdowns manage their own popup rendering when open
    displayDropdown.render(renderer);
    resolutionDropdown.render(renderer);
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

