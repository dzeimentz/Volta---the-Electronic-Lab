// Thermikmodell und Ereignisse (docs/testing.md): LED uebersteht einen kurzen Stromstoss, Dauerueberlast zerstoert
// sie innerhalb 0,3-1 s; Widerstand brennt bei Ueberlast durch; Batterie wird bei Kurzschluss heiss.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/wire.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {

std::size_t countEvents(Simulator& sim, SimEventType type, std::size_t component) {
    std::size_t n = 0;
    SimEvent e;
    while (sim.events().pop(e))
        if (e.type == type && e.component == component) ++n;
    return n;
}

// Laeuft, bis das Bauteil zerstoert ist oder maxS erreicht ist; liefert die Zeit der Zerstoerung oder -1.
double runUntilDestroyed(Simulator& sim, std::size_t idx, double maxS) {
    const double start = sim.timeS();
    while (sim.timeS() - start < maxS) {
        REQUIRE(sim.step() == Status::Ok);
        if (sim.probe().readings[idx].destroyed) return sim.timeS() - start;
    }
    return -1.0;
}

void runFor(Simulator& sim, double seconds) {
    const int steps = static_cast<int>(std::lround(seconds / kDefaultStepS));
    for (int i = 0; i < steps; ++i) REQUIRE(sim.step() == Status::Ok);
}

// 9 V -> Vorwiderstand R1 -> rote LED -> Masse
struct LedCircuit {
    Circuit circuit;
};
Circuit makeLed(double seriesOhm) {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{seriesOhm});
    c.add<Diode>("D1", a, kGround, DiodeParams::ledRed());
    return c;
}

} // namespace

TEST_CASE("LED ohne Vorwiderstand wird innerhalb 0,3-1 s zerstoert (Experiment 2.4)", "[thermik][diode]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(makeLed(kMinOhm)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[2].overloaded);
    CHECK(sim.probe().readings[2].tempK > kAmbientK);

    const double t = runUntilDestroyed(sim, 2, 2.0);
    INFO("Zerstoert nach " << t << " s");
    CHECK(t >= 0.3);
    CHECK(t <= 1.0);
    CHECK(countEvents(sim, SimEventType::Destroyed, 2) == 1);

    runFor(sim, 0.1);
    CHECK(std::fabs(sim.probe().readings[2].currentA) < 1e-6);
    CHECK_FALSE(sim.probe().readings[2].overloaded);
    CHECK(countEvents(sim, SimEventType::Destroyed, 2) == 0); // nur einmal gemeldet
    requireKirchhoff(sim);
}

TEST_CASE("LED uebersteht einen Stromstoss von 50 ms", "[thermik][diode]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(makeLed(kMinOhm)) == Status::Ok);
    runFor(sim, 0.05);
    CHECK_FALSE(sim.probe().readings[2].destroyed);
    const double peakK = sim.probe().readings[2].tempK;
    CHECK(peakK - kAmbientK < 20.0);

    static_cast<Resistor&>(sim.circuit().component(1)).setOhm(470.0); // Bedienhandlung: Vorwiderstand rein
    runFor(sim, 5.0);
    CHECK_FALSE(sim.probe().readings[2].destroyed);
    CHECK_FALSE(sim.probe().readings[2].overloaded);
    CHECK(sim.probe().readings[2].tempK < peakK);
    CHECK(sim.probe().readings[2].currentA > 14e-3);
}

TEST_CASE("LED mit 470 Ohm leuchtet dauerhaft ohne Ueberlast", "[thermik][diode]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(makeLed(470.0)) == Status::Ok);
    runFor(sim, 60.0);
    const ComponentReading& led = sim.probe().readings[2];
    CHECK_FALSE(led.destroyed);
    CHECK_FALSE(led.overloaded);
    // Dauertemperatur: Tamb + P * Rth mit P = 1,86 V * 15,1 mA = 28 mW -> ca. +9,8 K (nach 60 s / tau 12,6 s erreicht)
    CHECK(led.tempK - kAmbientK == Approx(led.powerW * 350.0).epsilon(0.02));
}

TEST_CASE("Erwaermung folgt der Exponentialfunktion erster Ordnung", "[thermik][widerstand]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, kGround, ResistorParams{470.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    runFor(sim, 5.0);
    const ComponentReading& r = sim.probe().readings[1];
    const double expected = r.powerW * 340.0 * (1.0 - std::exp(-5.0 / 10.0)); // tau 10 s
    CHECK(r.tempK - kAmbientK == Approx(expected).epsilon(0.01));
    CHECK_FALSE(r.overloaded); // 0,17 W < 0,25 W
}

TEST_CASE("Widerstand 100 Ohm an 9 V ist ueberlastet und brennt durch", "[thermik][widerstand]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, kGround, ResistorParams{100.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[1].overloaded); // ca. 0,79 W > 0,25 W
    // Dauertemperatur waere 20 + 0,79 * 340 = 290 Grad C > 232 Grad C: Zerstoerung nach ca. 15 s
    const double t = runUntilDestroyed(sim, 1, 60.0);
    INFO("Durchgebrannt nach " << t << " s");
    CHECK(t > 5.0);
    CHECK(t < 30.0);
    CHECK(countEvents(sim, SimEventType::Destroyed, 1) == 1);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(std::fabs(sim.probe().readings[1].currentA) < 1e-6);
    requireKirchhoff(sim);
}

TEST_CASE("Kurzschluss ueber eine Leitung macht die Batterie heiss", "[thermik][batterie]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Wire>("W1", plus, kGround);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[1].overloaded); // Leitung glueht
    CHECK_FALSE(sim.probe().readings[0].overloaded);

    double hotAt = -1.0;
    for (int i = 0; i < 10000 && hotAt < 0.0; ++i) {
        REQUIRE(sim.step() == Status::Ok);
        if (sim.probe().readings[0].overloaded) hotAt = sim.timeS();
    }
    INFO("Heiss nach " << hotAt << " s");
    CHECK(hotAt > 1.0);
    CHECK(hotAt < 5.0);
    CHECK(countEvents(sim, SimEventType::BatteryHot, 0) == 1);
    CHECK_FALSE(sim.probe().readings[0].destroyed);
    CHECK_FALSE(sim.probe().readings[1].destroyed);
    runFor(sim, 1.0);
    CHECK(countEvents(sim, SimEventType::BatteryHot, 0) == 0); // nicht erneut, solange heiss

    // Kurzschluss entfernt (neuer Aufbau mit uebernommenem Batteriezustand): Batterie kuehlt ab
    const ComponentState hot = sim.circuit().component(0).saveState();
    Circuit open;
    const NodeId p2 = open.addNode();
    open.add<Battery>("B1", p2, kGround).restoreState(hot);
    REQUIRE(sim.setCircuit(std::move(open)) == Status::Ok);
    CHECK(static_cast<const Battery&>(sim.circuit().component(0)).isHot());
    runFor(sim, 120.0);
    CHECK_FALSE(sim.probe().readings[0].overloaded);
    CHECK(sim.probe().readings[0].tempK < 318.15);
}

TEST_CASE("Undo nach Zerstoerung: LED heil und kalt", "[thermik][undo]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(makeLed(kMinOhm)) == Status::Ok);
    const ComponentState before = sim.circuit().component(2).saveState();
    REQUIRE(runUntilDestroyed(sim, 2, 2.0) > 0.0);
    CHECK(sim.probe().readings[2].tempK > 398.0);

    sim.circuit().component(2).restoreState(before);
    static_cast<Resistor&>(sim.circuit().component(1)).setOhm(470.0);
    REQUIRE(sim.solveOperatingPoint() == Status::Ok);
    CHECK_FALSE(sim.probe().readings[2].destroyed);
    CHECK(sim.probe().readings[2].tempK == Approx(kAmbientK));
    runFor(sim, 1.0);
    CHECK_FALSE(sim.probe().readings[2].destroyed);
    CHECK(sim.probe().readings[2].currentA > 14e-3);
}

TEST_CASE("LED an 100 Ohm (ca. 67 mA): starke Ueberlast zerstoert nach 3 s", "[thermik][diode]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(makeLed(100.0)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[2].currentA > 2.5 * 0.02);
    CHECK(sim.probe().readings[2].overloaded);
    const double t = runUntilDestroyed(sim, 2, 10.0);
    INFO("Zerstoert nach " << t << " s");
    CHECK(t == Approx(3.0).margin(0.01));
    CHECK(countEvents(sim, SimEventType::Destroyed, 2) == 1);
}

TEST_CASE("LED bei ca. 40 mA: ueberlastet, aber nicht zerstoert", "[thermik][diode]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(makeLed(170.0)) == Status::Ok);
    runFor(sim, 30.0);
    const ComponentReading& led = sim.probe().readings[2];
    CHECK(led.currentA > 0.03);
    CHECK(led.currentA < 0.05);
    CHECK(led.overloaded);
    CHECK_FALSE(led.destroyed);
}

TEST_CASE("Thermik und Zerstoerung bleiben unter Schrittweitenhalbierung gleich", "[thermik][newton]") {
    Circuit c = makeLed(kMinOhm);
    const NodeId extra = c.addNode();
    c.add<Battery>("B2", extra, kGround);
    c.add<StubbornResistor>("X1", extra, kGround); // erzwingt Halbierung in jedem Schritt
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& stub = static_cast<StubbornResistor&>(sim.circuit().component(4));
    double t = -1.0;
    while (sim.timeS() < 2.0 && t < 0.0) {
        stub.failIterations = kMaxNewtonIter + 1;
        REQUIRE(sim.step() == Status::Ok);
        if (sim.probe().readings[2].destroyed) t = sim.timeS();
    }
    INFO("Zerstoert nach " << t << " s (mit Halbschritten)");
    CHECK(t == Approx(0.484).margin(0.002));
    CHECK(countEvents(sim, SimEventType::Destroyed, 2) == 1);
}
