// Wechselstromanalyse (Volta 3, Konzept Kap. 4): Referenzschaltungen gegen die Formel bzw. gegen den Zeitbereich.
#include <algorithm>
#include <cmath>
#include <vector>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/inductor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/sources.hpp"
#include "volta/core/components/transistor.hpp"
#include "volta/core/simulator.hpp"

using namespace volta::core;
using namespace volta::core::test;

namespace {
constexpr double kPi = 3.14159265358979323846;

std::vector<double> logSweep(double f0, double f1, int n) {
    std::vector<double> f(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) f[static_cast<std::size_t>(i)] = f0 * std::pow(f1 / f0, i / double(n - 1));
    return f;
}
} // namespace

TEST_CASE("Wechselstromanalyse: RC-Tiefpass folgt 1/(1 + j w R C)", "[ac]") {
    Circuit c;
    const NodeId g = c.addNode(), out = c.addNode();
    c.add<FunctionGenerator>("G", g, kGround); // Ausgangswiderstand 50 Ohm
    c.add<Resistor>("R", g, out, ResistorParams{950.0});
    c.add<Capacitor>("C", out, kGround, CapacitorParams::film(100e-9));
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);

    const std::vector<double> f{1.0, 100.0, 1591.55, 10e3, 1e6};
    std::vector<Complex> h;
    REQUIRE(sim.acSweep(0, f, out, kGround, h) == Status::Ok);
    for (std::size_t i = 0; i < f.size(); ++i) {
        const Complex ref = 1.0 / Complex(1.0, 2.0 * kPi * f[i] * 1000.0 * 100e-9);
        CHECK(std::abs(h[i] - ref) <= 1e-4 * std::abs(ref) + 1e-9);
    }
    // Grenzfrequenz: -3 dB und -45 Grad
    CHECK(std::abs(h[2]) == Catch::Approx(1.0 / std::sqrt(2.0)).epsilon(1e-3));
    CHECK(std::arg(h[2]) == Catch::Approx(-kPi / 4.0).epsilon(1e-3));
}

TEST_CASE("Wechselstromanalyse: Parallelschwingkreis schwingt bei der Thomson-Frequenz", "[ac]") {
    // 100 uH und 253,3 pF: f0 = 1 / (2 pi sqrt(L C)) = 1,000 MHz (Mittelwelle)
    const double l = 100e-6, cap = 253.3e-12;
    const double f0 = 1.0 / (2.0 * kPi * std::sqrt(l * cap));
    Circuit c;
    const NodeId g = c.addNode(), t = c.addNode();
    c.add<FunctionGenerator>("G", g, kGround);
    c.add<Resistor>("R", g, t, ResistorParams{100e3});
    c.add<Inductor>("L", t, kGround, InductorParams{l, 2.0, 1e6, 0.1});
    c.add<Capacitor>("C", t, kGround, CapacitorParams::film(cap));
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);

    const std::vector<double> f = logSweep(0.5e6, 2e6, 2001);
    std::vector<Complex> h;
    REQUIRE(sim.acSweep(0, f, t, kGround, h) == Status::Ok);
    std::size_t peak = 0;
    for (std::size_t i = 0; i < h.size(); ++i)
        if (std::abs(h[i]) > std::abs(h[peak])) peak = i;
    CHECK(near(f[peak], f0, 0.0));
    // weit neben der Resonanz praktisch kein Durchlass, im Maximum deutlich
    CHECK(std::abs(h.front()) < 0.1 * std::abs(h[peak]));
    CHECK(std::abs(h.back()) < 0.1 * std::abs(h[peak]));
    // bei Gleichstrom schliesst die Spule kurz (Wicklungswiderstand 2 Ohm gegen 100 kOhm)
    std::vector<Complex> dc;
    REQUIRE(sim.acSweep(0, {0.0}, t, kGround, dc) == Status::Ok);
    CHECK(std::abs(dc[0]) < 1e-4);
}

TEST_CASE("Wechselstromanalyse: Emitterschaltung stimmt mit dem Zeitbereich ueberein", "[ac]") {
    Circuit c;
    const NodeId plus = c.addNode(), g = c.addNode(), b = c.addNode(), col = c.addNode(), e = c.addNode();
    c.add<Battery>("B", plus, kGround);
    auto& gen = c.add<FunctionGenerator>("G", g, kGround);
    c.add<Capacitor>("Cin", b, g, CapacitorParams::electrolytic(10e-6)); // Plus an der Basis (1,4 V)
    c.add<Resistor>("R1", plus, b, ResistorParams{47e3});
    c.add<Resistor>("R2", b, kGround, ResistorParams{10e3});
    c.add<Resistor>("Rc", plus, col, ResistorParams{2.2e3});
    c.add<Npn>("T", b, col, e);
    c.add<Resistor>("Re", e, kGround, ResistorParams{470.0});
    c.add<Capacitor>("Ce", e, kGround, CapacitorParams::electrolytic(100e-6));
    gen.setFrequency(1000.0);
    gen.setAmplitude(1e-3); // klein gegen Vt: lineare Naeherung gilt
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);

    // eingeschwungen (Koppel- und Emitterkondensator geladen)
    for (int i = 0; i < 50000; ++i) REQUIRE(sim.step(10e-6) == Status::Ok);
    std::vector<Complex> h;
    REQUIRE(sim.acSweep(1, {1000.0}, col, kGround, h) == Status::Ok);
    const double expected = 1e-3 * std::abs(h[0]);
    REQUIRE(expected > 0.05); // Verstaerkung deutlich > 50

    double lo = 1e9, hi = -1e9;
    for (int i = 0; i < 1000; ++i) { // 10 ms = 10 Perioden
        REQUIRE(sim.step(10e-6) == Status::Ok);
        const double v = sim.probe().voltage(col);
        lo = std::min(lo, v);
        hi = std::max(hi, v);
    }
    CHECK(near(0.5 * (hi - lo), expected, 1e-4));
    requireKirchhoff(sim);
}

TEST_CASE("Wechselstromanalyse aendert den Zustand nicht und prueft die Anregung", "[ac]") {
    auto make = [] {
        Circuit c;
        const NodeId plus = c.addNode(), g = c.addNode(), out = c.addNode();
        c.add<Battery>("B", plus, kGround);
        c.add<Resistor>("R1", plus, out, ResistorParams{1e3});
        c.add<FunctionGenerator>("G", g, kGround);
        c.add<Capacitor>("C", g, out, CapacitorParams::film(1e-6));
        c.add<Resistor>("R2", out, kGround, ResistorParams{1e3});
        return c;
    };
    const NodeId out = 3;
    Simulator sim, twin;
    REQUIRE(sim.setCircuit(make()) == Status::Ok);
    REQUIRE(twin.setCircuit(make()) == Status::Ok);
    for (int i = 0; i < 10; ++i) {
        REQUIRE(sim.step(1e-4) == Status::Ok);
        REQUIRE(twin.step(1e-4) == Status::Ok);
    }
    std::vector<Complex> h;
    CHECK(sim.acSweep(1, {100.0}, out, kGround, h) == Status::InvalidCircuit); // Widerstand ist keine Anregung
    REQUIRE(sim.acSweep(2, {100.0, 1e4}, out, kGround, h) == Status::Ok);
    CHECK(sim.probe().voltage(out) == twin.probe().voltage(out));
    // Hochpass aus C und R1 || R2 (Batterie wirkt als Kurzschluss): bei 10 kHz fast 1
    CHECK(std::abs(h[1]) == Catch::Approx(500.0 / 550.0).epsilon(0.01));
    for (int i = 0; i < 10; ++i) {
        REQUIRE(sim.step(1e-4) == Status::Ok);
        REQUIRE(twin.step(1e-4) == Status::Ok);
    }
    CHECK(sim.probe().voltage(out) == Catch::Approx(twin.probe().voltage(out)).margin(1e-12));
}

TEST_CASE("Praediktor und Toleranz (Volta 3) aendern das Ergebnis nur im Rahmen der Toleranz", "[volta3]") {
    auto make = [] {
        Circuit c;
        const NodeId plus = c.addNode(), g = c.addNode(), b = c.addNode(), col = c.addNode(), e = c.addNode();
        c.add<Battery>("B", plus, kGround);
        auto& gen = c.add<FunctionGenerator>("G", g, kGround);
        c.add<Capacitor>("Cin", b, g, CapacitorParams::electrolytic(10e-6));
        c.add<Resistor>("R1", plus, b, ResistorParams{47e3});
        c.add<Resistor>("R2", b, kGround, ResistorParams{10e3});
        c.add<Resistor>("Rc", plus, col, ResistorParams{2.2e3});
        c.add<Npn>("T", b, col, e);
        c.add<Resistor>("Re", e, kGround, ResistorParams{470.0});
        gen.setFrequency(1000.0);
        gen.setAmplitude(0.05);
        return c;
    };
    Simulator plain, fast;
    REQUIRE(plain.setCircuit(make()) == Status::Ok);
    REQUIRE(fast.setCircuit(make()) == Status::Ok);
    fast.setPredictor(true);
    fast.setRelativeTolerance(1e-4);
    CHECK(fast.relativeTolerance() == 1e-4);
    long itPlain = 0, itFast = 0;
    for (int i = 0; i < 20000; ++i) {
        REQUIRE(plain.step(20e-6) == Status::Ok);
        REQUIRE(fast.step(20e-6) == Status::Ok);
        itPlain += plain.lastStepIterations();
        itFast += fast.lastStepIterations();
        REQUIRE(std::fabs(fast.probe().voltage(4) - plain.probe().voltage(4)) < 1e-3);
    }
    CHECK(itFast < itPlain);
    requireKirchhoff(fast);
    // Grenzen
    fast.setRelativeTolerance(1.0);
    CHECK(fast.relativeTolerance() == 1e-2);
    fast.setRelativeTolerance(std::nan(""));
    CHECK(fast.relativeTolerance() == kTolRel);
}
