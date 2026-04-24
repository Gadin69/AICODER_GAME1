#include "GameConsole.h"
#include "../rendering/Renderer.h"
#include <iostream>
#include <algorithm>
#include <sstream>

GameConsole::GameConsole() {}

GameConsole::~GameConsole() {}

void GameConsole::initialize(float x, float y, float width, float height, const sf::Font& font) {
    fontPtr = &font;
    
    // Background
    background.setSize(sf::Vector2f(width, height));
    background.setPosition(sf::Vector2f(x, y));
    background.setFillColor(sf::Color(20, 20, 30, 230));  // Dark semi-transparent
    background.setOutlineThickness(2.0f);
    background.setOutlineColor(sf::Color(100, 100, 150));
    
    // Input field at bottom
    inputField.initialize(x + 10, y + height - 40, width - 20, 30, "> ", font);
    inputField.setMaxLength(200);
    inputField.setCallback([this](const std::string& command) {
        processCommand(command);
        inputField.text = "";  // Clear input
    });
    
    // Welcome message
    addOutput("Console initialized. Type 'help' for available commands.", sf::Color(100, 200, 100));
    
    // Register built-in commands
    registerCommand("help", [this](const std::vector<std::string>& args) {
        addOutput("Available commands:", sf::Color(200, 200, 100));
        for (const auto& cmd : commands) {
            addOutput("  " + cmd.name + " - " + cmd.description, sf::Color::White);
        }
    }, "Show available commands");
    
    registerCommand("clear", [this](const std::vector<std::string>& args) {
        outputLines.clear();
        scrollOffset = 0.0f;
    }, "Clear console output");
    
    registerCommand("mass", [this](const std::vector<std::string>& args) {
        addOutput("Mass audit logs are in mass_audit.log file", sf::Color(200, 200, 100));
    }, "Show mass conservation info");
    
    initialized = true;
}

void GameConsole::render(Renderer& renderer) {
    if (!visible || !initialized) return;
    
    // Render background
    renderer.drawRectangle(background);
    
    // Render output lines
    renderOutput(renderer);
    
    // Render input field
    inputField.render(renderer);
}

void GameConsole::handleToggle() {
    visible = !visible;
    
    if (visible) {
        // Focus input when opening
        inputField.isFocused = true;
    } else {
        // Unfocus when closing
        inputField.isFocused = false;
    }
}

void GameConsole::handleMousePress(const sf::Vector2f& mousePos) {
    if (!visible) return;
    inputField.handleMousePress(mousePos);
}

void GameConsole::handleKeyPress(const sf::Event::KeyPressed& keyEvent) {
    if (!visible) return;
    inputField.handleKeyPress(keyEvent);
}

void GameConsole::handleTextEntered(const sf::Event::TextEntered& textEvent) {
    if (!visible) return;
    
    // Filter out backtick character (from toggle key)
    if (textEvent.unicode == '`' || textEvent.unicode == '~') {
        return;  // Ignore the toggle key character
    }
    
    inputField.handleTextEntered(textEvent);
}

void GameConsole::update(float deltaTime) {
    if (!visible) return;
    
    // Update cursor blink
    // Note: UITextInput has a private method for this, we'll handle it in the main loop
}

void GameConsole::registerCommand(const std::string& name, std::function<void(const std::vector<std::string>&)> handler, const std::string& description) {
    Command cmd;
    cmd.name = name;
    cmd.description = description;
    cmd.handler = handler;
    commands.push_back(cmd);
}

void GameConsole::processCommand(const std::string& commandText) {
    if (commandText.empty()) return;
    
    // DEBUG: Show what was actually received
    std::cout << "[CONSOLE] Received command: '" << commandText << "'" << std::endl;
    
    // Echo command
    addOutput("> " + commandText, sf::Color(150, 150, 200));
    
    // Parse command and arguments
    std::istringstream iss(commandText);
    std::vector<std::string> tokens;
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.empty()) return;
    
    std::string commandName = tokens[0];
    
    // DEBUG: Show parsed command name
    std::cout << "[CONSOLE] Parsed command name: '" << commandName << "'" << std::endl;
    std::cout << "[CONSOLE] Registered commands: ";
    for (const auto& cmd : commands) {
        std::cout << cmd.name << " ";
    }
    std::cout << std::endl;
    
    // Remove first token (command name), keep arguments
    tokens.erase(tokens.begin());
    
    // Find and execute command
    bool found = false;
    for (const auto& cmd : commands) {
        if (cmd.name == commandName) {
            cmd.handler(tokens);
            found = true;
            break;
        }
    }
    
    if (!found) {
        addOutput("Unknown command: " + commandName + " (type 'help' for list)", sf::Color(255, 100, 100));
    }
}

void GameConsole::addOutput(const std::string& text, sf::Color color) {
    ConsoleLine line;
    line.text = text;
    line.color = color;
    outputLines.push_back(line);
    
    // Keep only last 100 lines to prevent memory issues
    if (outputLines.size() > 100) {
        outputLines.erase(outputLines.begin());
    }
    
    // Auto-scroll to bottom
    scrollOffset = 0.0f;
}

void GameConsole::renderOutput(Renderer& renderer) {
    if (!fontPtr) return;
    
    float consoleX = background.getPosition().x + 10;
    float consoleY = background.getPosition().y + 10;
    float consoleWidth = background.getSize().x - 20;
    float consoleHeight = background.getSize().y - 60;  // Leave room for input field
    
    float lineHeight = 20.0f;
    float currentY = consoleY - scrollOffset;
    
    // Start from bottom and work up
    int startIndex = std::max(0, (int)outputLines.size() - (int)(consoleHeight / lineHeight));
    
    for (size_t i = startIndex; i < outputLines.size(); i++) {
        if (currentY > consoleY + consoleHeight) break;
        
        sf::Text text(*fontPtr);
        text.setCharacterSize(14);
        text.setFillColor(outputLines[i].color);
        text.setString(outputLines[i].text);
        text.setPosition(sf::Vector2f(consoleX, currentY));
        
        renderer.drawText(text);
        currentY += lineHeight;
    }
}
