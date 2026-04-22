#include "core/Engine.h"
#include "core/Logger.h"
#include "core/Settings.h"
#include "rendering/Renderer.h"
#include "rendering/TileMap.h"
#include "simulation/Grid.h"
#include "simulation/SimulationManager.h"
#include "simulation/ElementTypes.h"
#include "ui/MainMenu.h"
#include "ecs/Entity.h"
#include <SFML/Graphics.hpp>

#define DEBUG false  // Set to true to enable debug console output
#define DEVELOPER_MODE true  // Set to true to enable admin/dev tools (sliders, overlays, etc.)

// Global variables for demo
Renderer renderer;
TileMap tileMap;
Grid simGrid;
SimulationManager simManager;
MainMenu mainMenu;

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

// UI Slider for simulation time step
struct UISlider {
    sf::RectangleShape track;
    sf::RectangleShape thumb;
    sf::Text* label = nullptr;
    sf::Text* valueText = nullptr;
    sf::Font* fontPtr = nullptr;
    float minValue;
    float maxValue;
    float currentValue;
    bool isDragging = false;
    bool initialized = false;
    
    void initialize(float x, float y, float width, float minVal, float maxVal, float defaultVal, const std::string& labelText, const sf::Font& font) {
        minValue = minVal;
        maxValue = maxVal;
        currentValue = defaultVal;
        fontPtr = const_cast<sf::Font*>(&font);
        
        // Track background
        track.setSize(sf::Vector2f(width, 20));
        track.setPosition(sf::Vector2f(x, y));
        track.setFillColor(sf::Color(50, 50, 50));
        track.setOutlineColor(sf::Color(100, 100, 100));
        track.setOutlineThickness(2.0f);
        
        // Thumb (draggable part)
        thumb.setSize(sf::Vector2f(15, 30));
        updateThumbPosition();
        thumb.setFillColor(sf::Color(100, 150, 255));
        thumb.setOutlineColor(sf::Color(150, 200, 255));
        thumb.setOutlineThickness(1.0f);
        
        // Label
        label = new sf::Text(font, labelText, 16);
        label->setFillColor(sf::Color::White);
        label->setPosition(sf::Vector2f(x, y - 25));
        
        // Value text
        valueText = new sf::Text(font, "", 14);
        valueText->setFillColor(sf::Color(200, 200, 200));
        valueText->setPosition(sf::Vector2f(x + width + 10, y - 2));
        updateValueText();
        
        initialized = true;
    }
    
    void updateThumbPosition() {
        float ratio = (currentValue - minValue) / (maxValue - minValue);
        float thumbX = track.getPosition().x + ratio * (track.getSize().x - thumb.getSize().x);
        thumb.setPosition(sf::Vector2f(thumbX, track.getPosition().y - 5));
    }
    
    void updateValueText() {
        if (valueText) {
            valueText->setString(std::to_string(currentValue).substr(0, 4) + "s");
        }
    }
    
    void handleMousePress(const sf::Vector2f& mousePos) {
        sf::FloatRect thumbBounds = thumb.getGlobalBounds();
        if (thumbBounds.contains(mousePos)) {
            isDragging = true;
        }
    }
    
    void handleMouseRelease() {
        isDragging = false;
    }
    
    void handleMouseMove(const sf::Vector2f& mousePos) {
        if (!isDragging) return;
        
        float trackLeft = track.getPosition().x;
        float trackRight = track.getPosition().x + track.getSize().x;
        
        // Clamp mouse position to track bounds
        float clampedX = std::max(trackLeft, std::min(trackRight, mousePos.x));
        
        // Calculate ratio and update value
        float ratio = (clampedX - trackLeft) / (trackRight - trackLeft);
        currentValue = minValue + ratio * (maxValue - minValue);
        
        updateThumbPosition();
        updateValueText();
    }
    
    void render(Renderer& renderer) {
        if (!initialized) return;
        renderer.drawRectangle(track);
        renderer.drawRectangle(thumb);
        if (label) renderer.drawText(*label);
        if (valueText) renderer.drawText(*valueText);
    }
};

UISlider* timeStepSlider = nullptr;
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
    if (DEBUG) LOG_INFO("Initializing demo");
    
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
    
    simulationInitialized = true;
    if (DEBUG) LOG_INFO("Demo initialized with grid: " + std::to_string(settings.gridWidth) + "x" + std::to_string(settings.gridHeight));
}

void initializeFonts() {
    // Load font
    if (!font.openFromFile("assets/fonts/arial.ttf")) {
        LOG_ERROR("Failed to load font!");
    }
    infoText = new sf::Text(font, "", 20);
    infoText->setFillColor(sf::Color::White);
    infoText->setPosition(sf::Vector2f(10, 10));
    
    // Initialize developer tools (only in DEVELOPER_MODE)
#if DEVELOPER_MODE
    if (isAdminMode) {
        timeStepSlider = new UISlider();
        timeStepSlider->initialize(10, 960, 300, 0.01f, 0.5f, 0.05f, "Sim Time Step:", font);
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
    
    if (DEBUG) std::cout << "Tilemap setup complete" << std::endl;
    
    if (DEBUG) LOG_INFO("Demo initialized");
}

void handleMouseInput(const sf::Event& event) {
    if (event.is<sf::Event::MouseButtonPressed>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseButton) {
            // Get mouse position in screen coordinates
            sf::Vector2i mousePos = sf::Mouse::getPosition(renderer.getRenderWindow());
            
            // Convert to tile coordinates (32px tiles)
            int tileX = mousePos.x / 32;
            int tileY = mousePos.y / 32;
            
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
                    
                    if (DEBUG) std::cout << "Placed " << name << " at (" << tileX << ", " << tileY << ")" << std::endl;
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
    if (!showElementInspector || !inspectorText) return;
    
    // Get mouse position
    sf::Vector2i mousePos = sf::Mouse::getPosition(renderer.getRenderWindow());
    int tileX = mousePos.x / 32;
    int tileY = mousePos.y / 32;
    
    if (simGrid.isValidPosition(tileX, tileY)) {
        // THREAD-SAFE: Lock grid before reading cell data
        simGrid.lock();
        
        const Cell& cell = simGrid.getCell(tileX, tileY);
        ElementProperties props = ElementTypes::getElement(cell.elementType);
        
        // Build inspection string with metric and imperial
        std::string info = "Position: (" + std::to_string(tileX) + ", " + std::to_string(tileY) + ")\n";
        info += "Element: " + props.name + "\n";
        
        // Temperature in Celsius and Fahrenheit
        float tempF = (cell.temperature * 9.0f / 5.0f) + 32.0f;
        info += "Temperature: " + std::to_string((int)cell.temperature) + "°C / " + std::to_string((int)tempF) + "°F\n";
        
        if (cell.elementType != ElementType::Empty) {
            // Density in kg/m³ and lb/ft³
            float densityImperial = props.density * 0.06243f;
            info += "Density: " + std::to_string((int)props.density) + " kg/m³ / " + std::to_string((int)densityImperial) + " lb/ft³\n";
            info += "Phase: ";
            if (props.isSolid) info += "Solid";
            else if (props.isLiquid) info += "Liquid";
            else if (props.isGas) info += "Gas";
            info += "\n";
            
            if (props.isLiquid || props.isSolid) {
                float meltF = (props.meltingPoint * 9.0f / 5.0f) + 32.0f;
                info += "Melting Point: " + std::to_string((int)props.meltingPoint) + "°C / " + std::to_string((int)meltF) + "°F\n";
            }
            if (props.isLiquid || props.isGas) {
                float boilF = (props.boilingPoint * 9.0f / 5.0f) + 32.0f;
                info += "Boiling Point: " + std::to_string((int)props.boilingPoint) + "°C / " + std::to_string((int)boilF) + "°F\n";
            }
            
            // Thermal conductivity in W/(m·K) and BTU/(hr·ft·°F)
            float kImperial = props.thermalConductivity * 0.5779f;
            info += "Thermal Conductivity: " + std::to_string(props.thermalConductivity).substr(0, 4) + " W/(m·K) / " + 
                    std::to_string(kImperial).substr(0, 4) + " BTU/(hr·ft·°F)\n";
            
            // Specific heat in J/(kg·K) and BTU/(lb·°F)
            float cpImperial = props.specificHeatCapacity * 0.000238846f;
            info += "Specific Heat: " + std::to_string((int)props.specificHeatCapacity) + " J/(kg·K) / " + 
                    std::to_string(cpImperial).substr(0, 5) + " BTU/(lb·°F)";
        }
        
        inspectorText->setString(info);
        
        // UNLOCK grid after reading
        simGrid.unlock();
        
        // Position near mouse but not under it
        float textX = mousePos.x + 20;
        float textY = mousePos.y - 10;
        
        // Keep on screen
        if (textX + 300 > 1280) textX = mousePos.x - 320;
        if (textY + 200 > 720) textY = 720 - 200;
        
        inspectorText->setPosition(sf::Vector2f(textX, textY));
    } else {
        inspectorText->setString("");
    }
#endif
}

void updateSimulation(float deltaTime) {
    // Apply admin time step override if in admin mode
    float simDeltaTime = deltaTime;
    if (isAdminMode && timeStepSlider) {
        simDeltaTime = timeStepSlider->currentValue;
    }
    
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
        renderer.drawText(*infoText);
    }
    
    // Render admin UI (only in admin mode)
    if (isAdminMode && timeStepSlider) {
        timeStepSlider->render(renderer);
    }
    
    // Render element inspector (dev mode)
#if DEVELOPER_MODE
    if (showElementInspector && inspectorText) {
        renderer.drawText(*inspectorText);
    }
#endif
    
    renderer.endFrame();
}

int main() {
    try {
        // Set logger to only show warnings and errors (set to DEBUG to see everything)
        if (!DEBUG) {
            Logger::getInstance().setLogLevel(LogLevel::WARNING);
        }
        
        if (DEBUG) std::cout << "=== Game Engine Starting ===" << std::endl;
        if (DEBUG) LOG_INFO("Starting Game Engine");
        
        // Create engine
        Engine engine;
        
        // Configure window
        WindowConfig config;
        config.title = "ONI-like Game Engine";
        config.width = 1280;
        config.height = 720;
        config.vsync = true;
        
        // Initialize engine
        engine.initialize(config);
        
        if (DEBUG) LOG_INFO("Engine initialized, setting up renderer...");
        
        // Initialize renderer with engine window
        renderer.initialize(engine.getWindow().getRenderWindow());
        
        if (DEBUG) LOG_INFO("Renderer initialized, setting up demo...");
        
        // Initialize demo
        initializeDemo();
        
        if (DEBUG) LOG_INFO("Demo initialized, starting game loop...");
        
        // Setup camera to show the tile grid (40x30 tiles at 32px = 1280x960)
        Camera& cam = renderer.getCamera();
        cam.setPosition(640.0f, 480.0f);  // Center of 1280x960 grid
        renderer.setCamera(cam);
        
        // Set up callbacks
        engine.onEvent = [&engine](const sf::Event& event) {
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
                    // THREAD-SAFE: Lock grid before resetting
                    simGrid.lock();
                    
                    simGrid.clear();
                    // Rebuild walls
                    for (int x = 0; x < 40; ++x) {
                        simGrid.setCellType(x, 0, ElementType::Solid);
                        simGrid.setCellType(x, 29, ElementType::Solid);
                    }
                    for (int y = 0; y < 30; ++y) {
                        simGrid.setCellType(0, y, ElementType::Solid);
                        simGrid.setCellType(39, y, ElementType::Solid);
                    }
                    for (int x = 10; x < 30; ++x) {
                        simGrid.setCellType(x, 15, ElementType::Solid);
                    }
                    simGrid.setCellType(15, 15, ElementType::Empty);
                    simGrid.setCellType(16, 15, ElementType::Empty);
                    simGrid.setCellType(17, 15, ElementType::Empty);
                    
                    // UNLOCK grid
                    simGrid.unlock();
                    break;
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
                    engine.stop();
                    break;
                    }
                }
            }
            
            // Handle slider input (developer mode only)
#if DEVELOPER_MODE
            if (isAdminMode && timeStepSlider) {
                if (event.is<sf::Event::MouseButtonPressed>()) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(renderer.getRenderWindow());
                    timeStepSlider->handleMousePress(sf::Vector2f(mousePos.x, mousePos.y));
                } else if (event.is<sf::Event::MouseButtonReleased>()) {
                    timeStepSlider->handleMouseRelease();
                } else if (event.is<sf::Event::MouseMoved>()) {
                    auto mouseMove = event.getIf<sf::Event::MouseMoved>();
                    if (mouseMove) {
                        timeStepSlider->handleMouseMove(sf::Vector2f(mouseMove->position.x, mouseMove->position.y));
                    }
                }
            }
#endif
            
            handleMouseInput(event);
        };
        
        engine.onUpdate = [&engine](void) {
            updateSimulation(engine.getTime().getDeltaTime());
        };
        
        engine.onRender = []() {
            try {
                renderDemo();
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
