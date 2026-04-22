# Modular Custom Game Engine - ONI-like Simulation Game

A custom C++ game engine for building ONI-like simulation games with fluid/gas dynamics, ECS architecture, and tile-based rendering.

## Features Implemented

### Phase 1: Foundation & Grid Rendering ✓
- **Core Engine**: Game loop, window management, time system, configuration
- **Rendering System**: SFML-based renderer, camera, sprite batching, tile map
- **Grid System**: Interactive grid with mouse placement, multiple tile types
- **Logging System**: Thread-safe singleton logger with multiple log levels

### Phase 2: Simulation Engine ✓
- **Cellular Automaton**: Cell-based simulation with state updates
- **Fluid Simulation**: Gas rises, liquid falls, density-based interactions
- **Temperature System**: Heat diffusion between neighboring cells
- **Element Types**: O2, CO2, Water, Lava, Solid, Contaminated Water

### Phase 3: Entity Component System ✓
- **ECS Architecture**: Entity, Component, System, World implementation
- **Components**: Position, Velocity, Sprite, Health components
- **World Management**: Entity creation/destruction, component storage

### Phase 4-5: (Planned)
- AI behavior trees, pathfinding
- Multi-threaded optimization
- Building, resource, and crafting systems

## Project Structure

```
game_engine/
├── CMakeLists.txt              # Build configuration
├── src/
│   ├── core/                   # Engine core
│   │   ├── Engine.h/cpp        # Main game loop
│   │   ├── Window.h/cpp        # Window management
│   │   ├── Time.h/cpp          # Time/delta time
│   │   ├── Config.h/cpp        # Configuration system
│   │   └── Logger.h            # Logging system
│   ├── ecs/                    # Entity Component System
│   │   ├── Entity.h/cpp        # Entity management
│   │   ├── Component.h         # Component definitions
│   │   ├── System.h            # System base class
│   │   └── World.h/cpp         # World ECS container
│   ├── rendering/              # Rendering systems
│   │   ├── Renderer.h/cpp      # Main renderer
│   │   ├── TileMap.h/cpp       # Tile-based rendering
│   │   ├── SpriteBatch.h/cpp   # Sprite batching
│   │   └── Camera.h/cpp        # Camera system
│   ├── simulation/             # Game simulation
│   │   ├── Grid.h/cpp          # Simulation grid
│   │   ├── Cell.h              # Cell structure
│   │   ├── FluidSim.h/cpp      # Fluid simulation
│   │   └── ElementTypes.h/cpp  # Element definitions
│   ├── entities/               # Game entities
│   ├── systems/                # Game systems
│   └── main.cpp                # Entry point
└── assets/                     # Game assets
    ├── sprites/
    ├── tiles/
    └── fonts/
```

## Dependencies

- **C++17** or higher
- **CMake 3.15+**
- **SFML 2.5+** (graphics, window, system)
- **Visual Studio 2019+** or **MinGW** (Windows)

## Build Instructions

### Windows (Visual Studio)

1. **Install SFML**:
   - Download from https://www.sfml-dev.org/download.php
   - Extract to `C:\SFML` or set environment variable `SFML_DIR`

2. **Install CMake**:
   - Download from https://cmake.org/download/

3. **Build**:
   ```bash
   cd game_engine
   mkdir build
   cd build
   cmake .. -G "Visual Studio 16 2019"
   cmake --build . --config Release
   ```

4. **Run**:
   ```bash
   cd bin\Release
   .\GameEngine.exe
   ```

### Windows (MinGW)

1. **Install MinGW-w64** and add to PATH
2. **Install SFML** for MinGW
3. **Build**:
   ```bash
   cd game_engine
   mkdir build
   cd build
   cmake .. -G "MinGW Makefiles"
   cmake --build .
   ```

### Linux

1. **Install dependencies**:
   ```bash
   sudo apt-get install cmake libsfml-dev
   ```

2. **Build**:
   ```bash
   cd game_engine
   mkdir build && cd build
   cmake ..
   make -j4
   ```

3. **Run**:
   ```bash
   ./bin/GameEngine
   ```

## Controls

- **Left Click**: Place selected element
- **Right Click**: Clear cell
- **1**: Water
- **2**: Lava
- **3**: Oxygen (O2)
- **4**: CO2
- **5**: Solid wall
- **R**: Reset simulation
- **Escape**: Exit

## Demo Features

The included demo showcases:
- Interactive grid with mouse-based tile placement
- Real-time fluid simulation (water flows down, gas rises)
- Temperature diffusion between cells
- Density-based element interactions
- Solid walls with gaps for fluid flow testing
- Multiple element types with different behaviors

## Architecture

### Entity Component System (ECS)

```cpp
// Create entity
Entity entity = world.createEntity();

// Add components
world.addComponent<PositionComponent>(entity.getId(), {100.0f, 200.0f});
world.addComponent<VelocityComponent>(entity.getId(), {10.0f, 0.0f});

// Create and add system
class MovementSystem : public System {
    void update(World& world, float deltaTime) override {
        // Process entities with Position + Velocity
    }
};
world.addSystem(std::make_unique<MovementSystem>());
```

### Fluid Simulation

The simulation uses cellular automaton rules:
- **Gases**: Rise upward, spread diagonally
- **Liquids**: Fall downward, spread horizontally
- **Temperature**: Diffuses between neighbors
- **Density**: Heavier elements displace lighter ones

### Rendering Pipeline

1. Clear screen
2. Apply camera transform
3. Render visible tiles (optimized with view frustum)
4. Render sprites (batched)
5. Render UI overlay

## Performance Considerations

- **Spatial Optimization**: Only renders visible tiles
- **Fixed Timestep**: Deterministic simulation updates
- **Dirty Tracking**: Only updates changed cells
- **Alternating Scan**: Reduces simulation bias

## Next Steps

### Phase 2.3: Optimization
- [ ] Chunked grid updates
- [ ] Multi-threaded simulation
- [ ] Object pooling
- [ ] Spatial partitioning (Quadtree)

### Phase 3: Advanced ECS
- [ ] Movement system
- [ ] Render system for entities
- [ ] Input handling system
- [ ] Character entities with AI

### Phase 4: AI & Pathfinding
- [ ] A* pathfinding algorithm
- [ ] Flow field pathfinding
- [ ] Behavior trees
- [ ] Task assignment system

### Phase 5: Content Systems
- [ ] Building construction/demolition
- [ ] Resource management
- [ ] Crafting/production chains
- [ ] Save/load system

## License

This project is open source and available for educational purposes.

## Credits

Built with:
- SFML - Simple and Fast Multimedia Library
- CMake - Cross-platform build system
- C++17 - Modern C++ features

Inspired by Klei Entertainment's Oxygen Not Included (ONI).
