#pragma once

#include "UIButton.h"
#include "UIBorder.h"
#include "MainMenu.h"
#include "../save/SaveManager.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class Renderer;

struct SaveEntry {
    sf::RectangleShape background;
    sf::Texture* thumbnail = nullptr;
    sf::Text* nameText = nullptr;
    sf::Text* dateText = nullptr;
    sf::Text* detailsText = nullptr;
    SaveMetadata metadata;
    bool isSelected = false;
    bool isHovered = false;
    
    SaveEntry() = default;
    ~SaveEntry() {
        delete thumbnail;
        delete nameText;
        delete dateText;
        delete detailsText;
    }
    
    // Move constructor
    SaveEntry(SaveEntry&& other) noexcept
        : thumbnail(other.thumbnail),
          nameText(other.nameText),
          dateText(other.dateText),
          detailsText(other.detailsText),
          metadata(std::move(other.metadata)),
          isSelected(other.isSelected),
          isHovered(other.isHovered) {
        other.thumbnail = nullptr;
        other.nameText = nullptr;
        other.dateText = nullptr;
        other.detailsText = nullptr;
    }
    
    // Move assignment
    SaveEntry& operator=(SaveEntry&& other) noexcept {
        if (this != &other) {
            delete thumbnail;
            delete nameText;
            delete dateText;
            delete detailsText;
            
            thumbnail = other.thumbnail;
            nameText = other.nameText;
            dateText = other.dateText;
            detailsText = other.detailsText;
            metadata = std::move(other.metadata);
            isSelected = other.isSelected;
            isHovered = other.isHovered;
            
            other.thumbnail = nullptr;
            other.nameText = nullptr;
            other.dateText = nullptr;
            other.detailsText = nullptr;
        }
        return *this;
    }
    
    // Delete copy operations
    SaveEntry(const SaveEntry&) = delete;
    SaveEntry& operator=(const SaveEntry&) = delete;
};

class LoadGameMenu {
public:
    LoadGameMenu();
    ~LoadGameMenu();
    
    void initialize(sf::RenderWindow& window);
    bool isInitialized() const;
    void setCameFromMainMenu(bool value) { cameFromMainMenu = value; }
    bool getCameFromMainMenu() const { return cameFromMainMenu; }
    
    void render(Renderer& renderer);
    MenuAction handleEvent(const sf::Event& event);
    
    std::string getSelectedSavePath() const;
    
private:
    sf::Font font;
    sf::RenderWindow* window = nullptr;
    
    // Layout borders
    UIBorder mainBorder;           // Full-screen container
    UIBorder titleBorder;          // Top title area
    UIBorder saveListBorder;       // Left save list
    UIBorder detailsBorder;        // Right details panel
    UIBorder buttonBorder;         // Bottom buttons
    
    std::vector<SaveEntry> saveEntries;
    int selectedIndex = -1;
    std::string selectedSavePath;
    bool initialized = false;
    bool fontLoaded = false;
    
    UIButton loadButton;
    UIButton deleteButton;
    UIButton backButton;
    
    MenuAction lastAction = MenuAction::None;
    bool cameFromMainMenu = false;
    
    sf::Vector2u lastWindowSize;
    float windowWidth = 0.0f;
    float windowHeight = 0.0f;
    
    void buildMenu();
    void refreshSaveList();
    void selectSave(int index);
    void renderDetailsPanel(Renderer& renderer);
    std::string formatTimestamp(time_t timestamp) const;
    bool isMouseOverEntry(const SaveEntry& entry, const sf::Vector2f& mousePos) const;
};
