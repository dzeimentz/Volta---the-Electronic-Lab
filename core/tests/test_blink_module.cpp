// Blinkmodul als Teilschaltung: blinkt mit ca. 1 Hz, Takt einstellbar, Kirchhoff, keine Konvergenzprobleme.
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/transistor.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/modules/blink_module.hpp"

using namespace volta::core;
using namespace volta::core::test;

namespace {

struct BlinkResult {
    int maxDepth = 0;
    int maxStepIterations = 0;
    bool anyOverload = false;
    double frequencyHz = 0.0;
    double dutyOn = 0.0;
    double onCurrentA = 0.0;
    int edges = 0;
    std::size_t convergenceEvents = 0;
};

// Auftakt-Aufbau: Batterie, Blinkmodul, LED mit 470 Ohm von plus nach aus; optional Widerstand takt -> plus.
BlinkResult runBlink(double rateOhm, double seconds, Simulator& sim) {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId out = c.addNode();
    const NodeId rate = c.addNode();
    const NodeId led = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    const SubcircuitRange range = addBlinkModule(c, "BL1", {plus, kGround, out, rate});
    REQUIRE(range.count == 11);
    REQUIRE(c.component(range.first).id() == "BL1.R_takt");
    c.add<Resistor>("RV", plus, led, ResistorParams{470.0});
    const std::size_t ledIndex = c.componentCount();
    c.add<Diode>("D1", led, out, DiodeParams::ledRed());
    if (rateOhm > 0.0) c.add<Resistor>("RT", rate, plus, ResistorParams{rateOhm});
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);

    BlinkResult r;
    std::vector<double> risingEdges;
    bool on = false;
    int onSteps = 0;
    int counted = 0;
    const int steps = static_cast<int>(seconds / kDefaultStepS);
    for (int i = 0; i < steps; ++i) {
        const Status st = sim.step();
        INFO("Zeitpunkt " << sim.timeS() << " s, Newton-Iterationen " << sim.lastNewtonIterations());
        REQUIRE(st == Status::Ok);
        r.maxDepth = std::max(r.maxDepth, sim.lastStepMaxDepth());
        r.maxStepIterations = std::max(r.maxStepIterations, sim.lastStepIterations());
        for (const auto& reading : sim.probe().readings) r.anyOverload = r.anyOverload || reading.overloaded;
        if (i % 7 == 0) requireKirchhoff(sim); // auch waehrend des Umkippens
        const double iLed = sim.probe().readings[ledIndex].currentA;
        const bool nowOn = on ? iLed > 1e-3 : iLed > 5e-3; // Hysterese
        if (nowOn && !on) risingEdges.push_back(sim.timeS());
        if (nowOn != on) ++r.edges;
        on = nowOn;
        if (sim.timeS() > 2.0) { // Einschwingen abwarten
            ++counted;
            if (on) {
                ++onSteps;
                r.onCurrentA = std::max(r.onCurrentA, iLed);
            }
        }
    }
    if (risingEdges.size() >= 3) {
        const double span = risingEdges.back() - risingEdges[1];
        r.frequencyHz = static_cast<double>(risingEdges.size() - 2) / span;
    }
    r.dutyOn = counted > 0 ? static_cast<double>(onSteps) / counted : 0.0;
    SimEvent e;
    while (sim.events().pop(e))
        if (e.type == SimEventType::ConvergenceTrouble) ++r.convergenceEvents;
    return r;
}

} // namespace

TEST_CASE("Blinkmodul laesst eine LED mit ca. 1 Hz blinken (Auftakt)", "[modul][blink]") {
    Simulator sim;
    const auto t0 = std::chrono::steady_clock::now();
    const BlinkResult r = runBlink(0.0, 10.0, sim);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    INFO("Frequenz " << r.frequencyHz << " Hz, Einschaltanteil " << r.dutyOn << ", LED " << r.onCurrentA * 1e3
                     << " mA, 10 s Simulation in " << ms << " ms, Halbierungstiefe " << r.maxDepth
                     << ", hoechstens " << r.maxStepIterations << " Iterationen je Schritt");
    CHECK(r.maxDepth <= kMaxStepHalvings - 3); // Reserve
    CHECK_FALSE(r.anyOverload);
    CHECK(r.edges >= 10);
    CHECK(r.frequencyHz > 0.5);
    CHECK(r.frequencyHz < 2.0);
    CHECK(r.dutyOn > 0.3);
    CHECK(r.dutyOn < 0.7);
    CHECK(r.onCurrentA > 12e-3); // LED leuchtet normal hell
    CHECK(r.onCurrentA < 20e-3);
    CHECK(r.convergenceEvents == 0);
    requireKirchhoff(sim);
    requireFinite(sim.probe());
    for (const auto& reading : sim.probe().readings) CHECK_FALSE(reading.destroyed);
}

TEST_CASE("Blinkmodul: Widerstand an takt macht das Blinken schneller", "[modul][blink]") {
    Simulator slow;
    Simulator fast;
    const BlinkResult a = runBlink(0.0, 8.0, slow);
    const BlinkResult b = runBlink(4700.0, 8.0, fast);
    INFO("ohne: " << a.frequencyHz << " Hz, mit 4,7 kOhm: " << b.frequencyHz << " Hz");
    CHECK(b.frequencyHz > 1.1 * a.frequencyHz);
}

TEST_CASE("Diskrete Kippstufen aus Katalogteilen schwingen mit Reserve bei der Halbierung", "[modul][blink][robust]") {
    struct Variant {
        double rc;
        double rb;
    };
    for (const Variant v : {Variant{470.0, 22000.0}, Variant{220.0, 10000.0}, Variant{100.0, 4700.0}}) {
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId c1 = c.addNode();
        const NodeId c2 = c.addNode();
        const NodeId b1 = c.addNode();
        const NodeId b2 = c.addNode();
        c.add<Battery>("B1", plus, kGround);
        c.add<Resistor>("RC1", plus, c1, ResistorParams{v.rc});
        c.add<Resistor>("RC2", plus, c2, ResistorParams{v.rc});
        c.add<Resistor>("RB1", plus, b1, ResistorParams{v.rb});
        c.add<Resistor>("RB2", plus, b2, ResistorParams{v.rb * 1.2});
        c.add<Capacitor>("C1", c1, b2, CapacitorParams::electrolytic(22e-6));
        c.add<Capacitor>("C2", c2, b1, CapacitorParams::electrolytic(22e-6));
        c.add<Npn>("T1", b1, c1, kGround);
        c.add<Npn>("T2", b2, c2, kGround);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        int failed = 0;
        int maxDepth = 0;
        int switches = 0;
        bool high = false;
        for (int i = 0; i < 5000; ++i) {
            if (sim.step() != Status::Ok) ++failed;
            maxDepth = std::max(maxDepth, sim.lastStepMaxDepth());
            const bool nowHigh = sim.probe().voltage(c1) > 4.5;
            if (nowHigh != high) ++switches;
            high = nowHigh;
        }
        INFO("RC " << v.rc << " Ohm, RB " << v.rb << " Ohm: Tiefe " << maxDepth << ", Umschaltungen " << switches);
        CHECK(failed == 0);
        CHECK(maxDepth <= kMaxStepHalvings - 3);
        CHECK(switches >= 4);
        requireKirchhoff(sim);
    }
}
