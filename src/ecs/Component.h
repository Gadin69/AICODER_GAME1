#pragma once

#include <cstdint>

class Component {
public:
    virtual ~Component() = default;
};

struct PositionComponent : public Component {
    float x, y;
    PositionComponent() : x(0), y(0) {}
    PositionComponent(float px, float py) : x(px), y(py) {}
};

struct VelocityComponent : public Component {
    float vx, vy;
    VelocityComponent() : vx(0), vy(0) {}
    VelocityComponent(float pvx, float pvy) : vx(pvx), vy(pvy) {}
};

struct SpriteComponent : public Component {
    int textureId;
    int frameWidth;
    int frameHeight;
    int currentFrame;
    SpriteComponent() : textureId(0), frameWidth(32), frameHeight(32), currentFrame(0) {}
};

struct HealthComponent : public Component {
    float currentHealth;
    float maxHealth;
    HealthComponent() : currentHealth(100), maxHealth(100) {}
    HealthComponent(float health) : currentHealth(health), maxHealth(health) {}
};
