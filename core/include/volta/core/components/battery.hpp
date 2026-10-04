#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

struct BatteryParams {
    int cells = 6;              // Reihe aus 1,5-V-Zellen; Volta 1 bietet 2, 3 und 6 Zellen (3 / 4,5 / 9 V)
    double cellVolt = 1.5;      // Nennspannung Alkali-Mangan-Zelle
    double cellInnerOhm = 0.25; // Ausgangswert, typisch 0,15-0,3 Ohm je frischer Zelle; Kalibrierung Phase 0
};

// Batterie des Bedienpults (Katalogtyp `batterie`): ideale Quelle cells * cellVolt mit Innenwiderstand.
// Anschluss 0 = Plus, Anschluss 1 = Minus. Erwaermung bei Kurzschluss folgt mit dem Thermikmodell.
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

private:
    BatteryParams m_params;
};

} // namespace volta::core
