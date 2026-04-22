#pragma once

#include <string>

enum class ElementType {
    Empty,
    Vacuum,  // Perfect void - no heat transfer
    Solid,  // Rock/Stone
    Gas_O2,  // Oxygen/Steam
    Gas_CO2,  // Carbon Dioxide
    Liquid_Water,
    Liquid_Lava,
    ContaminatedWater
};

// Forward declaration
class Cell;

// Element base class - all elements inherit from this
class Element {
public:
    // Constructor
    Element(const std::string& name, float density, float defaultTemp,
            float meltPoint, float boilPoint, float specHeat, float thermCond,
            float latentFusion, float latentVap, bool isFluid, bool isGas,
            bool isLiquid, bool isSolid, float visc, float expandRate, float heatTransRate)
        : name(name), density(density), defaultTemperature(defaultTemp),
          meltingPoint(meltPoint), boilingPoint(boilPoint),
          specificHeatCapacity(specHeat), thermalConductivity(thermCond),
          latentHeatOfFusion(latentFusion), latentHeatOfVaporization(latentVap),
          isFluid(isFluid), isGas(isGas), isLiquid(isLiquid), isSolid(isSolid),
          viscosity(visc), expansionRate(expandRate), heatTransferRate(heatTransRate) {}
    
    virtual ~Element() = default;
    
    // Physical properties
    std::string name;
    float density;  // kg/m³
    
    // Thermal properties
    float defaultTemperature;  // °C - temperature when placed
    float meltingPoint;  // °C (solid → liquid)
    float boilingPoint;  // °C (liquid → gas)
    float specificHeatCapacity;  // J/(kg·K)
    float thermalConductivity;  // W/(m·K)
    float latentHeatOfFusion;  // J/kg
    float latentHeatOfVaporization;  // J/kg
    
    // State flags
    bool isFluid;
    bool isGas;
    bool isLiquid;
    bool isSolid;
    
    // Behavior
    float viscosity;  // Resistance to flow
    float expansionRate;  // Thermal expansion
    float heatTransferRate;  // Heat exchange speed multiplier
};

// Type alias for backward compatibility
typedef Element ElementProperties;

// Concrete element types
class EmptyElement : public Element {
public:
    EmptyElement() : Element("Empty", 0.0f, 20.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                             false, false, false, false, 0.0f, 0.0f, 1.0f) {}
};

class RockElement : public Element {
public:
    RockElement() : Element("Rock", 2700.0f, 20.0f, 1200.0f, 3000.0f, 790.0f, 3.0f,
                            400000.0f, 6000000.0f, false, false, false, true,
                            0.0f, 0.00001f, 1.5f) {}
};

class SteamElement : public Element {
public:
    SteamElement() : Element("Steam", 0.6f, 100.0f, -218.8f, -183.0f, 2000.0f, 0.026f,
                             14000.0f, 2260000.0f, true, true, false, false,
                             0.0f, 0.00367f, 0.5f) {}
};

class CO2Element : public Element {
public:
    CO2Element() : Element("CO2", 1.98f, 20.0f, -78.5f, -56.6f, 840.0f, 0.016f,
                           0.0f, 574000.0f, true, true, false, false,
                           0.0f, 0.00367f, 0.5f) {}
};

class WaterElement : public Element {
public:
    WaterElement() : Element("Water", 1000.0f, 20.0f, 0.0f, 100.0f, 4186.0f, 0.6f,
                             334000.0f, 2260000.0f, true, false, true, false,
                             0.001f, 0.00021f, 1.0f) {}
};

class LavaElement : public Element {
public:
    LavaElement() : Element("Lava", 3100.0f, 1200.0f, 700.0f, 1200.0f, 840.0f, 1.5f,
                            400000.0f, 6000000.0f, true, false, true, false,
                            1000.0f, 0.00003f, 2.0f) {}
};

class ContaminatedWaterElement : public Element {
public:
    ContaminatedWaterElement() : Element("Contaminated Water", 1050.0f, 20.0f, -2.0f, 101.0f,
                                         4000.0f, 0.5f, 320000.0f, 2200000.0f,
                                         true, false, true, false, 0.0012f, 0.00021f, 1.0f) {}
};

class VacuumElement : public Element {
public:
    VacuumElement() : Element("Vacuum", 0.0f, -273.15f, 0.0f, 0.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, false, false, false, false,
                              0.0f, 0.0f, 0.0f) {}  // Zero heat transfer!
};

// Element registry - provides access to element instances
class ElementTypes {
public:
    static const Element& getElement(ElementType type);
    static std::string getTypeName(ElementType type);
    
private:
    // Singleton instances of each element type
    static EmptyElement emptyInstance;
    static VacuumElement vacuumInstance;
    static RockElement solidInstance;
    static SteamElement gasO2Instance;
    static CO2Element gasCO2Instance;
    static WaterElement waterInstance;
    static LavaElement lavaInstance;
    static ContaminatedWaterElement contaminatedWaterInstance;
};
