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
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::Liquid_Lava;
        return ElementType::Empty;
    }
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
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::Liquid_Water;
        if (temp >= 100.0f) return ElementType::Gas_O2;  // Sublimation
        return ElementType::Empty;
    }
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
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > sublimationPoint) return ElementType::Gas_CO2;
        return ElementType::Empty;
    }
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
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::ContaminatedWater;
        return ElementType::Empty;
    }
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
    ElementType getPhaseAtTemperature(float temp) const override { return ElementType::Empty; }  // Never changes phase
};

// ============================================================================
// FROZEN OIL ELEMENT
// ============================================================================
class Solid_OilElement : public SolidElement {
public:
    Solid_OilElement() : SolidElement(
        "Frozen Oil", 850.0f, -50.0f, 
        -40.0f, 300.0f, 1800.0f, 0.2f,
        0.0f, 200000.0f,
        0.0f, 0.00005f, 1.1f, -40.0f, 300.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::Liquid_Oil;
        return ElementType::Empty;
    }
};

// ============================================================================
// NEW SOLIDS
// ============================================================================

// IRON - Common metal, magnetic
class IronElement : public SolidElement {
public:
    IronElement() : SolidElement(
        "Iron", 7874.0f, 20.0f,
        1538.0f, 2861.0f, 450.0f, 80.0f,
        247000.0f, 6340000.0f,
        0.0f, 0.000012f, 2.5f, 1538.0f, 2861.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::Liquid_Iron;  // Molten iron
        return ElementType::Empty;
    }
};

// COPPER - Excellent conductor
class CopperElement : public SolidElement {
public:
    CopperElement() : SolidElement(
        "Copper", 8960.0f, 20.0f,
        1085.0f, 2562.0f, 385.0f, 401.0f,
        205000.0f, 4730000.0f,
        0.0f, 0.000017f, 3.0f, 1085.0f, 2562.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::Liquid_Copper;
        return ElementType::Empty;
    }
};

// ALUMINUM - Lightweight metal
class AluminumElement : public SolidElement {
public:
    AluminumElement() : SolidElement(
        "Aluminum", 2700.0f, 20.0f,
        660.0f, 2519.0f, 900.0f, 237.0f,
        397000.0f, 10900000.0f,
        0.0f, 0.000023f, 2.8f, 660.0f, 2519.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::Liquid_Aluminum;
        return ElementType::Empty;
    }
};

// SILVER - Precious metal, high thermal conductivity
class SilverElement : public SolidElement {
public:
    SilverElement() : SolidElement(
        "Silver", 10490.0f, 20.0f,
        962.0f, 2162.0f, 235.0f, 429.0f,
        105000.0f, 2540000.0f,
        0.0f, 0.000019f, 3.2f, 962.0f, 2162.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::Liquid_Silver;
        return ElementType::Empty;
    }
};

// GOLD - Dense precious metal
class GoldElement : public SolidElement {
public:
    GoldElement() : SolidElement(
        "Gold", 19320.0f, 20.0f,
        1064.0f, 2970.0f, 129.0f, 318.0f,
        63000.0f, 1570000.0f,
        0.0f, 0.000014f, 2.9f, 1064.0f, 2970.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::Liquid_Gold;
        return ElementType::Empty;
    }
};

// LEAD - Heavy, soft, low melt point
class LeadElement : public SolidElement {
public:
    LeadElement() : SolidElement(
        "Lead", 11340.0f, 20.0f,
        327.0f, 1749.0f, 129.0f, 35.0f,
        23000.0f, 178000.0f,
        0.0f, 0.000029f, 1.8f, 327.0f, 1749.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::Liquid_Lead;
        return ElementType::Empty;
    }
};

// ZINC - Common industrial metal
class ZincElement : public SolidElement {
public:
    ZincElement() : SolidElement(
        "Zinc", 7140.0f, 20.0f,
        420.0f, 907.0f, 388.0f, 116.0f,
        112000.0f, 169000.0f,
        0.0f, 0.00003f, 2.0f, 420.0f, 907.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp > meltingPoint) return ElementType::Liquid_Zinc;
        return ElementType::Empty;
    }
};

// ============================================================================
// LIQUID METALS
// ============================================================================

// LIQUID IRON - Molten iron
class LiquidIronElement : public LiquidElement {
public:
    LiquidIronElement() : LiquidElement(
        "Liquid Iron", 7000.0f, 1600.0f,  // Slightly less dense than solid
        1538.0f, 2861.0f, 820.0f, 40.0f,  // Lower thermal conductivity than solid
        247000.0f, 6340000.0f,
        0.005f, 0.00002f, 2.0f, 1538.0f, 2861.0f  // Low viscosity for molten metal
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_Iron;
        if (temp >= boilingPoint) return ElementType::Empty;  // Iron vapor
        return ElementType::Empty;
    }
};

// LIQUID COPPER - Molten copper
class LiquidCopperElement : public LiquidElement {
public:
    LiquidCopperElement() : LiquidElement(
        "Liquid Copper", 8000.0f, 1150.0f,
        1085.0f, 2562.0f, 500.0f, 150.0f,
        205000.0f, 4730000.0f,
        0.004f, 0.000025f, 2.5f, 1085.0f, 2562.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_Copper;
        if (temp >= boilingPoint) return ElementType::Empty;
        return ElementType::Empty;
    }
};

// LIQUID ALUMINUM - Molten aluminum
class LiquidAluminumElement : public LiquidElement {
public:
    LiquidAluminumElement() : LiquidElement(
        "Liquid Aluminum", 2380.0f, 700.0f,
        660.0f, 2519.0f, 1100.0f, 90.0f,
        397000.0f, 10900000.0f,
        0.001f, 0.00003f, 2.2f, 660.0f, 2519.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_Aluminum;
        if (temp >= boilingPoint) return ElementType::Empty;
        return ElementType::Empty;
    }
};

// LIQUID SILVER - Molten silver
class LiquidSilverElement : public LiquidElement {
public:
    LiquidSilverElement() : LiquidElement(
        "Liquid Silver", 9300.0f, 1000.0f,
        962.0f, 2162.0f, 290.0f, 170.0f,
        105000.0f, 2540000.0f,
        0.003f, 0.000025f, 2.8f, 962.0f, 2162.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_Silver;
        if (temp >= boilingPoint) return ElementType::Empty;
        return ElementType::Empty;
    }
};

// LIQUID GOLD - Molten gold
class LiquidGoldElement : public LiquidElement {
public:
    LiquidGoldElement() : LiquidElement(
        "Liquid Gold", 17300.0f, 1100.0f,
        1064.0f, 2970.0f, 150.0f, 120.0f,
        63000.0f, 1570000.0f,
        0.003f, 0.00002f, 2.4f, 1064.0f, 2970.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_Gold;
        if (temp >= boilingPoint) return ElementType::Empty;
        return ElementType::Empty;
    }
};

// LIQUID LEAD - Molten lead
class LiquidLeadElement : public LiquidElement {
public:
    LiquidLeadElement() : LiquidElement(
        "Liquid Lead", 10660.0f, 400.0f,
        327.0f, 1749.0f, 140.0f, 15.0f,
        23000.0f, 178000.0f,
        0.002f, 0.000035f, 1.5f, 327.0f, 1749.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_Lead;
        if (temp >= boilingPoint) return ElementType::Empty;
        return ElementType::Empty;
    }
};

// LIQUID ZINC - Molten zinc
class LiquidZincElement : public LiquidElement {
public:
    LiquidZincElement() : LiquidElement(
        "Liquid Zinc", 6570.0f, 450.0f,
        420.0f, 907.0f, 480.0f, 50.0f,
        112000.0f, 169000.0f,
        0.004f, 0.000035f, 1.8f, 420.0f, 907.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Solid_Zinc;
        if (temp >= boilingPoint) return ElementType::Empty;
        return ElementType::Empty;
    }
};
