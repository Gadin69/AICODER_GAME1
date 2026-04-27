#include "UIElementSelector.h"
#include "../../rendering/Renderer.h"
#include <algorithm>
#include <cmath>
#include <iostream>

UIElementSelector::UIElementSelector() {
}

UIElementSelector::~UIElementSelector() {
    delete leftArrow;
    delete rightArrow;
    delete elementNameText;
    delete temperatureSlider;
    delete temperatureInput;
    delete tempLabel;
    delete tempDisplay;
    delete massSlider;
    delete massInput;
    delete massLabel;
    delete massDisplay;
}

void UIElementSelector::initialize(float x, float y, const sf::Font& font) {
    position = sf::Vector2f(x, y);
    size = sf::Vector2f(288.0f, 192.0f);  // 3" x 2" at 96 DPI
    fontPtr = &font;
    
    // Populate valid elements
    populateValidElements();
    
    // Background border (handled by UIBorder container)
    
    // Element preview box (center top)
    float previewSize = 40.0f;
    float previewX = x + (size.x - previewSize) / 2.0f;
    float previewY = y + 10.0f;
    elementPreview.setSize(sf::Vector2f(previewSize, previewSize));
    elementPreview.setPosition(sf::Vector2f(previewX, previewY));
    elementPreview.setOutlineThickness(2.0f);
    elementPreview.setOutlineColor(sf::Color(255, 255, 255));
    
    // Left arrow button
    leftArrow = new UIButton();
    leftArrow->initialize(previewX - 45.0f, previewY + 5.0f, 35.0f, 30.0f, "<", font);
    leftArrow->setCallback([this]() {
        currentElementIndex = (currentElementIndex - 1 + validElements.size()) % validElements.size();
        currentElement = validElements[currentElementIndex];
        std::cout << "[ElementSelector] Left arrow - Index: " << currentElementIndex << "/" << validElements.size() 
                  << " Element: " << ElementTypes::getTypeName(currentElement) << std::endl;
        onElementChanged();
    });
    
    // Right arrow button
    rightArrow = new UIButton();
    rightArrow->initialize(previewX + previewSize + 10.0f, previewY + 5.0f, 35.0f, 30.0f, ">", font);
    rightArrow->setCallback([this]() {
        currentElementIndex = (currentElementIndex + 1) % validElements.size();
        currentElement = validElements[currentElementIndex];
        std::cout << "[ElementSelector] Right arrow - Index: " << currentElementIndex << "/" << validElements.size() 
                  << " Element: " << ElementTypes::getTypeName(currentElement) << std::endl;
        onElementChanged();
    });
    
    // Element name text
    elementNameText = new sf::Text(font, "", 16);
    elementNameText->setFillColor(sf::Color::White);
    elementNameText->setPosition(sf::Vector2f(x, previewY + previewSize + 5.0f));
    
    // Temperature label
    tempLabel = new sf::Text(font, "Temperature:", 14);
    tempLabel->setFillColor(sf::Color::White);
    tempLabel->setPosition(sf::Vector2f(x + 10.0f, y + 70.0f));
    
    // Temperature slider (left side)
    temperatureSlider = new UISlider();
    temperatureSlider->initialize(x + 10.0f, y + 90.0f, 120.0f, -273.0f, 5000.0f, 20.0f, "", font);
    temperatureSlider->valueText->setString("");  // Hide default value text
    
    // Temperature number input (right side)
    temperatureInput = new UINumberInput();
    temperatureInput->initialize(x + 140.0f, y + 85.0f, 130.0f, 30.0f, -273.0f, 5000.0f, 20.0f, font);
    
    // Temperature display (C / F)
    tempDisplay = new sf::Text(font, "", 13);
    tempDisplay->setFillColor(sf::Color(255, 255, 100));
    tempDisplay->setPosition(sf::Vector2f(x + 10.0f, y + 115.0f));
    
    // Mass label
    massLabel = new sf::Text(font, "Mass:", 14);
    massLabel->setFillColor(sf::Color::White);
    massLabel->setPosition(sf::Vector2f(x + 10.0f, y + 125.0f));
    
    // Mass slider (left side)
    massSlider = new UISlider();
    massSlider->initialize(x + 10.0f, y + 145.0f, 120.0f, 1.0f, 10000.0f, 100.0f, "", font);
    massSlider->valueText->setString("");  // Hide default value text
    
    // Mass number input (right side)
    massInput = new UINumberInput();
    massInput->initialize(x + 140.0f, y + 140.0f, 130.0f, 30.0f, 1.0f, 10000.0f, 100.0f, font);
    
    // Mass display
    massDisplay = new sf::Text(font, "", 13);
    massDisplay->setFillColor(sf::Color(255, 255, 100));
    massDisplay->setPosition(sf::Vector2f(x + 10.0f, y + 170.0f));
    
    // Mark as initialized
    initialized = true;
    
    // Update initial state
    onElementChanged();
    updateTempDisplay();
    updateMassDisplay();
}

void UIElementSelector::populateValidElements() {
    // Clear existing elements first (in case this is called multiple times)
    validElements.clear();
    
    // Dynamically iterate through ALL ElementType enum values
    // This automatically includes any new elements added to the enum
    int enumCount = static_cast<int>(ElementType::Gas_Nitrogen) + 1;  // Last element + 1
    std::cout << "[ElementSelector] Enum count: " << enumCount << std::endl;
    
    for (int i = 0; i < enumCount; i++) {
        ElementType type = static_cast<ElementType>(i);
        
        // Skip Empty element (not placeable)
        if (type == ElementType::Empty) continue;
        
        validElements.push_back(type);
    }
    
    std::cout << "[ElementSelector] Found " << validElements.size() << " elements" << std::endl;
    
    // Print ALL elements in the list
    std::cout << "[ElementSelector] Complete element list:" << std::endl;
    for (size_t i = 0; i < validElements.size(); i++) {
        std::cout << "  [" << i << "] " << ElementTypes::getTypeName(validElements[i]) << std::endl;
    }
    
    // Start with Water
    currentElement = ElementType::Liquid_Water;
    // Find Water's index in the validElements list
    for (size_t i = 0; i < validElements.size(); i++) {
        if (validElements[i] == ElementType::Liquid_Water) {
            currentElementIndex = i;
            break;
        }
    }
}

void UIElementSelector::updateElementPreview() {
    const Element& props = ElementTypes::getElement(currentElement);
    
    // Update preview box color
    Cell tempCell;
    tempCell.elementType = currentElement;
    tempCell.mass = props.density;
    tempCell.temperature = temperatureSlider->currentValue;
    tempCell.updateColor();
    
    elementPreview.setFillColor(tempCell.color);
    
    // Update element name
    elementNameText->setString(props.name);
    
    // Center element name
    float textWidth = elementNameText->getLocalBounds().size.x;
    elementNameText->setPosition(sf::Vector2f(
        position.x + (size.x - textWidth) / 2.0f,
        elementPreview.getPosition().y + elementPreview.getSize().y + 5.0f
    ));
}

void UIElementSelector::onElementChanged() {
    updateElementPreview();
    
    // Set temperature to midpoint between phase change points
    // This keeps the element in a stable phase when selected
    const Element& props = ElementTypes::getElement(currentElement);
    
    // Calculate midpoint between freezing and boiling points
    float phaseTempLow = props.freezingPoint;
    float phaseTempHigh = props.boilingPoint;
    
    // For elements with valid phase change points, set temp to midpoint
    if (phaseTempLow != 0.0f || phaseTempHigh != 0.0f) {
        float targetTemp = (phaseTempLow + phaseTempHigh) / 2.0f;
        
        // Clamp to slider range
        targetTemp = std::max(-273.0f, std::min(5000.0f, targetTemp));
        
        // Update slider and input
        isUpdatingFromSlider = true;
        temperatureSlider->currentValue = targetTemp;
        temperatureSlider->updateThumbPosition();
        temperatureInput->setValue(targetTemp);
        isUpdatingFromSlider = false;
        updateTempDisplay();
    }
}

void UIElementSelector::onTemperatureChanged(float newTemp) {
    updateTempDisplay();
    
    // DISABLED: Auto phase change when browsing elements
    // This was causing the element index to jump around when temperature triggered phase changes
    // Phase changes should only happen when placing cells, not when selecting elements
    /*
    const Element& props = ElementTypes::getElement(currentElement);
    ElementType newPhase = props.getPhaseAtTemperature(newTemp);
    
    if (newPhase != ElementType::Empty && newPhase != currentElement) {
        currentElement = newPhase;
        
        // Find new index
        for (size_t i = 0; i < validElements.size(); i++) {
            if (validElements[i] == currentElement) {
                currentElementIndex = static_cast<int>(i);
                break;
            }
        }
        
        updateElementPreview();
    }
    */
}

void UIElementSelector::onMassChanged(float newMass) {
    updateMassDisplay();
}

void UIElementSelector::updateTempDisplay() {
    float tempC = temperatureSlider->currentValue;
    float tempF = (tempC * 9.0f / 5.0f) + 32.0f;
    
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "%.0f C / %.0f F", tempC, tempF);
    tempDisplay->setString(buffer);
}

void UIElementSelector::updateMassDisplay() {
    float mass = massSlider->currentValue;
    
    char buffer[100];
    if (mass >= 1000.0f) {
        snprintf(buffer, sizeof(buffer), "%.0f kg", mass);
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f kg", mass);
    }
    massDisplay->setString(buffer);
}

void UIElementSelector::updateComponentPositions() {
    // Update all component positions relative to this element's position
    if (!initialized) return;
    
    float x = position.x;
    float y = position.y;
    
    // Element preview
    float previewSize = 40.0f;
    float previewX = x + (size.x - previewSize) / 2.0f;
    float previewY = y + 10.0f;
    elementPreview.setPosition(sf::Vector2f(previewX, previewY));
    
    // Arrows
    if (leftArrow) {
        leftArrow->setPosition(previewX - 45.0f, previewY + 5.0f);
    }
    if (rightArrow) {
        rightArrow->setPosition(previewX + previewSize + 10.0f, previewY + 5.0f);
    }
    
    // Element name
    if (elementNameText) {
        float textWidth = elementNameText->getLocalBounds().size.x;
        elementNameText->setPosition(sf::Vector2f(
            x + (size.x - textWidth) / 2.0f,
            previewY + previewSize + 5.0f
        ));
    }
    
    // Temperature controls
    if (tempLabel) tempLabel->setPosition(sf::Vector2f(x + 10.0f, y + 70.0f));
    if (temperatureSlider) {
        temperatureSlider->setPosition(x + 10.0f, y + 90.0f);
        temperatureSlider->setSize(120.0f, 30.0f);
    }
    if (temperatureInput) temperatureInput->setPosition(x + 140.0f, y + 85.0f);
    if (tempDisplay) tempDisplay->setPosition(sf::Vector2f(x + 10.0f, y + 115.0f));
    
    // Mass controls
    if (massLabel) massLabel->setPosition(sf::Vector2f(x + 10.0f, y + 125.0f));
    if (massSlider) {
        massSlider->setPosition(x + 10.0f, y + 145.0f);
        massSlider->setSize(120.0f, 30.0f);
    }
    if (massInput) massInput->setPosition(x + 140.0f, y + 140.0f);
    if (massDisplay) massDisplay->setPosition(sf::Vector2f(x + 10.0f, y + 170.0f));
}

void UIElementSelector::render(Renderer& renderer) {
    if (!initialized || !isVisible) return;
    
    // Poll slider values and sync with number inputs
    float currentTemp = temperatureSlider->currentValue;
    float currentMass = massSlider->currentValue;
    
    // Check if slider changed (compare with last known values)
    static float lastTemp = currentTemp;
    static float lastMass = currentMass;
    
    if (std::abs(currentTemp - lastTemp) > 0.1f) {
        temperatureInput->setValue(currentTemp);
        onTemperatureChanged(currentTemp);
        lastTemp = currentTemp;
    }
    
    if (std::abs(currentMass - lastMass) > 0.1f) {
        massInput->setValue(currentMass);
        onMassChanged(currentMass);
        lastMass = currentMass;
    }
    
    // Draw element preview box
    renderer.drawRectangle(elementPreview);
    
    // Draw text
    if (elementNameText) renderer.drawText(*elementNameText);
    if (tempLabel) renderer.drawText(*tempLabel);
    if (tempDisplay) renderer.drawText(*tempDisplay);
    if (massLabel) renderer.drawText(*massLabel);
    if (massDisplay) renderer.drawText(*massDisplay);
    
    // Render child components
    if (leftArrow) leftArrow->render(renderer);
    if (rightArrow) rightArrow->render(renderer);
    if (temperatureSlider) temperatureSlider->render(renderer);
    if (temperatureInput) temperatureInput->render(renderer);
    if (massSlider) massSlider->render(renderer);
    if (massInput) massInput->render(renderer);
}

void UIElementSelector::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized || !isVisible) return;
    
    if (leftArrow) leftArrow->handleMousePress(mousePos);
    if (rightArrow) rightArrow->handleMousePress(mousePos);
    if (temperatureSlider) temperatureSlider->handleMousePress(mousePos);
    if (temperatureInput) temperatureInput->handleMousePress(mousePos);
    if (massSlider) massSlider->handleMousePress(mousePos);
    if (massInput) massInput->handleMousePress(mousePos);
}

void UIElementSelector::handleMouseRelease() {
    if (!initialized || !isVisible) return;
    
    if (leftArrow) leftArrow->handleMouseRelease();
    if (rightArrow) rightArrow->handleMouseRelease();
    if (temperatureSlider) temperatureSlider->handleMouseRelease();
    if (temperatureInput) temperatureInput->handleMouseRelease();
    if (massSlider) massSlider->handleMouseRelease();
    if (massInput) massInput->handleMouseRelease();
}

void UIElementSelector::handleMouseMove(const sf::Vector2f& mousePos) {
    if (!initialized || !isVisible) return;
    
    if (leftArrow) leftArrow->handleMouseMove(mousePos);
    if (rightArrow) rightArrow->handleMouseMove(mousePos);
    if (temperatureSlider) temperatureSlider->handleMouseMove(mousePos);
    if (temperatureInput) temperatureInput->handleMouseMove(mousePos);
    if (massSlider) massSlider->handleMouseMove(mousePos);
    if (massInput) massInput->handleMouseMove(mousePos);
}

void UIElementSelector::handleKeyPress(const sf::Event::KeyPressed& event) {
    if (!initialized || !isVisible) return;
    
    if (temperatureInput) temperatureInput->handleKeyPress(event);
    if (massInput) massInput->handleKeyPress(event);
}

void UIElementSelector::handleTextEntered(const sf::Event::TextEntered& event) {
    if (!initialized || !isVisible) return;
    
    if (temperatureInput) temperatureInput->handleTextEntered(event);
    if (massInput) massInput->handleTextEntered(event);
}

void UIElementSelector::setPosition(float x, float y) {
    position = sf::Vector2f(x, y);
    updateComponentPositions();
}

void UIElementSelector::setSize(float width, float height) {
    size = sf::Vector2f(width, height);
    updateComponentPositions();
}
