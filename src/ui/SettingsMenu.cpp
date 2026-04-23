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
    buttons.clear();
    
    if (!initialized) {
        std::cerr << "ERROR: buildMenu called but menu not initialized" << std::endl;
        return;
    }
    
    auto& settings = SettingsManager::getInstance().getSettings();
    
    selectedDisplayMode = static_cast<int>(settings.displayMode);
    vsyncEnabled = settings.vsync;
    gridWidthInput = static_cast<int>(settings.gridWidth);
    gridHeightInput = static_cast<int>(settings.gridHeight);
    
    for (size_t i = 0; i < resolutions.size(); ++i) {
        if (resolutions[i].first == settings.screenWidth && 
            resolutions[i].second == settings.screenHeight) {
            selectedResolution = static_cast<int>(i);
            break;
        }
    }
    
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
    
    // Display Mode
    Button displayBtn;
    displayBtn.label = "Display: ";
    std::string modes[] = {"Windowed", "Fullscreen", "Borderless"};
    displayBtn.label += modes[selectedDisplayMode];
    displayBtn.background.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    displayBtn.background.setPosition(sf::Vector2f((windowWidth - buttonWidth) / 2.0f, startY));
    displayBtn.background.setFillColor(sf::Color(50, 50, 50));
    displayBtn.background.setOutlineColor(sf::Color(100, 100, 100));
    displayBtn.background.setOutlineThickness(2.0f);
    displayBtn.text = new sf::Text(font, displayBtn.label, fontSize);
    displayBtn.text->setFillColor(sf::Color::White);
    auto txtBounds = displayBtn.text->getLocalBounds();
    displayBtn.text->setPosition(sf::Vector2f(
        displayBtn.background.getPosition().x + 20,
        displayBtn.background.getPosition().y + (buttonHeight - txtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(displayBtn));
    
    // Resolution
    Button resBtn;
    resBtn.label = "Resolution: " + std::to_string(resolutions[selectedResolution].first) + 
                   "x" + std::to_string(resolutions[selectedResolution].second);
    resBtn.background.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    resBtn.background.setPosition(sf::Vector2f((windowWidth - buttonWidth) / 2.0f, startY + spacing));
    resBtn.background.setFillColor(sf::Color(50, 50, 50));
    resBtn.background.setOutlineColor(sf::Color(100, 100, 100));
    resBtn.background.setOutlineThickness(2.0f);
    resBtn.text = new sf::Text(font, resBtn.label, fontSize);
    resBtn.text->setFillColor(sf::Color::White);
    auto resTxtBounds = resBtn.text->getLocalBounds();
    resBtn.text->setPosition(sf::Vector2f(
        resBtn.background.getPosition().x + 20,
        resBtn.background.getPosition().y + (buttonHeight - resTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(resBtn));
    
    // VSync
    Button vsyncBtn;
    vsyncBtn.label = "VSync: " + std::string(vsyncEnabled ? "ON" : "OFF");
    vsyncBtn.background.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    vsyncBtn.background.setPosition(sf::Vector2f((windowWidth - buttonWidth) / 2.0f, startY + spacing * 2));
    vsyncBtn.background.setFillColor(sf::Color(50, 50, 50));
    vsyncBtn.background.setOutlineColor(sf::Color(100, 100, 100));
    vsyncBtn.background.setOutlineThickness(2.0f);
    vsyncBtn.text = new sf::Text(font, vsyncBtn.label, fontSize);
    vsyncBtn.text->setFillColor(sf::Color::White);
    auto vsyncTxtBounds = vsyncBtn.text->getLocalBounds();
    vsyncBtn.text->setPosition(sf::Vector2f(
        vsyncBtn.background.getPosition().x + 20,
        vsyncBtn.background.getPosition().y + (buttonHeight - vsyncTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(vsyncBtn));
    
    // Grid Width
    Button gridWBtn;
    gridWBtn.label = "Grid Width: " + std::to_string(gridWidthInput);
    gridWBtn.background.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    gridWBtn.background.setPosition(sf::Vector2f((windowWidth - buttonWidth) / 2.0f, startY + spacing * 3));
    gridWBtn.background.setFillColor(sf::Color(50, 50, 50));
    gridWBtn.background.setOutlineColor(sf::Color(100, 100, 100));
    gridWBtn.background.setOutlineThickness(2.0f);
    gridWBtn.text = new sf::Text(font, gridWBtn.label, fontSize);
    gridWBtn.text->setFillColor(sf::Color::White);
    auto gridWTxtBounds = gridWBtn.text->getLocalBounds();
    gridWBtn.text->setPosition(sf::Vector2f(
        gridWBtn.background.getPosition().x + 20,
        gridWBtn.background.getPosition().y + (buttonHeight - gridWTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(gridWBtn));
    
    // Grid Height
    Button gridHBtn;
    gridHBtn.label = "Grid Height: " + std::to_string(gridHeightInput);
    gridHBtn.background.setSize(sf::Vector2f(buttonWidth, buttonHeight));
    gridHBtn.background.setPosition(sf::Vector2f((windowWidth - buttonWidth) / 2.0f, startY + spacing * 4));
    gridHBtn.background.setFillColor(sf::Color(50, 50, 50));
    gridHBtn.background.setOutlineColor(sf::Color(100, 100, 100));
    gridHBtn.background.setOutlineThickness(2.0f);
    gridHBtn.text = new sf::Text(font, gridHBtn.label, fontSize);
    gridHBtn.text->setFillColor(sf::Color::White);
    auto gridHTxtBounds = gridHBtn.text->getLocalBounds();
    gridHBtn.text->setPosition(sf::Vector2f(
        gridHBtn.background.getPosition().x + 20,
        gridHBtn.background.getPosition().y + (buttonHeight - gridHTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(gridHBtn));
    
    // Apply
    Button applyBtn;
    float actionButtonWidth = buttonWidth * 0.5f;
    applyBtn.label = "Apply";
    applyBtn.background.setSize(sf::Vector2f(actionButtonWidth, buttonHeight));
    applyBtn.background.setPosition(sf::Vector2f(windowWidth / 2.0f - actionButtonWidth - spacing * 0.5f, startY + spacing * 5 + buttonHeight * 0.3f));
    applyBtn.background.setFillColor(sf::Color(40, 120, 40));
    applyBtn.background.setOutlineColor(sf::Color(60, 180, 60));
    applyBtn.background.setOutlineThickness(2.0f);
    applyBtn.text = new sf::Text(font, "Apply", fontSize);
    applyBtn.text->setFillColor(sf::Color::White);
    auto applyTxtBounds = applyBtn.text->getLocalBounds();
    applyBtn.text->setPosition(sf::Vector2f(
        applyBtn.background.getPosition().x + (actionButtonWidth - applyTxtBounds.size.x) / 2.0f,
        applyBtn.background.getPosition().y + (buttonHeight - applyTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(applyBtn));
    
    // Back
    Button backBtn;
    backBtn.label = "Back";
    backBtn.background.setSize(sf::Vector2f(actionButtonWidth, buttonHeight));
    backBtn.background.setPosition(sf::Vector2f(windowWidth / 2.0f + spacing * 0.5f, startY + spacing * 5 + buttonHeight * 0.3f));
    backBtn.background.setFillColor(sf::Color(120, 40, 40));
    backBtn.background.setOutlineColor(sf::Color(180, 60, 60));
    backBtn.background.setOutlineThickness(2.0f);
    backBtn.text = new sf::Text(font, "Back", fontSize);
    backBtn.text->setFillColor(sf::Color::White);
    auto backTxtBounds = backBtn.text->getLocalBounds();
    backBtn.text->setPosition(sf::Vector2f(
        backBtn.background.getPosition().x + (actionButtonWidth - backTxtBounds.size.x) / 2.0f,
        backBtn.background.getPosition().y + (buttonHeight - backTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(backBtn));
    
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
        100.0f, 1000.0f, 400.0f,
        "Camera Acceleration:", font
    );
    
    // Camera Max Speed slider
    cameraMaxSpeedSlider.initialize(
        (windowWidth - sliderWidth) / 2.0f, sliderY + spacing * 3.0f,
        sliderWidth,
        200.0f, 1200.0f, 600.0f,
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
    
    updateButtonHover(mousePos);
    
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
    
    renderButtons(renderer);
    
    // Render camera control sliders
    cameraSpeedSlider.render(renderer);
    cameraAccelSlider.render(renderer);
    cameraMaxSpeedSlider.render(renderer);
}

MenuAction SettingsMenu::handleEvent(const sf::Event& event) {
    if (!initialized) return MenuAction::None;
    
    // Handle slider events
    if (event.is<sf::Event::MouseButtonPressed>()) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(*window);
        sf::Vector2f pos(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));
        cameraSpeedSlider.handleMousePress(pos);
        cameraAccelSlider.handleMousePress(pos);
        cameraMaxSpeedSlider.handleMousePress(pos);
        if (cameraSpeedSlider.isDragging || cameraAccelSlider.isDragging || cameraMaxSpeedSlider.isDragging) {
            cameraSettingsChanged = true;
        }
    } else if (event.is<sf::Event::MouseButtonReleased>()) {
        cameraSpeedSlider.handleMouseRelease();
        cameraAccelSlider.handleMouseRelease();
        cameraMaxSpeedSlider.handleMouseRelease();
    } else if (event.is<sf::Event::MouseMoved>()) {
        auto mouseMove = event.getIf<sf::Event::MouseMoved>();
        if (mouseMove) {
            sf::Vector2f pos(static_cast<float>(mouseMove->position.x), static_cast<float>(mouseMove->position.y));
            cameraSpeedSlider.handleMouseMove(pos);
            cameraAccelSlider.handleMouseMove(pos);
            cameraMaxSpeedSlider.handleMouseMove(pos);
        }
    }
    
    if (event.is<sf::Event::MouseButtonPressed>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseButton && mouseButton->button == sf::Mouse::Button::Left) {
            sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);
            sf::Vector2f clickPos(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
            
            for (size_t i = 0; i < buttons.size(); ++i) {
                if (isMouseOverButton(buttons[i], clickPos)) {
                    if (buttons[i].label.find("Display:") == 0) {
                        selectedDisplayMode = (selectedDisplayMode + 1) % 3;
                        updateButtonText(buttons[i]);
                    } else if (buttons[i].label.find("Resolution:") == 0) {
                        selectedResolution = (selectedResolution + 1) % resolutions.size();
                        updateButtonText(buttons[i]);
                    } else if (buttons[i].label.find("VSync:") == 0) {
                        vsyncEnabled = !vsyncEnabled;
                        updateButtonText(buttons[i]);
                    } else if (buttons[i].label.find("Grid Width:") == 0) {
                        gridWidthInput = std::min(4000, gridWidthInput + 20);
                        updateButtonText(buttons[i]);
                    } else if (buttons[i].label.find("Grid Height:") == 0) {
                        gridHeightInput = std::min(4000, gridHeightInput + 20);
                        updateButtonText(buttons[i]);
                    } else if (buttons[i].label == "Apply") {
                        auto& settings = SettingsManager::getInstance().getSettings();
                        settings.displayMode = static_cast<DisplayMode>(selectedDisplayMode);
                        settings.screenWidth = resolutions[selectedResolution].first;
                        settings.screenHeight = resolutions[selectedResolution].second;
                        settings.vsync = vsyncEnabled;
                        settings.gridWidth = gridWidthInput;
                        settings.gridHeight = gridHeightInput;
                        
                        // Apply camera settings if changed
                        if (cameraSettingsChanged) {
                            settings.cameraScrollSpeed = cameraSpeedSlider.currentValue;
                            settings.cameraAcceleration = cameraAccelSlider.currentValue;
                            settings.cameraMaxSpeed = cameraMaxSpeedSlider.currentValue;
                        }
                        
                        return MenuAction::ApplySettings;
                    } else if (buttons[i].label == "Back") {
                        return MenuAction::Back;
                    }
                    break;
                }
            }
        }
    }
    
    return MenuAction::None;
}

void SettingsMenu::updateButtonText(Button& btn) {
    if (btn.label.find("Display:") == 0) {
        std::string modes[] = {"Windowed", "Fullscreen", "Borderless"};
        btn.label = "Display: " + std::string(modes[selectedDisplayMode]);
    } else if (btn.label.find("Resolution:") == 0) {
        btn.label = "Resolution: " + 
                    std::to_string(resolutions[selectedResolution].first) + 
                    "x" + std::to_string(resolutions[selectedResolution].second);
    } else if (btn.label.find("VSync:") == 0) {
        btn.label = "VSync: " + std::string(vsyncEnabled ? "ON" : "OFF");
    } else if (btn.label.find("Grid Width:") == 0) {
        btn.label = "Grid Width: " + std::to_string(gridWidthInput);
    } else if (btn.label.find("Grid Height:") == 0) {
        btn.label = "Grid Height: " + std::to_string(gridHeightInput);
    }
    
    if (btn.text) {
        btn.text->setString(btn.label);
        auto textBounds = btn.text->getLocalBounds();
        float btnWidth = btn.background.getSize().x;
        float btnHeight = btn.background.getSize().y;
        btn.text->setPosition(sf::Vector2f(
            btn.background.getPosition().x + (btnWidth - textBounds.size.x) / 2.0f,
            btn.background.getPosition().y + (btnHeight - textBounds.size.y) / 2.0f
        ));
    }
}

void SettingsMenu::renderButtons(Renderer& renderer) {
    for (auto& btn : buttons) {
        if (btn.isHovered) {
            btn.background.setFillColor(sf::Color(80, 80, 80));
        } else {
            btn.background.setFillColor(sf::Color(50, 50, 50));
        }
        
        renderer.drawRectangle(btn.background);
        if (btn.text) renderer.drawText(*btn.text);
    }
}

bool SettingsMenu::isMouseOverButton(const Button& btn, const sf::Vector2f& mousePos) {
    sf::Vector2f pos = btn.background.getPosition();
    sf::Vector2f size = btn.background.getSize();
    return mousePos.x >= pos.x && mousePos.x <= pos.x + size.x &&
           mousePos.y >= pos.y && mousePos.y <= pos.y + size.y;
}

void SettingsMenu::updateButtonHover(const sf::Vector2f& mousePos) {
    for (auto& btn : buttons) {
        btn.isHovered = isMouseOverButton(btn, mousePos);
    }
}
