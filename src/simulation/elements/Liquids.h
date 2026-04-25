#pragma once

#include "BaseElements.h"

// ============================================================================
// WATER ELEMENT
// ============================================================================
class WaterElement : public LiquidElement {
public:
    WaterElement() : LiquidElement(
        "Water", 1000.0f, 20.0f, 0.0f, 100.0f, 4186.0f, 0.6f,
        334000.0f, 2260000.0f,
        0.001f, 0.00021f, 1.0f, 0.0f, 100.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_Ice;
        if (temp >= boilingPoint) return ElementType::Gas_O2;
        return ElementType::Empty;  // No change
    }
};

// ============================================================================
// LAVA ELEMENT
// ============================================================================
class LavaElement : public LiquidElement {
public:
    LavaElement() : LiquidElement(
        "Lava", 3100.0f, 1200.0f, 700.0f, 3000.0f, 840.0f, 1.5f,
        400000.0f, 6000000.0f,
        50.0f, 0.00003f, 2.0f, 700.0f, 3000.0f, 0.0f, 3000.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid;
        if (temp >= boilingPoint) return ElementType::Gas_Lava;
        return ElementType::Empty;
    }
};

// ============================================================================
// CONTAMINATED WATER ELEMENT
// ============================================================================
class ContaminatedWaterElement : public LiquidElement {
public:
    ContaminatedWaterElement() : LiquidElement(
        "Contaminated Water", 1050.0f, 20.0f, -2.0f, 101.0f,
        4000.0f, 0.5f, 320000.0f, 2200000.0f,
        0.0012f, 0.00021f, 1.0f, -2.0f, 101.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_ContaminatedWater;
        if (temp >= boilingPoint) return ElementType::Gas_O2;  // Purification
        return ElementType::Empty;
    }
};

// ============================================================================
// OIL ELEMENT (Floats on water due to lower density: 800 kg/m³)
// ============================================================================
class OilElement : public LiquidElement {
public:
    OilElement() : LiquidElement(
        "Oil", 800.0f, 20.0f, -40.0f, 300.0f,
        2000.0f, 0.15f,  // Lower thermal conductivity than water
        0.0f, 200000.0f,  // No fusion latent heat (amorphous solid), vaporization heat
        0.05f, 0.0001f, 1.0f, -40.0f, 300.0f  // Higher viscosity than water
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_Oil;
        if (temp >= boilingPoint) return ElementType::Gas_Oil;
        return ElementType::Empty;
    }
};
