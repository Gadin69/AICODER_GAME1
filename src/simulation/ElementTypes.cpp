#include "ElementTypes.h"
#include "Cell.h"
#include <algorithm>

// Initialize static singleton instances
EmptyElement ElementTypes::emptyInstance;
VacuumElement ElementTypes::vacuumInstance;
RockElement ElementTypes::solidInstance;
IceElement ElementTypes::iceInstance;
DryIceElement ElementTypes::dryIceInstance;
Solid_OilElement ElementTypes::solidOilInstance;
IndestructibleInsulatorElement ElementTypes::indestructibleInsulatorInstance;
SteamElement ElementTypes::gasO2Instance;
Gas_LavaElement ElementTypes::gasLavaInstance;
CO2Element ElementTypes::gasCO2Instance;
Gas_OilElement ElementTypes::gasOilInstance;
WaterElement ElementTypes::waterInstance;
LavaElement ElementTypes::lavaInstance;
OilElement ElementTypes::oilInstance;
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
        case ElementType::Solid_Oil:
            return solidOilInstance;
        case ElementType::Solid_IndestructibleInsulator:
            return indestructibleInsulatorInstance;
        case ElementType::Gas_O2:
            return gasO2Instance;
        case ElementType::Gas_Lava:
            return gasLavaInstance;
        case ElementType::Gas_CO2:
            return gasCO2Instance;
        case ElementType::Gas_Oil:
            return gasOilInstance;
        case ElementType::Liquid_Water:
            return waterInstance;
        case ElementType::Liquid_Lava:
            return lavaInstance;
        case ElementType::Liquid_Oil:
            return oilInstance;
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

// Base implementation of applyPhaseChange - handles common phase change logic
void Element::applyPhaseChange(Cell& cell, ElementType newType, float oldMass) const {
    const Element& newProps = ElementTypes::getElement(newType);
    
    // Preserve temperature (but clamp to valid range for new phase)
    float preservedTemp = cell.temperature;
    float newMass = oldMass;
    
    // Mass conversion rules
    if (isLiquid && newProps.isGas) {
        newMass = oldMass;  // All mass converts
    } else if (isGas && newProps.isLiquid) {
        newMass = oldMass;  // Preserve mass
    }
    // Solid <-> Liquid: Keep mass as-is
    // Solid <-> Gas: Keep mass as-is
    
    // Apply changes
    cell.elementType = newType;
    cell.mass = newMass;
    
    // TEMPERATURE CLAMPING: Ensure temperature is reasonable for the new phase
    // This prevents "super-cooled" elements that freeze everything to absolute zero
    if (newProps.isSolid) {
        // Solids: Clamp to [freezingPoint - 300, freezingPoint]
        // Ice at -273°C is unrealistic; ice should be near its freezing point
        float maxTemp = newProps.freezingPoint;
        float minTemp = maxTemp - 300.0f;  // Allow some supercooling but not to absolute zero
        preservedTemp = std::max(minTemp, std::min(preservedTemp, maxTemp));
    } else if (newProps.isLiquid) {
        // Liquids: Clamp to [freezingPoint, boilingPoint]
        preservedTemp = std::max(newProps.freezingPoint, std::min(preservedTemp, newProps.boilingPoint));
    } else if (newProps.isGas) {
        // Gases: Clamp to [boilingPoint, boilingPoint + 2000]
        preservedTemp = std::max(newProps.boilingPoint, std::min(preservedTemp, newProps.boilingPoint + 2000.0f));
    }
    
    cell.temperature = preservedTemp;
    
    // Pressure for gases
    if (newProps.isGas) {
        cell.pressure = (newMass * 8.314f * (cell.temperature + 273.15f)) / 0.001f;  // Ideal gas law
    } else {
        cell.pressure = 0.0f;
    }
    
    // Reset velocity (but give liquids gravity to fall immediately)
    cell.velocityX = 0.0f;
    cell.velocityY = newProps.isLiquid ? 2.0f : 0.0f;
    
    // Update color to new element
    cell.updateColor();
    
    // Mark cell as updated so it gets simulated next frame
    cell.updated = true;
    
    // Prevent below absolute zero
    if (cell.temperature < -273.15f) {
        cell.temperature = 20.0f;  // Room temp (neutral)
    }
}
