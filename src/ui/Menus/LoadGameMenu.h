#pragma once

#include "../UIElements/UIButton.h"
#include "../UIElements/UIBorder.h"
#include "../UIElements/UIScrollBorder.h"
#include "../UIElements/UISaveEntry.h"
#include "../UIElements/UIThumbnailViewer.h"
#include "MainMenu.h"
#include "../../save/SaveManager.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class Renderer;

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
    UIScrollBorder saveListBorder; // Left save list (scrollable)
    UIBorder detailsBorder;        // Right details panel
    UIBorder detailsInfoBorder;    // Info text area within details
    UIBorder buttonBorder;         // Bottom buttons
    
    std::vector<UISaveEntry> saveEntries;
    int selectedIndex = -1;
    int lastSelectedIndex = -1;  // Track previous selection to avoid reloading thumbnail
    std::string selectedSavePath;
    bool initialized = false;
    bool fontLoaded = false;
    
    UIButton loadButton;
    UIButton deleteButton;
    UIButton backButton;
    
    UIThumbnailViewer thumbnailViewer;  // Thumbnail display
    
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
    bool isMouseOverContainer(const UISaveEntry& entry, const sf::Vector2f& mousePos) const;
};
