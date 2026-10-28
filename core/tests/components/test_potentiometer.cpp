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

namespace {
// 9 V (ideal) an der Teilbahn a-s: a an Plus, Schleifer und b an Masse. Index 1 = Potentiometer.
Simulator wiperAtGround(double position) {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround, ideal());
    c.add<Potentiometer>("P1", plus, kGround, kGround, PotentiometerParams{}, position);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    return sim;
}
void runFor(Simulator& sim, double seconds) {
    const int n = static_cast<int>(std::lround(seconds / kDefaultStepS));
    for (int i = 0; i < n; ++i) REQUIRE(sim.step() == Status::Ok);
}
Potentiometer& pot(Simulator& sim) { return static_cast<Potentiometer&>(sim.circuit().component(1)); }
} // namespace

TEST_CASE("Potentiometer: Schleifer am Anschlag direkt an der Batterie brennt die Bahn durch", "[bauteil][potentiometer][grenzwert]") {
    // Belastbarkeit eines kurzen Bahnstuecks: 0,25 W * 0,25 = 62,5 mW, Rth 340 K/W / 0,25 = 1360 K/W, tau 10 s,
    // Grenze 505 K: Zerstoerung nach t = -tau * ln(1 - 211,85 K / (P * 1360 K/W))
    SECTION("10 Ohm: 8,1 W, nach etwa 0,19 s") {
        Simulator sim = wiperAtGround(0.001);
        runFor(sim, 0.15);
        CHECK(sim.probe().readings[1].overloaded);
        CHECK_FALSE(sim.probe().readings[1].destroyed);
        runFor(sim, 0.1);
        CHECK(sim.probe().readings[1].destroyed);
        REQUIRE(sim.step() == Status::Ok);
        CHECK(std::fabs(sim.probe().readings[0].currentA) < 1e-6);
        requireKirchhoff(sim);
    }
    SECTION("100 Ohm: 0,81 W, nach etwa 2,1 s") {
        Simulator sim = wiperAtGround(0.01);
        runFor(sim, 2.0);
        CHECK_FALSE(sim.probe().readings[1].destroyed);
        CHECK(sim.probe().readings[1].tempK > 480.0);
        runFor(sim, 0.3);
        CHECK(sim.probe().readings[1].destroyed);
        requireKirchhoff(sim);
    }
}

TEST_CASE("Potentiometer: dauerhaft ab etwa 2,5-facher Belastbarkeit durchgebrannt", "[bauteil][potentiometer][grenzwert]") {
    SECTION("2,3-fach: heiss und ueberlastet, haelt aber") {
        Simulator sim = wiperAtGround(81.0 / (2.3 * 0.0625) / 10000.0);
        runFor(sim, 60.0);
        CHECK(sim.probe().readings[1].overloaded);
        CHECK_FALSE(sim.probe().readings[1].destroyed);
        CHECK(sim.probe().readings[1].tempK == Approx(kAmbientK + 2.3 * 85.0).epsilon(0.02));
    }
    SECTION("2,8-fach: nach etwa 22 s") {
        Simulator sim = wiperAtGround(81.0 / (2.8 * 0.0625) / 10000.0);
        runFor(sim, 20.0);
        CHECK_FALSE(sim.probe().readings[1].destroyed);
        runFor(sim, 4.0);
        CHECK(sim.probe().readings[1].destroyed);
    }
}

TEST_CASE("Potentiometer: Bahn kuehlt nur langsam ab, pulsierende Ueberlast brennt sie durch", "[bauteil][potentiometer][grenzwert]") {
    // je 0,9 s 9 V an 100 Ohm, 0,1 s Mittelstellung (5 kOhm, kaum Leistung); ohne Waermegedaechtnis hielte sie ewig
    Simulator sim = wiperAtGround(0.01);
    runFor(sim, 0.9);
    CHECK_FALSE(sim.probe().readings[1].destroyed);
    bool destroyed = false;
    for (int cycle = 0; cycle < 5 && !destroyed; ++cycle) {
        pot(sim).setPosition(0.5);
        runFor(sim, 0.1);
        pot(sim).setPosition(0.01);
        runFor(sim, 0.9);
        destroyed = sim.probe().readings[1].destroyed;
    }
    CHECK(destroyed);
    requireKirchhoff(sim);
}

TEST_CASE("Potentiometer: Undo stellt die Temperatur der Teilbahnen wieder her", "[bauteil][potentiometer][undo]") {
    Simulator sim = wiperAtGround(0.01); // brennt nach etwa 2,14 s durch
    runFor(sim, 1.5);
    const ComponentState warm = pot(sim).saveState();
    CHECK(pot(sim).tempASK() > 400.0);
    CHECK(pot(sim).tempSBK() == Approx(kAmbientK).margin(1.0)); // s-b liegt kurzgeschlossen, bleibt kalt
    runFor(sim, 1.0);
    REQUIRE(sim.probe().readings[1].destroyed);

    pot(sim).restoreState(warm); // zurueck: heil, aber so warm wie nach 1,5 s
    runFor(sim, 0.4);
    CHECK_FALSE(sim.probe().readings[1].destroyed);
    runFor(sim, 0.5); // zusammen mehr als die fehlenden 0,64 s
    CHECK(sim.probe().readings[1].destroyed);

    pot(sim).restoreState(ComponentState{}); // Reparieren: neue, kalte Bahn
    CHECK(pot(sim).tempASK() == Approx(kAmbientK));
    runFor(sim, 1.5);
    CHECK_FALSE(sim.probe().readings[1].destroyed);
    requireKirchhoff(sim);
}

TEST_CASE("Potentiometer: Dimmer mit Vorwiderstand ueberlebt jede Stellung", "[bauteil][potentiometer][grenzwert]") {
    // Experiment 2.18: 470 Ohm in Reihe mit der Teilbahn a-s an 9 V (LED hier vereinfacht als 2 V weniger, also 7 V).
    // Hoechstens 49 / (4 * 470) = 26 mW bei 470 Ohm Teilbahn: ueberall unter 62,5 mW eines kurzen Bahnstuecks.
    for (double pos : {0.0, 0.005, 0.01, 0.015, 0.02, 0.025, 0.03, 0.047, 0.1, 0.5, 1.0}) {
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId mid = c.addNode();
        c.add<Battery>("B1", plus, kGround, BatteryParams{6, 7.0 / 6.0, kMinOhm});
        c.add<Resistor>("R1", plus, mid, ResistorParams{470.0});
        c.add<Potentiometer>("P1", mid, kGround, kGround, PotentiometerParams{}, pos);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        for (int i = 0; i < 2000; ++i) REQUIRE(sim.step() == Status::Ok);
        INFO("Stellung " << pos);
        CHECK_FALSE(sim.probe().readings[2].destroyed);
        CHECK_FALSE(sim.probe().readings[2].overloaded);
        const double rAS = std::max(pos * 10000.0, 1.0);
        CHECK(sim.probe().readings[2].currentA == Approx(7.0 / (470.0 + rAS)).epsilon(0.02));
        requireKirchhoff(sim);
    }
}
