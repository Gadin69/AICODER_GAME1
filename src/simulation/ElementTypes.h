#pragma once

#include <string>

enum class ElementType {
    Empty,
    Solid,
    Gas_O2,
    Gas_CO2,
    Liquid_Water,
    Liquid_Lava,
    ContaminatedWater
};

struct ElementProperties {
    std::string name;
    ElementType type;
    float density;
    float temperature;
    bool isFluid;
    bool isGas;
    bool isLiquid;
    bool isSolid;
};

class ElementTypes {
public:
    static ElementProperties getProperties(ElementType type);
    static std::string getTypeName(ElementType type);
};
