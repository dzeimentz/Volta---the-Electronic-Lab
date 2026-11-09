#include "volta/core/components/switch.hpp"

#include <algorithm>
#include <iterator>
#include <utility>

namespace volta::core {

namespace {
// Prellfolge (Zeitpunkte ab der Stellungsaenderung, an denen der Kontakt umspringt); ungerade Anzahl, damit am
// Ende die neue Stellung gilt
constexpr double kBounceCloseS[] = {0.0, 0.4e-3, 0.7e-3, 1.3e-3, 1.6e-3, 2.5e-3, 2.8e-3};
constexpr double kBounceOpenS[] = {0.0, 0.3e-3, 0.5e-3, 1.2e-3, 1.4e-3};
constexpr double kBounceMarginS = 1e-7;
} // namespace

Switch::Switch(std::string id, NodeId a, NodeId b, bool closed)
    : ComponentModel(std::move(id), {a, b}), m_closed(closed), m_contact(closed) {}

void Switch::beginSolve(const StampContext& ctx) noexcept {
    if (!m_bounce) return;
    if (ctx.isDc()) { // Arbeitspunkt: kein Prellen
        m_bouncePending = false;
        m_bounceStartS = -1.0;
        m_contact = m_closed;
        return;
    }
    m_stepStartS = ctx.timeS - ctx.dtS;
    m_stepEndS = ctx.timeS;
    if (m_bouncePending) {
        m_bouncePending = false;
        m_bounceStartS = m_stepStartS;
    }
    if (m_bounceStartS < 0.0) {
        m_contact = m_closed;
        return;
    }
    const double* seq = m_closed ? kBounceCloseS : kBounceOpenS;
    const std::size_t n = m_closed ? std::size(kBounceCloseS) : std::size(kBounceOpenS);
    // Zustand im Schritt (die Ereignisortung teilt den Schritt an jedem Umschaltpunkt)
    const double since = m_stepEndS - m_bounceStartS - kBounceMarginS;
    std::size_t toggles = 0;
    while (toggles < n && seq[toggles] <= since) ++toggles;
    if (toggles == 0) toggles = 1;
    if (toggles >= n) {
        m_bounceStartS = -1.0;
        m_contact = m_closed;
        return;
    }
    m_contact = (toggles % 2 == 1) ? m_closed : !m_closed;
}

double Switch::eventFraction(const SolutionView& /*accepted*/, const SolutionView& /*next*/,
                             double dtS) const noexcept {
    if (!m_bounce || m_bounceStartS < 0.0 || !(dtS > 0.0)) return 1.0;
    const double* seq = m_closed ? kBounceCloseS : kBounceOpenS;
    const std::size_t n = m_closed ? std::size(kBounceCloseS) : std::size(kBounceOpenS);
    for (std::size_t k = 0; k < n; ++k) {
        const double t = m_bounceStartS + seq[k];
        if (t > m_stepStartS + kBounceMarginS && t < m_stepEndS - kBounceMarginS)
            return std::min(1.0, (t - m_stepStartS) / dtS + kBounceMarginS / dtS);
    }
    return 1.0;
}

void Switch::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), conductance());
}

double Switch::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * conductance();
    return t == 0 ? i : -i;
}

ComponentReading Switch::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = m_closed ? 1.0 : 0.0;
    return r;
}

ChangeoverSwitch::ChangeoverSwitch(std::string id, NodeId common, NodeId a, NodeId b, bool toB)
    : ComponentModel(std::move(id), {common, a, b}), m_toB(toB) {}

void ChangeoverSwitch::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), gA());
    sys.stampConductance(terminal(0), terminal(2), gB());
}

double ChangeoverSwitch::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double vc = x.voltage(terminal(0));
    const double ia = (vc - x.voltage(terminal(1))) * gA(); // von der Mitte nach A
    const double ib = (vc - x.voltage(terminal(2))) * gB();
    if (t == 0) return ia + ib;
    return t == 1 ? -ia : -ib;
}

ComponentReading ChangeoverSwitch::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = m_toB ? 1.0 : 0.0;
    return r;
}

} // namespace volta::core
