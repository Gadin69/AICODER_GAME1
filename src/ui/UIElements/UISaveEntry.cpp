#include "UISaveEntry.h"
#include "../../rendering/Renderer.h"
#include <sstream>
#include <iomanip>
#include <iostream>

UISaveEntry::UISaveEntry() {}

UISaveEntry::~UISaveEntry() {
    delete nameText;
    delete dateTimeText;
    delete detailsText;
}

UISaveEntry::UISaveEntry(UISaveEntry&& other) noexcept
    : UIBorder(std::move(other)),
      savePath(std::move(other.savePath)),
      metadata(std::move(other.metadata)),
      selected(other.selected),
      font(other.font),
      nameText(other.nameText),
      dateTimeText(other.dateTimeText),
      detailsText(other.detailsText) {
    other.nameText = nullptr;
    other.dateTimeText = nullptr;
    other.detailsText = nullptr;
}

UISaveEntry& UISaveEntry::operator=(UISaveEntry&& other) noexcept {
    if (this != &other) {
        delete nameText;
        delete dateTimeText;
        delete detailsText;
        
        UIBorder::operator=(std::move(other));
        savePath = std::move(other.savePath);
        metadata = std::move(other.metadata);
        selected = other.selected;
        font = other.font;
        nameText = other.nameText;
        dateTimeText = other.dateTimeText;
        detailsText = other.detailsText;
        
        other.nameText = nullptr;
        other.dateTimeText = nullptr;
        other.detailsText = nullptr;
    }
    return *this;
}

void UISaveEntry::initialize(const sf::Font& font, const SaveMetadata& metadata, float width) {
    this->font = &font;
    this->metadata = metadata;
    this->savePath = metadata.filePath;
    
    // Initialize border
    UIBorder::initialize(0, 0, width, 70.0f);  // 70px height per entry
    setBackgroundColor(sf::Color(40, 40, 45));
    setBorderColor(sf::Color(70, 70, 80));
    setBorderThickness(2.0f);
    
    // Create text elements
    nameText = new sf::Text(font, metadata.saveName, 18);
    nameText->setFillColor(sf::Color::White);
    nameText->setPosition(sf::Vector2f(15.0f, 15.0f));  // 15px padding from left
    
    // Setup date+time text (right-justified)
    std::tm tm;
    localtime_s(&tm, &metadata.timestamp);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm);
    
    dateTimeText = new sf::Text(font, buffer, 14);
    dateTimeText->setFillColor(sf::Color(160, 160, 170));
    // Will be positioned in setSize or when width is known
    
    // Setup details text (below, left-aligned)
    std::string details = std::to_string(metadata.gridWidth) + "x" + 
                         std::to_string(metadata.gridHeight) + " | " +
                         std::to_string(metadata.cellCount) + " cells";
    
    detailsText = new sf::Text(font, details, 13);
    detailsText->setFillColor(sf::Color(130, 130, 140));
    detailsText->setPosition(sf::Vector2f(15.0f, 42.0f));  // Below name
    
    updateColors();
    updateDateTimePosition(width);  // Position date text based on width
}

void UISaveEntry::render(Renderer& renderer) {
    if (!initialized) return;
    
    // Get the border's screen position
    sf::Vector2f entryPos = getPosition();
    
    // Draw border background (already at correct screen position)
    renderer.drawRectangle(getBorder());
    
    // Temporarily offset text positions to screen coordinates
    sf::Vector2f originalNamePos = nameText->getPosition();
    sf::Vector2f originalDateTimePos = dateTimeText->getPosition();
    sf::Vector2f originalDetailsPos = detailsText->getPosition();
    
    nameText->setPosition(sf::Vector2f(entryPos.x + originalNamePos.x, entryPos.y + originalNamePos.y));
    dateTimeText->setPosition(sf::Vector2f(entryPos.x + originalDateTimePos.x, entryPos.y + originalDateTimePos.y));
    detailsText->setPosition(sf::Vector2f(entryPos.x + originalDetailsPos.x, entryPos.y + originalDetailsPos.y));
    
    // Draw text elements at screen coordinates
    if (nameText) renderer.drawText(*nameText);
    if (dateTimeText) renderer.drawText(*dateTimeText);
    if (detailsText) renderer.drawText(*detailsText);
    
    // Restore local coordinates
    nameText->setPosition(originalNamePos);
    dateTimeText->setPosition(originalDateTimePos);
    detailsText->setPosition(originalDetailsPos);
}

void UISaveEntry::setSelected(bool selected) {
    this->selected = selected;
    updateColors();
}

void UISaveEntry::updateColors() {
    if (selected) {
        setBackgroundColor(sf::Color(60, 60, 100));
        setBorderColor(sf::Color(100, 100, 200));
    } else {
        setBackgroundColor(sf::Color(40, 40, 45));
        setBorderColor(sf::Color(70, 70, 80));
    }
}

void UISaveEntry::updateDateTimePosition(float entryWidth) {
    if (!dateTimeText) return;
    
    // Right-justify: position at right edge minus 15px padding minus text width
    sf::FloatRect bounds = dateTimeText->getLocalBounds();
    float textWidth = bounds.size.x;
    float xPos = entryWidth - 15.0f - textWidth;
    
    // Ensure minimum spacing from name (at least 20px)
    float minSpacing = 20.0f;
    float nameEndPos = 15.0f;
    if (nameText) {
        nameEndPos += nameText->getLocalBounds().size.x;
    }
    
    if (xPos < nameEndPos + minSpacing) {
        xPos = nameEndPos + minSpacing;
    }
    
    dateTimeText->setPosition(sf::Vector2f(xPos, 17.0f));
}

void UISaveEntry::updateTextPositions() {
    // Could add dynamic positioning based on window size if needed
}
