#pragma once

#include "Window.h"
#include "Time.h"
#include <functional>
#include <memory>

class Engine {
public:
    Engine();
    ~Engine();

    void initialize(const WindowConfig& windowConfig);
    void run();
    void shutdown();

    bool isRunning() const;
    void stop();

    Window& getWindow();
    Time& getTime();

    // Callbacks
    std::function<void()> onUpdate;
    std::function<void()> onRender;
    std::function<void(const sf::Event&)> onEvent;

private:
    void processEvents();
    void update(float deltaTime);
    void render();

    Window window;
    Time time;
    bool running;
    bool initialized;
};
