// Referenzschaltungen mit analytischer Loesung (docs/testing.md), jeweils mit Kirchhoff-Pruefung.
#include <catch2/catch_test_macros.hpp>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/resistor.hpp"

using namespace volta::core;
using namespace volta::core::test;

namespace {
// Nahezu ideale Quelle fuer Referenzschaltungen: Innenwiderstand je Zelle an der unteren Grenze kMinOhm
// (gesamt cells * 1 mOhm, gegenueber kOhm-Lasten < 1e-5 relativ).
BatteryParams idealCells(int cells) { return {cells, 1.5, kMinOhm}; }
} // namespace

TEST_CASE("Referenz 1: Spannungsteiler 9 V mit 1 kOhm / 2 kOhm ergibt 6 V", "[referenz][dc]") {
    Circuit c;
    const NodeId top = c.addNode();
    const NodeId mid = c.addNode();
    c.add<Battery>("B1", top, kGround, idealCells(6));
    c.add<Resistor>("R1", top, mid, ResistorParams{1000.0});
    c.add<Resistor>("R2", mid, kGround, ResistorParams{2000.0});

    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const Probe& p = sim.probe();
    CHECK(nearV(p.voltage(top), 9.0));
    CHECK(nearV(p.voltage(mid), 6.0));
    CHECK(exact(p.voltage(mid), 9.0 * 2000.0 / (3000.0 + 0.006)));
    // Batterie gibt 3 mA ab: Strom am Pluspol hinein ist negativ, Leistung negativ (abgegeben).
    CHECK(nearA(p.readings[0].currentA, -3e-3));
    CHECK(p.readings[0].powerW < 0.0);
    CHECK(nearA(p.readings[1].currentA, 3e-3));
    CHECK(near(p.readings[1].powerW, 9e-3, 1e-6)); // (3 V)^2 / 1 kOhm
    requireKirchhoff(sim);
}

TEST_CASE("Referenz 4: gemischte Reihen- und Parallelschaltung", "[referenz][dc]") {
    // 9 V -> R1 = 1 kOhm -> Knoten m -> (R2 = 2 kOhm || R3 = 2 kOhm) -> Masse
    // Rges = 1 kOhm + 1 kOhm = 2 kOhm; I = 4,5 mA; V(m) = 4,5 V; je Zweig 2,25 mA
    Circuit c;
    const NodeId top = c.addNode();
    const NodeId m = c.addNode();
    c.add<Battery>("B1", top, kGround, idealCells(6));
    c.add<Resistor>("R1", top, m, ResistorParams{1000.0});
    c.add<Resistor>("R2", m, kGround, ResistorParams{2000.0});
    c.add<Resistor>("R3", m, kGround, ResistorParams{2000.0});

    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const Probe& p = sim.probe();
    CHECK(nearV(p.voltage(m), 4.5));
    CHECK(nearA(p.readings[1].currentA, 4.5e-3));
    CHECK(nearA(p.readings[2].currentA, 2.25e-3));
    CHECK(nearA(p.readings[3].currentA, 2.25e-3));
    requireKirchhoff(sim);
}

TEST_CASE("Referenz 4b: Bruecke aus fuenf Widerstaenden (nicht reduzierbar)", "[referenz][dc]") {
    // Wheatstone-Bruecke, 9 V; R1=1k (top-a), R2=2k (top-b), R3=2k (a-gnd), R4=1k (b-gnd), R5=1k (a-b).
    // Knotengleichungen: (9-Va)/1k = Va/2k + (Va-Vb)/1k ; (9-Vb)/2k + (Va-Vb)/1k = Vb/1k
    // -> 9 = 2,5 Va - Vb ; 4,5 + Va = 2,5 Vb  ->  Va = 36/7 V, Vb = 27/7 V, I(R5) = 9/7 mA
    Circuit c;
    const NodeId top = c.addNode();
    const NodeId a = c.addNode();
    const NodeId b = c.addNode();
    c.add<Battery>("B1", top, kGround, idealCells(6));
    c.add<Resistor>("R1", top, a, ResistorParams{1000.0});
    c.add<Resistor>("R2", top, b, ResistorParams{2000.0});
    c.add<Resistor>("R3", a, kGround, ResistorParams{2000.0});
    c.add<Resistor>("R4", b, kGround, ResistorParams{1000.0});
    c.add<Resistor>("R5", a, b, ResistorParams{1000.0});

    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(nearV(sim.probe().voltage(a), 36.0 / 7.0));
    CHECK(nearV(sim.probe().voltage(b), 27.0 / 7.0));
    CHECK(nearA(sim.probe().readings[5].currentA, 9e-3 / 7.0));
    requireKirchhoff(sim);
}

TEST_CASE("Zeitschritte aendern eine rein ohmsche Schaltung nicht", "[referenz][transient]") {
    Circuit c;
    const NodeId top = c.addNode();
    const NodeId mid = c.addNode();
    c.add<Battery>("B1", top, kGround);
    c.add<Resistor>("R1", top, mid, ResistorParams{470.0});
    c.add<Resistor>("R2", mid, kGround, ResistorParams{470.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const double v0 = sim.probe().voltage(mid);
    for (int i = 0; i < 100; ++i) REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.timeS() == Catch::Approx(0.1));
    CHECK(sim.probe().timeS == Catch::Approx(0.1));
    CHECK(sim.probe().voltage(mid) == Catch::Approx(v0));
    requireKirchhoff(sim);
}
