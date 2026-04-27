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

// ============================================================================
// OIL VAPOR ELEMENT
// ============================================================================
class Gas_OilElement : public GasElement {
public:
    Gas_OilElement() : GasElement(
        "Oil Vapor", 2.5f, 300.0f, -40.0f, 300.0f,
        1500.0f, 0.02f,  // Moderate specific heat, low thermal conductivity
        0.0f, 200000.0f,
        0.0f, 0.003f, 0.4f, -40.0f, 300.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp < condensationPoint - 5.0f) return ElementType::Liquid_Oil;
        return ElementType::Empty;
    }
};

// ============================================================================
// NEW GASES
// ============================================================================

// HYDROGEN GAS - Lightest, highly flammable
class HydrogenElement : public GasElement {
public:
    HydrogenElement() : GasElement(
        "Hydrogen", 0.09f, -253.0f, -259.0f, -253.0f,
        14300.0f, 0.18f,
        58000.0f, 454000.0f,
        0.0f, 0.00367f, 0.8f, -259.0f, -253.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;  // Solid hydrogen (too cryogenic)
        if (temp < condensationPoint) return ElementType::Liquid_Hydrogen;
        return ElementType::Empty;
    }
};

// METHANE GAS (Natural Gas) - Flammable fuel
class MethaneElement : public GasElement {
public:
    MethaneElement() : GasElement(
        "Methane", 0.72f, 20.0f, -182.0f, -161.0f,
        2200.0f, 0.034f,
        59000.0f, 510000.0f,
        0.0f, 0.00367f, 0.4f, -182.0f, -161.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp < condensationPoint) return ElementType::Liquid_Methane;
        return ElementType::Empty;
    }
};

// AMMONIA GAS - Toxic, used in refrigeration
class AmmoniaElement : public GasElement {
public:
    AmmoniaElement() : GasElement(
        "Ammonia", 0.77f, 20.0f, -78.0f, -33.0f,
        2200.0f, 0.026f,
        332000.0f, 1370000.0f,
        0.0f, 0.00367f, 0.5f, -78.0f, -33.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp < condensationPoint) return ElementType::Liquid_Ammonia;
        return ElementType::Empty;
    }
};

// CHLORINE GAS - Heavy, toxic, green-yellow
class ChlorineElement : public GasElement {
public:
    ChlorineElement() : GasElement(
        "Chlorine", 3.2f, 20.0f, -101.0f, -34.0f,
        480.0f, 0.009f,
        12000.0f, 20400.0f,
        0.0f, 0.00367f, 0.3f, -101.0f, -34.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp < condensationPoint) return ElementType::Liquid_Chlorine;
        return ElementType::Empty;
    }
};

// SULFUR DIOXIDE GAS - Heavy, volcanic/industrial
class SulfurDioxideElement : public GasElement {
public:
    SulfurDioxideElement() : GasElement(
        "Sulfur Dioxide", 2.93f, 20.0f, -72.0f, -10.0f,
        640.0f, 0.009f,
        11500.0f, 24900.0f,
        0.0f, 0.00367f, 0.4f, -72.0f, -10.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp < condensationPoint) return ElementType::Liquid_SulfurDioxide;
        return ElementType::Empty;
    }
};

// PROPANE GAS - Fuel gas, heavier than air
class PropaneElement : public GasElement {
public:
    PropaneElement() : GasElement(
        "Propane", 2.01f, 20.0f, -188.0f, -42.0f,
        1670.0f, 0.018f,
        3500.0f, 438000.0f,
        0.0f, 0.00367f, 0.4f, -188.0f, -42.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp < condensationPoint) return ElementType::Liquid_Propane;
        return ElementType::Empty;
    }
};

// ============================================================================
// GAS FROM LIQUID ELEMENTS (vaporized liquids)
// ============================================================================

// MERCURY VAPOR - Toxic, heavy gas
class GasMercuryElement : public GasElement {
public:
    GasMercuryElement() : GasElement(
        "Mercury Vapor", 6.97f, 400.0f, -39.0f, 357.0f,
        100.0f, 0.009f,
        0.0f, 295000.0f,
        0.0f, 0.00367f, 0.3f, -39.0f, 357.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp < condensationPoint - 10.0f) return ElementType::Liquid_Mercury;
        return ElementType::Empty;
    }
};

// ETHANOL VAPOR - Flammable alcohol vapor
class GasEthanolElement : public GasElement {
public:
    GasEthanolElement() : GasElement(
        "Ethanol Vapor", 1.59f, 100.0f, -114.0f, 78.0f,
        1500.0f, 0.016f,
        0.0f, 841000.0f,
        0.0f, 0.00367f, 0.5f, -114.0f, 78.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp < condensationPoint - 5.0f) return ElementType::Liquid_Ethanol;
        return ElementType::Empty;
    }
};

// ACETONE VAPOR - Highly volatile solvent vapor
class GasAcetoneElement : public GasElement {
public:
    GasAcetoneElement() : GasElement(
        "Acetone Vapor", 2.0f, 80.0f, -95.0f, 56.0f,
        1300.0f, 0.014f,
        0.0f, 518000.0f,
        0.0f, 0.00367f, 0.6f, -95.0f, 56.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp < condensationPoint - 5.0f) return ElementType::Liquid_Acetone;
        return ElementType::Empty;
    }
};

// GLYCEROL VAPOR - Thick, syrupy vapor
class GasGlycerolElement : public GasElement {
public:
    GasGlycerolElement() : GasElement(
        "Glycerol Vapor", 2.5f, 320.0f, 18.0f, 290.0f,
        1800.0f, 0.02f,
        0.0f, 917000.0f,
        0.0f, 0.00367f, 0.4f, 18.0f, 290.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp < condensationPoint - 10.0f) return ElementType::Liquid_Glycerol;
        return ElementType::Empty;
    }
};

// SULFURIC ACID VAPOR - Highly corrosive gas
class GasSulfuricAcidElement : public GasElement {
public:
    GasSulfuricAcidElement() : GasElement(
        "Sulfuric Acid Vapor", 3.3f, 370.0f, 10.0f, 337.0f,
        900.0f, 0.012f,
        0.0f, 502000.0f,
        0.0f, 0.00367f, 0.3f, 10.0f, 337.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp < condensationPoint - 10.0f) return ElementType::Liquid_SulfuricAcid;
        return ElementType::Empty;
    }
};

// OXYGEN GAS - Supports combustion
class GasOxygenElement : public GasElement {
public:
    GasOxygenElement() : GasElement(
        "Oxygen Gas", 1.43f, -150.0f, -219.0f, -183.0f,
        920.0f, 0.026f,
        0.0f, 213000.0f,
        0.0f, 0.00367f, 0.7f, -219.0f, -183.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;
        if (temp < condensationPoint) return ElementType::Liquid_Oxygen;
        return ElementType::Empty;
    }
};

// BROMINE VAPOR - Toxic, reddish-brown gas
class GasBromineElement : public GasElement {
public:
    GasBromineElement() : GasElement(
        "Bromine Vapor", 7.5f, 80.0f, -7.0f, 59.0f,
        300.0f, 0.008f,
        0.0f, 93000.0f,
        0.0f, 0.00367f, 0.3f, -7.0f, 59.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp < condensationPoint - 5.0f) return ElementType::Liquid_Bromine;
        return ElementType::Empty;
    }
};

// NITROGEN GAS - Inert atmospheric gas
class GasNitrogenElement : public GasElement {
public:
    GasNitrogenElement() : GasElement(
        "Nitrogen Gas", 1.25f, -150.0f, -210.0f, -196.0f,
        1040.0f, 0.026f,
        0.0f, 199000.0f,
        0.0f, 0.00367f, 0.7f, -210.0f, -196.0f
    ) {}
    
    ElementType getPhaseAtTemperature(float temp) const override {
        if (temp <= freezingPoint) return ElementType::Empty;  // Solid nitrogen (too cryogenic)
        if (temp < condensationPoint) return ElementType::Liquid_Nitrogen;
        return ElementType::Empty;
    }
};
