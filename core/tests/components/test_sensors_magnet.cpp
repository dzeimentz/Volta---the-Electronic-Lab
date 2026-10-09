// Fotowiderstand, Heissleiter, Elektromagnet, Relais: Kennlinien gegen Referenzwerte, Traegheit, Schaltschwellen.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/magnet.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/sensors.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
void run(Simulator& sim, double seconds) {
    const int steps = static_cast<int>(std::lround(seconds / kDefaultStepS));
    for (int k = 0; k < steps; ++k) {
        REQUIRE(sim.step() == Status::Ok);
        if (k % 17 == 0) requireKirchhoff(sim);
    }
}
} // namespace

TEST_CASE("Fotowiderstand folgt der Kennlinie und reagiert traege", "[sensor]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Photoresistor>("LDR1", plus, kGround, PhotoresistorParams{}, 10.0);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& ldr = static_cast<Photoresistor&>(sim.circuit().component(1));
    run(sim, 0.2);
    // 10 lx: R = 1 / (1/1 MOhm + 1/10 kOhm) = 9,90 kOhm
    const double r10 = 1.0 / (1.0 / 1e6 + 1.0 / 1e4);
    const ComponentReading& r = sim.probe().readings[1];
    CHECK(near(r.voltV / r.currentA, r10, 1.0));
    // 1000 lx: Fotoanteil 10 kOhm * 100^-0,7 = 397,9 Ohm
    ldr.setIlluminance(1000.0);
    run(sim, 0.005); // traege: nach 5 ms noch nicht da
    CHECK(ldr.resistanceOhm() > 1000.0);
    run(sim, 0.3);
    const double r1000 = 1.0 / (1.0 / 1e6 + std::pow(100.0, 0.7) / 1e4);
    CHECK(near(ldr.resistanceOhm(), r1000, 0.5));
    // dunkel: Dunkelwiderstand
    ldr.setIlluminance(0.0);
    run(sim, 3.0); // Abklingen mit tau 0,1 s
    CHECK(near(ldr.resistanceOhm(), 1e6, 100.0));
}

TEST_CASE("Heissleiter: R(T) nach B-Formel, folgt der Umgebung mit Zeitkonstante", "[sensor]") {
    ThermistorParams tp;
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId mid = c.addNode();
    c.add<Battery>("B1", plus, kGround, BatteryParams{2}); // 3 V: kaum Eigenerwaermung
    c.add<Resistor>("R1", plus, mid, ResistorParams{10000.0});
    c.add<Thermistor>("NTC1", mid, kGround, tp, 298.15);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& ntc = static_cast<Thermistor&>(sim.circuit().component(2));
    CHECK(ntc.resistanceAt(298.15) == Approx(10000.0));
    CHECK(ntc.resistanceAt(323.15) == Approx(10000.0 * std::exp(3950.0 * (1.0 / 323.15 - 1.0 / 298.15))));
    run(sim, 0.1);
    // Teiler 10k/10k bei 25 Grad C: halbe Spannung (Eigenerwaermung < 0,1 K)
    const double vBat = sim.probe().readings[0].voltV;
    CHECK(near(sim.probe().readings[2].voltV, vBat / 2.0, 0.01));
    // Foehn: 60 Grad C Luft; nach 1 tau (4 s) ca. 63 % des Wegs, nach 30 s eingeschwungen
    ntc.setAmbient(333.15);
    run(sim, 4.0);
    const double t4 = sim.probe().readings[2].tempK;
    CHECK(t4 == Approx(298.15 + 0.632 * 35.0).margin(1.5));
    run(sim, 26.0);
    CHECK(sim.probe().readings[2].tempK == Approx(333.15).margin(0.3));
    const double rHot = sim.probe().readings[2].voltV / sim.probe().readings[2].currentA;
    CHECK(near(rHot, ntc.resistanceAt(333.15), 50.0));
    CHECK(rHot < 3500.0); // heiss: deutlich kleiner
}

TEST_CASE("Elektromagnet: Ohmsches Gesetz, Feldstaerke folgt dem Strom", "[magnet]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Electromagnet>("EM1", plus, kGround);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    run(sim, 0.05);
    const ComponentReading& r = sim.probe().readings[1];
    CHECK(nearA(r.currentA, r.voltV / 100.0));
    CHECK(r.level == Approx(r.currentA / 0.09));
    CHECK(r.level > 0.9);
    CHECK_FALSE(r.overloaded); // ca. 0,8 W
}

TEST_CASE("Relais zieht mit Verzoegerung an, faellt mit Hysterese ab, Kontakt schaltet", "[magnet]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId coil = c.addNode();
    const NodeId lamp = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    // Spulenstrom ueber einen Vorwiderstand einstellbar
    c.add<Resistor>("RS", plus, coil, ResistorParams{1.0, 5.0});
    c.add<Relay>("K1", coil, kGround, plus, lamp);
    c.add<Resistor>("RL", lamp, kGround, ResistorParams{1000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& rs = static_cast<Resistor&>(sim.circuit().component(1));
    auto& k = static_cast<Relay&>(sim.circuit().component(2));
    // ca. 50 mA: angezogen, Last am Kontakt bekommt Strom
    run(sim, 0.05);
    CHECK(k.isClosed());
    CHECK(sim.probe().readings[2].currentA > 0.045);
    CHECK(sim.probe().readings[3].currentA > 8e-3);
    // 600 Ohm davor: ca. 11,5 mA, unter der Abfallschwelle 12 mA
    rs.setOhm(600.0);
    run(sim, 0.003);
    CHECK(k.isClosed()); // Abfallverzoegerung 4 ms
    run(sim, 0.01);
    CHECK_FALSE(k.isClosed());
    CHECK(std::fabs(sim.probe().readings[3].currentA) < 1e-6);
    // Hysterese: 270 Ohm -> ca. 20 mA zwischen Abfall- und Anzugsschwelle: bleibt abgefallen
    rs.setOhm(270.0);
    run(sim, 0.05);
    CHECK_FALSE(k.isClosed());
    rs.setOhm(1.0);
    run(sim, 0.005);
    CHECK_FALSE(k.isClosed()); // Anzugsverzoegerung 8 ms
    run(sim, 0.01);
    CHECK(k.isClosed());
}
