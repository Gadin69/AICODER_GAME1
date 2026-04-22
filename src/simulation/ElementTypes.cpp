#include "ElementTypes.h"

ElementProperties ElementTypes::getProperties(ElementType type) {
    switch (type) {
        case ElementType::Empty:
            return {
                "Empty", type,
                0.0f,           // density
                0.0f,           // meltingPoint
                0.0f,           // boilingPoint
                0.0f,           // specificHeatCapacity
                0.0f,           // thermalConductivity
                0.0f,           // latentHeatOfFusion
                0.0f,           // latentHeatOfVaporization
                false, false, false, false,
                0.0f,           // viscosity
                0.0f            // expansionRate
            };
        
        case ElementType::Solid:  // Granite/Rock
            return {
                "Rock", type,
                2700.0f,         // density (kg/m³)
                1200.0f,         // meltingPoint (°C)
                3000.0f,         // boilingPoint (°C)
                790.0f,          // specificHeatCapacity J/(kg·K)
                3.0f,            // thermalConductivity W/(m·K)
                400000.0f,       // latentHeatOfFusion J/kg
                6000000.0f,      // latentHeatOfVaporization J/kg
                false, false, false, true,
                0.0f,            // viscosity (solid doesn't flow)
                0.00001f         // expansionRate (minimal)
            };
        
        case ElementType::Gas_O2:  // Oxygen/Water Vapor
            return {
                "Steam", type,
                0.6f,            // density (kg/m³) at 100°C
                -218.8f,         // meltingPoint (°C)
                -183.0f,         // boilingPoint (°C) for O2
                2000.0f,         // specificHeatCapacity J/(kg·K)
                0.026f,          // thermalConductivity W/(m·K)
                14000.0f,        // latentHeatOfFusion J/kg
                2260000.0f,      // latentHeatOfVaporization J/kg (water)
                true, true, false, false,
                0.0f,            // viscosity (gas flows freely)
                0.00367f         // expansionRate (gases expand a lot)
            };
        
        case ElementType::Gas_CO2:
            return {
                "CO2", type,
                1.98f,           // density (kg/m³)
                -78.5f,          // sublimationPoint (°C)
                -56.6f,          // triplePoint (°C)
                840.0f,          // specificHeatCapacity J/(kg·K)
                0.016f,          // thermalConductivity W/(m·K)
                0.0f,            // latentHeatOfFusion (sublimates)
                574000.0f,       // latentHeatOfSublimation J/kg
                true, true, false, false,
                0.0f,            // viscosity
                0.00367f         // expansionRate
            };
        
        case ElementType::Liquid_Water:
            return {
                "Water", type,
                1000.0f,         // density (kg/m³)
                0.0f,            // meltingPoint (°C)
                100.0f,          // boilingPoint (°C)
                4186.0f,         // specificHeatCapacity J/(kg·K) - HIGH!
                0.6f,            // thermalConductivity W/(m·K)
                334000.0f,       // latentHeatOfFusion J/kg
                2260000.0f,      // latentHeatOfVaporization J/kg - HUGE!
                true, false, true, false,
                0.001f,          // viscosity (Pa·s)
                0.00021f         // expansionRate per °C
            };
        
        case ElementType::Liquid_Lava:  // Basaltic magma
            return {
                "Lava", type,
                3100.0f,         // density (kg/m³)
                700.0f,          // meltingPoint (°C)
                1200.0f,         // boilingPoint (°C)
                840.0f,          // specificHeatCapacity J/(kg·K)
                1.5f,            // thermalConductivity W/(m·K)
                400000.0f,       // latentHeatOfFusion J/kg
                6000000.0f,      // latentHeatOfVaporization J/kg
                true, false, true, false,
                1000.0f,         // viscosity (VERY high - flows slowly)
                0.00003f         // expansionRate
            };
        
        case ElementType::ContaminatedWater:
            return {
                "Contaminated Water", type,
                1050.0f,         // density (slightly heavier)
                -2.0f,           // meltingPoint (depressed)
                101.0f,          // boilingPoint (elevated)
                4000.0f,         // specificHeatCapacity
                0.5f,            // thermalConductivity
                320000.0f,       // latentHeatOfFusion
                2200000.0f,      // latentHeatOfVaporization
                true, false, true, false,
                0.0012f,         // viscosity (slightly higher)
                0.00021f         // expansionRate
            };
        
        default:
            return {
                "Unknown", type,
                0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                false, false, false, false,
                0.0f, 0.0f
            };
    }
}

std::string ElementTypes::getTypeName(ElementType type) {
    return getProperties(type).name;
}
