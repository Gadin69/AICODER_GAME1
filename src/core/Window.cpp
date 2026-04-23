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
        {
            windowState = sf::State::Fullscreen;
            // In fullscreen, validate and use the configured resolution
            // Get available video modes and find closest match
            auto modes = sf::VideoMode::getFullscreenModes();
            bool found = false;
            for (const auto& mode : modes) {
                if (mode.size.x == config.width && mode.size.y == config.height) {
                    windowSize = mode.size;
                    found = true;
                    break;
                }
            }
            if (!found) {
                // If exact match not found, use desktop mode
                LOG_WARNING("Requested resolution " + std::to_string(config.width) + "x" + 
                           std::to_string(config.height) + " not supported in fullscreen, using desktop mode");
                windowSize = sf::VideoMode::getDesktopMode().size;
            }
            break;
        }
        case DisplayMode::BorderlessWindowed:
        {
            windowState = sf::State::Windowed;
            // Borderless uses desktop resolution
            windowSize = sf::VideoMode::getDesktopMode().size;
            break;
        }
        case DisplayMode::Windowed:
        default:
        {
            windowState = sf::State::Windowed;
            // In windowed mode, ensure it fits on screen
            // Cap to 80% of desktop size or the configured size, whichever is smaller
            sf::Vector2u desktopSize = sf::VideoMode::getDesktopMode().size;
            unsigned int maxWidth = static_cast<unsigned int>(desktopSize.x * 0.8f);
            unsigned int maxHeight = static_cast<unsigned int>(desktopSize.y * 0.8f);
            windowSize.x = std::min(config.width, maxWidth);
            windowSize.y = std::min(config.height, maxHeight);
            break;
        }
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
    
    // DEBUG: Log actual window size after creation
    sf::Vector2u actualSize = renderWindow.getSize();
    sf::Vector2u desktopSize = sf::VideoMode::getDesktopMode().size;
    std::string displayModeStr = (config.displayMode == DisplayMode::Fullscreen) ? "Fullscreen" :
                                 (config.displayMode == DisplayMode::BorderlessWindowed) ? "Borderless" : "Windowed";
    
    LOG_INFO("=== WINDOW DEBUG ===");
    LOG_INFO("Config resolution: " + std::to_string(config.width) + "x" + std::to_string(config.height));
    LOG_INFO("Desktop resolution: " + std::to_string(desktopSize.x) + "x" + std::to_string(desktopSize.y));
    LOG_INFO("Requested windowSize: " + std::to_string(windowSize.x) + "x" + std::to_string(windowSize.y));
    LOG_INFO("Actual window size after create: " + std::to_string(actualSize.x) + "x" + std::to_string(actualSize.y));
    LOG_INFO("Display mode: " + displayModeStr);
    LOG_INFO("====================");
    
    LOG_INFO("Window created: " + std::to_string(windowSize.x) + "x" + std::to_string(windowSize.y) + 
             " (" + displayModeStr + ")");
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
