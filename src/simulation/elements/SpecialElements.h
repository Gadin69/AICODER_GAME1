#pragma once

#include "../ElementTypes.h"

// ============================================================================
// EMPTY ELEMENT
// ============================================================================
class EmptyElement : public Element {
public:
    EmptyElement() : Element("Empty", 0.0f, 20.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                             false, false, false, false, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f) {}
};

// ============================================================================
// VACUUM ELEMENT
// ============================================================================
class VacuumElement : public Element {
public:
    VacuumElement() : Element("Vacuum", 0.0f, 20.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, false, false, false, false,
                              0.0f, 0.0f, 0.0f, 0.0f, 0.0f) {}  // Room temp (neutral, won't corrupt cells)
    
    // Override: Vacuum cannot transfer heat
    bool canTransferHeat() const override { return false; }
};
