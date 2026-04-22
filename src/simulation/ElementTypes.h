#pragma once

#include <string>

enum class ElementType {
    Empty,
    Solid,  // Rock/Stone
    Gas_O2,  // Oxygen/Steam
    Gas_CO2,  // Carbon Dioxide
    Liquid_Water,
    Liquid_Lava,
    ContaminatedWater
};

struct ElementProperties {
    std::string name;
    ElementType type;
    
    // Physical properties
    float density;  // kg/m³
    
    // Thermal properties (real physics data)
    float meltingPoint;  // °C (solid → liquid)
    float boilingPoint;  // °C (liquid → gas)
    float specificHeatCapacity;  // J/(kg·K) - energy to raise 1kg by 1°C
    float thermalConductivity;  // W/(m·K) - how well it conducts heat
    float latentHeatOfFusion;  // J/kg - energy to melt
    float latentHeatOfVaporization;  // J/kg - energy to vaporize
    
    // State flags
    bool isFluid;
    bool isGas;
    bool isLiquid;
    bool isSolid;
    
    // Behavior
    float viscosity;  // Resistance to flow (higher = slower)
    float expansionRate;  // How much it expands when heated
};

class ElementTypes {
public:
    static ElementProperties getProperties(ElementType type);
    static std::string getTypeName(ElementType type);
};
