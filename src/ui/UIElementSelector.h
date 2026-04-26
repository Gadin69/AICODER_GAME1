#pragma once

#include "UIElement.h"
#include "UISlider.h"
#include "UINumberInput.h"
#include "UIButton.h"
#include "../simulation/Cell.h"
#include "../simulation/ElementTypes.h"
#include <SFML/Graphics.hpp>
#include <vector>

class Renderer;  // Forward declaration

class UIElementSelector : public UIElement {
public:
    UIElementSelector();
    ~UIElementSelector() override;
    
    void initialize(float x, float y, const sf::Font& font);
    void render(Renderer& renderer) override;
    
    void handleMousePress(const sf::Vector2f& mousePos) override;
    void handleMouseRelease() override;
    void handleMouseMove(const sf::Vector2f& mousePos) override;
    void handleKeyPress(const sf::Event::KeyPressed& event) override;
    void handleTextEntered(const sf::Event::TextEntered& event) override;
    
    // Override to update internal component positions when moved
    void setPosition(float x, float y) override;
    void setSize(float width, float height) override;
    
    // Getters for selected values
    ElementType getSelectedElement() const { return currentElement; }
    float getSelectedTemperature() const { return temperatureSlider->currentValue; }
    float getSelectedMass() const { return massSlider->currentValue; }
    
    // DevMode visibility
    void setDevMode(bool enabled) { isVisible = enabled; }
    
private:
    // Valid elements list (excludes Empty)
    std::vector<ElementType> validElements;
    int currentElementIndex = 0;
    ElementType currentElement = ElementType::Vacuum;
    
    // UI Components
    UIButton* leftArrow = nullptr;
    UIButton* rightArrow = nullptr;
    sf::RectangleShape elementPreview;
    sf::Text* elementNameText = nullptr;
    
    UISlider* temperatureSlider = nullptr;
    UINumberInput* temperatureInput = nullptr;
    sf::Text* tempLabel = nullptr;
    sf::Text* tempDisplay = nullptr;
    
    UISlider* massSlider = nullptr;
    UINumberInput* massInput = nullptr;
    sf::Text* massLabel = nullptr;
    sf::Text* massDisplay = nullptr;
    
    // Internal state
    bool isUpdatingFromSlider = false;  // Prevent infinite loops
    bool isVisible = false;  // DevMode visibility
    
    // Helper methods
    void populateValidElements();
    void updateElementPreview();
    void onElementChanged();
    void onTemperatureChanged(float newTemp);
    void onMassChanged(float newMass);
    void updateTempDisplay();
    void updateMassDisplay();
    void updateComponentPositions();
};
