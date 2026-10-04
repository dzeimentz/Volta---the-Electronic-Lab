// Batterie aus 1,5-V-Zellen (3 / 4,5 / 9 V) mit Innenwiderstand.
#include <catch2/catch_test_macros.hpp>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/wire.hpp"

using namespace volta::core;
using namespace volta::core::test;

TEST_CASE("Batterie: Klemmenspannung unter Last fuer 2, 3 und 6 Zellen", "[bauteil][batterie]") {
    for (int cells : {2, 3, 6}) {
        Circuit c;
        const NodeId plus = c.addNode();
        c.add<Battery>("B1", plus, kGround, BatteryParams{cells});
        c.add<Resistor>("R1", plus, kGround, ResistorParams{10.0}); // Ri macht hier 5-13 % aus
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);

        const double emf = 1.5 * cells;
        const double ri = 0.25 * cells;
        INFO(cells << " Zellen");
        // GMIN am Pluspol (1e-9 S) ist gegenueber 10 Ohm vernachlaessigbar (< 1e-8 relativ)
        CHECK(exact(sim.probe().voltage(plus), emf * 10.0 / (10.0 + ri)));
        CHECK(exact(sim.probe().readings[0].currentA, -emf / (10.0 + ri)));
        requireKirchhoff(sim);
    }
}

TEST_CASE("Batterie: Leerlaufspannung entspricht der Zellenzahl", "[bauteil][batterie]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround, BatteryParams{3});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    // Nur GMIN belastet: 4,5 V * 1e-9 S * 0,75 Ohm ist vernachlaessigbar.
    CHECK(nearV(sim.probe().voltage(plus), 4.5));
    requireKirchhoff(sim);
}

TEST_CASE("Batterie: Kurzschluss ueber eine Leitung bleibt endlich", "[bauteil][batterie][robust]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Wire>("W1", plus, kGround);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    requireFinite(sim.probe());
    CHECK(near(sim.probe().readings[0].currentA, -9.0 / (1.5 + kWireOhm), 1e-6)); // ca. 5,96 A
    requireKirchhoff(sim);
}

TEST_CASE("Batterie: Zellenzahl wird auf 1..6 begrenzt", "[bauteil][batterie][robust]") {
    Battery low("B1", 1, 0, BatteryParams{0});
    CHECK(low.params().cells == 1);
    Battery high("B2", 1, 0, BatteryParams{12});
    CHECK(high.params().cells == 6);
    high.setCells(2);
    CHECK(high.emfV() == 3.0);
    CHECK(high.innerOhm() == 0.5);
    Battery badRi("B3", 1, 0, BatteryParams{6, 1.5, -1.0});
    CHECK(badRi.innerOhm() > 0.0);
}

TEST_CASE("Batterie: Zellenzahl als Bedienhandlung zwischen Zeitschritten", "[bauteil][batterie]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, kGround, ResistorParams{1000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(nearV(sim.probe().voltage(plus), 9.0));
    static_cast<Battery&>(sim.circuit().component(0)).setCells(2);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(nearV(sim.probe().voltage(plus), 3.0 * 1000.0 / 1000.5));
    requireKirchhoff(sim);
}
