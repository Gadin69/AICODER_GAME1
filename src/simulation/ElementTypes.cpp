#include "ElementTypes.h"
#include "Cell.h"

// Initialize static singleton instances
EmptyElement ElementTypes::emptyInstance;
VacuumElement ElementTypes::vacuumInstance;
RockElement ElementTypes::solidInstance;
IceElement ElementTypes::iceInstance;
DryIceElement ElementTypes::dryIceInstance;
IndestructibleInsulatorElement ElementTypes::indestructibleInsulatorInstance;
SteamElement ElementTypes::gasO2Instance;
Gas_LavaElement ElementTypes::gasLavaInstance;
CO2Element ElementTypes::gasCO2Instance;
WaterElement ElementTypes::waterInstance;
LavaElement ElementTypes::lavaInstance;
ContaminatedWaterElement ElementTypes::contaminatedWaterInstance;
Solid_ContaminatedWaterElement ElementTypes::solidContaminatedWaterInstance;

const Element& ElementTypes::getElement(ElementType type) {
    switch (type) {
        case ElementType::Empty:
            return emptyInstance;
        case ElementType::Vacuum:
            return vacuumInstance;
        case ElementType::Solid:
            return solidInstance;
        case ElementType::Solid_Ice:
            return iceInstance;
        case ElementType::Solid_DryIce:
            return dryIceInstance;
        case ElementType::Solid_IndestructibleInsulator:
            return indestructibleInsulatorInstance;
        case ElementType::Gas_O2:
            return gasO2Instance;
        case ElementType::Gas_Lava:
            return gasLavaInstance;
        case ElementType::Gas_CO2:
            return gasCO2Instance;
        case ElementType::Liquid_Water:
            return waterInstance;
        case ElementType::Liquid_Lava:
            return lavaInstance;
        case ElementType::ContaminatedWater:
            return contaminatedWaterInstance;
        case ElementType::Solid_ContaminatedWater:
            return solidContaminatedWaterInstance;
        default:
            return emptyInstance;
    }
}

std::string ElementTypes::getTypeName(ElementType type) {
    return getElement(type).name;
}
