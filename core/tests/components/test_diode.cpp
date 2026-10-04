// Diode und Leuchtdiode: Kennlinie, Referenzschaltung 2, Sperrrichtung, Newton-Robustheit, Zustand.
// Referenzwerte stammen aus einem vom Simulator unabhaengigen Bisektionsloeser fuer U - I*R = n*N*Vd(I).
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/resistor.hpp"

using namespace volta::core;
using namespace volta::core::test;

namespace {

// Flussspannung einer Diode beim Strom i (Shockley umgekehrt plus Bahnwiderstand).
double forwardVoltage(const Diode& d, double i) {
    const double nVt = d.thermalVoltageV() * d.params().n;
    return nVt * std::log1p(i / d.saturationCurrentA()) + i * d.params().rsOhm;
}

// Strom in einer Reihe aus Quelle (emf, ri), Widerstand r und count gleichen Dioden, per Bisektion.
double seriesCurrent(double emf, double ri, double r, const Diode& d, int count = 1) {
    double lo = 0.0;
    double hi = emf / (ri + r + d.params().rsOhm * count + 1e-9);
    for (int k = 0; k < 200; ++k) {
        const double mid = 0.5 * (lo + hi);
        const double residual = emf - mid * (ri + r) - count * forwardVoltage(d, mid);
        (residual > 0.0 ? lo : hi) = mid;
    }
    return 0.5 * (lo + hi);
}

} // namespace

TEST_CASE("Diode: Saettigungsstrom reproduziert die Flussspannung beim Bezugsstrom", "[bauteil][diode]") {
    for (const DiodeParams& p : {DiodeParams::silicon(), DiodeParams::germanium(), DiodeParams::ledRed(),
                                 DiodeParams::ledYellow(), DiodeParams::ledGreen(), DiodeParams::ledBlue()}) {
        Diode d("D1", 1, 0, p);
        INFO("Uf = " << p.ufV << " V");
        CHECK(std::fabs(forwardVoltage(d, p.ifRefA) - p.ufV) < 1e-9);
        CHECK(d.saturationCurrentA() > 0.0);
        CHECK(d.criticalVoltageV() > 0.0);
        CHECK(d.criticalVoltageV() < p.ufV + 0.5);
    }
}

TEST_CASE("Rote LED: feste Handwerte als unabhaengige Referenz", "[referenz][diode]") {
    // Von Hand (Pruefbericht Schritt 2): Vt(20 Grad C) = k*T/q = 25,26 mV; n = 2, Rs = 5 Ohm, Uf = 1,9 V bei 20 mA
    // -> Is = 0,02 / (exp((1,9 - 0,1) / 0,05052) - 1) = 6,74e-18 A. An 9 V (Ri 1,5 Ohm) ueber 470 Ohm:
    // I = 15,140 mA, Uf = 1,8616 V.
    Diode d("D1", 1, 0, DiodeParams::ledRed());
    CHECK(d.thermalVoltageV() == Catch::Approx(0.02526).epsilon(1e-3));
    CHECK(d.saturationCurrentA() == Catch::Approx(6.74e-18).epsilon(0.01));

    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{470.0});
    c.add<Diode>("D1", a, kGround, DiodeParams::ledRed());
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(sim.probe().readings[2].currentA == Catch::Approx(15.140e-3).epsilon(1e-3));
    CHECK(sim.probe().readings[2].voltV == Catch::Approx(1.8616).epsilon(1e-3));
    // Innere Knoten (Bahnwiderstand) sind in der Probe nicht sichtbar.
    CHECK(sim.probe().nodeVolt.size() == sim.circuit().nodeCount());
}

TEST_CASE("Referenz 2: rote LED an 9 V ueber 470 Ohm ergibt ca. 15 mA", "[referenz][diode]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    const NodeId k = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{470.0});
    c.add<Diode>("D1", a, k, DiodeParams::ledRed());
    c.add<Resistor>("W1", k, kGround, ResistorParams{kWireOhm});

    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(sim.lastNewtonIterations() < kMaxNewtonIter);

    const auto& d = static_cast<const Diode&>(sim.circuit().component(2));
    const double expected = seriesCurrent(9.0, 1.5 + kWireOhm, 470.0, d); // (9 - 1,86) / 471,5 = 15,14 mA
    const ComponentReading& led = sim.probe().readings[2];
    CHECK(nearA(led.currentA, expected));
    CHECK(led.currentA > 14e-3);
    CHECK(led.currentA < 15.5e-3);
    CHECK(nearV(led.voltV, forwardVoltage(d, led.currentA)));
    CHECK(led.voltV > 1.8);
    CHECK(led.voltV < 2.0);
    requireKirchhoff(sim);
}

TEST_CASE("Siliziumdiode an 9 V ueber 1 kOhm", "[referenz][diode]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{1000.0});
    c.add<Diode>("D1", a, kGround, DiodeParams::silicon());
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);

    const auto& d = static_cast<const Diode&>(sim.circuit().component(2));
    const double expected = seriesCurrent(9.0, 1.5, 1000.0, d); // ca. (9 - 0,69) / 1001,5 = 8,3 mA
    CHECK(nearA(sim.probe().readings[2].currentA, expected));
    CHECK(nearV(sim.probe().voltage(a), forwardVoltage(d, expected)));
    CHECK(sim.probe().voltage(a) > 0.6);
    CHECK(sim.probe().voltage(a) < 0.75);
    requireKirchhoff(sim);
}

TEST_CASE("Vier Siliziumdioden in Reihe (Newton von null aus)", "[referenz][diode][newton]") {
    Circuit c;
    NodeId prev = c.addNode();
    c.add<Battery>("B1", prev, kGround);
    const NodeId first = c.addNode();
    c.add<Resistor>("R1", prev, first, ResistorParams{1000.0});
    prev = first;
    for (int i = 0; i < 4; ++i) {
        const NodeId next = i == 3 ? kGround : c.addNode();
        c.add<Diode>("D" + std::to_string(i), prev, next, DiodeParams::silicon());
        prev = next;
    }
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const auto& d = static_cast<const Diode&>(sim.circuit().component(2));
    const double expected = seriesCurrent(9.0, 1.5, 1000.0, d, 4);
    for (std::size_t i = 2; i < 6; ++i) CHECK(nearA(sim.probe().readings[i].currentA, expected));
    requireKirchhoff(sim);
}

TEST_CASE("LED in Sperrrichtung leuchtet nicht", "[bauteil][diode]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId k = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, k, ResistorParams{470.0});
    c.add<Diode>("D1", kGround, k, DiodeParams::ledRed()); // Kathode an Plus-Seite
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const ComponentReading& led = sim.probe().readings[2];
    CHECK(std::fabs(led.currentA) < 1e-6);
    CHECK(nearV(led.voltV, -9.0));
    requireKirchhoff(sim);
}

TEST_CASE("LED ohne Vorwiderstand an 9 V: endlich, konvergent, Strom weit ueber Grenzwert", "[robust][diode]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Diode>("D1", plus, kGround, DiodeParams::ledRed());
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    requireFinite(sim.probe());
    const auto& d = static_cast<const Diode&>(sim.circuit().component(1));
    const double expected = seriesCurrent(9.0, 1.5, 0.0, d); // ca. 1 A, Begrenzung nur durch Ri und Rs
    CHECK(nearA(sim.probe().readings[1].currentA, expected));
    CHECK(sim.probe().readings[1].currentA > 10.0 * d.params().imaxA);
    for (int i = 0; i < 100; ++i) REQUIRE(sim.step() == Status::Ok);
    requireKirchhoff(sim);
}

TEST_CASE("Rote und blaue LED parallel: nur die rote leuchtet", "[referenz][diode]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{470.0});
    c.add<Diode>("D1", a, kGround, DiodeParams::ledRed());
    c.add<Diode>("D2", a, kGround, DiodeParams::ledBlue());
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(sim.probe().readings[2].currentA > 14e-3);
    CHECK(sim.probe().readings[3].currentA < 1e-6);
    requireKirchhoff(sim);
}

TEST_CASE("Antiparallele Dioden an wechselnder Polung", "[robust][diode]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround, BatteryParams{2});
    c.add<Resistor>("R1", plus, a, ResistorParams{100.0});
    c.add<Diode>("D1", a, kGround, DiodeParams::silicon());
    c.add<Diode>("D2", kGround, a, DiodeParams::silicon());
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(sim.probe().readings[2].currentA > 1e-3);
    CHECK(std::fabs(sim.probe().readings[3].currentA) < 1e-6);
    requireKirchhoff(sim);
}

TEST_CASE("Diode ohne Bahnwiderstand braucht keinen inneren Knoten", "[bauteil][diode]") {
    DiodeParams p = DiodeParams::silicon();
    p.rsOhm = 0.0;
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{1000.0});
    c.add<Diode>("D1", a, kGround, p);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const auto& d = static_cast<const Diode&>(sim.circuit().component(2));
    CHECK(d.internalNodeCount() == 0);
    CHECK(nearA(sim.probe().readings[2].currentA, seriesCurrent(9.0, 1.5, 1000.0, d)));
    requireKirchhoff(sim);
}

TEST_CASE("Diode: unsinnige Kennwerte werden abgefangen", "[bauteil][diode][robust]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Diode d("D1", 1, 0, DiodeParams{nan, -1.0, 0.0, -5.0, 0.02});
    CHECK(std::isfinite(d.saturationCurrentA()));
    CHECK(d.saturationCurrentA() > 0.0);
    CHECK(d.params().n > 0.0);
    CHECK(d.params().rsOhm == 0.0);
    CHECK(std::isfinite(d.criticalVoltageV()));
    CHECK(std::isfinite(d.junctionCurrentA(1000.0))); // lineare Fortsetzung statt Ueberlauf
}

TEST_CASE("LED-Schaltung bleibt ueber Zeitschritte stabil", "[transient][diode]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{470.0});
    c.add<Diode>("D1", a, kGround, DiodeParams::ledGreen());
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const double i0 = sim.probe().readings[2].currentA;
    int maxIter = 0;
    for (int i = 0; i < 1000; ++i) {
        REQUIRE(sim.step() == Status::Ok);
        maxIter = std::max(maxIter, sim.lastNewtonIterations());
    }
    CHECK(maxIter <= 3); // Start vom angenommenen Zustand: sofortige Konvergenz
    CHECK(std::fabs(sim.probe().readings[2].currentA - i0) < 1e-9);
}

TEST_CASE("Zerstoerte LED sperrt, Undo stellt sie wieder her", "[bauteil][diode][undo]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{470.0});
    c.add<Diode>("D1", a, kGround, DiodeParams::ledRed());
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    ComponentModel& led = sim.circuit().component(2);
    const ComponentState intact = led.saveState();

    ComponentState broken = intact;
    broken.destroyed = true;
    led.restoreState(broken);
    REQUIRE(sim.solveOperatingPoint() == Status::Ok);
    CHECK(sim.probe().readings[2].destroyed);
    CHECK(std::fabs(sim.probe().readings[2].currentA) < 1e-6);
    CHECK(nearV(sim.probe().voltage(a), 9.0));
    requireKirchhoff(sim);

    led.restoreState(intact);
    REQUIRE(sim.solveOperatingPoint() == Status::Ok);
    CHECK_FALSE(sim.probe().readings[2].destroyed);
    CHECK(sim.probe().readings[2].currentA > 14e-3);
}

TEST_CASE("Zerstoerte Dioden: step() konvergiert, Undo ueber step() kehrt zurueck", "[robust][diode][undo]") {
    struct Case {
        const char* name;
        DiodeParams params;
        double seriesOhm;
    };
    for (const Case& k : {Case{"Ge an 100 Ohm", DiodeParams::germanium(), 100.0},
                          Case{"Si ohne Vorwiderstand", DiodeParams::silicon(), 0.0},
                          Case{"LED rot ohne Vorwiderstand", DiodeParams::ledRed(), 0.0},
                          Case{"LED blau an 470 Ohm", DiodeParams::ledBlue(), 470.0}}) {
        INFO(k.name);
        Circuit c;
        const NodeId plus = c.addNode();
        NodeId a = plus;
        c.add<Battery>("B1", plus, kGround);
        if (k.seriesOhm > 0.0) {
            a = c.addNode();
            c.add<Resistor>("R1", plus, a, ResistorParams{k.seriesOhm});
        }
        Diode& d = c.add<Diode>("D1", a, kGround, k.params);
        const std::size_t idx = c.componentCount() - 1;
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        (void)d;
        ComponentModel& model = sim.circuit().component(idx);
        const double iIntact = sim.probe().readings[idx].currentA;
        const ComponentState intact = model.saveState();

        ComponentState broken = intact;
        broken.destroyed = true;
        model.restoreState(broken);
        for (int i = 0; i < 20; ++i) REQUIRE(sim.step() == Status::Ok);
        CHECK(sim.lastNewtonIterations() <= 3);
        CHECK(std::fabs(sim.probe().readings[idx].currentA) < 1e-6);
        requireKirchhoff(sim);

        model.restoreState(intact); // Undo ohne neuen Arbeitspunkt: grosser Sprung im Newton-Verfahren
        REQUIRE(sim.step() == Status::Ok);
        CHECK(sim.lastNewtonIterations() < kMaxNewtonIter);
        for (int i = 0; i < 5; ++i) REQUIRE(sim.step() == Status::Ok);
        CHECK(sim.probe().readings[idx].currentA == Catch::Approx(iIntact).epsilon(1e-4));
        requireKirchhoff(sim);
    }
}

TEST_CASE("Germaniumdiode in Sperrrichtung: Sperrstrom etwa Is", "[bauteil][diode]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId k = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, k, ResistorParams{1000.0});
    c.add<Diode>("D1", kGround, k, DiodeParams::germanium());
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const auto& d = static_cast<const Diode&>(sim.circuit().component(2));
    // Sperrstrom = -Is (GMIN-Anteil 9 nA vernachlaessigbar gegen ca. 1,7 uA)
    CHECK(d.saturationCurrentA() == Catch::Approx(1.7e-6).epsilon(0.1));
    CHECK(sim.probe().readings[2].currentA == Catch::Approx(-d.saturationCurrentA()).epsilon(0.02));
    requireKirchhoff(sim);
}

TEST_CASE("Diode: unsinnig grosses Is ergibt endliche Begrenzung", "[robust][diode]") {
    Diode d("D1", 1, 0, DiodeParams{0.1, 1.0, 1.0, 0.0, 1.0}); // Is ca. 0,5 A
    CHECK(d.criticalVoltageV() >= d.thermalVoltageV());
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround, BatteryParams{2});
    c.add<Resistor>("R1", plus, a, ResistorParams{10.0});
    c.add<Diode>("D1", a, kGround, DiodeParams{0.1, 1.0, 1.0, 0.0, 1.0});
    c.add<Diode>("D2", kGround, a, DiodeParams{0.1, 1.0, 1.0, 0.0, 1.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    requireFinite(sim.probe());
    for (int i = 0; i < 10; ++i) REQUIRE(sim.step() == Status::Ok);
    requireKirchhoff(sim);
}
