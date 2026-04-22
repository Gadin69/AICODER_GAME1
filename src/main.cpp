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
sf::Text* infoText = nullptr;

void initializeDemo() {
    LOG_INFO("Initializing demo");
    
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
    if (!font.openFromFile("assets/fonts/arial.ttf")) {
        LOG_ERROR("Failed to load font!");
    }
    infoText = new sf::Text(font, "", 20);
    infoText->setFillColor(sf::Color::White);
    infoText->setPosition(sf::Vector2f(10, 10));
    
    LOG_INFO("Demo initialized");
}

void handleMouseInput(const sf::Event& event) {
    if (event.is<sf::Event::MouseButtonPressed>()) {
        auto mouseButton = event.getIf<sf::Event::MouseButtonPressed>();
        if (mouseButton) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(renderer.getRenderWindow());
            sf::Vector2f worldPos = renderer.getCamera().screenToWorld(mousePos.x, mousePos.y);
            sf::Vector2i tilePos = tileMap.getTileAt(worldPos.x, worldPos.y);
            
            if (simGrid.isValidPosition(tilePos.x, tilePos.y)) {
                if (mouseButton->button == sf::Mouse::Button::Left) {
                    // Place element
                    simGrid.setCellType(tilePos.x, tilePos.y, currentElement);
                } else if (mouseButton->button == sf::Mouse::Button::Right) {
                    // Clear cell
                    simGrid.setCellType(tilePos.x, tilePos.y, ElementType::Empty);
                }
            }
        }
    }
}

void updateSimulation(float deltaTime) {
    // Update fluid simulation
    fluidSim.update(simGrid, deltaTime);
    
    // TODO: Sync tilemap with simulation grid for visualization
    // Currently disabled - needs proper color updating in fluidSim
}

void renderDemo() {
    renderer.beginFrame();
    
    // Draw tile map
    renderer.drawTileMap(tileMap);
    
    // Draw info text
    std::string elementName = ElementTypes::getTypeName(currentElement);
    if (infoText) {
        infoText->setString("Left Click: Place " + elementName + " | Right Click: Clear | 1-5: Change Element | R: Reset");
        renderer.drawText(*infoText);
    }
    
    renderer.endFrame();
}

int main() {
    try {
        std::cout << "=== Game Engine Starting ===" << std::endl;
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
        
        LOG_INFO("Engine initialized, setting up renderer...");
        
        // Initialize renderer with engine window
        renderer.initialize(engine.getWindow().getRenderWindow());
        
        LOG_INFO("Renderer initialized, setting up demo...");
        
        // Initialize demo
        initializeDemo();
        
        LOG_INFO("Demo initialized, starting game loop...");
        
        // Setup camera to match window size
        sf::Vector2u windowSize = engine.getWindow().getSize();
        Camera& cam = renderer.getCamera();
        cam.setPosition(windowSize.x / 2.0f, windowSize.y / 2.0f);
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
