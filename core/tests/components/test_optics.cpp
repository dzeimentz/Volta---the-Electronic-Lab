// Fototransistor, IR-LED und Optokoppler (ab Volta 2): Lichtstrom, Saettigung, Kopplung, Abschattung, Zerstoerung.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/optics.hpp"
#include "volta/core/components/resistor.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

TEST_CASE("Fototransistor: Strom waechst mit dem Licht, Saettigung bei viel Licht", "[referenz][optik]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId col = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, col, ResistorParams{10e3});
    auto& pt = c.add<Phototransistor>("Q1", col, kGround, PhototransistorParams{}, 300.0);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    for (int k = 0; k < 5; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    // Handrechnung: IL = 0,5 uA/lx * 300 lx + 10 nA; Ic = IL * (1 + Uce / 50 V) mit Uce = Ub - 10 kOhm * Ic
    const double ub = sim.probe().voltage(plus);
    const double il = 0.5e-6 * 300.0 + 10e-9;
    const double ic = il * (1.0 + ub / 50.0) / (1.0 + il * 10e3 / 50.0);
    CHECK(sim.probe().readings[2].currentA == Approx(ic).epsilon(0.02));
    CHECK(sim.probe().voltage(col) > 7.0);
    requireKirchhoff(sim);

    pt.setIlluminance(5000.0); // Taschenlampe: IL 2,5 mA, mehr als 9 V / 10 kOhm -> gesaettigt
    for (int k = 0; k < 5; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    CHECK(sim.probe().voltage(col) < 0.3);
    CHECK(sim.probe().readings[2].currentA == Approx(ub / 10e3).epsilon(0.03));
    requireKirchhoff(sim);

    pt.setIlluminance(0.0); // dunkel: nur der Dunkelstrom
    for (int k = 0; k < 5; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    CHECK(std::fabs(sim.probe().readings[2].currentA) < 20e-9);
    CHECK(!pt.isOverloaded());
}

TEST_CASE("Fototransistor verkehrt herum: kaum Strom", "[optik]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId e = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, e, ResistorParams{1000.0});
    c.add<Phototransistor>("Q1", kGround, e, PhototransistorParams{}, 5000.0); // Kollektor an 0V
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    for (int k = 0; k < 5; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    // hoechstens 2 % von 2,5 mA
    CHECK(std::fabs(sim.probe().readings[2].currentA) < 0.051e-3);
    requireKirchhoff(sim);
}

TEST_CASE("IR-LED strahlt auf den Fototransistor, Karte schattet ab", "[referenz][optik]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    const NodeId col = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{470.0});
    auto& led = c.add<Diode>("D1", a, kGround, DiodeParams::irLed());
    c.add<Resistor>("R2", plus, col, ResistorParams{1000.0});
    auto& pt = c.add<Phototransistor>("Q1", col, kGround, PhototransistorParams{}, 0.0);
    REQUIRE(pt.addEmitter(&led, 0.1));
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    for (int k = 0; k < 5; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    // IR-LED: Uf um 1,3 V, Strom (Ub - Uf) / 470 Ohm
    const double iLed = led.currentA();
    CHECK(sim.probe().voltage(a) == Approx(1.3).margin(0.08));
    CHECK(iLed == Approx((sim.probe().voltage(plus) - sim.probe().voltage(a)) / 470.0).epsilon(0.01));
    CHECK(pt.lightCurrentA() == Approx(10e-9 + 0.1 * iLed).epsilon(0.01));
    const double ub = sim.probe().voltage(plus);
    CHECK(sim.probe().readings[4].currentA == Approx(pt.lightCurrentA() * (1.0 + sim.probe().voltage(col) / 50.0)).epsilon(0.02));
    CHECK(sim.probe().voltage(col) < ub - 1.0); // merklicher Strom
    requireKirchhoff(sim);

    pt.setShade(0.0); // Karte davor
    for (int k = 0; k < 3; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    CHECK(pt.lightCurrentA() == Approx(10e-9).epsilon(1e-6));
    CHECK(sim.probe().voltage(col) > ub - 0.01);
}

TEST_CASE("Optokoppler: Ausgang folgt dem LED-Strom mit CTR 100 %", "[referenz][optik]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    const NodeId col = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{8200.0});
    auto& ok = c.add<Optocoupler>("OK1", a, kGround, col, kGround);
    c.add<Resistor>("R2", plus, col, ResistorParams{1000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    for (int k = 0; k < 5; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    const double iLed = ok.led().currentA();
    CHECK(iLed == Approx((sim.probe().voltage(plus) - sim.probe().voltage(a)) / 8200.0).epsilon(0.01));
    CHECK(sim.probe().voltage(a) == Approx(1.15).margin(0.1)); // PC817: 1,2 V bei 20 mA, etwas weniger bei 1 mA
    const ComponentReading& r = sim.probe().readings[2];
    const double vce = sim.probe().voltage(col);
    CHECK(r.terminalCurrentA[2] == Approx((10e-9 + iLed) * (1.0 + vce / 50.0)).epsilon(0.02));
    CHECK(r.terminalCurrentA[0] == Approx(iLed).epsilon(0.01));
    requireKirchhoff(sim);
}

TEST_CASE("Optokoppler: LED ohne Vorwiderstand geht kaputt", "[optik][zerstoerung]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId col = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    auto& ok = c.add<Optocoupler>("OK1", plus, kGround, col, kGround);
    c.add<Resistor>("R2", plus, col, ResistorParams{1000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    bool destroyed = false;
    for (int k = 0; k < 5000 && !destroyed; ++k) {
        REQUIRE(sim.step(1e-3) == Status::Ok);
        destroyed = ok.isDestroyed();
    }
    CHECK(destroyed);
    for (int k = 0; k < 5; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    CHECK(std::fabs(sim.probe().readings[1].currentA) < 1e-6); // offen
    CHECK(sim.probe().voltage(col) > sim.probe().voltage(plus) - 0.01); // Ausgang sperrt
    requireFinite(sim.probe());
    // Undo: Zustand von vorher (intakt) wiederherstellen
    ComponentState fresh;
    ok.restoreState(fresh);
    CHECK(!ok.isDestroyed());
}
