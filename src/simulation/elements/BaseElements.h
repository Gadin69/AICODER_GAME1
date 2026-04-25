#pragma once

#include "../ElementTypes.h"

// Forward declaration
class Cell;

// ============================================================================
// LIQUID BASE CLASS
// ============================================================================
class LiquidElement : public Element {
public:
    LiquidElement(const std::string& name, float density, float defaultTemp,
                  float meltPoint, float boilPoint, float specHeat, float thermCond,
                  float latentFusion, float latentVap,
                  float visc, float expandRate, float heatTransRate,
                  float freezePoint, float condensePoint,
                  float sublimePoint = 0.0f, float vaporizePoint = 0.0f)
        : Element(name, density, defaultTemp, meltPoint, boilPoint, specHeat, thermCond,
                  latentFusion, latentVap, true, false, true, false,
                  visc, expandRate, heatTransRate, freezePoint, condensePoint,
                  sublimePoint, vaporizePoint) {}
    
    // Virtual methods for custom liquid behavior
    virtual bool canSpreadSideways() const { return true; }
    virtual float getFlowRate() const { return 1.0f / (1.0f + viscosity * 0.1f); }
    virtual void onContactWith(Cell& self, Cell& other) {}
};

// ============================================================================
// GAS BASE CLASS
// ============================================================================
class GasElement : public Element {
public:
    GasElement(const std::string& name, float density, float defaultTemp,
               float meltPoint, float boilPoint, float specHeat, float thermCond,
               float latentFusion, float latentVap,
               float visc, float expandRate, float heatTransRate,
               float freezePoint, float condensePoint,
               float sublimePoint = 0.0f, float vaporizePoint = 0.0f)
        : Element(name, density, defaultTemp, meltPoint, boilPoint, specHeat, thermCond,
                  latentFusion, latentVap, true, true, false, false,
                  visc, expandRate, heatTransRate, freezePoint, condensePoint,
                  sublimePoint, vaporizePoint) {}
    
    // Virtual methods for custom gas behavior
    virtual float getExpansionRate() const { return expansionRate; }
    virtual bool canCompress() const { return true; }
};

// ============================================================================
// SOLID BASE CLASS
// ============================================================================
class SolidElement : public Element {
public:
    SolidElement(const std::string& name, float density, float defaultTemp,
                 float meltPoint, float boilPoint, float specHeat, float thermCond,
                 float latentFusion, float latentVap,
                 float visc, float expandRate, float heatTransRate,
                 float freezePoint, float condensePoint,
                 float sublimePoint = 0.0f, float vaporizePoint = 0.0f)
        : Element(name, density, defaultTemp, meltPoint, boilPoint, specHeat, thermCond,
                  latentFusion, latentVap, false, false, false, true,
                  visc, expandRate, heatTransRate, freezePoint, condensePoint,
                  sublimePoint, vaporizePoint) {}
    
    // Virtual methods for custom solid behavior
    virtual bool canTransferHeat() const { return true; }
    virtual bool isDestructible() const { return true; }
    virtual void onPhaseChange(Cell& cell, ElementType newType) {}
};
