#include "LoadGameMenu.h"
#include "../../rendering/Renderer.h"
#include <iostream>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>

LoadGameMenu::LoadGameMenu() {
}

LoadGameMenu::~LoadGameMenu() {
}

void LoadGameMenu::initialize(sf::RenderWindow& window) {
    this->window = &window;
    
    // Load font only once
    if (!fontLoaded) {
        if (!font.openFromFile("assets/fonts/arial.ttf")) {
            std::cerr << "ERROR: Failed to load font for LoadGameMenu" << std::endl;
            initialized = false;
            return;
        }
        fontLoaded = true;
        std::cout << "[LoadGameMenu] Font loaded successfully" << std::endl;
    }
    
    initialized = true;
    std::cout << "[LoadGameMenu] Initializing load menu..." << std::endl;
    buildMenu();
    std::cout << "[LoadGameMenu] Load menu initialized" << std::endl;
}

bool LoadGameMenu::isInitialized() const {
    return initialized;
}

void LoadGameMenu::buildMenu() {
    if (!initialized) {
        std::cerr << "[LoadGameMenu] buildMenu called but not initialized" << std::endl;
        return;
    }
    
    if (!window) {
        std::cerr << "[LoadGameMenu] ERROR: window is null in buildMenu" << std::endl;
        initialized = false;
        return;
    }
    
    try {
        std::cout << "[LoadGameMenu] Building menu..." << std::endl;
        
        sf::Vector2u windowSize = window->getSize();
        lastWindowSize = windowSize;
        windowWidth = static_cast<float>(windowSize.x);
        windowHeight = static_cast<float>(windowSize.y);
        
        // CRITICAL: Clear previous children before rebuilding
        mainBorder.clearChildren();
        titleBorder.clearChildren();
        saveListBorder.clearChildren();
        detailsBorder.clearChildren();
        buttonBorder.clearChildren();
        
        // Main border (full screen) - OPAQUE background to hide game UI
        mainBorder.initialize(0, 0, windowWidth, windowHeight);
        mainBorder.setBackgroundColor(sf::Color(30, 30, 40, 255));  // Fully opaque
        
        // Title border (top 10%)
        titleBorder.initialize(0, 0, windowWidth, windowHeight * 0.10f);
        
        // Save list border (left 60%, middle area) - NOW UIScrollBorder
        saveListBorder.initialize(
            windowWidth * 0.05f,
            windowHeight * 0.12f,
            windowWidth * 0.55f,
            windowHeight * 0.73f,
            ScrollbarSide::Right,
            12.0f
        );
        saveListBorder.setBackgroundColor(sf::Color(35, 35, 45));
        
        // Details border (right 35%, middle area)
        detailsBorder.initialize(
            windowWidth * 0.62f,
            windowHeight * 0.12f,
            windowWidth * 0.33f,
            windowHeight * 0.73f
        );
        
        // Button border (bottom 12%)
        buttonBorder.initialize(
            0,
            windowHeight * 0.88f,
            windowWidth,
            windowHeight * 0.12f
        );
        
        // CRITICAL: Add all child borders to mainBorder so they get rendered
        mainBorder.addChild(&titleBorder, 0, 0, 1.0f, 0.10f);
        mainBorder.addChild(&saveListBorder, 0.05f, 0.12f, 0.55f, 0.73f);
        mainBorder.addChild(&detailsBorder, 0.62f, 0.12f, 0.33f, 0.73f);
        mainBorder.addChild(&buttonBorder, 0, 0.88f, 1.0f, 0.12f);
        
        // Add buttons to buttonBorder with relative positioning
        float buttonRelWidth = 0.18f;   // 18% of buttonBorder width
        float buttonRelHeight = 0.70f;  // 70% of buttonBorder height (larger clickable area)
        float buttonRelY = 0.15f;       // Centered vertically with more padding
        
        loadButton.initialize(0, 0, 100, 40, "Load", font);
        loadButton.setCallback([this]() {
            lastAction = MenuAction::Play;  // Reuse Play as Load action
        });
        buttonBorder.addChild(&loadButton, 0.05f, buttonRelY, buttonRelWidth, buttonRelHeight);
        
        deleteButton.initialize(0, 0, 100, 40, "Delete", font);
        deleteButton.setCallback([this]() {
            if (selectedIndex >= 0 && selectedIndex < static_cast<int>(saveEntries.size())) {
                SaveManager::getInstance().deleteSave(saveEntries[selectedIndex].metadata.filePath);
                refreshSaveList();
                selectedIndex = -1;
                selectedSavePath = "";
            }
        });
        buttonBorder.addChild(&deleteButton, 0.25f, buttonRelY, buttonRelWidth, buttonRelHeight);
        
        backButton.initialize(0, 0, 100, 40, "Back", font);
        backButton.setCallback([this]() {
            std::cout << "[LoadGameMenu] Back button CALLBACK triggered!" << std::endl;
            lastAction = MenuAction::Back;
        });
        buttonBorder.addChild(&backButton, 0.77f, buttonRelY, buttonRelWidth, buttonRelHeight);
        
        std::cout << "[LoadGameMenu] buttonBorder has " << buttonBorder.getChildCount() << " children" << std::endl;
        
        refreshSaveList();
    } catch (const std::exception& e) {
        std::cerr << "[LoadGameMenu] ERROR building menu: " << e.what() << std::endl;
        initialized = false;
    }
}

void LoadGameMenu::refreshSaveList() {
    // Clear existing entries
    saveEntries.clear();
    saveListBorder.clearChildren();  // Clear UIScrollBorder children
    selectedIndex = -1;
    selectedSavePath = "";
    
    std::cout << "[LoadGameMenu] Refreshing save list..." << std::endl;
    
    try {
        auto saves = SaveManager::getInstance().getSaveList();
        std::cout << "[LoadGameMenu] Found " << saves.size() << " save(s)" << std::endl;
        
        if (!window) return;
        
        // Reserve space to prevent vector reallocation (which would invalidate pointers)
        saveEntries.reserve(saves.size());
        
        float listWidth = windowWidth * 0.55f;
        float entryHeight = 70.0f;  // Height per save entry
        float spacing = 10.0f;
        
        for (size_t i = 0; i < saves.size(); ++i) {
            // Create entry directly in vector to avoid move issues
            saveEntries.emplace_back();
            auto& entry = saveEntries.back();
            
            entry.initialize(font, saves[i], listWidth - 24);
            // Add top spacing for first entry (same as spacing between entries)
            entry.setPosition(10, spacing + i * (entryHeight + spacing));
            entry.setSize(listWidth - 24, entryHeight);  // Set size BEFORE adding to scroll border
            
            // Add to scroll border with absolute positioning (no relative coords)
            saveListBorder.addChild(&entry);
        }
        
        std::cout << "[LoadGameMenu] Save list refreshed - Total entries: " << saveEntries.size() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[LoadGameMenu] ERROR refreshing save list: " << e.what() << std::endl;
    }
}

void LoadGameMenu::selectSave(int index) {
    if (index < 0 || index >= static_cast<int>(saveEntries.size())) {
        selectedIndex = -1;
        selectedSavePath = "";
        return;
    }
    
    // Deselect previous
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(saveEntries.size())) {
        saveEntries[selectedIndex].setSelected(false);
    }
    
    // Select new
    selectedIndex = index;
    saveEntries[index].setSelected(true);
    selectedSavePath = saveEntries[index].getSavePath();
}

bool LoadGameMenu::isMouseOverContainer(const UISaveEntry& entry, const sf::Vector2f& mousePos) const {
    sf::Vector2f pos = entry.getPosition();
    sf::Vector2f size = entry.getSize();
    sf::FloatRect bounds(sf::Vector2f(pos.x, pos.y), sf::Vector2f(size.x, size.y));
    return bounds.contains(mousePos);
}

std::string LoadGameMenu::formatTimestamp(time_t timestamp) const {
    if (timestamp == 0) return "Unknown";
    
    std::tm tm;
    localtime_s(&tm, &timestamp);
    
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", &tm);
    return std::string(buffer);
}

std::string LoadGameMenu::getSelectedSavePath() const {
    return selectedSavePath;
}

void LoadGameMenu::render(Renderer& renderer) {
    if (!initialized) {
        std::cerr << "[LoadGameMenu] render called but not initialized" << std::endl;
        return;
    }
    
    if (!window) {
        std::cerr << "[LoadGameMenu] ERROR: window is null in render" << std::endl;
        return;
    }
    
    // Check if window size changed
    sf::Vector2u currentWindowSize = window->getSize();
    if (lastWindowSize.x != currentWindowSize.x || lastWindowSize.y != currentWindowSize.y) {
        buildMenu();
    }
    
    // std::cout << "[LoadGameMenu] Rendering mainBorder..." << std::endl;
    // Render main border (handles all children including saveListBorder and UISaveEntry objects)
    mainBorder.render(renderer);
    
    // Show message if no saves
    if (saveEntries.empty()) {
        sf::Text noSaves(font, "No saves found", 20);
        noSaves.setFillColor(sf::Color(150, 150, 150));
        noSaves.setPosition(sf::Vector2f(windowWidth * 0.25f, windowHeight * 0.4f));
        renderer.drawText(noSaves);
    }
    
    // Render details panel if something selected
    if (selectedIndex >= 0) {
        renderDetailsPanel(renderer);
    }
}

void LoadGameMenu::renderDetailsPanel(Renderer& renderer) {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(saveEntries.size())) return;
    
    const auto& meta = saveEntries[selectedIndex].getMetadata();
    
    sf::Vector2u windowSize = window->getSize();
    float windowWidth = static_cast<float>(windowSize.x);
    float windowHeight = static_cast<float>(windowSize.y);
    
    // Panel background
    float panelWidth = windowWidth * 0.35f;
    float panelHeight = windowHeight * 0.6f;
    float panelX = windowWidth * 0.6f;
    float panelY = windowHeight * 0.1f;
    
    sf::RectangleShape panel(sf::Vector2f(panelWidth, panelHeight));
    panel.setPosition(sf::Vector2f(panelX, panelY));
    panel.setFillColor(sf::Color(25, 25, 25));
    panel.setOutlineColor(sf::Color(80, 80, 80));
    panel.setOutlineThickness(2.0f);
    renderer.drawRectangle(panel);
    
    // Details text
    float x = panelX + 20;
    float y = panelY + 20;
    
    auto drawDetail = [&](const std::string& label, const std::string& value) {
        sf::Text labelText(font, label + ":", 16);
        labelText.setFillColor(sf::Color(150, 150, 150));
        labelText.setPosition(sf::Vector2f(x, y));
        renderer.drawText(labelText);
        
        sf::Text valueText(font, value, 16);
        valueText.setFillColor(sf::Color::White);
        valueText.setPosition(sf::Vector2f(x + 120, y));
        renderer.drawText(valueText);
        
        y += 30;
    };
    
    drawDetail("Name", meta.saveName);
    drawDetail("Date", formatTimestamp(meta.timestamp));
    drawDetail("Grid Size", std::to_string(meta.gridWidth) + "x" + std::to_string(meta.gridHeight));
    drawDetail("Cells", std::to_string(meta.cellCount));
    
    if (!meta.notes.empty()) {
        y += 10;
        sf::Text notesLabel(font, "Notes:", 16);
        notesLabel.setFillColor(sf::Color(150, 150, 150));
        notesLabel.setPosition(sf::Vector2f(x, y));
        renderer.drawText(notesLabel);
        
        y += 25;
        // Word wrap notes
        std::string notes = meta.notes;
        std::string line;
        std::istringstream stream(notes);
        while (std::getline(stream, line)) {
            sf::Text notesText(font, line, 14);
            notesText.setFillColor(sf::Color(200, 200, 200));
            notesText.setPosition(sf::Vector2f(x + 10, y));
            renderer.drawText(notesText);
            y += 22;
        }
    }
}

MenuAction LoadGameMenu::handleEvent(const sf::Event& event) {
    if (!initialized) {
        return MenuAction::None;
    }
    
    if (!window) {
        return MenuAction::None;
    }
    
    lastAction = MenuAction::None;
    
    if (event.is<sf::Event::MouseButtonPressed>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseButton && mouseButton->button == sf::Mouse::Button::Left) {
            sf::Vector2i sfMousePos = sf::Mouse::getPosition(*window);
            sf::Vector2f clickPos(static_cast<float>(sfMousePos.x), static_cast<float>(sfMousePos.y));
            
            // Forward to saveListBorder for scrollbar handling first
            saveListBorder.handleMousePress(clickPos);
            
            // Check save entries (use stored absolute positions + scroll border screen position)
            // CRITICAL: Must convert from LOCAL to SCREEN coordinates for hit detection
            // Entry position is relative to scroll border, need to add scroll border's screen position
            // and subtract scroll offset to get actual screen position.
            //
            // Common bug: Using entry.getPosition() directly gives local coords, causing
            // mouse hit detection to be offset from visual position.
            // Fix: screenPos = scrollBorderPos + entryLocalPos - scrollOffset
            
            sf::Vector2f scrollBorderPos = saveListBorder.getPosition();
            float scrollOffset = saveListBorder.getScrollOffset();
            
            for (size_t i = 0; i < saveEntries.size(); ++i) {
                // Use the entry's local position (not screen position)
                sf::Vector2f entryLocalPos = saveEntries[i].getPosition();
                sf::Vector2f entrySize = saveEntries[i].getSize();
                
                // Convert to screen coordinates: scroll border position + local position - scroll offset
                sf::Vector2f entryScreenPos(
                    scrollBorderPos.x + entryLocalPos.x,
                    scrollBorderPos.y + entryLocalPos.y - scrollOffset
                );
                
                sf::FloatRect bounds(entryScreenPos, entrySize);
                
                if (bounds.contains(clickPos)) {
                    selectSave(static_cast<int>(i));
                    break;
                }
            }
            
            // Route to border (handles button events)
            mainBorder.handleMousePress(clickPos);
        }
    } else if (event.is<sf::Event::MouseButtonReleased>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonReleased>();
        if (mouseButton && mouseButton->button == sf::Mouse::Button::Left) {
            saveListBorder.handleMouseRelease();
            mainBorder.handleMouseRelease();
        }
    } else if (event.is<sf::Event::MouseMoved>()) {
        auto mouseMove = event.getIf<sf::Event::MouseMoved>();
        if (mouseMove) {
            sf::Vector2f pos(static_cast<float>(mouseMove->position.x), static_cast<float>(mouseMove->position.y));
            
            // Forward to saveListBorder for scrollbar hover
            saveListBorder.handleMouseMove(pos);
            
            // Route to border (handles button hover)
            mainBorder.handleMouseMove(pos);
        }
    }
    
    return lastAction;
}
