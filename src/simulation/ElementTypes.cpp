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

// NEW GAS INSTANCES
HydrogenElement ElementTypes::gasHydrogenInstance;
MethaneElement ElementTypes::gasMethaneInstance;
AmmoniaElement ElementTypes::gasAmmoniaInstance;
ChlorineElement ElementTypes::gasChlorineInstance;
SulfurDioxideElement ElementTypes::gasSulfurDioxideInstance;
PropaneElement ElementTypes::gasPropaneInstance;

// NEW LIQUID INSTANCES
MercuryElement ElementTypes::liquidMercuryInstance;
EthanolElement ElementTypes::liquidEthanolInstance;
AcetoneElement ElementTypes::liquidAcetoneInstance;
GlycerolElement ElementTypes::liquidGlycerolInstance;
SulfuricAcidElement ElementTypes::liquidSulfuricAcidInstance;
LiquidNitrogenElement ElementTypes::liquidNitrogenInstance;
LiquidOxygenElement ElementTypes::liquidOxygenInstance;
BromineElement ElementTypes::liquidBromineInstance;

// NEW SOLID INSTANCES
IronElement ElementTypes::solidIronInstance;
CopperElement ElementTypes::solidCopperInstance;
AluminumElement ElementTypes::solidAluminumInstance;
SilverElement ElementTypes::solidSilverInstance;
GoldElement ElementTypes::solidGoldInstance;
LeadElement ElementTypes::solidLeadInstance;
ZincElement ElementTypes::solidZincInstance;

// LIQUID METAL INSTANCES
LiquidIronElement ElementTypes::liquidIronInstance;
LiquidCopperElement ElementTypes::liquidCopperInstance;
LiquidAluminumElement ElementTypes::liquidAluminumInstance;
LiquidSilverElement ElementTypes::liquidSilverInstance;
LiquidGoldElement ElementTypes::liquidGoldInstance;
LiquidLeadElement ElementTypes::liquidLeadInstance;
LiquidZincElement ElementTypes::liquidZincInstance;

// LIQUID GAS FORMS (condensed gases)
LiquidMethaneElement ElementTypes::liquidMethaneInstance;
LiquidAmmoniaElement ElementTypes::liquidAmmoniaInstance;
LiquidChlorineElement ElementTypes::liquidChlorineInstance;
LiquidSulfurDioxideElement ElementTypes::liquidSulfurDioxideInstance;
LiquidPropaneElement ElementTypes::liquidPropaneInstance;

// GAS LIQUID FORMS (vaporized liquids)
GasMercuryElement ElementTypes::gasMercuryInstance;
GasEthanolElement ElementTypes::gasEthanolInstance;
GasAcetoneElement ElementTypes::gasAcetoneInstance;
GasGlycerolElement ElementTypes::gasGlycerolInstance;
GasSulfuricAcidElement ElementTypes::gasSulfuricAcidInstance;
GasOxygenElement ElementTypes::gasOxygenInstance;
GasBromineElement ElementTypes::gasBromineInstance;

// CRYOGENIC ELEMENTS
LiquidHydrogenElement ElementTypes::liquidHydrogenInstance;
GasNitrogenElement ElementTypes::gasNitrogenInstance;

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
        
        // NEW GASES
        case ElementType::Gas_Hydrogen:
            return gasHydrogenInstance;
        case ElementType::Gas_Methane:
            return gasMethaneInstance;
        case ElementType::Gas_Ammonia:
            return gasAmmoniaInstance;
        case ElementType::Gas_Chlorine:
            return gasChlorineInstance;
        case ElementType::Gas_SulfurDioxide:
            return gasSulfurDioxideInstance;
        case ElementType::Gas_Propane:
            return gasPropaneInstance;
        
        // NEW LIQUIDS
        case ElementType::Liquid_Mercury:
            return liquidMercuryInstance;
        case ElementType::Liquid_Ethanol:
            return liquidEthanolInstance;
        case ElementType::Liquid_Acetone:
            return liquidAcetoneInstance;
        case ElementType::Liquid_Glycerol:
            return liquidGlycerolInstance;
        case ElementType::Liquid_SulfuricAcid:
            return liquidSulfuricAcidInstance;
        case ElementType::Liquid_Nitrogen:
            return liquidNitrogenInstance;
        case ElementType::Liquid_Oxygen:
            return liquidOxygenInstance;
        case ElementType::Liquid_Bromine:
            return liquidBromineInstance;
        
        // NEW SOLIDS
        case ElementType::Solid_Iron:
            return solidIronInstance;
        case ElementType::Solid_Copper:
            return solidCopperInstance;
        case ElementType::Solid_Aluminum:
            return solidAluminumInstance;
        case ElementType::Solid_Silver:
            return solidSilverInstance;
        case ElementType::Solid_Gold:
            return solidGoldInstance;
        case ElementType::Solid_Lead:
            return solidLeadInstance;
        case ElementType::Solid_Zinc:
            return solidZincInstance;
        
        // LIQUID METALS
        case ElementType::Liquid_Iron:
            return liquidIronInstance;
        case ElementType::Liquid_Copper:
            return liquidCopperInstance;
        case ElementType::Liquid_Aluminum:
            return liquidAluminumInstance;
        case ElementType::Liquid_Silver:
            return liquidSilverInstance;
        case ElementType::Liquid_Gold:
            return liquidGoldInstance;
        case ElementType::Liquid_Lead:
            return liquidLeadInstance;
        case ElementType::Liquid_Zinc:
            return liquidZincInstance;
        
        // LIQUID GAS FORMS
        case ElementType::Liquid_Methane:
            return liquidMethaneInstance;
        case ElementType::Liquid_Ammonia:
            return liquidAmmoniaInstance;
        case ElementType::Liquid_Chlorine:
            return liquidChlorineInstance;
        case ElementType::Liquid_SulfurDioxide:
            return liquidSulfurDioxideInstance;
        case ElementType::Liquid_Propane:
            return liquidPropaneInstance;
        
        // GAS LIQUID FORMS
        case ElementType::Gas_Mercury:
            return gasMercuryInstance;
        case ElementType::Gas_Ethanol:
            return gasEthanolInstance;
        case ElementType::Gas_Acetone:
            return gasAcetoneInstance;
        case ElementType::Gas_Glycerol:
            return gasGlycerolInstance;
        case ElementType::Gas_SulfuricAcid:
            return gasSulfuricAcidInstance;
        case ElementType::Gas_Oxygen:
            return gasOxygenInstance;
        case ElementType::Gas_Bromine:
            return gasBromineInstance;
        
        // CRYOGENIC ELEMENTS
        case ElementType::Liquid_Hydrogen:
            return liquidHydrogenInstance;
        case ElementType::Gas_Nitrogen:
            return gasNitrogenInstance;
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
