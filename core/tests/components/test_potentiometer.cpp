// Potentiometer (Pult-Drehregler): Spannungsteiler nach Stellung, Bedienhandlung, Anschlaege, Ueberlast.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/potentiometer.hpp"
#include "volta/core/components/resistor.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
BatteryParams ideal() { return {6, 1.5, kMinOhm}; }
} // namespace

TEST_CASE("Potentiometer als Spannungsteiler folgt der Stellung", "[bauteil][potentiometer]") {
    for (double pos : {0.0, 0.25, 0.5, 0.9, 1.0}) {
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId s = c.addNode();
        c.add<Battery>("B1", plus, kGround, ideal());
        c.add<Potentiometer>("P1", plus, s, kGround, PotentiometerParams{}, pos);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        INFO("Stellung " << pos);
        // Schleifer unbelastet: U_s = 9 V * R_sb / R_ges (Restwiderstand 1 Ohm an den Anschlaegen)
        const double rAS = std::max(pos * 10000.0, 1.0);
        const double rSB = std::max((1.0 - pos) * 10000.0, 1.0);
        CHECK(sim.probe().voltage(s) == Approx(9.0 * rSB / (rAS + rSB)).epsilon(1e-4).margin(1e-6));
        CHECK(sim.probe().readings[1].level == pos);
        requireKirchhoff(sim);
    }
}

TEST_CASE("Potentiometer: Stellung ist eine Bedienhandlung zwischen zwei Schritten", "[bauteil][potentiometer]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId s = c.addNode();
    c.add<Battery>("B1", plus, kGround, ideal());
    c.add<Potentiometer>("P1", plus, s, kGround);
    c.add<Resistor>("RL", s, kGround, ResistorParams{1e6}); // kaum Last
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(sim.probe().voltage(s) == Approx(4.5).epsilon(1e-2));
    static_cast<Potentiometer&>(sim.circuit().component(1)).setPosition(0.8);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().voltage(s) == Approx(9.0 * 0.2).epsilon(1e-2));
    static_cast<Potentiometer&>(sim.circuit().component(1)).setPosition(7.0); // unsinnig -> Anschlag
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().voltage(s) < 0.01);
    requireKirchhoff(sim);
}

TEST_CASE("Potentiometer: Schleifer am Anschlag direkt an der Batterie brennt die Bahn durch", "[bauteil][potentiometer][grenzwert]") {
    // Schleifer an Masse, Stellung nahe a: kurzes Bahnstueck a-s liegt an 9 V
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Potentiometer>("P1", plus, kGround, kGround, PotentiometerParams{}, 0.01);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok); // 9 V an 100 Ohm: 0,81 W >> 0,25 W * 0,02
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[1].destroyed);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(std::fabs(sim.probe().readings[0].currentA) < 1e-6);
    requireKirchhoff(sim);
}
