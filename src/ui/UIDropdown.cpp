#include "UIDropdown.h"
#include "rendering/Renderer.h"

UIDropdown::UIDropdown() {
}

UIDropdown::~UIDropdown() {
    delete selectedText;
    for (auto text : optionTexts) {
        delete text;
    }
}

void UIDropdown::initialize(float x, float y, float width, float height, const sf::Font& font) {
    // Set base class properties
    position = sf::Vector2f(x, y);
    size = sf::Vector2f(width, height);
    fontPtr = &font;
    
    itemHeight = height;
    
    // Dropdown box
    dropdownBox.setSize(sf::Vector2f(width, height));
    dropdownBox.setPosition(sf::Vector2f(x, y));
    dropdownBox.setFillColor(boxColor);
    dropdownBox.setOutlineColor(sf::Color(120, 120, 120));
    dropdownBox.setOutlineThickness(2.0f);
    
    // Selected text
    selectedText = new sf::Text(font, "Select...", 16);
    selectedText->setFillColor(sf::Color::White);
    selectedText->setPosition(sf::Vector2f(x + 8.0f, y + height / 2.0f - 8.0f));
    
    // Arrow (▼)
    arrow.setSize(sf::Vector2f(10.0f, 10.0f));
    arrow.setPosition(sf::Vector2f(x + width - 25.0f, y + height / 2.0f - 5.0f));
    arrow.setFillColor(sf::Color(200, 200, 200));
    
    initialized = true;
}

void UIDropdown::render(Renderer& renderer) {
    if (!initialized) return;
    
    // Draw main dropdown box
    renderer.drawRectangle(dropdownBox);
    renderer.drawText(*selectedText);
    renderer.drawRectangle(arrow);
    
    // Draw options if open
    if (isOpen) {
        for (size_t i = 0; i < optionBoxes.size(); ++i) {
            renderer.drawRectangle(optionBoxes[i]);
            if (i < optionTexts.size()) {
                renderer.drawText(*optionTexts[i]);
            }
        }
    }
}

void UIDropdown::handleMousePress(const sf::Vector2f& mousePos) {
    if (!initialized) return;
    
    // Toggle dropdown when clicking on main box
    if (dropdownBox.getGlobalBounds().contains(mousePos)) {
        isOpen = !isOpen;
        if (isOpen) {
            buildOptionList();
        }
        return;
    }
    
    // Select option if open
    if (isOpen) {
        for (size_t i = 0; i < optionBoxes.size(); ++i) {
            if (optionBoxes[i].getGlobalBounds().contains(mousePos)) {
                selectedIndex = static_cast<int>(i);
                isOpen = false;
                hoveredIndex = -1;
                updateDisplay();
                
                if (onSelect && selectedIndex >= 0) {
                    onSelect(selectedIndex, options[selectedIndex]);
                }
                return;
            }
        }
        
        // Click outside - close dropdown
        isOpen = false;
        hoveredIndex = -1;
    }
}

void UIDropdown::handleMouseMove(const sf::Vector2f& mousePos) {
    if (!initialized || !isOpen) return;
    
    // Update hovered option
    int newHovered = -1;
    for (size_t i = 0; i < optionBoxes.size(); ++i) {
        if (optionBoxes[i].getGlobalBounds().contains(mousePos)) {
            newHovered = static_cast<int>(i);
            break;
        }
    }
    
    if (newHovered != hoveredIndex) {
        hoveredIndex = newHovered;
        
        // Update colors
        for (size_t i = 0; i < optionBoxes.size(); ++i) {
            if (static_cast<int>(i) == hoveredIndex) {
                optionBoxes[i].setFillColor(hoverColor);
            } else if (static_cast<int>(i) == selectedIndex) {
                optionBoxes[i].setFillColor(selectedColor);
            } else {
                optionBoxes[i].setFillColor(boxColor);
            }
        }
    }
}

void UIDropdown::addOption(const std::string& option) {
    options.push_back(option);
    
    // Auto-select first option if none selected
    if (selectedIndex < 0 && options.size() == 1) {
        selectedIndex = 0;
        updateDisplay();
    }
}

void UIDropdown::clearOptions() {
    options.clear();
    selectedIndex = -1;
    
    for (auto text : optionTexts) {
        delete text;
    }
    optionTexts.clear();
    optionBoxes.clear();
    
    updateDisplay();
}

int UIDropdown::getSelectedIndex() const {
    return selectedIndex;
}

std::string UIDropdown::getSelectedValue() const {
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size())) {
        return options[selectedIndex];
    }
    return "";
}

void UIDropdown::setSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(options.size())) {
        selectedIndex = index;
        updateDisplay();
    }
}

void UIDropdown::setCallback(std::function<void(int, const std::string&)> callback) {
    onSelect = callback;
}

void UIDropdown::updateDisplay() {
    if (!initialized || !selectedText) return;
    
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size())) {
        selectedText->setString(options[selectedIndex]);
    } else {
        selectedText->setString("Select...");
    }
}

void UIDropdown::buildOptionList() {
    if (!initialized || options.empty()) return;
    
    // Clear old options
    for (auto text : optionTexts) {
        delete text;
    }
    optionTexts.clear();
    optionBoxes.clear();
    
    float boxX = dropdownBox.getPosition().x;
    float boxY = dropdownBox.getPosition().y + dropdownBox.getSize().y;
    float boxWidth = dropdownBox.getSize().x;
    
    for (size_t i = 0; i < options.size(); ++i) {
        // Create option box
        sf::RectangleShape optionBox(sf::Vector2f(boxWidth, itemHeight));
        optionBox.setPosition(sf::Vector2f(boxX, boxY + i * itemHeight));
        
        // Set color based on selection
        if (static_cast<int>(i) == selectedIndex) {
            optionBox.setFillColor(selectedColor);
        } else {
            optionBox.setFillColor(boxColor);
        }
        optionBox.setOutlineColor(sf::Color(80, 80, 80));
        optionBox.setOutlineThickness(1.0f);
        optionBoxes.push_back(optionBox);
        
        // Create option text
        sf::Text* text = new sf::Text(*fontPtr, options[i], 16);
        text->setFillColor(sf::Color::White);
        text->setPosition(sf::Vector2f(boxX + 8.0f, boxY + i * itemHeight + itemHeight / 2.0f - 8.0f));
        optionTexts.push_back(text);
    }
}
