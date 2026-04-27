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

// ============================================================================
// NEW LIQUIDS
// ============================================================================

// MERCURY - Heavy liquid metal
class MercuryElement : public LiquidElement {
public:
    MercuryElement() : LiquidElement(
        "Mercury", 13534.0f, 20.0f, -39.0f, 357.0f,
        140.0f, 8.3f,
        11400.0f, 295000.0f,
        0.0015f, 0.00018f, 2.0f, -39.0f, 357.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;  // Solid mercury
        if (temp >= boilingPoint) return ElementType::Gas_Mercury;  // Mercury vapor
        return ElementType::Empty;
    }
};

// ETHANOL (Alcohol) - Flammable liquid
class EthanolElement : public LiquidElement {
public:
    EthanolElement() : LiquidElement(
        "Ethanol", 789.0f, 20.0f, -114.0f, 78.0f,
        2440.0f, 0.17f,
        109000.0f, 841000.0f,
        0.0012f, 0.0011f, 0.8f, -114.0f, 78.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_Ethanol;
        return ElementType::Empty;
    }
};

// ACETONE - Highly volatile solvent
class AcetoneElement : public LiquidElement {
public:
    AcetoneElement() : LiquidElement(
        "Acetone", 784.0f, 20.0f, -95.0f, 56.0f,
        2100.0f, 0.16f,
        98000.0f, 518000.0f,
        0.0003f, 0.0014f, 0.7f, -95.0f, 56.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_Acetone;
        return ElementType::Empty;
    }
};

// GLYCEROL - Very viscous, thick syrup
class GlycerolElement : public LiquidElement {
public:
    GlycerolElement() : LiquidElement(
        "Glycerol", 1261.0f, 20.0f, 18.0f, 290.0f,
        2430.0f, 0.29f,
        200000.0f, 917000.0f,
        1.5f, 0.0005f, 0.6f, 18.0f, 290.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_Glycerol;
        return ElementType::Empty;
    }
};

// SULFURIC ACID - Corrosive, dense
class SulfuricAcidElement : public LiquidElement {
public:
    SulfuricAcidElement() : LiquidElement(
        "Sulfuric Acid", 1830.0f, 20.0f, 10.0f, 337.0f,
        1400.0f, 0.35f,
        102000.0f, 502000.0f,
        0.025f, 0.00057f, 0.9f, 10.0f, 337.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_SulfuricAcid;
        return ElementType::Empty;
    }
};

// LIQUID NITROGEN - Cryogenic, -196°C
class LiquidNitrogenElement : public LiquidElement {
public:
    LiquidNitrogenElement() : LiquidElement(
        "Liquid Nitrogen", 808.0f, -196.0f, -210.0f, -196.0f,
        2040.0f, 0.14f,
        25700.0f, 199000.0f,
        0.00016f, 0.002f, 0.5f, -210.0f, -196.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_Nitrogen;
        return ElementType::Empty;
    }
};

// LIQUID OXYGEN - Cryogenic, supports combustion
class LiquidOxygenElement : public LiquidElement {
public:
    LiquidOxygenElement() : LiquidElement(
        "Liquid Oxygen", 1141.0f, -183.0f, -219.0f, -183.0f,
        1700.0f, 0.15f,
        13900.0f, 213000.0f,
        0.0002f, 0.002f, 0.6f, -219.0f, -183.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_Oxygen;
        return ElementType::Empty;
    }
};

// BROMINE - Only liquid nonmetal at room temp, reddish-brown
class BromineElement : public LiquidElement {
public:
    BromineElement() : LiquidElement(
        "Bromine", 3103.0f, 20.0f, -7.0f, 59.0f,
        470.0f, 0.12f,
        6600.0f, 9300.0f,
        0.0f, 0.0011f, 0.5f, -7.0f, 59.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_Bromine;
        return ElementType::Empty;
    }
};

// ============================================================================
// LIQUID GAS FORMS (condensed gases)
// ============================================================================

// LIQUID METHANE - Cryogenic fuel
class LiquidMethaneElement : public LiquidElement {
public:
    LiquidMethaneElement() : LiquidElement(
        "Liquid Methane", 424.0f, -162.0f, -182.0f, -161.0f,
        3400.0f, 0.14f,
        59000.0f, 510000.0f,
        0.0001f, 0.003f, 0.5f, -182.0f, -161.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_Methane;
        return ElementType::Empty;
    }
};

// LIQUID AMMONIA - Refrigerant
class LiquidAmmoniaElement : public LiquidElement {
public:
    LiquidAmmoniaElement() : LiquidElement(
        "Liquid Ammonia", 682.0f, -40.0f, -78.0f, -33.0f,
        4700.0f, 0.54f,
        332000.0f, 1370000.0f,
        0.00025f, 0.0025f, 0.6f, -78.0f, -33.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_Ammonia;
        return ElementType::Empty;
    }
};

// LIQUID CHLORINE - Toxic industrial chemical
class LiquidChlorineElement : public LiquidElement {
public:
    LiquidChlorineElement() : LiquidElement(
        "Liquid Chlorine", 1560.0f, -40.0f, -101.0f, -34.0f,
        890.0f, 0.13f,
        12000.0f, 204000.0f,
        0.00035f, 0.002f, 0.4f, -101.0f, -34.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_Chlorine;
        return ElementType::Empty;
    }
};

// LIQUID SULFUR DIOXIDE - Volcanic/industrial
class LiquidSulfurDioxideElement : public LiquidElement {
public:
    LiquidSulfurDioxideElement() : LiquidElement(
        "Liquid Sulfur Dioxide", 1460.0f, -15.0f, -72.0f, -10.0f,
        1200.0f, 0.18f,
        11500.0f, 24900.0f,
        0.0003f, 0.0022f, 0.5f, -72.0f, -10.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_SulfurDioxide;
        return ElementType::Empty;
    }
};

// LIQUID PROPANE - Liquid fuel
class LiquidPropaneElement : public LiquidElement {
public:
    LiquidPropaneElement() : LiquidElement(
        "Liquid Propane", 580.0f, -45.0f, -188.0f, -42.0f,
        2400.0f, 0.12f,
        3500.0f, 438000.0f,
        0.0002f, 0.003f, 0.5f, -188.0f, -42.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp >= boilingPoint) return ElementType::Gas_Propane;
        return ElementType::Empty;
    }
};

// LIQUID HYDROGEN - Cryogenic, lightest liquid
class LiquidHydrogenElement : public LiquidElement {
public:
    LiquidHydrogenElement() : LiquidElement(
        "Liquid Hydrogen", 71.0f, -253.0f, -259.0f, -253.0f,
        14300.0f, 0.14f,
        58000.0f, 454000.0f,
        0.00001f, 0.004f, 0.5f, -259.0f, -253.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;  // Solid hydrogen (too cryogenic)
        if (temp >= boilingPoint) return ElementType::Gas_Hydrogen;
        return ElementType::Empty;
    }
};
