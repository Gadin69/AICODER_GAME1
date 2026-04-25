#pragma once

#include "BaseElements.h"

// ============================================================================
// ROCK ELEMENT
// ============================================================================
class RockElement : public SolidElement {
public:
    RockElement() : SolidElement(
        "Rock", 2700.0f, 20.0f, 1200.0f, 3000.0f, 790.0f, 3.0f,
        400000.0f, 6000000.0f,
        0.0f, 0.00001f, 1.5f, 1200.0f, 3000.0f
    ) {}
};

// ============================================================================
// ICE ELEMENT
// ============================================================================
class IceElement : public SolidElement {
public:
    IceElement() : SolidElement(
        "Ice", 917.0f, -10.0f, 0.0f, 100.0f, 2090.0f, 2.18f,
        334000.0f, 2260000.0f,
        0.0f, 0.00005f, 1.2f, 0.0f, 100.0f
    ) {}
};

// ============================================================================
// DRY ICE ELEMENT
// ============================================================================
class DryIceElement : public SolidElement {
public:
    DryIceElement() : SolidElement(
        "Dry Ice", 1.56f, -78.5f, -78.5f, -56.6f,
        840.0f, 0.016f, 0.0f, 574000.0f,
        0.0f, 0.0f, 0.5f, -78.5f, -56.6f
    ) {}
};

// ============================================================================
// SOLID CONTAMINATED WATER ELEMENT
// ============================================================================
class Solid_ContaminatedWaterElement : public SolidElement {
public:
    Solid_ContaminatedWaterElement() : SolidElement(
        "Frozen Contaminated Water", 980.0f, -10.0f, 
        -2.0f, 101.0f, 2000.0f, 2.0f,
        320000.0f, 2200000.0f,
        0.0f, 0.00005f, 1.1f, -2.0f, 101.0f
    ) {}
};

// ============================================================================
// INDESTRUCTIBLE INSULATOR ELEMENT (Custom Element)
// - No heat transfer (like vacuum)
// - Cannot be destroyed or overwritten (except in dev/admin mode)
// ============================================================================
class IndestructibleInsulatorElement : public SolidElement {
public:
    IndestructibleInsulatorElement() : SolidElement(
        "Indestructible Insulator", 5000.0f, 20.0f,
        999999.0f, 999999.0f,  // Extremely high phase change temps (practically never changes)
        0.0f, 0.0f,             // No specific heat or thermal conductivity
        0.0f, 0.0f,             // No latent heat
        0.0f, 0.0f, 0.0f,       // No viscosity, expansion, or heat transfer
        999999.0f, 999999.0f    // Extremely high freeze/condense points
    ) {}
    
    // Override virtual methods for custom behavior
    bool canTransferHeat() const override { return false; }  // No heat transfer like vacuum
    bool isDestructible() const override { return false; }   // Cannot be destroyed
};
