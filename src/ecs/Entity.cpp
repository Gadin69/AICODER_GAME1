#include "Entity.h"

Entity::Entity(uint32_t id)
    : id(id)
    , active(true)
{
}

uint32_t Entity::getId() const {
    return id;
}

bool Entity::isActive() const {
    return active;
}
