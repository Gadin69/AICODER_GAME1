#include "Entity.h"
#include <algorithm>

World::World()
    : nextEntityId(0)
{
}

Entity World::createEntity() {
    uint32_t id = nextEntityId++;
    activeEntities.push_back(id);
    entityActive.resize(nextEntityId, true);
    return Entity(id);
}

void World::destroyEntity(uint32_t entityId) {
    if (entityId < entityActive.size() && entityActive[entityId]) {
        entityActive[entityId] = false;
        components.erase(entityId);
        
        activeEntities.erase(
            std::remove(activeEntities.begin(), activeEntities.end(), entityId),
            activeEntities.end()
        );
    }
}

void World::addSystem(std::unique_ptr<System> system) {
    systems.push_back(std::move(system));
    
    // Sort systems by order
    std::sort(systems.begin(), systems.end(), 
        [](const std::unique_ptr<System>& a, const std::unique_ptr<System>& b) {
            return a->getOrder() < b->getOrder();
        });
}

void World::updateSystems(float deltaTime) {
    for (auto& system : systems) {
        system->update(*this, deltaTime);
    }
}

const std::vector<uint32_t>& World::getActiveEntities() const {
    return activeEntities;
}
