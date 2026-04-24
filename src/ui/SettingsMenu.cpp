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
    
    std::cout << "[SETTINGS] Building tab-based menu..." << std::endl;
    
    auto& settings = SettingsManager::getInstance().getSettings();
    
    // Get actual window size
    sf::Vector2u windowSize = window->getSize();
    lastWindowSize = windowSize;
    windowWidth = static_cast<float>(windowSize.x);
    windowHeight = static_cast<float>(windowSize.y);
    
    std::cout << "[SETTINGS] Window size: " << windowWidth << "x" << windowHeight << std::endl;
    
    // Calculate main border dimensions (80% width, 85% height, centered)
    float mainWidth = windowWidth * 0.80f;
    float mainHeight = windowHeight * 0.85f;
    float mainX = (windowWidth - mainWidth) / 2.0f;
    float mainY = (windowHeight - mainHeight) / 2.0f;
    
    mainBorder.initialize(mainX, mainY, mainWidth, mainHeight);
    
    // Calculate common dimensions
    titleHeight = windowHeight * 0.08f;
    buttonHeight = windowHeight * 0.07f;
    float spacing = windowHeight * 0.02f;
    
    // Build all layout sections
    buildTitle();
    buildSidebar();
    buildContent();
    buildButtons();
    
    // Set initial category (Graphics)
    switchCategory(1);
}

void SettingsMenu::buildTitle() {
    // Title at top of main border
    float titleWidth = mainBorder.getSize().x;
    float titleX = mainBorder.getPosition().x;
    float titleY = mainBorder.getPosition().y;
    
    titleBorder.initialize(titleX, titleY, titleWidth, titleHeight);
    
    // Add "Settings" title text
    unsigned int titleFontSize = static_cast<unsigned int>(windowHeight * 0.05f);
    sf::Text* titleText = new sf::Text(font, "Settings", titleFontSize);
    titleText->setFillColor(sf::Color(220, 220, 220));
    
    // Center title in title border
    auto titleBounds = titleText->getLocalBounds();
    float titleTextX = titleX + (titleWidth - titleBounds.size.x) / 2.0f;
    float titleTextY = titleY + (titleHeight - titleBounds.size.y) / 2.0f - titleBounds.position.y;
    titleText->setPosition(sf::Vector2f(titleTextX, titleTextY));
    
    titleBorder.addChild(titleText, 0.0f, 0.0f);  // Position will be updated by border
}

void SettingsMenu::buildSidebar() {
    // Left sidebar: 25% width of main border
    float sidebarWidth = mainBorder.getSize().x * 0.25f;
    float sidebarHeight = mainBorder.getSize().y - titleHeight - buttonHeight;
    float sidebarX = mainBorder.getPosition().x;
    float sidebarY = mainBorder.getPosition().y + titleHeight;
    
    sidebarBorder.initialize(sidebarX, sidebarY, sidebarWidth, sidebarHeight);
    
    // Create 5 category tab buttons stacked vertically
    float tabHeight = (sidebarHeight - 20.0f) / 5.0f;  // 20px total spacing
    float tabY = sidebarY + 10.0f;  // Start 10px from top
    
    // Controls Tab
    controlsTab.initialize(sidebarX + 10.0f, tabY, sidebarWidth - 20.0f, tabHeight - 4.0f, "Controls", font);
    controlsTab.setColors(sf::Color(50, 50, 50), sf::Color(60, 60, 60), sf::Color(70, 70, 70));
    controlsTab.setCallback([this]() { switchCategory(0); });
    sidebarBorder.addChild(&controlsTab, 0.02f, 0.0f, 0.96f, 0.19f);
    
    // Graphics Tab
    tabY += tabHeight;
    graphicsTab.initialize(sidebarX + 10.0f, tabY, sidebarWidth - 20.0f, tabHeight - 4.0f, "Graphics", font);
    graphicsTab.setColors(sf::Color(50, 50, 50), sf::Color(60, 60, 60), sf::Color(70, 70, 70));
    graphicsTab.setCallback([this]() { switchCategory(1); });
    sidebarBorder.addChild(&graphicsTab, 0.02f, 0.20f, 0.96f, 0.19f);
    
    // Audio Tab
    tabY += tabHeight;
    audioTab.initialize(sidebarX + 10.0f, tabY, sidebarWidth - 20.0f, tabHeight - 4.0f, "Audio", font);
    audioTab.setColors(sf::Color(50, 50, 50), sf::Color(60, 60, 60), sf::Color(70, 70, 70));
    audioTab.setCallback([this]() { switchCategory(2); });
    sidebarBorder.addChild(&audioTab, 0.02f, 0.40f, 0.96f, 0.19f);
    
    // Video Tab
    tabY += tabHeight;
    videoTab.initialize(sidebarX + 10.0f, tabY, sidebarWidth - 20.0f, tabHeight - 4.0f, "Video", font);
    videoTab.setColors(sf::Color(50, 50, 50), sf::Color(60, 60, 60), sf::Color(70, 70, 70));
    videoTab.setCallback([this]() { switchCategory(3); });
    sidebarBorder.addChild(&videoTab, 0.02f, 0.60f, 0.96f, 0.19f);
    
    // Gameplay Tab
    tabY += tabHeight;
    gameplayTab.initialize(sidebarX + 10.0f, tabY, sidebarWidth - 20.0f, tabHeight - 4.0f, "Gameplay", font);
    gameplayTab.setColors(sf::Color(50, 50, 50), sf::Color(60, 60, 60), sf::Color(70, 70, 70));
    gameplayTab.setCallback([this]() { switchCategory(4); });
    sidebarBorder.addChild(&gameplayTab, 0.02f, 0.80f, 0.96f, 0.19f);
}

void SettingsMenu::buildContent() {
    // Right content area: 75% width of main border
    float contentWidth = mainBorder.getSize().x * 0.75f;
    float contentHeight = mainBorder.getSize().y - titleHeight - buttonHeight;
    float contentX = mainBorder.getPosition().x + mainBorder.getSize().x * 0.25f;
    float contentY = mainBorder.getPosition().y + titleHeight;
    
    contentBorder.initialize(contentX, contentY, contentWidth, contentHeight);
    
    // Initialize all UI components (but don't add to border yet - will add in switchCategory)
    auto& settings = SettingsManager::getInstance().getSettings();
    
    // Calculate component sizes
    float componentWidth = contentWidth * 0.90f;
    float componentHeight = contentHeight * 0.12f;
    float compX = contentX + contentWidth * 0.05f;
    float compStartY = contentY + 20.0f;
    
    // Display Mode Dropdown (for Graphics tab)
    displayDropdown.initialize(compX, compStartY, componentWidth, componentHeight, font);
    displayDropdown.addOption("Windowed");
    displayDropdown.addOption("Fullscreen");
    displayDropdown.addOption("Borderless");
    displayDropdown.setSelectedIndex(static_cast<int>(settings.displayMode));
    
    // Resolution Dropdown (for Graphics tab)
    float resY = compStartY + componentHeight + 15.0f;
    resolutionDropdown.initialize(compX, resY, componentWidth, componentHeight, font);
    for (const auto& res : resolutions) {
        resolutionDropdown.addOption(std::to_string(res.first) + "x" + std::to_string(res.second));
    }
    for (size_t i = 0; i < resolutions.size(); ++i) {
        if (resolutions[i].first == settings.screenWidth && resolutions[i].second == settings.screenHeight) {
            resolutionDropdown.setSelectedIndex(static_cast<int>(i));
            break;
        }
    }
    
    // VSync Toggle (for Gameplay tab)
    float vsyncY = compStartY;
    vsyncToggle.initialize(compX, vsyncY, componentWidth * 0.4f, componentHeight * 0.8f, "VSync", font, settings.vsync);
    
    // Grid Width Input (for Gameplay tab)
    float gridY = compStartY + componentHeight + 15.0f;
    gridWidthInput.initialize(compX, gridY, componentWidth, componentHeight, 20.0f, 4000.0f, static_cast<float>(settings.gridWidth), font);
    gridWidthInput.setStep(20.0f);
    
    // Grid Height Input (for Gameplay tab)
    float gridHeightY = gridY + componentHeight + 15.0f;
    gridHeightInput.initialize(compX, gridHeightY, componentWidth, componentHeight, 20.0f, 4000.0f, static_cast<float>(settings.gridHeight), font);
    gridHeightInput.setStep(20.0f);
    
    // Camera Speed Slider (for Video tab)
    float sliderWidth = componentWidth * 0.85f;
    cameraSpeedSlider.initialize(compX, compStartY, sliderWidth, 50.0f, 500.0f, settings.cameraScrollSpeed, "Camera Speed:", font);
    
    // Camera Acceleration Slider (for Video tab)
    float accelY = compStartY + componentHeight + 20.0f;
    cameraAccelSlider.initialize(compX, accelY, sliderWidth, 100.0f, 1000.0f, settings.cameraAcceleration, "Camera Acceleration:", font);
    
    // Camera Max Speed Slider (for Video tab)
    float maxSpeedY = accelY + componentHeight + 20.0f;
    cameraMaxSpeedSlider.initialize(compX, maxSpeedY, sliderWidth, 200.0f, 1200.0f, settings.cameraMaxSpeed, "Camera Max Speed:", font);
    
    cameraSettingsChanged = false;
}

void SettingsMenu::buildButtons() {
    // Bottom button area
    float buttonWidth = mainBorder.getSize().x;
    float buttonX = mainBorder.getPosition().x;
    float buttonY = mainBorder.getPosition().y + mainBorder.getSize().y - buttonHeight;
    
    buttonBorder.initialize(buttonX, buttonY, buttonWidth, buttonHeight);
    
    // Back button (left side)
    float btnWidth = buttonWidth * 0.2f;
    float btnHeight = buttonHeight * 0.7f;
    float btnY = buttonY + (buttonHeight - btnHeight) / 2.0f;
    
    backButton.initialize(buttonX + 20.0f, btnY, btnWidth, btnHeight, "Back", font);
    backButton.setColors(sf::Color(120, 40, 40), sf::Color(150, 50, 50), sf::Color(180, 60, 60));
    backButton.setCallback([this]() {
        lastAction = MenuAction::Back;
    });
    buttonBorder.addChild(&backButton, 0.02f, 0.15f, 0.20f, 0.70f);
    
    // Apply button (center-left)
    float applyX = buttonX + buttonWidth * 0.35f;
    applyButton.initialize(applyX, btnY, btnWidth, btnHeight, "Apply", font);
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
        
        if (cameraSettingsChanged) {
            settings.cameraScrollSpeed = cameraSpeedSlider.currentValue;
            settings.cameraAcceleration = cameraAccelSlider.currentValue;
            settings.cameraMaxSpeed = cameraMaxSpeedSlider.currentValue;
        }
        
        lastAction = MenuAction::ApplySettings;
    });
    buttonBorder.addChild(&applyButton, 0.35f, 0.15f, 0.20f, 0.70f);
    
    // Reset to Default button (right side)
    float resetX = buttonX + buttonWidth * 0.70f;
    resetButton.initialize(resetX, btnY, btnWidth * 1.3f, btnHeight, "Reset to Default", font);
    resetButton.setColors(sf::Color(40, 100, 40), sf::Color(50, 130, 50), sf::Color(60, 160, 60));
    resetButton.setCallback([this]() {
        auto& settings = SettingsManager::getInstance().getSettings();
        settings.displayMode = DisplayMode::Windowed;
        settings.screenWidth = 1920;
        settings.screenHeight = 1080;
        settings.vsync = true;
        settings.gridWidth = 200;
        settings.gridHeight = 200;
        settings.cameraScrollSpeed = 200.0f;
        settings.cameraAcceleration = 500.0f;
        settings.cameraMaxSpeed = 800.0f;
        
        // Rebuild menu to reflect changes
        buildMenu();
    });
    buttonBorder.addChild(&resetButton, 0.70f, 0.15f, 0.28f, 0.70f);
}

void SettingsMenu::switchCategory(int category) {
    activeCategory = category;
    
    // Clear current content from contentBorder
    contentBorder.clearChildren();
    
    // Add appropriate UI components based on category
    switch(category) {
        case 0: // Controls
            // Add placeholder text
            {
                unsigned int fontSize = static_cast<unsigned int>(windowHeight * 0.03f);
                sf::Text* placeholder = new sf::Text(font, "Controls settings coming soon", fontSize);
                placeholder->setFillColor(sf::Color(180, 180, 180));
                contentBorder.addChild(placeholder, 0.35f, 0.45f);
            }
            break;
            
        case 1: // Graphics
            contentBorder.addChild(&displayDropdown, 0.05f, 0.05f, 0.90f, 0.15f);
            contentBorder.addChild(&resolutionDropdown, 0.05f, 0.25f, 0.90f, 0.15f);
            break;
            
        case 2: // Audio
            // Add placeholder text
            {
                unsigned int fontSize = static_cast<unsigned int>(windowHeight * 0.03f);
                sf::Text* placeholder = new sf::Text(font, "Audio settings coming soon", fontSize);
                placeholder->setFillColor(sf::Color(180, 180, 180));
                contentBorder.addChild(placeholder, 0.35f, 0.45f);
            }
            break;
            
        case 3: // Video
            contentBorder.addChild(&cameraSpeedSlider, 0.05f, 0.05f, 0.90f, 0.15f);
            contentBorder.addChild(&cameraAccelSlider, 0.05f, 0.28f, 0.90f, 0.15f);
            contentBorder.addChild(&cameraMaxSpeedSlider, 0.05f, 0.51f, 0.90f, 0.15f);
            break;
            
        case 4: // Gameplay
            contentBorder.addChild(&vsyncToggle, 0.05f, 0.05f, 0.40f, 0.15f);
            contentBorder.addChild(&gridWidthInput, 0.05f, 0.25f, 0.90f, 0.15f);
            contentBorder.addChild(&gridHeightInput, 0.05f, 0.45f, 0.90f, 0.15f);
            break;
    }
    
    // Update tab button colors
    updateTabColors();
}

void SettingsMenu::updateTabColors() {
    // Reset all tabs to inactive color
    sf::Color inactiveColor = sf::Color(50, 50, 50);
    sf::Color activeColor = sf::Color(60, 120, 60);  // Green highlight
    
    controlsTab.setColors(inactiveColor, sf::Color(60, 60, 60), sf::Color(70, 70, 70));
    graphicsTab.setColors(inactiveColor, sf::Color(60, 60, 60), sf::Color(70, 70, 70));
    audioTab.setColors(inactiveColor, sf::Color(60, 60, 60), sf::Color(70, 70, 70));
    videoTab.setColors(inactiveColor, sf::Color(60, 60, 60), sf::Color(70, 70, 70));
    gameplayTab.setColors(inactiveColor, sf::Color(60, 60, 60), sf::Color(70, 70, 70));
    
    // Set active tab color
    switch(activeCategory) {
        case 0: controlsTab.setColors(activeColor, sf::Color(70, 140, 70), sf::Color(80, 160, 80)); break;
        case 1: graphicsTab.setColors(activeColor, sf::Color(70, 140, 70), sf::Color(80, 160, 80)); break;
        case 2: audioTab.setColors(activeColor, sf::Color(70, 140, 70), sf::Color(80, 160, 80)); break;
        case 3: videoTab.setColors(activeColor, sf::Color(70, 140, 70), sf::Color(80, 160, 80)); break;
        case 4: gameplayTab.setColors(activeColor, sf::Color(70, 140, 70), sf::Color(80, 160, 80)); break;
    }
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
    
    // Draw dark background
    sf::RectangleShape bgShape(sf::Vector2f(windowWidth, windowHeight));
    bgShape.setFillColor(sf::Color(0, 0, 0, 200));
    bgShape.setPosition(sf::Vector2f(0, 0));
    renderer.drawRectangle(bgShape);
    
    // Render all borders in order (title, sidebar, content, buttons)
    titleBorder.render(renderer);
    sidebarBorder.render(renderer);
    contentBorder.render(renderer);
    buttonBorder.render(renderer);
    
    // Render dropdown popups AFTER borders (highest z-order)
    displayDropdown.render(renderer);
    resolutionDropdown.render(renderer);
}

MenuAction SettingsMenu::handleEvent(const sf::Event& event) {
    if (!initialized) return MenuAction::None;
    
    // Reset last action
    lastAction = MenuAction::None;
    
    sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);
    sf::Vector2f mousePosVec(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
    
    // Handle mouse press
    if (event.is<sf::Event::MouseButtonPressed>()) {
        // Handle category tab clicks
        controlsTab.handleMousePress(mousePosVec);
        graphicsTab.handleMousePress(mousePosVec);
        audioTab.handleMousePress(mousePosVec);
        videoTab.handleMousePress(mousePosVec);
        gameplayTab.handleMousePress(mousePosVec);
        
        // Handle content UI based on active category
        switch(activeCategory) {
            case 1: // Graphics
                displayDropdown.handleMousePress(mousePosVec);
                resolutionDropdown.handleMousePress(mousePosVec);
                break;
            case 3: // Video
                cameraSpeedSlider.handleMousePress(mousePosVec);
                cameraAccelSlider.handleMousePress(mousePosVec);
                cameraMaxSpeedSlider.handleMousePress(mousePosVec);
                if (cameraSpeedSlider.isDragging || cameraAccelSlider.isDragging || cameraMaxSpeedSlider.isDragging) {
                    cameraSettingsChanged = true;
                }
                break;
            case 4: // Gameplay
                vsyncToggle.handleMousePress(mousePosVec);
                gridWidthInput.handleMousePress(mousePosVec);
                gridHeightInput.handleMousePress(mousePosVec);
                break;
        }
        
        // Handle bottom buttons
        backButton.handleMousePress(mousePosVec);
    } else if (event.is<sf::Event::MouseButtonReleased>()) {
        // Handle slider releases
        cameraSpeedSlider.handleMouseRelease();
        cameraAccelSlider.handleMouseRelease();
        cameraMaxSpeedSlider.handleMouseRelease();
        
        // Release triggers callbacks for buttons
        controlsTab.handleMouseRelease();
        graphicsTab.handleMouseRelease();
        audioTab.handleMouseRelease();
        videoTab.handleMouseRelease();
        gameplayTab.handleMouseRelease();
        backButton.handleMouseRelease();
        resetButton.handleMouseRelease();
    } else if (event.is<sf::Event::MouseMoved>()) {
        auto mouseMove = event.getIf<sf::Event::MouseMoved>();
        if (mouseMove) {
            sf::Vector2f pos(static_cast<float>(mouseMove->position.x), static_cast<float>(mouseMove->position.y));
            
            // Handle tab hover
            controlsTab.handleMouseMove(pos);
            graphicsTab.handleMouseMove(pos);
            audioTab.handleMouseMove(pos);
            videoTab.handleMouseMove(pos);
            gameplayTab.handleMouseMove(pos);
            
            // Handle content UI based on active category
            switch(activeCategory) {
                case 1: // Graphics
                    displayDropdown.handleMouseMove(pos);
                    resolutionDropdown.handleMouseMove(pos);
                    break;
                case 3: // Video
                    cameraSpeedSlider.handleMouseMove(pos);
                    cameraAccelSlider.handleMouseMove(pos);
                    cameraMaxSpeedSlider.handleMouseMove(pos);
                    break;
                case 4: // Gameplay
                    gridWidthInput.handleMouseMove(pos);
                    gridHeightInput.handleMouseMove(pos);
                    break;
            }
            
            backButton.handleMouseMove(pos);
            resetButton.handleMouseMove(pos);
        }
    }
    
    // Handle keyboard events for number inputs
    if (event.is<sf::Event::KeyPressed>()) {
        auto keyEvent = event.getIf<sf::Event::KeyPressed>();
        if (keyEvent && activeCategory == 4) { // Gameplay tab
            gridWidthInput.handleKeyPress(*keyEvent);
            gridHeightInput.handleKeyPress(*keyEvent);
        }
    } else if (event.is<sf::Event::TextEntered>()) {
        auto textEvent = event.getIf<sf::Event::TextEntered>();
        if (textEvent && activeCategory == 4) { // Gameplay tab
            gridWidthInput.handleTextEntered(*textEvent);
            gridHeightInput.handleTextEntered(*textEvent);
        }
    }
    
    return lastAction;
}

