#include "ElementTypes.h"
#include "Cell.h"

// Initialize static singleton instances
EmptyElement ElementTypes::emptyInstance;
VacuumElement ElementTypes::vacuumInstance;
RockElement ElementTypes::solidInstance;
SteamElement ElementTypes::gasO2Instance;
CO2Element ElementTypes::gasCO2Instance;
WaterElement ElementTypes::waterInstance;
LavaElement ElementTypes::lavaInstance;
ContaminatedWaterElement ElementTypes::contaminatedWaterInstance;

const Element& ElementTypes::getElement(ElementType type) {
    switch (type) {
        case ElementType::Empty:
            return emptyInstance;
        case ElementType::Vacuum:
            return vacuumInstance;
        case ElementType::Solid:
            return solidInstance;
        case ElementType::Gas_O2:
            return gasO2Instance;
        case ElementType::Gas_CO2:
            return gasCO2Instance;
        case ElementType::Liquid_Water:
            return waterInstance;
        case ElementType::Liquid_Lava:
            return lavaInstance;
        case ElementType::ContaminatedWater:
            return contaminatedWaterInstance;
        default:
            return emptyInstance;
    }
}

std::string ElementTypes::getTypeName(ElementType type) {
    return getElement(type).name;
}
