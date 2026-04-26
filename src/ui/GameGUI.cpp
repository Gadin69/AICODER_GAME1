#include "GameGUI.h"
#include "../rendering/Renderer.h"

GameGUI::GameGUI() {}

GameGUI::~GameGUI() {
    if (simSpeedSlider) {
        delete simSpeedSlider;
        simSpeedSlider = nullptr;
    }
    if (elementSelector) {
        delete elementSelector;
        elementSelector = nullptr;
    }
}

void GameGUI::initialize(const sf::Font& font) {
    fontPtr = &font;
    
    // Build the skillbar layout
    buildSkillbarLayout();
    
    // Build element selector layout
    buildElementSelectorLayout();
    
    initialized = true;
}

void GameGUI::handleResize(int windowWidth, int windowHeight) {
    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;
    
    // Rebuild layout with new dimensions
    buildSkillbarLayout();
    buildElementSelectorLayout();
}

void GameGUI::render(Renderer& renderer) {
    if (!initialized) return;
    
    // Render skillbar border and all its children
    skillbarBorder.render(renderer);
    
    // Render element selector (DevMode only - use isAdminMode)
    extern bool isAdminMode;
    if (isAdminMode && elementSelector) {
        elementSelector->render(renderer);
    }
}

void GameGUI::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    // Route mouse events to slider
    if (simSpeedSlider) {
        simSpeedSlider->handleMousePress(mousePos);
    }
    
    // Route to element selector (DevMode only - use isAdminMode)
    extern bool isAdminMode;
    if (isAdminMode && elementSelector) {
        elementSelector->handleMousePress(mousePos);
    }
}

void GameGUI::handleMouseRelease() {
    if (!initialized) return;
    
    if (simSpeedSlider) {
        simSpeedSlider->handleMouseRelease();
    }
    
    extern bool isAdminMode;
    if (isAdminMode && elementSelector) {
        elementSelector->handleMouseRelease();
    }
}

void GameGUI::handleMouseMove(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    if (simSpeedSlider) {
        simSpeedSlider->handleMouseMove(mousePos);
    }
    
    extern bool isAdminMode;
    if (isAdminMode && elementSelector) {
        elementSelector->handleMouseMove(mousePos);
    }
}

bool GameGUI::isMouseOverUI(const sf::Vector2f& mousePos) const {
    if (!initialized) return false;
    
    // Check if mouse is over skillbar border
    if (skillbarBorder.containsPoint(mousePos)) return true;
    
    // Check if mouse is over element selector (DevMode only)
    extern bool isAdminMode;
    if (isAdminMode && elementSelector) {
        sf::Vector2f elemPos = elementSelector->getPosition();
        sf::Vector2f elemSize = elementSelector->getSize();
        if (mousePos.x >= elemPos.x && mousePos.x <= elemPos.x + elemSize.x &&
            mousePos.y >= elemPos.y && mousePos.y <= elemPos.y + elemSize.y) {
            return true;
        }
    }
    
    return false;
}

void GameGUI::buildSkillbarLayout() {
    if (!fontPtr) return;
    
    // Create skillbar border at bottom center
    float borderWidth = 600.0f;
    float borderHeight = 80.0f;
    float borderX = (currentWindowWidth - borderWidth) / 2.0f;  // Center X
    float borderY = currentWindowHeight - borderHeight - 20.0f;  // 20px from bottom
    
    skillbarBorder.initialize(borderX, borderY, borderWidth, borderHeight);
    
    // Create slider if it doesn't exist
    if (!simSpeedSlider) {
        simSpeedSlider = new UISlider();
        simSpeedSlider->initialize(0, 0, 300, 0.1f, 5.0f, 1.0f, "Sim Speed:", *fontPtr);
    }
    
    // Add slider as UIElement child (polymorphic - handles all internal rendering)
    // Position: 5% from left, 15% from top, 90% width, 70% height of border
    skillbarBorder.addChild(simSpeedSlider, 0.05f, 0.15f, 0.90f, 0.70f);
}

void GameGUI::buildElementSelectorLayout() {
    if (!fontPtr) return;
    
    // Create or recreate element selector
    if (!elementSelector) {
        elementSelector = new UIElementSelector();
    }
    
    // Position in upper-right corner
    // At 1920x1080: x = 1920 - 288 - 20 = 1612, y = 20
    float selectorX = currentWindowWidth - 288.0f - 20.0f;
    float selectorY = 20.0f;
    
    elementSelector->initialize(selectorX, selectorY, *fontPtr);
    
    // Set DevMode visibility (use isAdminMode)
    extern bool isAdminMode;
    elementSelector->setDevMode(isAdminMode);
}
