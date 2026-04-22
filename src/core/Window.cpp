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

    renderWindow.create(
        sf::VideoMode(sf::Vector2u(config.width, config.height)),
        config.title,
        config.fullscreen ? sf::State::Fullscreen : sf::State::Windowed,
        settings
    );

    renderWindow.setVerticalSyncEnabled(config.vsync);

    isCreated = true;
    LOG_INFO("Window created: " + std::to_string(config.width) + "x" + std::to_string(config.height));
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
    if (fullscreen != config.fullscreen) {
        config.fullscreen = fullscreen;
        close();
        create(config);
    }
}
