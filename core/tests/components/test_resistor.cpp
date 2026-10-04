// Widerstand: Arbeitspunkt nach Ohmschem Gesetz, Leistung, unsinnige Werte.
#include <catch2/catch_test_macros.hpp>

#include <limits>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/resistor.hpp"

using namespace volta::core;
using namespace volta::core::test;

TEST_CASE("Widerstand 470 Ohm an 4,5 V (3 Zellen, Innenwiderstand 0,75 Ohm)", "[bauteil][widerstand]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround, BatteryParams{3});
    c.add<Resistor>("R1", plus, kGround, ResistorParams{470.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);

    const double i = 4.5 / (470.0 + 0.75); // von Hand: 9,559 mA
    const ComponentReading& r = sim.probe().readings[1];
    CHECK(nearA(r.currentA, i));
    CHECK(nearV(r.voltV, i * 470.0));
    CHECK(near(r.powerW, i * i * 470.0, 1e-6));
    CHECK_FALSE(r.destroyed);
    requireKirchhoff(sim);
}

TEST_CASE("Widerstand: Strom und Spannung kehren sich bei Verpolung um", "[bauteil][widerstand]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", kGround, plus, ResistorParams{1000.0}); // Anschluss 0 an Masse
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(sim.probe().readings[1].currentA < 0.0);
    CHECK(sim.probe().readings[1].voltV < 0.0);
    CHECK(sim.probe().readings[1].powerW > 0.0); // ein Widerstand nimmt immer Leistung auf
    requireKirchhoff(sim);
}

TEST_CASE("Widerstand: unsinnige Werte werden abgefangen", "[bauteil][widerstand][robust]") {
    Resistor zero("R0", 1, 0, ResistorParams{0.0});
    CHECK(zero.params().ohm == kMinOhm);
    Resistor negative("Rn", 1, 0, ResistorParams{-5.0});
    CHECK(negative.params().ohm == kMinOhm);
    Resistor nan("Rx", 1, 0, ResistorParams{std::numeric_limits<double>::quiet_NaN()});
    CHECK(nan.params().ohm > 1e9); // praktisch offen
    nan.setOhm(220.0);
    CHECK(nan.params().ohm == 220.0);
}
