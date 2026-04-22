#include "Engine.h"
#include "Logger.h"

Engine::Engine()
    : running(false)
    , initialized(false)
{
}

Engine::~Engine() {
    shutdown();
}

void Engine::initialize(const WindowConfig& windowConfig) {
    if (initialized) {
        LOG_WARNING("Engine already initialized");
        return;
    }

    window.create(windowConfig);
    time.update();

    initialized = true;
    running = true;

    LOG_INFO("Engine initialized");
}

void Engine::run() {
    if (!initialized) {
        LOG_ERROR("Engine not initialized");
        return;
    }

    LOG_INFO("Starting game loop");

    while (running && window.isOpen()) {
        processEvents();
        update(time.getDeltaTime());
        render();
        time.update();
        window.display();
    }

    LOG_INFO("Game loop ended");
}

void Engine::shutdown() {
    if (initialized) {
        window.close();
        initialized = false;
        running = false;
        LOG_INFO("Engine shutdown");
    }
}

bool Engine::isRunning() const {
    return running && initialized;
}

void Engine::stop() {
    running = false;
}

Window& Engine::getWindow() {
    return window;
}

Time& Engine::getTime() {
    return time;
}

void Engine::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            running = false;
        }

        if (onEvent) {
            onEvent(event);
        }
    }
}

void Engine::update(float deltaTime) {
    if (onUpdate) {
        onUpdate();
    }
}

void Engine::render() {
    window.clear(sf::Color::Black);

    if (onRender) {
        onRender();
    }
}
