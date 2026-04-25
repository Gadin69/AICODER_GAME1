#pragma once

#include <string>

enum class ElementType {
    Empty,
    Vacuum,  // Perfect void - no heat transfer
    Solid,  // Rock/Stone
    Solid_Ice,  // Frozen water
    Solid_DryIce,  // Frozen CO2 (dry ice)
    Solid_IndestructibleInsulator,  // Indestructible insulator (no heat transfer)
    Gas_O2,  // Oxygen/Steam
    Gas_Lava,  // Vaporized lava/magma
    Gas_CO2,  // Carbon Dioxide
    Liquid_Water,
    Liquid_Lava,
    ContaminatedWater,
    Solid_ContaminatedWater  // Frozen contaminated water
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
            bool isLiquid, bool isSolid, float visc, float expandRate, 
            float heatTransRate, float freezePoint, float condensePoint,
            float sublimePoint = 0.0f, float vaporizePoint = 0.0f)
        : name(name), density(density), defaultTemperature(defaultTemp),
          meltingPoint(meltPoint), boilingPoint(boilPoint),
          specificHeatCapacity(specHeat), thermalConductivity(thermCond),
          latentHeatOfFusion(latentFusion), latentHeatOfVaporization(latentVap),
          isFluid(isFluid), isGas(isGas), isLiquid(isLiquid), isSolid(isSolid),
          viscosity(visc), expansionRate(expandRate), heatTransferRate(heatTransRate),
          freezingPoint(freezePoint), condensationPoint(condensePoint),
          sublimationPoint(sublimePoint), vaporizationPoint(vaporizePoint) {}
    
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
    
    // Phase change temperatures
    float freezingPoint;         // °C (liquid → solid)
    float condensationPoint;     // °C (gas → liquid)
    float sublimationPoint;      // °C (solid → gas, if applicable)
    float vaporizationPoint;     // °C (liquid → gas, alternative to boiling)
    
    // Virtual methods for polymorphic behavior (can be overridden by derived classes)
    virtual bool canTransferHeat() const { return true; }
    virtual bool isDestructible() const { return true; }
};

// Type alias for backward compatibility
typedef Element ElementProperties;

// Include organized element definitions
#include "elements/SpecialElements.h"
#include "elements/BaseElements.h"
#include "elements/Liquids.h"
#include "elements/Gases.h"
#include "elements/Solids.h"

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
    static IceElement iceInstance;
    static DryIceElement dryIceInstance;
    static IndestructibleInsulatorElement indestructibleInsulatorInstance;
    static SteamElement gasO2Instance;
    static Gas_LavaElement gasLavaInstance;
    static CO2Element gasCO2Instance;
    static WaterElement waterInstance;
    static LavaElement lavaInstance;
    static ContaminatedWaterElement contaminatedWaterInstance;
    static Solid_ContaminatedWaterElement solidContaminatedWaterInstance;
};
