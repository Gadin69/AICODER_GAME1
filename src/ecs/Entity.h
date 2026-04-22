#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include <typeindex>
#include "Component.h"
#include "System.h"

class Entity {
public:
    Entity(uint32_t id);
    
    uint32_t getId() const;
    bool isActive() const;
    
private:
    uint32_t id;
    bool active;
    
    friend class World;
};

class World {
public:
    World();
    
    Entity createEntity();
    void destroyEntity(uint32_t entityId);
    
    template<typename T>
    void addComponent(uint32_t entityId, T component);
    
    template<typename T>
    T* getComponent(uint32_t entityId);
    
    template<typename T>
    bool hasComponent(uint32_t entityId);
    
    void addSystem(std::unique_ptr<System> system);
    void updateSystems(float deltaTime);
    
    const std::vector<uint32_t>& getActiveEntities() const;
    
private:
    uint32_t nextEntityId;
    std::vector<uint32_t> activeEntities;
    std::vector<bool> entityActive;
    
    // Component storage: entityId -> component
    std::unordered_map<uint32_t, std::unordered_map<std::type_index, std::unique_ptr<Component>>> components;
    
    std::vector<std::unique_ptr<System>> systems;
};

// Template implementations
template<typename T>
void World::addComponent(uint32_t entityId, T component) {
    auto* comp = new T(component);
    components[entityId][std::type_index(typeid(T))] = std::unique_ptr<T>(comp);
}

template<typename T>
T* World::getComponent(uint32_t entityId) {
    auto it = components.find(entityId);
    if (it == components.end()) {
        return nullptr;
    }
    
    auto typeIt = it->second.find(std::type_index(typeid(T)));
    if (typeIt == it->second.end()) {
        return nullptr;
    }
    
    return static_cast<T*>(typeIt->second.get());
}

template<typename T>
bool World::hasComponent(uint32_t entityId) {
    return getComponent<T>(entityId) != nullptr;
}
