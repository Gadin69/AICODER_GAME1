#pragma once

#include <string>

enum class ElementType {
    Empty,
    Vacuum,  // Perfect void - no heat transfer
    Solid,  // Rock/Stone
    Solid_Ice,  // Frozen water
    Solid_DryIce,  // Frozen CO2 (dry ice)
    Solid_Oil,  // Frozen oil
    Solid_IndestructibleInsulator,  // Indestructible insulator (no heat transfer)
    Gas_O2,  // Oxygen/Steam
    Gas_Lava,  // Vaporized lava/magma
    Gas_CO2,  // Carbon Dioxide
    Gas_Oil,  // Oil vapor
    Liquid_Water,
    Liquid_Lava,
    Liquid_Oil,  // Oil (floats on water)
    ContaminatedWater,
    Solid_ContaminatedWater,  // Frozen contaminated water
    
    // NEW GASES
    Gas_Hydrogen,
    Gas_Methane,
    Gas_Ammonia,
    Gas_Chlorine,
    Gas_SulfurDioxide,
    Gas_Propane,
    
    // NEW LIQUIDS
    Liquid_Mercury,
    Liquid_Ethanol,
    Liquid_Acetone,
    Liquid_Glycerol,
    Liquid_SulfuricAcid,
    Liquid_Nitrogen,
    Liquid_Oxygen,
    Liquid_Bromine,
    
    // NEW SOLIDS
    Solid_Iron,
    Solid_Copper,
    Solid_Aluminum,
    Solid_Silver,
    Solid_Gold,
    Solid_Lead,
    Solid_Zinc,
    
    // LIQUID METALS
    Liquid_Iron,
    Liquid_Copper,
    Liquid_Aluminum,
    Liquid_Silver,
    Liquid_Gold,
    Liquid_Lead,
    Liquid_Zinc,
    
    // LIQUID GAS FORMS (condensed gases)
    Liquid_Methane,
    Liquid_Ammonia,
    Liquid_Chlorine,
    Liquid_SulfurDioxide,
    Liquid_Propane,
    
    // GAS LIQUID FORMS (vaporized liquids)
    Gas_Mercury,
    Gas_Ethanol,
    Gas_Acetone,
    Gas_Glycerol,
    Gas_SulfuricAcid,
    Gas_Oxygen,
    Gas_Bromine,
    
    // CRYOGENIC ELEMENTS
    Liquid_Hydrogen,
    Gas_Nitrogen
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
    
    // Phase change system - each element defines its own phase transitions
    virtual ElementType getPhaseAtTemperature(float temp) const {
        return ElementType::Empty;  // Default: no phase change
    }
    
    virtual void applyPhaseChange(Cell& cell, ElementType newType, float oldMass) const;
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
    static Solid_OilElement solidOilInstance;
    static IndestructibleInsulatorElement indestructibleInsulatorInstance;
    static SteamElement gasO2Instance;
    static Gas_LavaElement gasLavaInstance;
    static CO2Element gasCO2Instance;
    static Gas_OilElement gasOilInstance;
    static WaterElement waterInstance;
    static LavaElement lavaInstance;
    static OilElement oilInstance;
    static ContaminatedWaterElement contaminatedWaterInstance;
    static Solid_ContaminatedWaterElement solidContaminatedWaterInstance;
    
    // NEW GAS INSTANCES
    static HydrogenElement gasHydrogenInstance;
    static MethaneElement gasMethaneInstance;
    static AmmoniaElement gasAmmoniaInstance;
    static ChlorineElement gasChlorineInstance;
    static SulfurDioxideElement gasSulfurDioxideInstance;
    static PropaneElement gasPropaneInstance;
    
    // NEW LIQUID INSTANCES
    static MercuryElement liquidMercuryInstance;
    static EthanolElement liquidEthanolInstance;
    static AcetoneElement liquidAcetoneInstance;
    static GlycerolElement liquidGlycerolInstance;
    static SulfuricAcidElement liquidSulfuricAcidInstance;
    static LiquidNitrogenElement liquidNitrogenInstance;
    static LiquidOxygenElement liquidOxygenInstance;
    static BromineElement liquidBromineInstance;
    
    // NEW SOLID INSTANCES
    static IronElement solidIronInstance;
    static CopperElement solidCopperInstance;
    static AluminumElement solidAluminumInstance;
    static SilverElement solidSilverInstance;
    static GoldElement solidGoldInstance;
    static LeadElement solidLeadInstance;
    static ZincElement solidZincInstance;
    
    // LIQUID METAL INSTANCES
    static LiquidIronElement liquidIronInstance;
    static LiquidCopperElement liquidCopperInstance;
    static LiquidAluminumElement liquidAluminumInstance;
    static LiquidSilverElement liquidSilverInstance;
    static LiquidGoldElement liquidGoldInstance;
    static LiquidLeadElement liquidLeadInstance;
    static LiquidZincElement liquidZincInstance;
    
    // LIQUID GAS FORMS (condensed gases)
    static LiquidMethaneElement liquidMethaneInstance;
    static LiquidAmmoniaElement liquidAmmoniaInstance;
    static LiquidChlorineElement liquidChlorineInstance;
    static LiquidSulfurDioxideElement liquidSulfurDioxideInstance;
    static LiquidPropaneElement liquidPropaneInstance;
    
    // GAS LIQUID FORMS (vaporized liquids)
    static GasMercuryElement gasMercuryInstance;
    static GasEthanolElement gasEthanolInstance;
    static GasAcetoneElement gasAcetoneInstance;
    static GasGlycerolElement gasGlycerolInstance;
    static GasSulfuricAcidElement gasSulfuricAcidInstance;
    static GasOxygenElement gasOxygenInstance;
    static GasBromineElement gasBromineInstance;
    
    // CRYOGENIC ELEMENTS
    static LiquidHydrogenElement liquidHydrogenInstance;
    static GasNitrogenElement gasNitrogenInstance;
};
