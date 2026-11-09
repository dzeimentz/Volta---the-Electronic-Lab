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
    void setClosed(bool closed) noexcept {
        if (m_bounce && closed != m_closed) m_bouncePending = true;
        m_closed = closed;
    }
    // Ab Volta 3 (Logik): Der Kontakt prellt nach jeder Stellungsaenderung knapp 3 ms lang (feste Folge, Abstaende
    // 0,2-0,9 ms; gemessene Taster prellen typisch 1-5 ms). Ohne setBounce(true) unveraendert.
    void setBounce(bool on) noexcept { m_bounce = on; }
    bool contactClosed() const noexcept { return m_bounce ? m_contact : m_closed; }

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    double eventFraction(const SolutionView& accepted, const SolutionView& next, double dtS) const noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    double conductance() const noexcept { return contactClosed() ? 1.0 / kContactClosedOhm : kContactOpenS; }
    bool m_closed = false;
    bool m_bounce = false;
    bool m_bouncePending = false;
    bool m_contact = false;       // wirksamer Kontakt beim Prellen
    double m_bounceStartS = -1.0; // Beginn des laufenden Prellens (< 0: keins)
    double m_stepStartS = 0.0;
    double m_stepEndS = 0.0;
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
