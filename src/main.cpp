#include "core/Engine.h"
#include "core/Logger.h"
#include "core/Settings.h"
#include "rendering/Renderer.h"
#include "rendering/TileMap.h"
#include "simulation/Grid.h"
#include "simulation/SimulationManager.h"
#include "simulation/ElementTypes.h"
#include "ui/MainMenu.h"
#include "ui/SettingsMenu.h"
#include "ui/PauseMenu.h"
#include "ecs/Entity.h"
#include "ui/UISlider.h"
#include <SFML/Graphics.hpp>

#define DEBUG_MODE false  // Set to true to enable debug console output
#define DEVELOPER_MODE true  // Set to true to enable admin/dev tools (sliders, overlays, etc.)

// Global variables for demo
Renderer renderer;
TileMap tileMap;
Grid simGrid;
SimulationManager simManager;
MainMenu mainMenu;
SettingsMenu settingsMenu;
PauseMenu pauseMenu;

enum class GameState {
    Menu,
    Playing,
    Paused,
    Settings
};
GameState gameState = GameState::Menu;
bool simulationInitialized = false;

ElementType currentElement = ElementType::Liquid_Water;
sf::Font font;
sf::Text* infoText = nullptr;

// UI Slider for simulation speed
UISlider* simSpeedSlider = nullptr;
bool isAdminMode = DEVELOPER_MODE;  // Auto-enabled if DEVELOPER_MODE is true
bool showElementInspector = DEVELOPER_MODE;  // Toggle element inspection overlay
sf::Text* inspectorText = nullptr;  // Hover inspection display

// Map overlay toggles (F keys)
bool showHeatMapOverlay = false;  // F3 - Temperature visualization
bool showPhaseOverlay = false;    // F4 - Phase state visualization
bool showDensityOverlay = false;  // F5 - Density visualization

// Forward declarations
void syncTileMap();
void updateElementInspector();

sf::Color getHeatMapColor(float temperature) {
    // Color gradient: Blue (cold) -> Cyan -> Green -> Yellow -> Red (hot)
    if (temperature < 0) {
        // Very cold: Dark blue
        return sf::Color(0, 0, 139);
    } else if (temperature < 50) {
        // Cold: Blue
        float t = temperature / 50.0f;
        return sf::Color(0, (uint8_t)(100 + t * 155), 255);
    } else if (temperature < 100) {
        // Cool: Cyan to Green
        float t = (temperature - 50) / 50.0f;
        return sf::Color(0, 255, (uint8_t)(255 - t * 255));
    } else if (temperature < 300) {
        // Warm: Green to Yellow
        float t = (temperature - 100) / 200.0f;
        return sf::Color((uint8_t)(t * 255), 255, 0);
    } else if (temperature < 700) {
        // Hot: Yellow to Orange
        float t = (temperature - 300) / 400.0f;
        return sf::Color(255, (uint8_t)(255 - t * 100), 0);
    } else if (temperature < 1000) {
        // Very hot: Orange to Red
        float t = (temperature - 700) / 300.0f;
        return sf::Color(255, (uint8_t)(155 - t * 155), 0);
    } else {
        // Extreme heat: Bright red to white
        float t = std::min((temperature - 1000) / 200.0f, 1.0f);
        return sf::Color(255, (uint8_t)(t * 255), (uint8_t)(t * 255));
    }
}

void initializeDemo() {
    if (DEBUG_MODE) LOG_INFO("Initializing demo");
    
    auto& settings = SettingsManager::getInstance().getSettings();
    
    // Initialize tile map with variable grid size from settings
    tileMap.initialize(settings.gridWidth, settings.gridHeight, 32.0f);
    
    // Initialize simulation grid
    simGrid.initialize(settings.gridWidth, settings.gridHeight);
    simManager.initialize(simGrid);
    
    // Create some solid walls
    simGrid.lock();  // THREAD-SAFE: Lock before initialization
    
    for (int x = 0; x < settings.gridWidth; ++x) {
        simGrid.setCellType(x, 0, ElementType::Solid);
        simGrid.setCellType(x, settings.gridHeight - 1, ElementType::Solid);
    }
    for (int y = 0; y < settings.gridHeight; ++y) {
        simGrid.setCellType(0, y, ElementType::Solid);
        simGrid.setCellType(settings.gridWidth - 1, y, ElementType::Solid);
    }
    
    // Add some internal walls (proportional to grid size)
    int wallStartX = settings.gridWidth / 4;
    int wallEndX = settings.gridWidth * 3 / 4;
    int wallY = settings.gridHeight / 2;
    for (int x = wallStartX; x < wallEndX; ++x) {
        simGrid.setCellType(x, wallY, ElementType::Solid);
    }
    // Add gap in wall
    simGrid.setCellType(wallStartX + 3, wallY, ElementType::Empty);
    simGrid.setCellType(wallStartX + 4, wallY, ElementType::Empty);
    simGrid.setCellType(wallStartX + 5, wallY, ElementType::Empty);
    
    // Add some test water and gas (proportional to grid size)
    int waterStartX = settings.gridWidth / 8;
    int waterEndX = settings.gridWidth / 4;
    for (int x = waterStartX; x < waterEndX; ++x) {
        simGrid.setCellType(x, settings.gridHeight - 2, ElementType::Liquid_Water);
    }
    int gasStartX = settings.gridWidth * 3 / 4;
    int gasEndX = settings.gridWidth * 7 / 8;
    for (int x = gasStartX; x < gasEndX; ++x) {
        simGrid.setCellType(x, 5, ElementType::Gas_O2);
    }
    
    // Update all cell colors
    for (int y = 0; y < simGrid.getHeight(); ++y) {
        for (int x = 0; x < simGrid.getWidth(); ++x) {
            Cell& cell = simGrid.getCell(x, y);
            cell.updateColor();
        }
    }
    
    simGrid.unlock();  // UNLOCK after initialization
    
    // Setup camera bounds to match grid
    float gridWidthPx = settings.gridWidth * 32.0f;
    float gridHeightPx = settings.gridHeight * 32.0f;
    
    renderer.getCamera().setGridBounds(0, 0, gridWidthPx, gridHeightPx);
    renderer.getCamera().setPosition(gridWidthPx / 2.0f, gridHeightPx / 2.0f);
    renderer.getCamera().setScrollSpeed(settings.cameraScrollSpeed);
    renderer.getCamera().setEdgeMargin(settings.cameraEdgeScrollMargin);
    
    // Initialize LOD chunk manager for distance-based simulation updates
    simManager.getChunkManager().initialize(settings.gridWidth, settings.gridHeight, 16);
    
    simulationInitialized = true;
    if (DEBUG_MODE) LOG_INFO("Demo initialized with grid: " + std::to_string(settings.gridWidth) + "x" + std::to_string(settings.gridHeight));
}

void initializeFonts() {
    // Load font
    if (!font.openFromFile("assets/fonts/arial.ttf")) {
        LOG_ERROR("Failed to load font!");
    }
    infoText = new sf::Text(font, "", 20);
    infoText->setFillColor(sf::Color::White);
    infoText->setPosition(sf::Vector2f(10, 10));
    
    // Initialize simulation speed slider (available to all players)
    // Position at bottom-left of screen (will be updated dynamically during render)
    std::cout << "[INIT] Creating simSpeedSlider..." << std::endl;
    simSpeedSlider = new UISlider();
    std::cout << "[INIT] simSpeedSlider created, pointer: " << simSpeedSlider << std::endl;
    simSpeedSlider->initialize(10, 10, 300, 0.1f, 5.0f, 1.0f, "Sim Speed:", font);
    std::cout << "[INIT] simSpeedSlider initialized" << std::endl;
    
    // Initialize developer tools (only in DEVELOPER_MODE)
#if DEVELOPER_MODE
    if (isAdminMode) {
        // Developer mode tools can be added here if needed
    }
    
    // Initialize element inspector text
    inspectorText = new sf::Text(font, "", 14);
    inspectorText->setFillColor(sf::Color::Yellow);
    inspectorText->setPosition(sf::Vector2f(10, 50));
#endif
    
    // Sync tilemap with simulation grid (do this LAST)
    // Simple version: just set walls manually
    TileInfo wallTile;
    wallTile.color = sf::Color(128, 128, 128);
    wallTile.solid = true;
    wallTile.name = "Solid";
    
    TileInfo waterTile;
    waterTile.color = sf::Color(50, 100, 255, 180);
    waterTile.solid = false;
    waterTile.name = "Water";
    
    TileInfo gasTile;
    gasTile.color = sf::Color(100, 150, 255, 100);
    gasTile.solid = false;
    gasTile.name = "O2";
    
    for (int x = 0; x < 40; ++x) {
        tileMap.setTile(x, 0, wallTile);
        tileMap.setTile(x, 29, wallTile);
    }
    for (int y = 0; y < 30; ++y) {
        tileMap.setTile(0, y, wallTile);
        tileMap.setTile(39, y, wallTile);
    }
    for (int x = 10; x < 30; ++x) {
        tileMap.setTile(x, 15, wallTile);
    }
    // Add water and gas
    for (int x = 5; x < 10; ++x) {
        tileMap.setTile(x, 28, waterTile);
    }
    for (int x = 30; x < 35; ++x) {
        tileMap.setTile(x, 5, gasTile);
    }
    
    if (DEBUG_MODE) std::cout << "Tilemap setup complete" << std::endl;
    
    if (DEBUG_MODE) LOG_INFO("Demo initialized");
}

void handleMouseInput(const sf::Event& event) {
    if (event.is<sf::Event::MouseButtonPressed>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseButton) {
            // Get mouse position in screen coordinates
            sf::Vector2i mousePos = sf::Mouse::getPosition(renderer.getRenderWindow());
            
            // CRITICAL FIX: Convert screen coordinates to world coordinates using camera
            sf::Vector2f worldPos = renderer.getCamera().screenToWorld(
                static_cast<float>(mousePos.x), 
                static_cast<float>(mousePos.y)
            );
            
            // Convert world coordinates to tile coordinates (32px tiles)
            int tileX = static_cast<int>(worldPos.x / 32.0f);
            int tileY = static_cast<int>(worldPos.y / 32.0f);
            
            if (simGrid.isValidPosition(tileX, tileY)) {
                if (mouseButton->button == sf::Mouse::Button::Left) {
                    // THREAD-SAFE: Lock grid before modifying cells
                    simGrid.lock();
                    
                    // Place element
                    simGrid.setCellType(tileX, tileY, currentElement);
                    
                    // Set properties from element (DATA-DRIVEN)
                    Cell& placedCell = simGrid.getCell(tileX, tileY);
                    ElementProperties props = ElementTypes::getElement(currentElement);
                    placedCell.temperature = props.defaultTemperature;
                    
                    // ONI-STYLE: Set initial MASS (full cell by default)
                    // Mass = density × volume (1m³ per cell)
                    placedCell.mass = props.density;  // Full cell
                    
                    // Update tile directly
                    sf::Color color = sf::Color::Transparent;
                    std::string name = "Empty";
                    if (currentElement == ElementType::Solid) {
                        color = sf::Color(128, 128, 128);
                        name = "Solid";
                    } else if (currentElement == ElementType::Liquid_Water) {
                        color = sf::Color(50, 100, 255, 180);
                        name = "Water";
                    } else if (currentElement == ElementType::Liquid_Lava) {
                        color = sf::Color(255, 100, 0, 200);
                        name = "Lava";
                    } else if (currentElement == ElementType::Gas_O2) {
                        color = sf::Color(100, 150, 255, 100);
                        name = "O2";
                    } else if (currentElement == ElementType::Gas_CO2) {
                        color = sf::Color(100, 100, 100, 120);
                        name = "CO2";
                    } else if (currentElement == ElementType::Vacuum) {
                        color = sf::Color(10, 10, 15);
                        name = "Vacuum";
                    }
                    
                    TileInfo tileInfo;
                    tileInfo.color = color;
                    tileInfo.solid = (currentElement == ElementType::Solid);
                    tileInfo.name = name;
                    tileMap.setTile(tileX, tileY, tileInfo);
                    
                    // UNLOCK grid after modifications
                    simGrid.unlock();
                    
                    if (DEBUG_MODE) std::cout << "Placed " << name << " at (" << tileX << ", " << tileY << ")" << std::endl;
                } else if (mouseButton->button == sf::Mouse::Button::Right) {
                    // THREAD-SAFE: Lock grid before clearing cells
                    simGrid.lock();
                    
                    // Clear cell
                    simGrid.setCellType(tileX, tileY, ElementType::Empty);
                    
                    TileInfo emptyTile;
                    emptyTile.color = sf::Color::Transparent;
                    emptyTile.solid = false;
                    emptyTile.name = "Empty";
                    tileMap.setTile(tileX, tileY, emptyTile);
                    
                    // UNLOCK grid
                    simGrid.unlock();
                }
            }
        }
    }
}

void syncTileMap() {
    int height = simGrid.getHeight();
    int width = simGrid.getWidth();
    
    // THREAD-SAFE: Lock grid before reading all cells
    simGrid.lock();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!simGrid.isValidPosition(x, y)) continue;
            
            const Cell& cell = simGrid.getCell(x, y);
            TileInfo tileInfo;
            
            // Apply overlay colors based on active visualization mode
            if (showHeatMapOverlay) {
                // Heat map: color by temperature
                if (cell.elementType == ElementType::Empty) {
                    tileInfo.color = sf::Color(20, 20, 20);  // Dark for empty
                } else {
                    tileInfo.color = getHeatMapColor(cell.temperature);
                }
            } else if (showPhaseOverlay) {
                // Phase overlay: color by state
                ElementProperties props = ElementTypes::getElement(cell.elementType);
                if (cell.elementType == ElementType::Empty) {
                    tileInfo.color = sf::Color(20, 20, 20);
                } else if (props.isSolid) {
                    tileInfo.color = sf::Color(100, 100, 255);  // Blue = Solid
                } else if (props.isLiquid) {
                    tileInfo.color = sf::Color(50, 200, 50);    // Green = Liquid
                } else if (props.isGas) {
                    tileInfo.color = sf::Color(255, 100, 50);   // Orange = Gas
                }
            } else if (showDensityOverlay) {
                // Density overlay: color by density
                ElementProperties props = ElementTypes::getElement(cell.elementType);
                if (cell.elementType == ElementType::Empty) {
                    tileInfo.color = sf::Color(20, 20, 20);
                } else {
                    // Map density to color (0-4000 kg/m³)
                    float densityNorm = std::min(props.density / 4000.0f, 1.0f);
                    tileInfo.color = sf::Color(
                        (uint8_t)(densityNorm * 255),
                        (uint8_t)((1.0f - densityNorm) * 255),
                        50
                    );
                }
            } else {
                // Default: normal element colors
                tileInfo.color = cell.color;
            }
            
            tileInfo.solid = (cell.elementType == ElementType::Solid);
            tileInfo.name = ElementTypes::getTypeName(cell.elementType);
            tileMap.setTile(x, y, tileInfo);
        }
    }
    
    // UNLOCK grid after reading all cells
    simGrid.unlock();
}

void updateElementInspector() {
#if DEVELOPER_MODE
    // Only update when Left Alt is held
    if (!inspectorText || !sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LAlt)) return;
    
    // Get mouse position in SCREEN coordinates
    sf::Vector2i mousePos = sf::Mouse::getPosition(renderer.getRenderWindow());
    
    // Convert to WORLD coordinates using camera
    sf::Vector2f worldPos = renderer.getCamera().screenToWorld((float)mousePos.x, (float)mousePos.y);
    
    // Convert world coordinates to tile coordinates (32px tiles)
    int tileX = (int)(worldPos.x / 32.0f);
    int tileY = (int)(worldPos.y / 32.0f);
    
    if (simGrid.isValidPosition(tileX, tileY)) {
        // THREAD-SAFE: Lock grid before reading cell data
        simGrid.lock();
        
        const Cell& cell = simGrid.getCell(tileX, tileY);
        ElementProperties props = ElementTypes::getElement(cell.elementType);
        
        // Build inspection string with metric and imperial
        std::string info = "Position: (" + std::to_string(tileX) + ", " + std::to_string(tileY) + ")\n";
        info += "Element: " + props.name + "\n";
        
        // Temperature in Celsius and Fahrenheit (1 decimal place)
        // Safety check for NaN/Inf values
        float displayTemp = cell.temperature;
        if (std::isnan(displayTemp) || std::isinf(displayTemp)) {
            displayTemp = -273.15f;  // Default to absolute zero if invalid
        }
        float tempF = (displayTemp * 9.0f / 5.0f) + 32.0f;
        char tempBuf[100];
        snprintf(tempBuf, sizeof(tempBuf), "Temperature: %.1f C / %.1f F\n", displayTemp, tempF);
        info += tempBuf;
        
        if (cell.elementType != ElementType::Empty) {
            // Density in kg/m^3 and lb/ft^3 (1 decimal place)
            float densityImperial = props.density * 0.06243f;
            char densityBuf[100];
            snprintf(densityBuf, sizeof(densityBuf), "Density: %.1f kg/m^3 / %.1f lb/ft^3\n", props.density, densityImperial);
            info += densityBuf;
            
            info += "Phase: ";
            if (props.isSolid) info += "Solid";
            else if (props.isLiquid) info += "Liquid";
            else if (props.isGas) info += "Gas";
            info += "\n";
            
            if (props.isLiquid || props.isSolid) {
                float meltF = (props.meltingPoint * 9.0f / 5.0f) + 32.0f;
                char meltBuf[100];
                snprintf(meltBuf, sizeof(meltBuf), "Melting Point: %.1f C / %.1f F\n", props.meltingPoint, meltF);
                info += meltBuf;
            }
            if (props.isLiquid || props.isGas) {
                float boilF = (props.boilingPoint * 9.0f / 5.0f) + 32.0f;
                char boilBuf[100];
                snprintf(boilBuf, sizeof(boilBuf), "Boiling Point: %.1f C / %.1f F\n", props.boilingPoint, boilF);
                info += boilBuf;
            }
            
            // Thermal conductivity in W/(m·K) and BTU/(hr·ft·F) (1 decimal place)
            float kImperial = props.thermalConductivity * 0.5779f;
            char kBuf[150];
            snprintf(kBuf, sizeof(kBuf), "Thermal Conductivity: %.1f W/(m*K) / %.1f BTU/(hr*ft*F)\n", props.thermalConductivity, kImperial);
            info += kBuf;
            
            // Specific heat in J/(kg·K) and BTU/(lb·F) (1 decimal place)
            float cpImperial = props.specificHeatCapacity * 0.000238846f;
            char cpBuf[150];
            snprintf(cpBuf, sizeof(cpBuf), "Specific Heat: %.1f J/(kg*K) / %.1f BTU/(lb*F)", props.specificHeatCapacity, cpImperial);
            info += cpBuf;
        }
        
        inspectorText->setString(info);
        
        // UNLOCK grid after reading
        simGrid.unlock();
        
        // Position near mouse but not under it (in SCREEN coordinates)
        float textX = mousePos.x + 20;
        float textY = mousePos.y - 10;
        
        // Get actual window size for clamping
        sf::Vector2u windowSize = renderer.getRenderWindow().getSize();
        
        // Keep on screen - flip to left side if too far right
        if (textX + 350 > (float)windowSize.x) textX = mousePos.x - 370;
        // Keep on screen - flip above mouse if too low
        if (textY + 250 > (float)windowSize.y) textY = mousePos.y - 260;
        
        inspectorText->setPosition(sf::Vector2f(textX, textY));
    } else {
        inspectorText->setString("");
    }
#endif
}

void updateSimulation(float deltaTime) {
    // Apply simulation speed multiplier from slider
    float simSpeed = 1.0f;
    if (simSpeedSlider) {
        simSpeed = simSpeedSlider->currentValue;
    }
    
    float simDeltaTime = deltaTime * simSpeed;
    
    // Update all simulation systems
    simManager.update(simDeltaTime);
    
    // Sync tilemap with simulation grid for visualization
    syncTileMap();
    
    // Update element inspector (dev mode)
#if DEVELOPER_MODE
    updateElementInspector();
#endif
}

void renderDemo() {
    static int frameCount = 0;
    if (frameCount % 60 == 0) {  // Print once per second at 60fps
        std::cout << "[RENDER] renderDemo called, frame: " << frameCount << std::endl;
    }
    frameCount++;
    
    renderer.beginFrame();
    
    // Render all tiles from tileMap
    sf::RectangleShape tileRect(sf::Vector2f(32, 32));
    int width = tileMap.getWidth();
    int height = tileMap.getHeight();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            TileInfo tile = tileMap.getTile(x, y);
            if (tile.color.a > 0) {  // Only draw non-transparent tiles
                tileRect.setPosition(sf::Vector2f(x * 32.0f, y * 32.0f));
                tileRect.setFillColor(tile.color);
                renderer.drawRectangle(tileRect);
            }
        }
    }
    
    // Draw info text
    std::string elementName = ElementTypes::getTypeName(currentElement);
    if (infoText) {
        std::string info = "Element: " + elementName + " | Left Click: Place | Right Click: Clear | 1-6: Change Element | R: Reset | ESC: Quit";
        if (isAdminMode) {
            info += " | [DEV MODE]";
            if (showHeatMapOverlay) info += " F3:HeatMap";
            if (showPhaseOverlay) info += " F4:Phase";
            if (showDensityOverlay) info += " F5:Density";
            if (!showHeatMapOverlay && !showPhaseOverlay && !showDensityOverlay) {
                info += " F3-F5:Overlays";
            }
            info += " F1:ToggleUI F2:Inspector";
        }
        infoText->setString(info);
        
        // Reset view to screen coordinates for UI
        sf::View defaultView = renderer.getRenderWindow().getDefaultView();
        renderer.getRenderWindow().setView(defaultView);
        renderer.drawText(*infoText);
        
        // Restore camera view for game rendering
        renderer.getCamera().applyTo(renderer.getRenderWindow());
    }
    
    // Render simulation speed slider (always visible during gameplay)
    std::cout << "[SLIDER CHECK] simSpeedSlider: " << (simSpeedSlider ? "NOT NULL" : "NULL") 
             << " gameState: " << static_cast<int>(gameState) << std::endl;
    
    if (simSpeedSlider && (gameState == GameState::Playing || gameState == GameState::Paused)) {
        // Update slider position to bottom of screen dynamically
        sf::Vector2u windowSize = renderer.getRenderWindow().getSize();
        float sliderY = static_cast<float>(windowSize.y) - 50.0f;  // 50px from bottom
        
        std::cout << "[SLIDER] Window: " << windowSize.x << "x" << windowSize.y 
                 << " SliderY: " << sliderY << std::endl;
        
        simSpeedSlider->track.setPosition(sf::Vector2f(10.0f, sliderY));
        simSpeedSlider->updateThumbPosition();
        simSpeedSlider->label->setPosition(sf::Vector2f(10.0f, sliderY - 25.0f));
        simSpeedSlider->valueText->setPosition(sf::Vector2f(320.0f, sliderY - 2.0f));
        
        // Reset view to screen coordinates for UI overlay
        sf::View defaultView = renderer.getRenderWindow().getDefaultView();
        renderer.getRenderWindow().setView(defaultView);
        
        std::cout << "[SLIDER] Track pos: " << simSpeedSlider->track.getPosition().x << "," 
                 << simSpeedSlider->track.getPosition().y << std::endl;
        
        simSpeedSlider->render(renderer);
        
        // Don't restore camera view - slider should be on top, endFrame will display it
    } else {
        // Ensure camera view is set for game rendering if no UI overlay
        renderer.getCamera().applyTo(renderer.getRenderWindow());
    }
    
    // Render admin UI (developer mode only)
#if DEVELOPER_MODE
    if (isAdminMode) {
        // Additional developer tools can be rendered here
    }
#endif
    
    // Render element inspector (dev mode) - only when Left Alt is held
#if DEVELOPER_MODE
    if (inspectorText && sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::LAlt)) {
        updateElementInspector();  // Update data
        renderer.drawText(*inspectorText);  // Render
    }
#endif
    
    renderer.endFrame();
}

int main() {
    try {
        // TEMPORARY: Force debug logging to see window creation info
        Logger::getInstance().setLogLevel(LogLevel::DEBUG);
        std::cout << "[DEBUG] Logger level set to DEBUG" << std::endl;
        
        if (DEBUG_MODE) std::cout << "=== Game Engine Starting ===" << std::endl;
        if (DEBUG_MODE) LOG_INFO("Starting Game Engine");
        
        // Load settings
        SettingsManager::getInstance().loadSettings();
        auto& settings = SettingsManager::getInstance().getSettings();
        
        // Create engine
        Engine engine;
        
        // Configure window from settings
        WindowConfig config;
        config.title = "ONI-like Game Engine";
        config.width = settings.screenWidth;
        config.height = settings.screenHeight;
        config.displayMode = settings.displayMode;
        config.vsync = settings.vsync;
        
        // Initialize engine
        engine.initialize(config);
        
        if (DEBUG_MODE) LOG_INFO("Engine initialized, setting up renderer...");
        
        // Initialize renderer with engine window
        renderer.initialize(engine.getWindow().getRenderWindow());
        
        // Set camera view size to match window dimensions
        sf::Vector2u windowSize = renderer.getRenderWindow().getSize();
        renderer.getCamera().setViewSize(static_cast<float>(windowSize.x), static_cast<float>(windowSize.y));
        
        // Initialize fonts and UI elements
        initializeFonts();
        
        // Initialize menus
        try {
            mainMenu.initialize(renderer.getRenderWindow());
            settingsMenu.initialize(renderer.getRenderWindow());
            pauseMenu.initialize(renderer.getRenderWindow());
            
            if (!mainMenu.isInitialized() || !settingsMenu.isInitialized() || !pauseMenu.isInitialized()) {
                std::cerr << "WARNING: Menu system failed to initialize, skipping to game" << std::endl;
                gameState = GameState::Playing;
                initializeDemo();
                simulationInitialized = true;
            }
        } catch (const std::exception& e) {
            std::cerr << "ERROR initializing menus: " << e.what() << std::endl;
            gameState = GameState::Playing;
            initializeDemo();
            simulationInitialized = true;
        }
        
        if (DEBUG_MODE) LOG_INFO("Renderer and menu initialized, starting game loop...");
        
        // Game state starts at Menu
        gameState = GameState::Menu;
        simulationInitialized = false;
        
        // Set up callbacks
        engine.onEvent = [&engine](const sf::Event& event) {
            // Route events based on game state
            if (gameState == GameState::Menu) {
                // Main menu
                MenuAction action = mainMenu.handleEvent(event);
                
                std::cout << "[MENU] Action: " << static_cast<int>(action) << std::endl;
                
                if (action == MenuAction::Play) {
                    std::cout << "[MENU] Play clicked! simulationInitialized: " << simulationInitialized << std::endl;
                    gameState = GameState::Playing;
                    if (!simulationInitialized) {
                        initializeDemo();
                        simulationInitialized = true;
                    }
                } else if (action == MenuAction::Settings) {
                    gameState = GameState::Settings;
                } else if (action == MenuAction::Quit) {
                    engine.stop();
                }
            } else if (gameState == GameState::Settings) {
                // Settings menu
                MenuAction action = settingsMenu.handleEvent(event);
                
                if (action == MenuAction::ApplySettings) {
                    SettingsManager::getInstance().saveSettings();
                    
                    auto& settings = SettingsManager::getInstance().getSettings();
                    WindowConfig newConfig;
                    newConfig.title = "ONI-like Game Engine";
                    newConfig.width = settings.screenWidth;
                    newConfig.height = settings.screenHeight;
                    newConfig.displayMode = settings.displayMode;
                    newConfig.vsync = settings.vsync;
                    engine.getWindow().applySettings(newConfig);
                    renderer.getRenderWindow().setVerticalSyncEnabled(settings.vsync);
                    
                    // Apply camera settings
                    renderer.getCamera().setScrollSpeed(settings.cameraScrollSpeed);
                    renderer.getCamera().setAcceleration(settings.cameraAcceleration);
                    renderer.getCamera().setMaxSpeed(settings.cameraMaxSpeed);
                    
                    if (simulationInitialized) {
                        gameState = GameState::Paused;
                    } else {
                        gameState = GameState::Menu;
                    }
                } else if (action == MenuAction::Back) {
                    if (simulationInitialized) {
                        gameState = GameState::Paused;
                    } else {
                        gameState = GameState::Menu;
                    }
                }
            } else if (gameState == GameState::Paused) {
                // Pause menu
                MenuAction action = pauseMenu.handleEvent(event);
                
                if (action == MenuAction::Resume) {
                    gameState = GameState::Playing;
                } else if (action == MenuAction::Settings) {
                    gameState = GameState::Settings;
                } else if (action == MenuAction::QuitToMain) {
                    engine.stop();
                }
            } else {
                // GAME PLAYING STATE - handle game input
                
                // Handle keyboard input
                if (event.is<sf::Event::KeyPressed>()) {
                    auto keyEvent = event.getIf<sf::Event::KeyPressed>();
                    if (keyEvent) {
                        switch (keyEvent->scancode) {
                    case sf::Keyboard::Scancode::Num1:
                        currentElement = ElementType::Liquid_Water;
                        break;
                    case sf::Keyboard::Scancode::Num2:
                        currentElement = ElementType::Liquid_Lava;
                        break;
                    case sf::Keyboard::Scancode::Num3:
                        currentElement = ElementType::Gas_O2;
                        break;
                    case sf::Keyboard::Scancode::Num4:
                        currentElement = ElementType::Gas_CO2;
                        break;
                    case sf::Keyboard::Scancode::Num5:
                        currentElement = ElementType::Solid;
                        break;
                    case sf::Keyboard::Scancode::Num6:
                        currentElement = ElementType::Vacuum;
                        break;
                    case sf::Keyboard::Scancode::R:
                    {
                        // THREAD-SAFE: Lock grid before resetting
                        simGrid.lock();
                        
                        simGrid.clear();
                        // Rebuild walls
                        auto& settings = SettingsManager::getInstance().getSettings();
                        for (int x = 0; x < settings.gridWidth; ++x) {
                            simGrid.setCellType(x, 0, ElementType::Solid);
                            simGrid.setCellType(x, settings.gridHeight - 1, ElementType::Solid);
                        }
                        for (int y = 0; y < settings.gridHeight; ++y) {
                            simGrid.setCellType(0, y, ElementType::Solid);
                            simGrid.setCellType(settings.gridWidth - 1, y, ElementType::Solid);
                        }
                        for (int x = settings.gridWidth / 4; x < 3 * settings.gridWidth / 4; ++x) {
                            simGrid.setCellType(x, settings.gridHeight / 2, ElementType::Solid);
                        }
                        int doorX = settings.gridWidth / 2;
                        simGrid.setCellType(doorX, settings.gridHeight / 2, ElementType::Empty);
                        simGrid.setCellType(doorX + 1, settings.gridHeight / 2, ElementType::Empty);
                        simGrid.setCellType(doorX - 1, settings.gridHeight / 2, ElementType::Empty);
                        
                        // UNLOCK grid
                        simGrid.unlock();
                        break;
                    }
#if DEVELOPER_MODE
                    case sf::Keyboard::Scancode::F1:  // Toggle admin mode (DEV ONLY)
                        isAdminMode = !isAdminMode;
                        break;
                    case sf::Keyboard::Scancode::F2:  // Toggle element inspector
                        showElementInspector = !showElementInspector;
                        break;
                    case sf::Keyboard::Scancode::F3:  // Toggle heat map overlay
                        showHeatMapOverlay = !showHeatMapOverlay;
                        break;
                    case sf::Keyboard::Scancode::F4:  // Toggle phase overlay
                        showPhaseOverlay = !showPhaseOverlay;
                        break;
                    case sf::Keyboard::Scancode::F5:  // Toggle density overlay
                        showDensityOverlay = !showDensityOverlay;
                        break;
#endif
                    case sf::Keyboard::Scancode::Escape:
                        // Pause game
                        gameState = GameState::Paused;
                        break;
                        }
                    }
                }
                
                // Handle simulation speed slider input (always active)
                if (simSpeedSlider) {
                    if (event.is<sf::Event::MouseButtonPressed>()) {
                        sf::Vector2i mousePos = sf::Mouse::getPosition(renderer.getRenderWindow());
                        simSpeedSlider->handleMousePress(sf::Vector2f(mousePos.x, mousePos.y));
                    } else if (event.is<sf::Event::MouseButtonReleased>()) {
                        simSpeedSlider->handleMouseRelease();
                    } else if (event.is<sf::Event::MouseMoved>()) {
                        auto mouseMove = event.getIf<sf::Event::MouseMoved>();
                        if (mouseMove) {
                            simSpeedSlider->handleMouseMove(sf::Vector2f(mouseMove->position.x, mouseMove->position.y));
                        }
                    }
                }
                
                // Handle +/- hotkeys for simulation speed
                if (event.is<sf::Event::KeyPressed>()) {
                    auto keyEvent = event.getIf<sf::Event::KeyPressed>();
                    if (keyEvent && simSpeedSlider) {
                        if (keyEvent->scancode == sf::Keyboard::Scancode::Equal) {
                            simSpeedSlider->currentValue = std::min(5.0f, simSpeedSlider->currentValue + 0.1f);
                            simSpeedSlider->updateThumbPosition();
                            simSpeedSlider->updateValueText();
                        } else if (keyEvent->scancode == sf::Keyboard::Scancode::Hyphen) {
                            simSpeedSlider->currentValue = std::max(0.1f, simSpeedSlider->currentValue - 0.1f);
                            simSpeedSlider->updateThumbPosition();
                            simSpeedSlider->updateValueText();
                        }
                    }
                }
                
                handleMouseInput(event);
            }
        };
        
        engine.onUpdate = [&engine](void) {
            float deltaTime = engine.getTime().getDeltaTime();
            
            // Update camera smooth scrolling
            if (gameState == GameState::Playing) {
                // Handle arrow keys for camera (MUST be before update())
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Up)) {
                    renderer.getCamera().handleArrowKeys(sf::Keyboard::Scancode::Up, true);
                } else {
                    renderer.getCamera().handleArrowKeys(sf::Keyboard::Scancode::Up, false);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Down)) {
                    renderer.getCamera().handleArrowKeys(sf::Keyboard::Scancode::Down, true);
                } else {
                    renderer.getCamera().handleArrowKeys(sf::Keyboard::Scancode::Down, false);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Left)) {
                    renderer.getCamera().handleArrowKeys(sf::Keyboard::Scancode::Left, true);
                } else {
                    renderer.getCamera().handleArrowKeys(sf::Keyboard::Scancode::Left, false);
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Scancode::Right)) {
                    renderer.getCamera().handleArrowKeys(sf::Keyboard::Scancode::Right, true);
                } else {
                    renderer.getCamera().handleArrowKeys(sf::Keyboard::Scancode::Right, false);
                }
                
                // Handle mouse edge scrolling (MUST be before update())
                sf::Vector2i mousePos = sf::Mouse::getPosition(renderer.getRenderWindow());
                sf::Vector2u windowSize = renderer.getRenderWindow().getSize();
                renderer.getCamera().handleMouseEdge(sf::Vector2f(mousePos.x, mousePos.y), windowSize);
                
                // Now update camera with all inputs set
                renderer.getCamera().update(deltaTime);
                
                // Update LOD chunk manager with current camera position
                sf::Vector2f camPos = renderer.getCamera().getPosition();
                sf::Vector2f viewSize = renderer.getCamera().getView().getSize();
                simManager.setCameraPosition(camPos.x, camPos.y, viewSize.x, viewSize.y);
                
                // Update simulation
                updateSimulation(deltaTime);
            }
        };
        
        engine.onRender = []() {
            try {
                if (gameState == GameState::Menu) {
                    // FIXED: Main menu renders in screen coordinates (no camera transform)
                    renderer.beginFrame(false);  // Don't apply camera for menu
                    mainMenu.render(renderer);
                    renderer.endFrame();
                } else if (gameState == GameState::Playing) {
                    renderDemo();
                } else if (gameState == GameState::Paused) {
                    renderDemo();
                    // FIXED: Reset view to screen coordinates before rendering pause menu overlay
                    sf::View defaultView = renderer.getRenderWindow().getDefaultView();
                    renderer.getRenderWindow().setView(defaultView);
                    pauseMenu.render(renderer);
                } else if (gameState == GameState::Settings) {
                    if (simulationInitialized) {
                        renderDemo();
                        // FIXED: Reset view to screen coordinates before rendering settings menu overlay
                        sf::View defaultView = renderer.getRenderWindow().getDefaultView();
                        renderer.getRenderWindow().setView(defaultView);
                    } else {
                        renderer.beginFrame(false);  // Don't apply camera for settings menu
                    }
                    settingsMenu.render(renderer);
                    renderer.endFrame();
                }
            } catch (const std::exception& e) {
                LOG_ERROR(std::string("Render error: ") + e.what());
            }
        };
    
        // Run game loop
        engine.run();
        
        // Shutdown
        engine.shutdown();
        
        LOG_INFO("Game Engine closed");
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("Exception: ") + e.what());
        std::cin.get();
    } catch (...) {
        LOG_ERROR("Unknown exception caught");
        std::cin.get();
    }
    
    return 0;
}
