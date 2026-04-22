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
    buttons.clear();
    
    if (!initialized) {
        std::cerr << "ERROR: buildMenu called but menu not initialized" << std::endl;
        return;
    }
    
    // Get actual window size
    sf::Vector2u windowSize = window->getSize();
    lastWindowSize = windowSize;  // Update tracked size
    float windowWidth = static_cast<float>(windowSize.x);
    float windowHeight = static_cast<float>(windowSize.y);
    
    // Scale UI elements
    float buttonWidth = windowWidth * 0.25f;
    float buttonHeight = windowHeight * 0.08f;
    float spacing = windowHeight * 0.03f;
    float startY = windowHeight / 2.0f - buttonHeight * 1.5f - spacing;
    unsigned int fontSize = static_cast<unsigned int>(windowHeight * 0.035f);
    
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
        
        btn.text = new sf::Text(font, labels[i], fontSize);
        btn.text->setFillColor(sf::Color::White);
        
        auto textBounds = btn.text->getLocalBounds();
        btn.text->setPosition(sf::Vector2f(
            btn.background.getPosition().x + (buttonWidth - textBounds.size.x) / 2.0f,
            btn.background.getPosition().y + (buttonHeight - textBounds.size.y) / 2.0f
        ));
        
        buttons.push_back(std::move(btn));
    }
}

void PauseMenu::render(Renderer& renderer) {
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
    bgShape.setFillColor(sf::Color(0, 0, 0, 150));
    bgShape.setPosition(sf::Vector2f(0, 0));
    renderer.drawRectangle(bgShape);
    
    unsigned int titleFontSize = static_cast<unsigned int>(windowHeight * 0.07f);
    sf::Text title(font, "PAUSED", titleFontSize);
    title.setFillColor(sf::Color(255, 200, 100));
    auto titleBounds = title.getLocalBounds();
    title.setPosition(sf::Vector2f(
        (windowWidth - titleBounds.size.x) / 2.0f,
        windowHeight * 0.2f
    ));
    renderer.drawText(title);
    
    renderButtons(renderer);
}

MenuAction PauseMenu::handleEvent(const sf::Event& event) {
    if (!initialized) return MenuAction::None;
    
    if (event.is<sf::Event::MouseButtonPressed>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseButton && mouseButton->button == sf::Mouse::Button::Left) {
            sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);
            sf::Vector2f clickPos(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
            
            for (size_t i = 0; i < buttons.size(); ++i) {
                if (isMouseOverButton(buttons[i], clickPos)) {
                    if (buttons[i].label == "Resume") {
                        return MenuAction::Resume;
                    } else if (buttons[i].label == "Settings") {
                        return MenuAction::Settings;
                    } else if (buttons[i].label == "Quit to Main") {
                        return MenuAction::QuitToMain;
                    }
                    break;
                }
            }
        }
    }
    
    return MenuAction::None;
}

void PauseMenu::renderButtons(Renderer& renderer) {
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

bool PauseMenu::isMouseOverButton(const Button& btn, const sf::Vector2f& mousePos) {
    sf::Vector2f pos = btn.background.getPosition();
    sf::Vector2f size = btn.background.getSize();
    return mousePos.x >= pos.x && mousePos.x <= pos.x + size.x &&
           mousePos.y >= pos.y && mousePos.y <= pos.y + size.y;
}

void PauseMenu::updateButtonHover(const sf::Vector2f& mousePos) {
    for (auto& btn : buttons) {
        btn.isHovered = isMouseOverButton(btn, mousePos);
    }
}
