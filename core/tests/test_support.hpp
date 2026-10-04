#pragma once

// Hilfen fuer Kerntests: Kirchhoff-Pruefung und Toleranzvergleich nach core/CLAUDE.md.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

#include "volta/core/simulator.hpp"

#include <string>

namespace volta::core::test {

// Testmodell: Widerstand, der sich als nichtlinear ausgibt und eine einstellbare Zahl von Newton-Iterationen
// als "begrenzt" meldet. Damit lassen sich Nichtkonvergenz und Schrittweitenhalbierung gezielt ausloesen.
class StubbornResistor final : public ComponentModel {
public:
    StubbornResistor(std::string id, NodeId a, NodeId b) : ComponentModel(std::move(id), {a, b}) {}

    int failIterations = 0; // so viele linearize()-Aufrufe melden noch "begrenzt"; < 0: immer
    int linearizeCalls = 0;

    bool isNonlinear() const noexcept override { return true; }
    bool linearize(const SolutionView&) noexcept override {
        ++linearizeCalls;
        if (failIterations < 0) return true;
        if (failIterations > 0) {
            --failIterations;
            return true;
        }
        return false;
    }
    void stamp(MnaSystem& sys, const StampContext&) const noexcept override {
        sys.stampConductance(terminal(0), terminal(1), 1e-3);
    }
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override {
        const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * 1e-3;
        return t == 0 ? i : -i;
    }
};

// Lineare Referenzschaltungen sind exakt loesbar: enge Toleranz, damit auch kleine Modellfehler auffallen.
inline bool exact(double actual, double expected) {
    return std::fabs(actual - expected) <= 1e-6 * std::fabs(expected) + 1e-9;
}

// Toleranz gegen Referenz: 2 % relativ oder absolut (1 mV bzw. 1 uA).
inline bool near(double actual, double expected, double absTol) {
    return std::fabs(actual - expected) <= std::max(0.02 * std::fabs(expected), absTol);
}
inline bool nearV(double actual, double expected) { return near(actual, expected, 1e-3); }
inline bool nearA(double actual, double expected) { return near(actual, expected, 1e-6); }

// Summe der Stroeme je Knoten (inkl. GMIN-Ableitung gegen Masse) muss < 1e-9 A sein.
inline void requireKirchhoff(const Simulator& sim) {
    const Circuit& c = sim.circuit();
    const Probe& p = sim.probe();
    REQUIRE(p.valid);
    for (NodeId n = 1; n < c.nodeCount(); ++n) {
        double sum = kGmin * p.voltage(n);
        for (std::size_t i = 0; i < c.componentCount(); ++i) {
            const ComponentModel& m = c.component(i);
            for (std::size_t t = 0; t < m.terminalCount(); ++t)
                if (m.terminal(t) == n) sum += p.readings[i].terminalCurrentA[t];
        }
        INFO("Knoten " << n << ": Stromsumme " << sum << " A");
        CHECK(std::fabs(sum) < 1e-9);
    }
}

// Kein Messwert der Probe darf NaN/Inf sein.
inline void requireFinite(const Probe& p) {
    for (double v : p.nodeVolt) REQUIRE(std::isfinite(v));
    for (const auto& r : p.readings) {
        REQUIRE(std::isfinite(r.voltV));
        REQUIRE(std::isfinite(r.currentA));
        REQUIRE(std::isfinite(r.powerW));
        REQUIRE(std::isfinite(r.tempK));
        for (double i : r.terminalCurrentA) REQUIRE(std::isfinite(i));
    }
}

} // namespace volta::core::test
