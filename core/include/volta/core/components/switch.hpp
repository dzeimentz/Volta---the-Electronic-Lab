#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Kontaktwiderstaende fuer Schalter, Taster und Reedkontakt.
inline constexpr double kContactClosedOhm = 0.02; // geschlossener Kontakt
inline constexpr double kContactOpenS = kGmin;   // offener Kontakt: nur Isolationsleitwert

// Ein-Aus-Schalter, Taster, Reedkontakt und Hauptschalter (Katalogtypen `schalter`, `taster`, `reedkontakt`).
// Die Stellung ist eine Bedienhandlung (setClosed) und gehoert nicht zum Undo-Zustand. Ob ein Taster federnd
// zurueckgeht oder ein Reedkontakt auf den Magneten reagiert, entscheidet die App.
class Switch final : public ComponentModel {
public:
    Switch(std::string id, NodeId a, NodeId b, bool closed = false);

    bool isClosed() const noexcept { return m_closed; }
    void setClosed(bool closed) noexcept { m_closed = closed; }

    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    double conductance() const noexcept { return m_closed ? 1.0 / kContactClosedOhm : kContactOpenS; }
    bool m_closed = false;
};

// Wechselschalter (Katalogtyp `wechselschalter`): Anschluss 0 = Mitte, 1 = Stellung A, 2 = Stellung B.
class ChangeoverSwitch final : public ComponentModel {
public:
    ChangeoverSwitch(std::string id, NodeId common, NodeId a, NodeId b, bool toB = false);

    bool isToB() const noexcept { return m_toB; }
    void setToB(bool toB) noexcept { m_toB = toB; }

    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    double gA() const noexcept { return m_toB ? kContactOpenS : 1.0 / kContactClosedOhm; }
    double gB() const noexcept { return m_toB ? 1.0 / kContactClosedOhm : kContactOpenS; }
    bool m_toB = false;
};

} // namespace volta::core
