#include "Window.h"
#include "Logger.h"

Window::Window()
    : isCreated(false)
{
}

Window::~Window() {
    close();
}

void Window::create(const WindowConfig& windowConfig) {
    config = windowConfig;

    sf::ContextSettings settings;
    settings.antiAliasingLevel = config.antialiasing;

    sf::State windowState;
    sf::Vector2u windowSize(config.width, config.height);
    
    switch (config.displayMode) {
        case DisplayMode::Fullscreen:
            windowState = sf::State::Fullscreen;
            break;
        case DisplayMode::BorderlessWindowed:
            windowState = sf::State::Windowed;
            windowSize = sf::VideoMode::getDesktopMode().size;
            break;
        case DisplayMode::Windowed:
        default:
            windowState = sf::State::Windowed;
            break;
    }

    renderWindow.create(
        sf::VideoMode(windowSize),
        config.title,
        windowState,
        settings
    );
    
    // Position borderless window at (0,0)
    if (config.displayMode == DisplayMode::BorderlessWindowed) {
        renderWindow.setPosition(sf::Vector2i(0, 0));
    }

    renderWindow.setVerticalSyncEnabled(config.vsync);

    isCreated = true;
    LOG_INFO("Window created: " + std::to_string(config.width) + "x" + std::to_string(config.height) + 
             " (" + (config.displayMode == DisplayMode::Fullscreen ? "Fullscreen" :
                    config.displayMode == DisplayMode::BorderlessWindowed ? "Borderless" : "Windowed") + ")");
}

void Window::close() {
    if (isCreated) {
        renderWindow.close();
        isCreated = false;
        LOG_INFO("Window closed");
    }
}

bool Window::isOpen() const {
    return isCreated && renderWindow.isOpen();
}

std::optional<sf::Event> Window::pollEvent() {
    return renderWindow.pollEvent();
}

void Window::display() {
    renderWindow.display();
}

void Window::clear(const sf::Color& color) {
    renderWindow.clear(color);
}

sf::RenderWindow& Window::getRenderWindow() {
    return renderWindow;
}

sf::Vector2u Window::getSize() const {
    return renderWindow.getSize();
}

sf::Vector2f Window::getCenter() const {
    sf::Vector2u size = renderWindow.getSize();
    return sf::Vector2f(size.x / 2.0f, size.y / 2.0f);
}

void Window::setVSync(bool enabled) {
    config.vsync = enabled;
    renderWindow.setVerticalSyncEnabled(enabled);
}

void Window::setFullscreen(bool fullscreen) {
    if (fullscreen && config.displayMode != DisplayMode::Fullscreen) {
        config.displayMode = DisplayMode::Fullscreen;
        close();
        create(config);
    } else if (!fullscreen && config.displayMode == DisplayMode::Fullscreen) {
        config.displayMode = DisplayMode::Windowed;
        close();
        create(config);
    }
}

void Window::setBorderless(bool borderless) {
    if (borderless && config.displayMode != DisplayMode::BorderlessWindowed) {
        config.displayMode = DisplayMode::BorderlessWindowed;
        close();
        create(config);
    } else if (!borderless && config.displayMode == DisplayMode::BorderlessWindowed) {
        config.displayMode = DisplayMode::Windowed;
        close();
        create(config);
    }
}

void Window::applySettings(const WindowConfig& newConfig) {
    config = newConfig;
    close();
    create(config);
}
