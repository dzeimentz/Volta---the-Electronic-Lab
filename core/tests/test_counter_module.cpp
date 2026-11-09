// Zaehler- und Anzeige-Module (ab Volta 3, Stufe 8): Binaerzaehler zaehlt Flanken und laesst sich ruecksetzen,
// Q0 teilt einen Schmitt-Oszillator durch zwei, Dekadenzaehler schiebt einen H-Ausgang durch Q0 ... Q9,
// Anzeige-Modul dekodiert BCD und zieht je Segment (U - 1,9 V) / 680 Ohm, Mindestversorgung.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/modules/counter_module.hpp"
#include "volta/core/modules/gate_module.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
constexpr double kDt = 20e-6;

void run(Simulator& sim, double seconds, double dtS = kDt) {
    const int steps = static_cast<int>(seconds / dtS + 0.5);
    for (int k = 0; k < steps; ++k) REQUIRE(sim.step(dtS) == Status::Ok);
}

// Takt per Schalter von +U auf den Eingang (10 kOhm nach minus): n Impulse
void pulse(Simulator& sim, Switch& sw, int n) {
    for (int k = 0; k < n; ++k) {
        sw.setClosed(true);
        run(sim, 1e-3);
        sw.setClosed(false);
        run(sim, 1e-3);
    }
}

struct CounterRig {
    Simulator sim;
    NodeId vcc{}, clk{}, rst{};
    std::array<NodeId, 10> q{};
    std::size_t clockSwitch = 0, resetSwitch = 0, counter = 0;
    Switch& clock() { return static_cast<Switch&>(sim.circuit().component(clockSwitch)); }
    Switch& reset() { return static_cast<Switch&>(sim.circuit().component(resetSwitch)); }
    bool high(std::size_t k) { return sim.probe().voltage(q[k]) > 0.5 * sim.probe().voltage(vcc); }
};

void build(CounterRig& rig, bool decade) {
    Circuit circ;
    rig.vcc = circ.addNode();
    rig.clk = circ.addNode();
    rig.rst = circ.addNode();
    circ.add<Battery>("B1", rig.vcc, kGround, BatteryParams{6});
    circ.add<Resistor>("RC", rig.clk, kGround, ResistorParams{10000.0});
    circ.add<Resistor>("RR", rig.rst, kGround, ResistorParams{10000.0});
    rig.clockSwitch = circ.componentCount();
    circ.add<Switch>("ST", rig.vcc, rig.clk);
    rig.resetSwitch = circ.componentCount();
    circ.add<Switch>("SR", rig.vcc, rig.rst);
    CounterModulePins p;
    p.plus = rig.vcc;
    p.clock = rig.clk;
    p.reset = rig.rst;
    const std::size_t outputs = decade ? 10 : 4;
    for (std::size_t k = 0; k < outputs; ++k) {
        p.q[k] = rig.q[k] = circ.addNode();
        circ.add<Resistor>("RL" + std::to_string(k), rig.q[k], kGround, ResistorParams{10000.0});
    }
    rig.counter = circ.componentCount();
    const SubcircuitRange r = decade ? addDecadeCounterModule(circ, "DZ1", p) : addBinaryCounterModule(circ, "ZM1", p);
    CHECK(r.count == (decade ? 7u : 1u));
    REQUIRE(rig.sim.setCircuit(std::move(circ)) == Status::Ok);
    run(rig.sim, 1e-3);
}
} // namespace

TEST_CASE("Zaehlermodul: zaehlt steigende Flanken, laeuft bei 16 ueber, reset setzt auf 0", "[counter]") {
    CounterRig rig;
    build(rig, false);
    CHECK(rig.sim.probe().readings[rig.counter].displayValue == 0.0);
    pulse(rig.sim, rig.clock(), 5);
    CHECK(rig.sim.probe().readings[rig.counter].displayValue == 5.0);
    CHECK(rig.high(0));
    CHECK_FALSE(rig.high(1));
    CHECK(rig.high(2));
    CHECK_FALSE(rig.high(3));
    // H-Ausgang an 10 kOhm: U * 10k / 10,2k
    const double u = rig.sim.probe().voltage(rig.vcc);
    CHECK(rig.sim.probe().voltage(rig.q[0]) == Approx(u * 10000.0 / 10200.0).epsilon(0.01));
    pulse(rig.sim, rig.clock(), 11); // 16: wieder 0
    CHECK(rig.sim.probe().readings[rig.counter].displayValue == 0.0);
    pulse(rig.sim, rig.clock(), 3);
    rig.reset().setClosed(true);
    run(rig.sim, 1e-3);
    pulse(rig.sim, rig.clock(), 2); // reset H haelt bei 0
    CHECK(rig.sim.probe().readings[rig.counter].displayValue == 0.0);
    rig.reset().setClosed(false);
    run(rig.sim, 1e-3);
    pulse(rig.sim, rig.clock(), 1);
    CHECK(rig.sim.probe().readings[rig.counter].displayValue == 1.0);
    requireKirchhoff(rig.sim);
    requireFinite(rig.sim.probe());
}

TEST_CASE("Zaehlermodul: Q0 teilt den Takt eines Schmitt-Oszillators durch zwei", "[counter]") {
    // Takt: NAND-Schmitt-Oszillator (R von y auf a, C von a nach minus), T = 2 (R + 200) C ln 1,5
    const double R = 10e3, C = 1e-6;
    const double T = 2.0 * (R + 200.0) * C * std::log(1.5);
    Circuit circ;
    const NodeId vcc = circ.addNode(), a = circ.addNode(), clk = circ.addNode(), q0 = circ.addNode();
    circ.add<Battery>("B1", vcc, kGround, BatteryParams{6});
    circ.add<Resistor>("R1", clk, a, ResistorParams{R});
    circ.add<Capacitor>("C1", a, kGround, CapacitorParams::film(C));
    GateModulePins g;
    g.plus = vcc;
    for (auto& n : g.io) n = circ.addNode();
    g.io[0] = a;
    g.io[1] = vcc;
    g.io[2] = clk;
    addGateModule(circ, "GM1", g);
    CounterModulePins p;
    p.plus = vcc;
    p.clock = clk;
    p.reset = kGround;
    for (std::size_t k = 0; k < 4; ++k) p.q[k] = circ.addNode();
    p.q[0] = q0;
    addBinaryCounterModule(circ, "ZM1", p);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
    run(sim, 0.1); // einschwingen
    double t = 0.0, first = -1.0, last = -1.0;
    int rises = 0;
    bool prev = sim.probe().voltage(q0) > 0.5 * sim.probe().voltage(vcc);
    while (t < 0.3) {
        REQUIRE(sim.step(kDt) == Status::Ok);
        t += kDt;
        const bool h = sim.probe().voltage(q0) > 0.5 * sim.probe().voltage(vcc);
        if (h && !prev) {
            if (first < 0.0) first = t;
            last = t;
            ++rises;
        }
        prev = h;
    }
    REQUIRE(rises >= 5);
    const double period = (last - first) / (rises - 1);
    CHECK(period == Approx(2.0 * T).epsilon(0.02));
    requireKirchhoff(sim);
}

TEST_CASE("Dekadenzaehler: genau ein Ausgang H, wandert mit jedem Takt, reset auf Q0", "[counter]") {
    CounterRig rig;
    build(rig, true);
    const auto onlyHigh = [&](std::size_t k) {
        for (std::size_t j = 0; j < 10; ++j)
            if (rig.high(j) != (j == k)) return false;
        return true;
    };
    CHECK(onlyHigh(0));
    pulse(rig.sim, rig.clock(), 7);
    CHECK(onlyHigh(7));
    CHECK(rig.sim.probe().readings[rig.counter].displayValue == 7.0);
    pulse(rig.sim, rig.clock(), 5); // 12 -> 2
    CHECK(onlyHigh(2));
    rig.reset().setClosed(true);
    run(rig.sim, 1e-3);
    rig.reset().setClosed(false);
    run(rig.sim, 1e-3);
    CHECK(onlyHigh(0));
    requireKirchhoff(rig.sim);
    requireFinite(rig.sim.probe());
}

TEST_CASE("Anzeige-Modul: BCD-Dekodierung, Segmentstrom, Pseudotetraden dunkel", "[counter]") {
    for (int v = 0; v < 16; ++v) {
        Circuit circ;
        const NodeId vcc = circ.addNode();
        circ.add<Battery>("B1", vcc, kGround, BatteryParams{6});
        std::array<NodeId, 4> bcd{};
        for (int k = 0; k < 4; ++k) bcd[static_cast<std::size_t>(k)] = ((v >> k) & 1) ? vcc : kGround;
        const std::size_t idx = circ.componentCount();
        addDisplayModule(circ, "AM1", vcc, kGround, bcd);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
        run(sim, 1e-3);
        const ComponentReading& r = sim.probe().readings[idx];
        const double u = sim.probe().voltage(vcc);
        if (v <= 9) {
            CHECK(r.displayValue == v);
            CHECK(r.displayFlags == sevenSegmentPattern(v));
            int n = 0;
            for (int k = 0; k < 7; ++k) n += (r.displayFlags >> k) & 1;
            CHECK(r.currentA == Approx(n * (u - 1.9) / 680.0).epsilon(0.02));
        } else {
            CHECK(r.displayValue == -1.0);
            CHECK(r.displayFlags == 0);
            CHECK(r.currentA == 0.0);
        }
        requireKirchhoff(sim);
    }
    CHECK(sevenSegmentPattern(8) == 0x7F);
    CHECK(sevenSegmentPattern(1) == 0x06);
}

TEST_CASE("Zaehler- und Anzeige-Modul: unter 3 V keine Funktion", "[counter]") {
    Circuit circ;
    const NodeId vcc = circ.addNode(), q0 = circ.addNode();
    circ.add<Battery>("B1", vcc, kGround, BatteryParams{1}); // 1,5 V
    CounterModulePins p;
    p.plus = vcc;
    p.clock = vcc;
    p.reset = kGround;
    for (std::size_t k = 0; k < 4; ++k) p.q[k] = circ.addNode();
    p.q[0] = q0;
    circ.add<Resistor>("RL", q0, kGround, ResistorParams{10000.0});
    addBinaryCounterModule(circ, "ZM1", p);
    const std::size_t am = circ.componentCount();
    addDisplayModule(circ, "AM1", vcc, kGround, {vcc, kGround, kGround, kGround});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
    run(sim, 1e-3);
    CHECK(std::fabs(sim.probe().voltage(q0)) < 1e-3);
    CHECK(sim.probe().readings[am].displayValue == -1.0);
    CHECK(sim.probe().readings[am].currentA == 0.0);
}
