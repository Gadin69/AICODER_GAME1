#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <map>

class Renderer;  // Forward declaration

class UIRadioButton {
public:
    UIRadioButton();
    ~UIRadioButton();
    
    void initialize(float x, float y, const std::string& label, int groupId, const sf::Font& font);
    void render(Renderer& renderer);
    
    void handleMousePress(const sf::Vector2f& mousePos);
    
    void setSelected(bool selected);
    void setGroupId(int id);
    static void clearGroup(int groupId);
    static void setCallback(int groupId, std::function<void(int)> callback);
    
    sf::CircleShape outerCircle;
    sf::CircleShape innerCircle;
    sf::Text* labelText = nullptr;
    sf::Font* fontPtr = nullptr;
    
    int groupId = 0;
    bool isSelected = false;
    bool initialized = false;
    
private:
    static std::map<int, std::vector<UIRadioButton*>> groups;
    static std::map<int, std::function<void(int)>> groupCallbacks;
    
    void updateVisuals();
    void notifyGroup();
};
