#include "core/Engine.h"
#include "core/Logger.h"
#include "rendering/Renderer.h"
#include "rendering/TileMap.h"
#include "simulation/Grid.h"
#include "simulation/FluidSim.h"
#include "simulation/ElementTypes.h"
#include "ecs/Entity.h"
#include <SFML/Graphics.hpp>

// Global variables for demo
Renderer renderer;
TileMap tileMap;
Grid simGrid;
FluidSim fluidSim;

ElementType currentElement = ElementType::Liquid_Water;
sf::Font font;
sf::Text infoText;

void initializeDemo() {
    LOG_INFO("Initializing demo");
    
    // Initialize renderer
    renderer.initialize(Engine().getWindow().getRenderWindow());
    
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
    
    // Load font
    font.loadFromFile("assets/fonts/arial.ttf");
    infoText.setFont(font);
    infoText.setCharacterSize(20);
    infoText.setFillColor(sf::Color::White);
    infoText.setPosition(10, 10);
    
    LOG_INFO("Demo initialized");
}

void handleMouseInput(sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(renderer.getRenderWindow());
        sf::Vector2f worldPos = renderer.getCamera().screenToWorld(mousePos.x, mousePos.y);
        sf::Vector2i tilePos = tileMap.getTileAt(worldPos.x, worldPos.y);
        
        if (simGrid.isValidPosition(tilePos.x, tilePos.y)) {
            if (event.mouseButton.button == sf::Mouse::Left) {
                // Place element
                simGrid.setCellType(tilePos.x, tilePos.y, currentElement);
            } else if (event.mouseButton.button == sf::Mouse::Right) {
                // Clear cell
                simGrid.setCellType(tilePos.x, tilePos.y, ElementType::Empty);
            }
        }
    }
}

void updateSimulation(float deltaTime) {
    // Update fluid simulation
    fluidSim.update(simGrid, deltaTime);
    
    // Sync tilemap with simulation grid
    for (int y = 0; y < simGrid.getHeight(); ++y) {
        for (int x = 0; x < simGrid.getWidth(); ++x) {
            const Cell& cell = simGrid.getCell(x, y);
            TileInfo tileInfo;
            tileInfo.color = cell.color;
            tileInfo.solid = (cell.elementType == ElementType::Solid);
            tileInfo.name = ElementTypes::getTypeName(cell.elementType);
            tileMap.setTile(x, y, tileInfo);
        }
    }
}

void renderDemo() {
    renderer.beginFrame();
    
    // Draw tile map
    renderer.drawTileMap(tileMap);
    
    // Draw info text
    std::string elementName = ElementTypes::getTypeName(currentElement);
    infoText.setString("Left Click: Place " + elementName + " | Right Click: Clear | 1-5: Change Element | R: Reset");
    renderer.drawText(infoText);
    
    renderer.endFrame();
}

int main() {
    LOG_INFO("Starting Game Engine");
    
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
    
    // Initialize demo
    initializeDemo();
    renderer.setCamera(renderer.getCamera());
    
    // Set up callbacks
    engine.onEvent = [&engine](const sf::Event& event) {
        // Handle keyboard input
        if (event.type == sf::Event::KeyPressed) {
            switch (event.key.code) {
                case sf::Keyboard::Num1:
                    currentElement = ElementType::Liquid_Water;
                    break;
                case sf::Keyboard::Num2:
                    currentElement = ElementType::Liquid_Lava;
                    break;
                case sf::Keyboard::Num3:
                    currentElement = ElementType::Gas_O2;
                    break;
                case sf::Keyboard::Num4:
                    currentElement = ElementType::Gas_CO2;
                    break;
                case sf::Keyboard::Num5:
                    currentElement = ElementType::Solid;
                    break;
                case sf::Keyboard::R:
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
                case sf::Keyboard::Escape:
                    engine.stop();
                    break;
            }
        }
        
        handleMouseInput(event);
    };
    
    engine.onUpdate = [&engine](void) {
        updateSimulation(engine.getTime().getDeltaTime());
    };
    
    engine.onRender = []() {
        renderDemo();
    };
    
    // Run game loop
    engine.run();
    
    // Shutdown
    engine.shutdown();
    
    LOG_INFO("Game Engine closed");
    
    return 0;
}
