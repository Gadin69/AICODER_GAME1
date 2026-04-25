#pragma once

#include "BaseElements.h"

// ============================================================================
// STEAM ELEMENT
// ============================================================================
class SteamElement : public GasElement {
public:
    SteamElement() : GasElement(
        "Steam", 0.6f, 100.0f, 0.0f, 100.0f, 2000.0f, 0.026f,
        0.0f, 2260000.0f,
        0.0f, 0.00367f, 0.5f, 0.0f, 100.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_Ice;  // Deposition
        if (temp < condensationPoint - 5.0f) return ElementType::Liquid_Water;  // Condensation
        return ElementType::Empty;  // Stay steam above 95°C
    }
};

// ============================================================================
// CO2 ELEMENT
// ============================================================================
class CO2Element : public GasElement {
public:
    CO2Element() : GasElement(
        "CO2", 1.98f, 20.0f, -78.5f, -56.6f, 840.0f, 0.016f,
        0.0f, 574000.0f,
        0.0f, 0.00367f, 0.5f, -78.5f, -56.6f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= sublimationPoint) return ElementType::Solid_DryIce;
        return ElementType::Empty;
    }
};

// ============================================================================
// GAS LAVA ELEMENT
// ============================================================================
class Gas_LavaElement : public GasElement {
public:
    Gas_LavaElement() : GasElement(
        "Gas Lava", 5.0f, 3000.0f, 700.0f, 3000.0f, 
        500.0f, 0.05f, 400000.0f, 8000000.0f, 
        0.0f, 0.001f, 0.3f, 700.0f, 3000.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp < condensationPoint - 1.0f) return ElementType::Liquid_Lava;
        return ElementType::Empty;
    }
};
