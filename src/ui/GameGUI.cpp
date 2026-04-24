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
    
    // Add slider components to border with relative positioning
    // Track: 10% from left, 25% from top, 60% width, 30% height
    skillbarBorder.addChild(&simSpeedSlider->track, 0.10f, 0.25f, 0.60f, 0.30f);
    
    // Label: 10% from left, 5% from top (above track)
    skillbarBorder.addChild(simSpeedSlider->label, 0.10f, 0.05f);
    
    // Value text: 72% from left, 25% from top (right of track)
    skillbarBorder.addChild(simSpeedSlider->valueText, 0.72f, 0.25f);
    
    // Thumb: positioned dynamically by slider, but we add it here for rendering
    // The slider's updateThumbPosition() will override the position
    skillbarBorder.addChild(&simSpeedSlider->thumb, 0.10f, 0.20f, 0.05f, 0.40f);
    
    // Update thumb position to match slider value
    simSpeedSlider->updateThumbPosition();
}
