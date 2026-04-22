#include "MainMenu.h"
#include <iostream>
#include <algorithm>

MainMenu::MainMenu() {
}

void MainMenu::initialize(sf::Font& font) {
    this->font = &font;
    initialized = true;
    buildMainMenu();
}

MenuState MainMenu::getState() const {
    return currentState;
}

void MainMenu::setState(MenuState state) {
    currentState = state;
    switch (state) {
        case MenuState::Main:
            buildMainMenu();
            break;
        case MenuState::Settings:
            buildSettingsMenu();
            break;
        case MenuState::Paused:
            buildPausedMenu();
            break;
    }
}

void MainMenu::setPaused(bool paused) {
    if (paused) {
        setState(MenuState::Paused);
    }
}

bool MainMenu::isInitialized() const {
    return initialized;
}

void MainMenu::buildMainMenu() {
    buttons.clear();
    sliders.clear();
    
    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    float buttonWidth = 300.0f;
    float buttonHeight = 60.0f;
    float spacing = 20.0f;
    float startY = windowHeight / 2.0f - 100.0f;
    
    std::vector<std::string> labels = {"Play", "Settings", "Quit"};
    
    for (size_t i = 0; i < labels.size(); ++i) {
        Button btn;
        btn.label = labels[i];
        btn.background.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        btn.background.setPosition(sf::Vector2f(
            (windowWidth - buttonWidth) / 2.0f,
            startY + i * (buttonHeight + spacing)
        ));
        btn.background.setFillColor(sf::Color(50, 50, 50));
        btn.background.setOutlineColor(sf::Color(100, 100, 100));
        btn.background.setOutlineThickness(2.0f);
        
        btn.text = new sf::Text(*font, labels[i], 24);
        btn.text->setFillColor(sf::Color::White);
        
        auto textBounds = btn.text->getLocalBounds();
        btn.text->setPosition(sf::Vector2f(
            btn.background.getPosition().x + (buttonWidth - textBounds.size.x) / 2.0f,
            btn.background.getPosition().y + (buttonHeight - textBounds.size.y) / 2.0f
        ));
        
        buttons.push_back(std::move(btn));
    }
}

void MainMenu::buildSettingsMenu() {
    buttons.clear();
    sliders.clear();
    
    auto& settings = SettingsManager::getInstance().getSettings();
    
    // Initialize from current settings
    selectedDisplayMode = static_cast<int>(settings.displayMode);
    vsyncEnabled = settings.vsync;
    gridWidthInput = static_cast<int>(settings.gridWidth);
    gridHeightInput = static_cast<int>(settings.gridHeight);
    
    // Find matching resolution
    for (size_t i = 0; i < resolutions.size(); ++i) {
        if (resolutions[i].first == settings.screenWidth && 
            resolutions[i].second == settings.screenHeight) {
            selectedResolution = static_cast<int>(i);
            break;
        }
    }
    
    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    float startY = 100.0f;
    float spacing = 80.0f;
    
    // Display Mode selector button
    Button displayBtn;
    displayBtn.label = "Display: ";
    std::string modes[] = {"Windowed", "Fullscreen", "Borderless"};
    displayBtn.label += modes[selectedDisplayMode];
    displayBtn.background.setSize(sf::Vector2f(400, 50));
    displayBtn.background.setPosition(sf::Vector2f((windowWidth - 400) / 2.0f, startY));
    displayBtn.background.setFillColor(sf::Color(50, 50, 50));
    displayBtn.background.setOutlineColor(sf::Color(100, 100, 100));
    displayBtn.background.setOutlineThickness(2.0f);
    displayBtn.text = new sf::Text(*font, displayBtn.label, 20);
    displayBtn.text->setFillColor(sf::Color::White);
    auto txtBounds = displayBtn.text->getLocalBounds();
    displayBtn.text->setPosition(sf::Vector2f(
        displayBtn.background.getPosition().x + 20,
        displayBtn.background.getPosition().y + (50 - txtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(displayBtn));
    
    // Resolution selector button
    Button resBtn;
    resBtn.label = "Resolution: " + std::to_string(resolutions[selectedResolution].first) + 
                   "x" + std::to_string(resolutions[selectedResolution].second);
    resBtn.background.setSize(sf::Vector2f(400, 50));
    resBtn.background.setPosition(sf::Vector2f((windowWidth - 400) / 2.0f, startY + spacing));
    resBtn.background.setFillColor(sf::Color(50, 50, 50));
    resBtn.background.setOutlineColor(sf::Color(100, 100, 100));
    resBtn.background.setOutlineThickness(2.0f);
    resBtn.text = new sf::Text(*font, resBtn.label, 20);
    resBtn.text->setFillColor(sf::Color::White);
    auto resTxtBounds = resBtn.text->getLocalBounds();
    resBtn.text->setPosition(sf::Vector2f(
        resBtn.background.getPosition().x + 20,
        resBtn.background.getPosition().y + (50 - resTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(resBtn));
    
    // VSync toggle button
    Button vsyncBtn;
    vsyncBtn.label = "VSync: " + std::string(vsyncEnabled ? "ON" : "OFF");
    vsyncBtn.background.setSize(sf::Vector2f(400, 50));
    vsyncBtn.background.setPosition(sf::Vector2f((windowWidth - 400) / 2.0f, startY + spacing * 2));
    vsyncBtn.background.setFillColor(sf::Color(50, 50, 50));
    vsyncBtn.background.setOutlineColor(sf::Color(100, 100, 100));
    vsyncBtn.background.setOutlineThickness(2.0f);
    vsyncBtn.text = new sf::Text(*font, vsyncBtn.label, 20);
    vsyncBtn.text->setFillColor(sf::Color::White);
    auto vsyncTxtBounds = vsyncBtn.text->getLocalBounds();
    vsyncBtn.text->setPosition(sf::Vector2f(
        vsyncBtn.background.getPosition().x + 20,
        vsyncBtn.background.getPosition().y + (50 - vsyncTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(vsyncBtn));
    
    // Grid Width input button
    Button gridWBtn;
    gridWBtn.label = "Grid Width: " + std::to_string(gridWidthInput);
    gridWBtn.background.setSize(sf::Vector2f(400, 50));
    gridWBtn.background.setPosition(sf::Vector2f((windowWidth - 400) / 2.0f, startY + spacing * 3));
    gridWBtn.background.setFillColor(sf::Color(50, 50, 50));
    gridWBtn.background.setOutlineColor(sf::Color(100, 100, 100));
    gridWBtn.background.setOutlineThickness(2.0f);
    gridWBtn.text = new sf::Text(*font, gridWBtn.label, 20);
    gridWBtn.text->setFillColor(sf::Color::White);
    auto gridWTxtBounds = gridWBtn.text->getLocalBounds();
    gridWBtn.text->setPosition(sf::Vector2f(
        gridWBtn.background.getPosition().x + 20,
        gridWBtn.background.getPosition().y + (50 - gridWTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(gridWBtn));
    
    // Grid Height input button
    Button gridHBtn;
    gridHBtn.label = "Grid Height: " + std::to_string(gridHeightInput);
    gridHBtn.background.setSize(sf::Vector2f(400, 50));
    gridHBtn.background.setPosition(sf::Vector2f((windowWidth - 400) / 2.0f, startY + spacing * 4));
    gridHBtn.background.setFillColor(sf::Color(50, 50, 50));
    gridHBtn.background.setOutlineColor(sf::Color(100, 100, 100));
    gridHBtn.background.setOutlineThickness(2.0f);
    gridHBtn.text = new sf::Text(*font, gridHBtn.label, 20);
    gridHBtn.text->setFillColor(sf::Color::White);
    auto gridHTxtBounds = gridHBtn.text->getLocalBounds();
    gridHBtn.text->setPosition(sf::Vector2f(
        gridHBtn.background.getPosition().x + 20,
        gridHBtn.background.getPosition().y + (50 - gridHTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(gridHBtn));
    
    // Apply button
    Button applyBtn;
    applyBtn.label = "Apply";
    applyBtn.background.setSize(sf::Vector2f(200, 50));
    applyBtn.background.setPosition(sf::Vector2f(windowWidth / 2.0f - 220, startY + spacing * 5 + 20));
    applyBtn.background.setFillColor(sf::Color(40, 120, 40));
    applyBtn.background.setOutlineColor(sf::Color(60, 180, 60));
    applyBtn.background.setOutlineThickness(2.0f);
    applyBtn.text = new sf::Text(*font, "Apply", 20);
    applyBtn.text->setFillColor(sf::Color::White);
    auto applyTxtBounds = applyBtn.text->getLocalBounds();
    applyBtn.text->setPosition(sf::Vector2f(
        applyBtn.background.getPosition().x + (200 - applyTxtBounds.size.x) / 2.0f,
        applyBtn.background.getPosition().y + (50 - applyTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(applyBtn));
    
    // Back button
    Button backBtn;
    backBtn.label = "Back";
    backBtn.background.setSize(sf::Vector2f(200, 50));
    backBtn.background.setPosition(sf::Vector2f(windowWidth / 2.0f + 20, startY + spacing * 5 + 20));
    backBtn.background.setFillColor(sf::Color(120, 40, 40));
    backBtn.background.setOutlineColor(sf::Color(180, 60, 60));
    backBtn.background.setOutlineThickness(2.0f);
    backBtn.text = new sf::Text(*font, "Back", 20);
    backBtn.text->setFillColor(sf::Color::White);
    auto backTxtBounds = backBtn.text->getLocalBounds();
    backBtn.text->setPosition(sf::Vector2f(
        backBtn.background.getPosition().x + (200 - backTxtBounds.size.x) / 2.0f,
        backBtn.background.getPosition().y + (50 - backTxtBounds.size.y) / 2.0f
    ));
    buttons.push_back(std::move(backBtn));
}

void MainMenu::buildPausedMenu() {
    buttons.clear();
    sliders.clear();
    
    float windowWidth = 1280.0f;
    float windowHeight = 720.0f;
    float buttonWidth = 300.0f;
    float buttonHeight = 60.0f;
    float spacing = 20.0f;
    float startY = windowHeight / 2.0f - 100.0f;
    
    std::vector<std::string> labels = {"Resume", "Settings", "Quit to Main"};
    
    for (size_t i = 0; i < labels.size(); ++i) {
        Button btn;
        btn.label = labels[i];
        btn.background.setSize(sf::Vector2f(buttonWidth, buttonHeight));
        btn.background.setPosition(sf::Vector2f(
            (windowWidth - buttonWidth) / 2.0f,
            startY + i * (buttonHeight + spacing)
        ));
        btn.background.setFillColor(sf::Color(50, 50, 50));
        btn.background.setOutlineColor(sf::Color(100, 100, 100));
        btn.background.setOutlineThickness(2.0f);
        
        btn.text = new sf::Text(*font, labels[i], 24);
        btn.text->setFillColor(sf::Color::White);
        
        auto textBounds = btn.text->getLocalBounds();
        btn.text->setPosition(sf::Vector2f(
            btn.background.getPosition().x + (buttonWidth - textBounds.size.x) / 2.0f,
            btn.background.getPosition().y + (buttonHeight - textBounds.size.y) / 2.0f
        ));
        
        buttons.push_back(std::move(btn));
    }
}

MenuAction MainMenu::render(Renderer& renderer, Window& window) {
    if (!initialized) return MenuAction::None;
    
    // Get mouse position
    sf::Vector2i sfMousePos = sf::Mouse::getPosition(window.getRenderWindow());
    mousePos = sf::Vector2f(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
    
    // Update hover states
    updateButtonHover(mousePos);
    
    // Render semi-transparent background for menus
    sf::RectangleShape overlay(sf::Vector2f(1280, 720));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));
    renderer.drawRectangle(overlay);
    
    // Render title for main menu
    if (currentState == MenuState::Main) {
        sf::Text title(*font, "ONI-like Simulation", 48);
        title.setFillColor(sf::Color(100, 200, 255));
        auto titleBounds = title.getLocalBounds();
        title.setPosition(sf::Vector2f(
            (1280 - titleBounds.size.x) / 2.0f,
            150.0f
        ));
        renderer.drawText(title);
    } else if (currentState == MenuState::Paused) {
        sf::Text title(*font, "PAUSED", 48);
        title.setFillColor(sf::Color(255, 200, 100));
        auto titleBounds = title.getLocalBounds();
        title.setPosition(sf::Vector2f(
            (1280 - titleBounds.size.x) / 2.0f,
            150.0f
        ));
        renderer.drawText(title);
    } else if (currentState == MenuState::Settings) {
        sf::Text title(*font, "Settings", 48);
        title.setFillColor(sf::Color(200, 200, 200));
        auto titleBounds = title.getLocalBounds();
        title.setPosition(sf::Vector2f(
            (1280 - titleBounds.size.x) / 2.0f,
            30.0f
        ));
        renderer.drawText(title);
    }
    
    // Render buttons
    renderButtons(renderer);
    
    return MenuAction::None;
}

void MainMenu::handleEvent(const sf::Event& event) {
    if (!initialized) return;
    
    if (event.is<sf::Event::MouseButtonPressed>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseButton && mouseButton->button == sf::Mouse::Button::Left) {
            sf::Vector2i sfMousePos = sf::Mouse::getPosition();
            sf::Vector2f clickPos(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
            
            for (size_t i = 0; i < buttons.size(); ++i) {
                if (isMouseOverButton(buttons[i], clickPos)) {
                    // Handle button clicks based on current state
                    if (currentState == MenuState::Main) {
                        if (buttons[i].label == "Play") {
                            // Will be handled by main loop
                        } else if (buttons[i].label == "Settings") {
                            // Will be handled by main loop
                        } else if (buttons[i].label == "Quit") {
                            // Will be handled by main loop
                        }
                    } else if (currentState == MenuState::Paused) {
                        if (buttons[i].label == "Resume") {
                            // Will be handled by main loop
                        } else if (buttons[i].label == "Settings") {
                            // Will be handled by main loop
                        } else if (buttons[i].label == "Quit to Main") {
                            // Will be handled by main loop
                        }
                    } else if (currentState == MenuState::Settings) {
                        if (buttons[i].label.find("Display:") == 0) {
                            selectedDisplayMode = (selectedDisplayMode + 1) % 3;
                            buttons[i].label = "Display: ";
                            std::string modes[] = {"Windowed", "Fullscreen", "Borderless"};
                            buttons[i].label += modes[selectedDisplayMode];
                            if (buttons[i].text) buttons[i].text->setString(buttons[i].label);
                        } else if (buttons[i].label.find("Resolution:") == 0) {
                            selectedResolution = (selectedResolution + 1) % resolutions.size();
                            buttons[i].label = "Resolution: " + 
                                               std::to_string(resolutions[selectedResolution].first) + 
                                               "x" + std::to_string(resolutions[selectedResolution].second);
                            if (buttons[i].text) buttons[i].text->setString(buttons[i].label);
                        } else if (buttons[i].label.find("VSync:") == 0) {
                            vsyncEnabled = !vsyncEnabled;
                            buttons[i].label = "VSync: " + std::string(vsyncEnabled ? "ON" : "OFF");
                            if (buttons[i].text) buttons[i].text->setString(buttons[i].label);
                        } else if (buttons[i].label.find("Grid Width:") == 0) {
                            gridWidthInput = std::min(4000, gridWidthInput + 20);
                            buttons[i].label = "Grid Width: " + std::to_string(gridWidthInput);
                            if (buttons[i].text) buttons[i].text->setString(buttons[i].label);
                        } else if (buttons[i].label.find("Grid Height:") == 0) {
                            gridHeightInput = std::min(4000, gridHeightInput + 20);
                            buttons[i].label = "Grid Height: " + std::to_string(gridHeightInput);
                            if (buttons[i].text) buttons[i].text->setString(buttons[i].label);
                        } else if (buttons[i].label == "Apply") {
                            // Will be handled by main loop
                        } else if (buttons[i].label == "Back") {
                            // Will be handled by main loop
                        }
                    }
                    break;
                }
            }
        }
    }
}

void MainMenu::renderButtons(Renderer& renderer) {
    for (auto& btn : buttons) {
        // Update colors based on hover state
        if (btn.isHovered) {
            btn.background.setFillColor(sf::Color(80, 80, 80));
        } else {
            btn.background.setFillColor(sf::Color(50, 50, 50));
        }
        
        renderer.drawRectangle(btn.background);
        if (btn.text) renderer.drawText(*btn.text);
    }
}

bool MainMenu::isMouseOverButton(const Button& button, const sf::Vector2f& mousePos) const {
    sf::FloatRect bounds = button.background.getGlobalBounds();
    return bounds.contains(mousePos);
}

void MainMenu::updateButtonHover(const sf::Vector2f& mousePos) {
    for (auto& btn : buttons) {
        btn.isHovered = isMouseOverButton(btn, mousePos);
    }
}
