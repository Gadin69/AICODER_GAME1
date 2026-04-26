#pragma once

#include "UIBorder.h"
#include "../../save/SaveManager.h"
#include <SFML/Graphics.hpp>
#include <string>

class UISaveEntry : public UIBorder {
public:
    UISaveEntry();
    ~UISaveEntry() override;
    
    // Delete copy operations (we have pointers)
    UISaveEntry(const UISaveEntry&) = delete;
    UISaveEntry& operator=(const UISaveEntry&) = delete;
    
    // Move operations
    UISaveEntry(UISaveEntry&& other) noexcept;
    UISaveEntry& operator=(UISaveEntry&& other) noexcept;
    
    // Initialize with save metadata
    void initialize(const sf::Font& font, const SaveMetadata& metadata, float width);
    
    // Override render to draw text elements
    void render(Renderer& renderer) override;
    
    // Getters
    std::string getSavePath() const { return savePath; }
    const SaveMetadata& getMetadata() const { return metadata; }
    
    // Visual feedback
    void setSelected(bool selected);
    bool getIsSelected() const { return selected; }

private:
    std::string savePath;
    bool selected = false;
    
    // Text elements (using pointers for SFML 3 compatibility)
    const sf::Font* font = nullptr;
    sf::Text* nameText = nullptr;
    sf::Text* dateTimeText = nullptr;
    sf::Text* detailsText = nullptr;
    
    void updateTextPositions();
    void updateColors();
    void updateDateTimePosition(float entryWidth);
    
public:
    SaveMetadata metadata;  // Public for access by LoadGameMenu
};
