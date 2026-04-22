#include "core/Engine.h"
#include "core/Logger.h"
#include "rendering/Renderer.h"
#include "rendering/TileMap.h"
#include "simulation/Grid.h"
#include "simulation/FluidSim.h"
#include "simulation/ElementTypes.h"
#include "ecs/Entity.h"
#include <SFML/Graphics.hpp>

#define DEBUG false  // Set to true to enable debug output

// Global variables for demo
Renderer renderer;
TileMap tileMap;
Grid simGrid;
FluidSim fluidSim;

ElementType currentElement = ElementType::Liquid_Water;
sf::Font font;
sf::Text* infoText = nullptr;

// Forward declarations
void syncTileMap();

void initializeDemo() {
    if (DEBUG) LOG_INFO("Initializing demo");
    
    // Initialize tile map (40x30 grid with 32px tiles)
    tileMap.initialize(40, 30, 32.0f);
    
    // Initialize simulation grid
    simGrid.initialize(40, 30);
    fluidSim.initialize(simGrid);
    
    // Create some solid walls
    for (int x = 0; x < 40; ++x) {
        simGrid.setCellType(x, 0, ElementType::Solid);
        simGrid.setCellType(x, 29, ElementType::Solid);
    }
    for (int y = 0; y < 30; ++y) {
        simGrid.setCellType(0, y, ElementType::Solid);
        simGrid.setCellType(39, y, ElementType::Solid);
    }
    
    // Add some internal walls
    for (int x = 10; x < 30; ++x) {
        simGrid.setCellType(x, 15, ElementType::Solid);
    }
    simGrid.setCellType(15, 15, ElementType::Empty);
    simGrid.setCellType(16, 15, ElementType::Empty);
    simGrid.setCellType(17, 15, ElementType::Empty);
    
    // Add some test water and gas
    for (int x = 5; x < 10; ++x) {
        simGrid.setCellType(x, 28, ElementType::Liquid_Water);
    }
    for (int x = 30; x < 35; ++x) {
        simGrid.setCellType(x, 5, ElementType::Gas_O2);
    }
    
    // Update all cell colors
    for (int y = 0; y < simGrid.getHeight(); ++y) {
        for (int x = 0; x < simGrid.getWidth(); ++x) {
            Cell& cell = simGrid.getCell(x, y);
            cell.updateColor();
        }
    }
    
    // Load font
    if (!font.openFromFile("assets/fonts/arial.ttf")) {
        LOG_ERROR("Failed to load font!");
    }
    infoText = new sf::Text(font, "", 20);
    infoText->setFillColor(sf::Color::White);
    infoText->setPosition(sf::Vector2f(10, 10));
    
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
                    // Place element
                    simGrid.setCellType(tileX, tileY, currentElement);
                    
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
                    }
                    
                    TileInfo tileInfo;
                    tileInfo.color = color;
                    tileInfo.solid = (currentElement == ElementType::Solid);
                    tileInfo.name = name;
                    tileMap.setTile(tileX, tileY, tileInfo);
                    if (DEBUG) std::cout << "Placed " << name << " at (" << tileX << ", " << tileY << ")" << std::endl;
                } else if (mouseButton->button == sf::Mouse::Button::Right) {
                    // Clear cell
                    simGrid.setCellType(tileX, tileY, ElementType::Empty);
                    TileInfo emptyTile;
                    emptyTile.color = sf::Color::Transparent;
                    emptyTile.solid = false;
                    emptyTile.name = "Empty";
                    tileMap.setTile(tileX, tileY, emptyTile);
                }
            }
        }
    }
}

void syncTileMap() {
    int height = simGrid.getHeight();
    int width = simGrid.getWidth();
    
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!simGrid.isValidPosition(x, y)) continue;
            
            const Cell& cell = simGrid.getCell(x, y);
            TileInfo tileInfo;
            tileInfo.color = cell.color;
            tileInfo.solid = (cell.elementType == ElementType::Solid);
            tileInfo.name = ElementTypes::getTypeName(cell.elementType);
            tileMap.setTile(x, y, tileInfo);
        }
    }
}

void updateSimulation(float deltaTime) {
    // Update fluid simulation
    fluidSim.update(simGrid, deltaTime);
    
    // Sync tilemap with simulation grid for visualization
    syncTileMap();
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
        infoText->setString("Element: " + elementName + " | Left Click: Place | Right Click: Clear | 1-5: Change Element | R: Reset | ESC: Quit");
        renderer.drawText(*infoText);
    }
    
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
                case sf::Keyboard::Scancode::R:
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
                    break;
                case sf::Keyboard::Scancode::Escape:
                    engine.stop();
                    break;
                    }
                }
            }
            
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
