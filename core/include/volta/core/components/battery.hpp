#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

struct BatteryParams {
    int cells = 6;              // Reihe aus 1,5-V-Zellen; Volta 1 bietet 2, 3 und 6 Zellen (3 / 4,5 / 9 V)
    double cellVolt = 1.5;      // Nennspannung Alkali-Mangan-Zelle
    double cellInnerOhm = 0.25; // Ausgangswert, typisch 0,15-0,3 Ohm je frischer Zelle; Kalibrierung Phase 0
    // Erwaermung durch Verlustleistung am Innenwiderstand (I*I*Ri). Didaktisch: Kurzschluss ueber eine Leitung
    // (ca. 6 A, 54 W bei 9 V) macht die Batterie nach ca. 2 s heiss.
    ThermalParams thermal{20.0, 60.0 / 20.0, 0.0}; // keine Zerstoerung
    double hotK = 333.15;                          // BatteryHot oberhalb 60 Grad C
    double coolK = 318.15;                         // erneute Meldung erst nach Abkuehlen unter 45 Grad C
};

// Batterie des Bedienpults (Katalogtyp `batterie`): ideale Quelle cells * cellVolt mit Innenwiderstand.
// Anschluss 0 = Plus, Anschluss 1 = Minus. Erwaermt sich bei Kurzschluss und meldet SimEventType::BatteryHot.
class Battery final : public ComponentModel {
public:
    static constexpr int kMinCells = 1;
    static constexpr int kMaxCells = 6;

    Battery(std::string id, NodeId plus, NodeId minus, BatteryParams params = {});

    const BatteryParams& params() const noexcept { return m_params; }
    void setCells(int cells) noexcept;
    double emfV() const noexcept { return m_params.cells * m_params.cellVolt; }
    double innerOhm() const noexcept { return m_params.cells * m_params.cellInnerOhm; }

    std::size_t branchCount() const noexcept override { return 1; }
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

    bool isHot() const noexcept { return m_hot; }

private:
    BatteryParams m_params;
    bool m_hot = false;
};

} // namespace volta::core
