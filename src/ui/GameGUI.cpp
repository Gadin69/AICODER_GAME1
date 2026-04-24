#include "GameGUI.h"
#include "../rendering/Renderer.h"

GameGUI::GameGUI() {}

GameGUI::~GameGUI() {
    if (simSpeedSlider) {
        delete simSpeedSlider;
        simSpeedSlider = nullptr;
    }
}

void GameGUI::initialize(const sf::Font& font) {
    fontPtr = &font;
    
    // Build the skillbar layout
    buildSkillbarLayout();
    
    initialized = true;
}

void GameGUI::handleResize(int windowWidth, int windowHeight) {
    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;
    
    // Rebuild layout with new dimensions
    buildSkillbarLayout();
}

void GameGUI::render(Renderer& renderer) {
    if (!initialized) return;
    
    // Render skillbar border and all its children
    skillbarBorder.render(renderer);
}

void GameGUI::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized || !simSpeedSlider) return;
    
    // Route mouse events to slider
    simSpeedSlider->handleMousePress(mousePos);
}

void GameGUI::handleMouseRelease() {
    if (!initialized || !simSpeedSlider) return;
    
    simSpeedSlider->handleMouseRelease();
}

void GameGUI::handleMouseMove(const sf::Vector2f& mousePos) {
    if (!initialized || !simSpeedSlider) return;
    
    simSpeedSlider->handleMouseMove(mousePos);
}

bool GameGUI::isMouseOverUI(const sf::Vector2f& mousePos) const {
    if (!initialized) return false;
    
    // Check if mouse is over skillbar border
    return skillbarBorder.containsPoint(mousePos);
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
