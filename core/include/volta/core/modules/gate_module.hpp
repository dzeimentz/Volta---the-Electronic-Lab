#pragma once

#include <array>
#include <string>

#include "volta/core/circuit.hpp"
#include "volta/core/component_model.hpp"
#include "volta/core/modules/blink_module.hpp"

namespace volta::core {

// Kennwerte eines NAND-Gatters mit Schmitt-Trigger-Eingaengen, angelehnt an den CMOS-Baustein 4093 (3-15 V):
// Schaltschwellen als Anteil der Versorgung (bei 10 V im Datenblatt typisch 5,9 V und 3,9 V), Ausgang ueber
// etwa 200 Ohm nach +U bzw. -U, Eingaenge stromlos.
struct NandGateParams {
    double upperFraction = 0.6; // steigende Eingangsspannung: ab hier H
    double lowerFraction = 0.4; // fallende Eingangsspannung: ab hier L
    double outOhm = 200.0;
    double minSupplyV = 3.0;    // darunter arbeitet das Gatter nicht (Ausgang offen)
    double ratedA = 0.01;       // Ausgangsstrom; darueber ueberlastet
    // Einschaltzustand: Ein echter Baustein entscheidet ein symmetrisches Flipflop durch kleine Unterschiede der
    // Gatter; hier fest: Gatter 1 und 3 starten mit Ausgang H, Gatter 2 und 4 mit L.
    bool startLow = false;
};

// Ein NAND-Gatter des Gatter-Moduls (Katalogtyp `gattermodul`, ab Volta 3). Logik ereignisgesteuert: Die
// Eingangszustaende (mit Hysterese) und der Ausgang werden je Loesungsvorgang festgeschrieben (das Gatter ist damit
// linear) und nach jedem angenommenen Schritt fortgeschrieben; eventFraction() teilt den Schritt, an dem ein Eingang
// eine Schwelle kreuzt (Ereignisortung wie beim 555). Ausgang H = Leitwert zu +U, L = Leitwert zu -U.
// Anschluesse: 0 plus, 1 minus, 2 a, 3 b, 4 y. Messwerte: voltV = Versorgung, level = Ausgang H (1) oder L (0),
// displayValue = Ausgangsspannung gegen -U.
class NandGate final : public ComponentModel {
public:
    NandGate(std::string id, NodeId plus, NodeId minus, NodeId a, NodeId b, NodeId y, NandGateParams params = {});

    bool outputHigh() const noexcept { return m_out; }
    bool inputHigh(int k) const noexcept { return m_in[static_cast<std::size_t>(k & 1)]; }
    bool powered() const noexcept { return m_powered; }

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    double eventFraction(const SolutionView& accepted, const SolutionView& next, double dtS) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    double rel(const SolutionView& x, std::size_t t) const noexcept; // gegen -U
    double outputCurrent(const SolutionView& x) const noexcept;      // aus dem Ausgang heraus
    NandGateParams m_params;
    std::array<bool, 2> m_in{false, false};
    bool m_out = true;
    bool m_powered = false;
    double m_lastOutA = 0.0;
};

// Gatter-Modul: vier NAND an gemeinsamer Versorgung (Katalog: plus, minus, a1, b1, y1, ..., a4, b4, y4)
struct GateModulePins {
    NodeId plus = kGround;
    NodeId minus = kGround;
    std::array<NodeId, 12> io{}; // a1, b1, y1, a2, b2, y2, a3, b3, y3, a4, b4, y4
};

SubcircuitRange addGateModule(Circuit& circuit, const std::string& prefix, const GateModulePins& pins);

} // namespace volta::core
