// Robustheit: absichtlich unsinnige Schaltungen und Fehlerpfade des Simulators (docs/testing.md).
// Erwartung: keine Ausnahme, kein NaN, passende Status- und Ereigniswerte.
#include <catch2/catch_test_macros.hpp>

#include <limits>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/resistor.hpp"

using namespace volta::core;
using namespace volta::core::test;

namespace {

std::size_t countEvents(Simulator& sim, SimEventType type) {
    std::size_t n = 0;
    SimEvent e;
    while (sim.events().pop(e))
        if (e.type == type) ++n;
    return n;
}

} // namespace

TEST_CASE("Leere Schaltung ist loesbar", "[robust]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(Circuit{}) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().valid);
    requireFinite(sim.probe());
}

TEST_CASE("Offener Knoten bleibt loesbar (GMIN)", "[robust]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId floating = c.addNode();
    const NodeId dangling = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, dangling, ResistorParams{1000.0}); // unbeschaltetes Bein
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(sim.probe().voltage(floating) == 0.0);
    CHECK(nearV(sim.probe().voltage(dangling), 9.0)); // ohne Strom kein Spannungsabfall
    requireKirchhoff(sim);
}

TEST_CASE("Schleife nur aus Leitungen ohne Quelle", "[robust]") {
    Circuit c;
    const NodeId a = c.addNode();
    const NodeId b = c.addNode();
    c.add<Resistor>("W1", a, b, ResistorParams{kWireOhm});
    c.add<Resistor>("W2", b, a, ResistorParams{kWireOhm});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    requireFinite(sim.probe());
    CHECK(sim.probe().voltage(a) == 0.0);
    requireKirchhoff(sim);
}

TEST_CASE("Zwei verschiedene Batterien parallel bleiben endlich", "[robust]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround, BatteryParams{6});
    c.add<Battery>("B2", plus, kGround, BatteryParams{2});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    requireFinite(sim.probe());
    // Ausgleichsstrom (9 V - 3 V) / (1,5 Ohm + 0,5 Ohm) = 3 A
    CHECK(near(sim.probe().readings[1].currentA, 3.0, 1e-6));
    requireKirchhoff(sim);
}

TEST_CASE("Ungueltige Netzliste wird abgewiesen, alte Schaltung bleibt", "[robust]") {
    Circuit good;
    const NodeId plus = good.addNode();
    good.add<Battery>("B1", plus, kGround);
    good.add<Resistor>("R1", plus, kGround, ResistorParams{1000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(good)) == Status::Ok);

    Circuit bad;
    bad.add<Resistor>("R9", 5, kGround); // Knoten 5 existiert nicht
    CHECK(sim.setCircuit(std::move(bad)) == Status::InvalidCircuit);
    CHECK(sim.circuit().componentCount() == 2);
    CHECK(sim.step() == Status::Ok);
}

TEST_CASE("Ohne Schaltung meldet der Simulator NoCircuit", "[robust]") {
    Simulator sim;
    CHECK(sim.step() == Status::NoCircuit);
    CHECK(sim.solveOperatingPoint() == Status::NoCircuit);
    CHECK_FALSE(sim.probe().valid);
}

TEST_CASE("Unsinnige Schrittweite bewegt die Zeit nicht", "[robust]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(Circuit{}) == Status::Ok);
    CHECK(sim.step(0.0) == Status::InvalidStep);
    CHECK(sim.step(-1.0) == Status::InvalidStep);
    CHECK(sim.step(std::numeric_limits<double>::quiet_NaN()) == Status::InvalidStep);
    CHECK(sim.step(std::numeric_limits<double>::infinity()) == Status::InvalidStep);
    CHECK(sim.timeS() == 0.0);
}

TEST_CASE("Nichtkonvergenz: Halbierung bis 1/16, dann Ereignis und letzter gueltiger Zustand", "[robust][newton]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<StubbornResistor>("X1", plus, kGround);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    const double t0 = sim.timeS();
    const double v0 = sim.probe().voltage(plus);
    countEvents(sim, SimEventType::ConvergenceTrouble);

    auto& stub = static_cast<StubbornResistor&>(sim.circuit().component(1));
    stub.failIterations = -1;
    stub.linearizeCalls = 0;
    CHECK(sim.step() == Status::NotConverged);
    // Versuche mit dt, dt/2, ..., dt/2^kMaxStepHalvings je kMaxNewtonIterTransient Iterationen
    CHECK(stub.linearizeCalls == (kMaxStepHalvings + 1) * kMaxNewtonIterTransient);
    CHECK(sim.lastStepMaxDepth() == kMaxStepHalvings);
    CHECK(sim.timeS() == t0);
    CHECK(sim.probe().voltage(plus) == v0);
    requireFinite(sim.probe());
    CHECK(countEvents(sim, SimEventType::ConvergenceTrouble) == 1);

    stub.failIterations = 0; // Schaltung erholt sich
    CHECK(sim.step() == Status::Ok);
    CHECK(sim.timeS() > t0);
}

TEST_CASE("Nichtkonvergenz: halbierter Schritt rettet den vollen Zeitschritt", "[robust][newton]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<StubbornResistor>("X1", plus, kGround);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    countEvents(sim, SimEventType::ConvergenceTrouble);

    auto& stub = static_cast<StubbornResistor&>(sim.circuit().component(1));
    stub.failIterations = kMaxNewtonIterTransient + 3; // erster Versuch scheitert, Halbschritte gelingen
    REQUIRE(sim.step(1e-3) == Status::Ok);
    CHECK(sim.timeS() == Catch::Approx(1e-3));
    CHECK(countEvents(sim, SimEventType::ConvergenceTrouble) == 0);
}

TEST_CASE("Arbeitspunkt ohne Konvergenz meldet Ereignis und liefert keine Messwerte", "[robust][newton]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<StubbornResistor>("X1", plus, kGround).failIterations = -1;
    Simulator sim;
    CHECK(sim.setCircuit(std::move(c)) == Status::NotConverged);
    CHECK_FALSE(sim.probe().valid);
    CHECK(countEvents(sim, SimEventType::ConvergenceTrouble) == 1);
}

TEST_CASE("Zustandsschnappschuss eines Bauteils (Undo)", "[undo]") {
    Resistor r("R1", 1, 0);
    ComponentState s = r.saveState();
    CHECK_FALSE(s.destroyed);
    CHECK(s.tempK == kAmbientK);
    s.destroyed = true;
    s.tempK = 400.0;
    r.restoreState(s);
    CHECK(r.isDestroyed());
    CHECK(r.tempK() == 400.0);
}

TEST_CASE("Rechenbudget je Zeitschritt begrenzt die Newton-Iterationen", "[robust][newton][echtzeit]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<StubbornResistor>("X1", plus, kGround);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& stub = static_cast<StubbornResistor&>(sim.circuit().component(1));
    // Konvergiert jeweils erst nach vielen Iterationen: jeder Teilschritt scheitert knapp, bis das Budget greift
    stub.failIterations = -1;
    stub.linearizeCalls = 0;
    CHECK(sim.step() == Status::NotConverged);
    CHECK(sim.lastStepIterations() <= kStepIterationBudget);
    CHECK(stub.linearizeCalls <= kStepIterationBudget);
}
