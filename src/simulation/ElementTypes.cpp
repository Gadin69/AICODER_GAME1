#include "ElementTypes.h"

ElementProperties ElementTypes::getProperties(ElementType type) {
    switch (type) {
        case ElementType::Empty:
            return {"Empty", type, 0.0f, 20.0f, false, false, false, false};
        case ElementType::Solid:
            return {"Solid", type, 1000.0f, 20.0f, false, false, false, true};
        case ElementType::Gas_O2:
            return {"Oxygen", type, 0.001f, 20.0f, true, true, false, false};
        case ElementType::Gas_CO2:
            return {"CO2", type, 0.002f, 20.0f, true, true, false, false};
        case ElementType::Liquid_Water:
            return {"Water", type, 1000.0f, 20.0f, true, false, true, false};
        case ElementType::Liquid_Lava:
            return {"Lava", type, 3000.0f, 1200.0f, true, false, true, false};
        case ElementType::ContaminatedWater:
            return {"Contaminated Water", type, 1000.0f, 20.0f, true, false, true, false};
        default:
            return {"Unknown", type, 0.0f, 20.0f, false, false, false, false};
    }
}

std::string ElementTypes::getTypeName(ElementType type) {
    return getProperties(type).name;
}
