cmake_minimum_required(VERSION 3.15)
project(ONI_Simulation VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find SFML
find_package(SFML 3.1 COMPONENTS Graphics Window System Audio REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/core/Engine.cpp
    src/core/Window.cpp
    src/core/Time.cpp
    src/core/Config.cpp
    src/core/Settings.cpp
    src/ecs/Entity.cpp
    src/ecs/World.cpp
    src/rendering/Renderer.cpp
    src/rendering/TileMap.cpp
    src/rendering/SpriteBatch.cpp
    src/rendering/Camera.cpp
    src/simulation/Grid.cpp
    src/simulation/FluidSim.cpp
    src/simulation/GasSim.cpp
    src/simulation/HeatSim.cpp
    src/simulation/SimulationManager.cpp
    src/simulation/ChunkManager.cpp
    src/simulation/HeatMap.cpp
    src/simulation/ElementTypes.cpp
    src/entities/Character.cpp
    src/entities/Building.cpp
    src/entities/AI/Pathfinding.cpp
    src/systems/BuildingSystem.cpp
    src/systems/ResourceSystem.cpp
    src/systems/CraftingSystem.cpp
    src/ui/Menus/MainMenu.cpp
    src/ui/Menus/SettingsMenu.cpp
    src/ui/Menus/PauseMenu.cpp
    src/ui/UIElements/UISlider.cpp
    src/ui/UIElements/UIButton.cpp
    src/ui/UIElements/UICheckbox.cpp
    src/ui/UIElements/UIProgressBar.cpp
    src/ui/UIElements/UIToggle.cpp
    src/ui/UIElements/UITextInput.cpp
    src/ui/UIElements/UIDropdown.cpp
    src/ui/UIElements/UIRadioButton.cpp
    src/ui/UIElements/UITooltip.cpp
    src/ui/UIElements/UINumberInput.cpp
    src/ui/GameConsole.cpp
    src/ui/UIElements/UIBorder.cpp
    src/ui/UIElements/UIElement.cpp
    src/ui/UIElements/UIElementSelector.cpp
    src/ui/GameGUI.cpp
    src/save/SaveManager.cpp
    src/ui/Menus/LoadGameMenu.cpp
    src/ui/Menus/SaveGameDialog.cpp
    src/ui/UIElements/UIScrollBorder.cpp
    src/ui/UIElements/UISaveEntry.cpp
)

# Header files
set(HEADERS
    src/core/Engine.h
    src/core/Window.h
    src/core/Time.h
    src/core/Config.h
    src/core/Logger.h
    src/core/Settings.h
    src/ecs/Entity.h
    src/ecs/Component.h
    src/ecs/System.h
    src/ecs/World.h
    src/rendering/Renderer.h
    src/rendering/TileMap.h
    src/rendering/SpriteBatch.h
    src/rendering/Camera.h
    src/simulation/Grid.h
    src/simulation/Cell.h
    src/simulation/FluidSim.h
    src/simulation/GasSim.h
    src/simulation/HeatSim.h
    src/simulation/SimulationSystem.h
    src/simulation/SimulationManager.h
    src/simulation/ChunkManager.h
    src/simulation/HeatMap.h
    src/simulation/ElementTypes.h
    src/entities/Character.h
    src/entities/Building.h
    src/entities/Item.h
    src/entities/AI/BehaviorTree.h
    src/entities/AI/Pathfinding.h
    src/systems/BuildingSystem.h
    src/systems/ResourceSystem.h
    src/systems/CraftingSystem.h
    src/ui/Menus/MainMenu.h
    src/ui/Menus/SettingsMenu.h
    src/ui/Menus/PauseMenu.h
    src/ui/UIElements/UISlider.h
    src/ui/UIElements/UIButton.h
    src/ui/UIElements/UICheckbox.h
    src/ui/UIElements/UIProgressBar.h
    src/ui/UIElements/UIToggle.h
    src/ui/UIElements/UITextInput.h
    src/ui/UIElements/UIDropdown.h
    src/ui/UIElements/UIRadioButton.h
    src/ui/UIElements/UITooltip.h
    src/ui/UIElements/UINumberInput.h
    src/ui/GameConsole.h
    src/ui/UIElements/UIBorder.h
    src/ui/UIElements/UIElement.h
    src/ui/UIElements/UIElementSelector.h
    src/ui/GameGUI.h
    src/save/SaveManager.h
    src/ui/Menus/LoadGameMenu.h
    src/ui/Menus/SaveGameDialog.h
    src/ui/UIElements/UIScrollBorder.h
    src/ui/UIElements/UISaveEntry.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link SFML
target_link_libraries(${PROJECT_NAME} sfml-graphics sfml-window sfml-system sfml-audio)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${PROJECT_NAME} PRIVATE "D:/SFML-3.1.0/include")

# Link directories
target_link_directories(${PROJECT_NAME} PRIVATE "D:/SFML-3.1.0/lib")

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy assets directory to build
file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/bin)

# Compiler warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()
