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
};
