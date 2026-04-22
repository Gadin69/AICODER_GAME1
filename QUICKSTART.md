# Quick Start Guide

## Prerequisites

1. **Install CMake**: https://cmake.org/download/
2. **Install SFML 2.5+**: https://www.sfml-dev.org/download.php
   - For Windows: Download the VC15 (2017) - 64-bit version
   - Extract to `C:\SFML` or set `SFML_DIR` environment variable

## Building (Windows)

### Option 1: Using Build Script (Easiest)
```bash
# Double-click build.bat or run:
.\build.bat
```

### Option 2: Manual Build
```bash
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

## Running

```bash
cd build\bin\Release
.\GameEngine.exe
```

## Demo Controls

Once the engine is running:

- **Left Click**: Place selected element on grid
- **Right Click**: Clear cell
- **Number Keys**:
  - `1` - Water (blue liquid, flows down)
  - `2` - Lava (orange liquid, hot, flows down)
  - `3` - Oxygen/O2 (blue gas, rises)
  - `4` - CO2 (gray gas, rises)
  - `5` - Solid wall (gray, blocks movement)
- **R** - Reset simulation
- **Escape** - Exit

## What to Expect

The demo shows:
1. A 40x30 grid with solid borders
2. A horizontal wall in the middle with a small gap
3. Click to place elements and watch them simulate:
   - Water falls and pools
   - Gas rises to the top
   - Lava flows and heats nearby cells
   - Temperature diffuses between cells

## Troubleshooting

### CMake can't find SFML
- Set environment variable: `set SFML_DIR=C:\Path\To\SFML`
- Or pass to CMake: `cmake .. -DSFML_DIR=C:\Path\To\SFML`

### Build fails
- Make sure you have Visual Studio 2019 or later installed
- Check that SFML is the 64-bit version
- Try: `cmake .. -G "Visual Studio 17 2022" -A x64`

### Font not loading
- The demo requires a font file at `assets/fonts/arial.ttf`
- You can copy any TTF font file to this location
- Or comment out the font loading lines in main.cpp

## Next Steps

1. Explore the code in `src/`
2. Modify element properties in `simulation/ElementTypes.cpp`
3. Add new components in `ecs/Component.h`
4. Create new systems by extending `ecs/System.h`

## Project Structure

```
src/
├── core/          - Engine foundation (Engine, Window, Time, Config)
├── ecs/           - Entity Component System
├── rendering/     - Rendering (Renderer, Camera, TileMap, SpriteBatch)
├── simulation/    - Game simulation (Grid, Cell, FluidSim)
├── entities/      - Game entities (Characters, Buildings, AI)
├── systems/       - Game systems (Building, Resource, Crafting)
└── main.cpp       - Entry point with demo
```

## Architecture Highlights

### Fluid Simulation
- Cellular automaton-based
- Updates at fixed intervals (configurable)
- Alternating scan direction for variety
- Density-based element interactions

### ECS Pattern
- Entities are just IDs
- Components are pure data
- Systems contain logic
- World manages everything

### Rendering
- SFML-based 2D rendering
- Camera with zoom and pan
- Tile-based rendering (only visible tiles)
- Sprite batching for efficiency

Have fun building your ONI-like game!
