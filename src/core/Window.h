#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

enum class DisplayMode {
    Windowed,
    Fullscreen,
    BorderlessWindowed
};

struct WindowConfig {
    std::string title = "Game Engine";
    unsigned int width = 1280;
    unsigned int height = 720;
    DisplayMode displayMode = DisplayMode::Windowed;
    bool vsync = true;
    unsigned int antialiasing = 0;
};

class Window {
public:
    Window();
    ~Window();

    void create(const WindowConfig& config);
    void close();

    bool isOpen() const;
    std::optional<sf::Event> pollEvent();
    void display();
    void clear(const sf::Color& color = sf::Color::Black);

    sf::RenderWindow& getRenderWindow();
    sf::Vector2u getSize() const;
    sf::Vector2f getCenter() const;

    void setVSync(bool enabled);
    void setFullscreen(bool fullscreen);
    void setBorderless(bool borderless);
    void applySettings(const WindowConfig& newConfig);

private:
    sf::RenderWindow renderWindow;
    WindowConfig config;
    bool isCreated;
};
