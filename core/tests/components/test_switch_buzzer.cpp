// Schalter, Wechselschalter (Treppenhausschaltung) und Summer.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/buzzer.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/switch.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

TEST_CASE("Schalter: offen kein Strom, geschlossen Ohmsches Gesetz", "[bauteil][schalter]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Switch>("S1", plus, a);
    c.add<Resistor>("R1", a, kGround, ResistorParams{1000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(std::fabs(sim.probe().readings[2].currentA) < 1e-8);
    CHECK(sim.probe().readings[1].level == 0.0);
    CHECK(sim.probe().readings[1].voltV == Approx(9.0).epsilon(1e-3)); // volle Spannung am offenen Kontakt

    static_cast<Switch&>(sim.circuit().component(1)).setClosed(true); // Bedienhandlung, kein Topologiewechsel
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[2].currentA == Approx(9.0 / (1000.0 + 1.5 + kContactClosedOhm)).epsilon(1e-6));
    CHECK(sim.probe().readings[1].level == 1.0);
    requireKirchhoff(sim);
}

TEST_CASE("Treppenhausschaltung mit zwei Wechselschaltern", "[referenz][schalter]") {
    // Plus -> W1 (Mitte) ; W1.A--W2.A, W1.B--W2.B ; W2 (Mitte) -> Lampe (hier 470 Ohm) -> Masse
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId la = c.addNode();
    const NodeId lb = c.addNode();
    const NodeId out = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<ChangeoverSwitch>("W1", plus, la, lb);
    c.add<ChangeoverSwitch>("W2", out, la, lb);
    c.add<Resistor>("R1", out, kGround, ResistorParams{470.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& w1 = static_cast<ChangeoverSwitch&>(sim.circuit().component(1));
    auto& w2 = static_cast<ChangeoverSwitch&>(sim.circuit().component(2));
    for (int s1 = 0; s1 < 2; ++s1) {
        for (int s2 = 0; s2 < 2; ++s2) {
            w1.setToB(s1 == 1);
            w2.setToB(s2 == 1);
            REQUIRE(sim.step() == Status::Ok);
            const bool on = sim.probe().readings[3].currentA > 1e-3;
            INFO("W1=" << s1 << " W2=" << s2);
            CHECK(on == (s1 == s2)); // Licht an, wenn beide in gleicher Stellung (XNOR)
            requireKirchhoff(sim);
        }
    }
}

TEST_CASE("Summer an 9 V: ca. 25 mA, volle Lautstaerke, 2,4 kHz", "[bauteil][summer]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Buzzer>("SU1", plus, kGround);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const ComponentReading& b = sim.probe().readings[1];
    CHECK(b.currentA > 0.022);
    CHECK(b.currentA < 0.027);
    CHECK(b.level > 0.95);
    CHECK(b.frequencyHz == 2400.0);
    requireKirchhoff(sim);
}

TEST_CASE("Summer verpolt bleibt stumm und stromlos", "[bauteil][summer]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Buzzer>("SU1", kGround, plus);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(std::fabs(sim.probe().readings[1].currentA) < 1e-6);
    CHECK(sim.probe().readings[1].level == 0.0);
    CHECK(sim.probe().readings[1].frequencyHz == 0.0);
}

TEST_CASE("Summer wird mit sinkender Spannung leiser", "[bauteil][summer]") {
    double previous = 2.0;
    for (int cells : {6, 3, 2, 1}) {
        Circuit c;
        const NodeId plus = c.addNode();
        c.add<Battery>("B1", plus, kGround, BatteryParams{cells});
        c.add<Buzzer>("SU1", plus, kGround);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        const double level = sim.probe().readings[1].level;
        INFO(cells << " Zellen: Lautstaerke " << level);
        CHECK(level < previous);
        previous = level;
    }
    CHECK(previous == 0.0); // 1,5 V: stumm
}
