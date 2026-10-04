// Glühlampe (Referenzschaltung 6: Einschaltstromstoss) und Motor (Leerlauf, Anlauf, Haftreibung, Blockade).
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/lamp.hpp"
#include "volta/core/components/motor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/switch.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {

void runFor(Simulator& sim, double seconds) {
    const int steps = static_cast<int>(std::lround(seconds / kDefaultStepS));
    for (int i = 0; i < steps; ++i) REQUIRE(sim.step() == Status::Ok);
}

Circuit lampAt(int cells, double cellInnerOhm, LampParams p = {}) {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround, BatteryParams{cells, 1.5, cellInnerOhm});
    c.add<Lamp>("L1", plus, kGround, p);
    return c;
}

} // namespace

TEST_CASE("Lampe: Kennwerte sind in sich stimmig", "[bauteil][lampe]") {
    Lamp l("L1", 1, 0);
    CHECK(l.hotResistanceOhm() == Approx(120.0));
    CHECK(l.resistanceOhm() == Approx(12.0)); // kalt: ein Zehntel
    CHECK(l.nominalTempK() == Approx(293.15 * std::pow(10.0, 1.0 / 1.2)).epsilon(1e-6)); // ca. 2000 K
    CHECK(l.steadyTempK(6.0) == Approx(l.nominalTempK()).epsilon(1e-6));
}

TEST_CASE("Referenz 6: Gluehlampe Einschaltstrom > 5 x Betriebsstrom", "[referenz][lampe]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(lampAt(3, 0.25)) == Status::Ok); // 4,5 V an Lampe 6 V / 50 mA
    REQUIRE(sim.step() == Status::Ok);
    const double inrush = sim.probe().readings[1].currentA; // Faden kalt: 4,5 V / (12 + 0,75) Ohm = 0,35 A
    CHECK(inrush == Approx(4.5 / 12.75).epsilon(0.02));
    runFor(sim, 1.0); // Fadenzeitkonstante 50 ms
    const double steady = sim.probe().readings[1].currentA;
    INFO("Einschaltstrom " << inrush << " A, Betriebsstrom " << steady << " A");
    CHECK(inrush > 5.0 * steady);
    // Kennlinie I ~ U^0,54: bei 4,5 V (Ri 0,75 Ohm vernachlaessigbar) ca. 50 mA * 0,75^0,54 = 43 mA
    CHECK(steady > 0.040);
    CHECK(steady < 0.046);
    requireKirchhoff(sim);
}

TEST_CASE("Lampe an Nennspannung: Nennstrom, volle Helligkeit", "[bauteil][lampe]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(lampAt(4, kMinOhm)) == Status::Ok); // 6 V
    runFor(sim, 1.0);
    const ComponentReading& l = sim.probe().readings[1];
    CHECK(l.currentA == Approx(0.05).epsilon(0.02));
    CHECK(l.level == Approx(1.0).epsilon(0.03));
    CHECK_FALSE(l.overloaded);
}

TEST_CASE("Lampe: Helligkeit steigt mit der Spannung, zwei Lampen in Reihe leuchten schwaecher", "[bauteil][lampe]") {
    Simulator one;
    REQUIRE(one.setCircuit(lampAt(3, 0.25)) == Status::Ok);
    runFor(one, 1.0);
    Simulator two;
    {
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId mid = c.addNode();
        c.add<Battery>("B1", plus, kGround, BatteryParams{3});
        c.add<Lamp>("L1", plus, mid);
        c.add<Lamp>("L2", mid, kGround);
        REQUIRE(two.setCircuit(std::move(c)) == Status::Ok);
    }
    runFor(two, 1.0);
    CHECK(two.probe().readings[1].level < one.probe().readings[1].level);
    CHECK(two.probe().readings[1].level > 0.0);
    CHECK(two.probe().readings[1].currentA == Approx(two.probe().readings[2].currentA).epsilon(1e-6));
    requireKirchhoff(two);
}

TEST_CASE("Lampe 6 V an 9 V brennt nach einigen Sekunden durch", "[lampe][grenzwert]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(lampAt(6, 0.25)) == Status::Ok);
    runFor(sim, 0.5);
    CHECK(sim.probe().readings[1].overloaded);
    CHECK(sim.probe().readings[1].level > 1.0);
    double t = -1.0;
    for (int i = 0; i < 20000 && t < 0.0; ++i) {
        REQUIRE(sim.step() == Status::Ok);
        if (sim.probe().readings[1].destroyed) t = sim.timeS();
    }
    INFO("Durchgebrannt nach " << t << " s");
    CHECK(t > 4.5);
    CHECK(t < 6.0);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(std::fabs(sim.probe().readings[1].currentA) < 1e-6);
}

TEST_CASE("Lampe 4,5 V an 9 V (doppelte Spannung) brennt sofort durch", "[lampe][grenzwert]") {
    LampParams p;
    p.uNomV = 4.5;
    Simulator sim;
    REQUIRE(sim.setCircuit(lampAt(6, 0.25, p)) == Status::Ok);
    runFor(sim, 0.5);
    CHECK(sim.probe().readings[1].destroyed);
}

TEST_CASE("Lampe: Undo nach dem Durchbrennen", "[lampe][undo]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(lampAt(6, 0.25)) == Status::Ok);
    const ComponentState fresh = sim.circuit().component(1).saveState();
    runFor(sim, 7.0);
    REQUIRE(sim.probe().readings[1].destroyed);
    sim.circuit().component(1).restoreState(fresh);
    REQUIRE(sim.step() == Status::Ok);
    CHECK_FALSE(sim.probe().readings[1].destroyed);
    CHECK(sim.probe().readings[1].currentA > 0.3); // wieder kalt: Einschaltstromstoss
}

// ---------------------------------------------------------------- Motor

namespace {
Circuit motorAt(int cells, double cellInnerOhm, double seriesOhm = 0.0, bool reversed = false) {
    Circuit c;
    const NodeId plus = c.addNode();
    NodeId a = plus;
    c.add<Battery>("B1", plus, kGround, BatteryParams{cells, 1.5, cellInnerOhm});
    if (seriesOhm > 0.0) {
        a = c.addNode();
        c.add<Resistor>("R1", plus, a, ResistorParams{seriesOhm});
    }
    if (reversed)
        c.add<Motor>("M1", kGround, a);
    else
        c.add<Motor>("M1", a, kGround);
    return c;
}
} // namespace

TEST_CASE("Motor: Leerlauf an 3 V nach Gleichgewichtsrechnung", "[referenz][motor]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(motorAt(2, kMinOhm)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    const double startI = sim.probe().readings[1].currentA; // Anlauf: nur Ra, 3 V / 8 Ohm
    CHECK(startI == Approx(3.0 / 8.0).epsilon(0.02));
    runFor(sim, 2.0);
    // Gleichgewicht: k*I = Tf + b*w und U = Ra*I + k*w  ->  w = (U - Ra*Tf/k) / (k + Ra*b/k)
    const MotorParams p;
    const double u = 3.0 - 2 * kMinOhm * sim.probe().readings[1].currentA;
    const double w = (u - p.raOhm * p.frictionNm / p.kVs) / (p.kVs + p.raOhm * p.viscousNms / p.kVs);
    const auto& m = static_cast<const Motor&>(sim.circuit().component(1));
    CHECK(m.omegaRadS() == Approx(w).epsilon(0.01)); // ca. 217 rad/s, 2080 U/min
    CHECK(sim.probe().readings[1].currentA == Approx((p.frictionNm + p.viscousNms * w) / p.kVs).epsilon(0.01));
    CHECK(sim.probe().readings[1].level == Approx(1.0).epsilon(1e-3)); // Leerlauf bei Nennspannung
    CHECK(m.rpm() > 1900.0);
    CHECK(startI > 10.0 * sim.probe().readings[1].currentA); // Anlaufstrom
    requireKirchhoff(sim);
}

TEST_CASE("Motor: Drehrichtung wechselt mit der Polung", "[motor]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(motorAt(2, 0.25, 0.0, true)) == Status::Ok);
    runFor(sim, 1.0);
    CHECK(static_cast<const Motor&>(sim.circuit().component(1)).omegaRadS() < -100.0);
    CHECK(sim.probe().readings[1].level < 0.0);
}

TEST_CASE("Motor: zu kleiner Strom ueberwindet die Haftreibung nicht", "[motor]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(motorAt(6, 0.25, 2000.0)) == Status::Ok); // ca. 4,5 mA -> Moment < Reibung
    runFor(sim, 1.0);
    CHECK(static_cast<const Motor&>(sim.circuit().component(2)).omegaRadS() == 0.0);
    CHECK(sim.probe().readings[2].currentA == Approx(9.0 / (2000.0 + 8.0 + 1.5)).epsilon(1e-3));
}

TEST_CASE("Motor laeuft nach dem Ausschalten aus und bleibt stehen", "[motor]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround, BatteryParams{2});
    c.add<Switch>("S1", plus, a, true);
    c.add<Motor>("M1", a, kGround);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    runFor(sim, 1.0);
    auto& m = static_cast<Motor&>(sim.circuit().component(2));
    const double running = m.omegaRadS();
    CHECK(running > 100.0);
    static_cast<Switch&>(sim.circuit().component(1)).setClosed(false);
    runFor(sim, 0.05);
    // Auslauf nur gegen Reibung: dw/dt = -(Tf + b*w)/J = ca. -134 rad/s^2 -> nach 50 ms ca. 6,7 rad/s weniger
    const MotorParams p;
    const double decel = (p.frictionNm + p.viscousNms * running) / p.inertiaKgm2;
    CHECK(m.omegaRadS() == Approx(running - 0.05 * decel).epsilon(0.01));
    runFor(sim, 2.0); // Stillstand nach ca. 1,6 s
    CHECK(m.omegaRadS() == 0.0);
    requireKirchhoff(sim);
}

TEST_CASE("Motor blockiert an 9 V: ueberlastet, Wicklung brennt durch", "[motor][thermik]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(motorAt(6, 0.25)) == Status::Ok);
    static_cast<Motor&>(sim.circuit().component(1)).setBlocked(true);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[1].currentA == Approx(9.0 / 9.5).epsilon(0.01));
    CHECK(sim.probe().readings[1].overloaded);
    double t = -1.0;
    for (int i = 0; i < 30000 && t < 0.0; ++i) {
        REQUIRE(sim.step() == Status::Ok);
        if (sim.probe().readings[1].destroyed) t = sim.timeS();
    }
    INFO("Durchgebrannt nach " << t << " s");
    CHECK(t > 5.0);
    CHECK(t < 20.0);
}

TEST_CASE("Motor: Undo stellt die Drehzahl her", "[motor][undo]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(motorAt(2, 0.25)) == Status::Ok);
    runFor(sim, 1.0);
    auto& m = static_cast<Motor&>(sim.circuit().component(1));
    const ComponentState running = m.saveState();
    m.setBlocked(true);
    runFor(sim, 0.1);
    m.setBlocked(false);
    CHECK(m.omegaRadS() == 0.0);
    m.restoreState(running);
    REQUIRE(sim.solveOperatingPoint() == Status::Ok);
    CHECK(m.omegaRadS() == Approx(running.internal[0]));
    CHECK(std::fabs(sim.probe().readings[1].currentA) < 0.05); // laufender Motor: kein Anlaufstrom
}

TEST_CASE("Lampe: Strom-Spannungs-Kennlinie I ~ U^0,54 (Strahlungsgesetz)", "[referenz][lampe]") {
    // Herleitung: P ~ T^4, R ~ T^1,2 -> R ~ P^0,3 -> P^1,3 ~ U^2 -> I = P/U ~ U^(2/1,3 - 1) = U^0,538
    double i3 = 0.0;
    double i9 = 0.0;
    for (int cells : {2, 6}) {
        Simulator sim;
        REQUIRE(sim.setCircuit(lampAt(cells, kMinOhm)) == Status::Ok);
        runFor(sim, 1.0);
        (cells == 2 ? i3 : i9) = sim.probe().readings[1].currentA;
    }
    INFO("3 V: " << i3 * 1e3 << " mA, 9 V: " << i9 * 1e3 << " mA");
    // Exponent aus den Endpunkten (Naeherung gilt fuer T >> T0)
    const double exponent = std::log(i9 / i3) / std::log(3.0);
    CHECK(exponent == Approx(0.538).margin(0.03));
    CHECK(i3 == Approx(0.05 * std::pow(0.5, 0.538)).epsilon(0.03)); // ca. 34 mA
}
