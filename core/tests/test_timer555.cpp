// Timer 555 (Verhaltensmodell mit Ereignisortung, ab Volta 2): astabil (Frequenz und Tastverhaeltnis nach
// Datenblatt), monostabil (1,1 R C), Reset, Mindestversorgung, Kirchhoff.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/modules/timer555_module.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
struct Astable {
    double periodS = 0.0, highS = 0.0;
    int rises = 0;
};

// Astabile Grundschaltung: R1 von +U nach DIS, R2 von DIS nach THR/TR, C von THR/TR nach GND, Last 1 kOhm am
// Ausgang. Misst Periode und Dauer des hohen Pegels aus den Flanken (nach dem ersten Zyklus).
Astable runAstable(double r1, double r2, double c, double dtS, double seconds, int cells = 6) {
    Circuit circ;
    const NodeId vcc = circ.addNode(), dis = circ.addNode(), cap = circ.addNode(), out = circ.addNode();
    circ.add<Battery>("B1", vcc, kGround, BatteryParams{cells});
    circ.add<Resistor>("R1", vcc, dis, ResistorParams{r1});
    circ.add<Resistor>("R2", dis, cap, ResistorParams{r2});
    circ.add<Capacitor>("C1", cap, kGround, CapacitorParams::film(c));
    circ.add<Resistor>("RL", out, kGround, ResistorParams{1000.0});
    Timer555Pins p;
    p.gnd = kGround;
    p.trigger = cap;
    p.out = out;
    p.reset = vcc;
    p.control = circ.addNode(); // CV offen (nur Teiler)
    p.threshold = cap;
    p.discharge = dis;
    p.vcc = vcc;
    const NodeId cv = p.control;
    addTimer555(circ, "IC1", p);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
    Astable r;
    double t = 0.0, lastRise = -1.0, firstRise = -1.0, lastFall = -1.0, highSum = 0.0;
    bool high = sim.probe().voltage(out) > 4.0, started = false;
    int highCount = 0;
    const int steps = static_cast<int>(seconds / dtS);
    // Flankenzeit aus der Ausgangsspannung linear zwischen zwei Schritten (der Teilschritt der Ereignisortung
    // liegt im Inneren des Schritts; gemessen wird hier nur das Raster)
    for (int k = 0; k < steps; ++k) {
        REQUIRE(sim.step(dtS) == Status::Ok);
        t += dtS;
        const bool now = sim.probe().voltage(out) > 4.0;
        if (now && !high && !started) { // erster Zyklus beginnt mit leerem Kondensator: nicht mitzaehlen
            started = true;
        } else if (now && !high) {
            if (firstRise < 0.0) firstRise = t;
            if (lastFall >= 0.0 && lastRise >= 0.0) {
                highSum += lastFall - lastRise;
                ++highCount;
            }
            lastRise = t;
            ++r.rises;
        } else if (!now && high) {
            lastFall = t;
        }
        high = now;
    }
    CHECK(sim.probe().voltage(cv) == Approx(2.0 / 3.0 * sim.probe().voltage(vcc)).epsilon(0.01));
    requireKirchhoff(sim);
    requireFinite(sim.probe());
    if (r.rises >= 2) r.periodS = (lastRise - firstRise) / (r.rises - 1);
    if (highCount > 0) r.highS = highSum / highCount;
    return r;
}
} // namespace

TEST_CASE("Timer 555 astabil: f = 1,44 / ((R1 + 2 R2) C) wie im Datenblatt", "[referenz][timer555]") {
    // R1 = 1 kOhm, R2 = 10 kOhm, C = 10 uF: T = ln2 (R1 + 2 R2) C = 145,6 ms, hoch ln2 (R1 + R2) C = 76,2 ms.
    // Das Modell liegt ca. 1 % darueber: DIS entlaedt nur bis ca. 0,09 V (9 mA durch R1 an 10 Ohm) wie der echte 555.
    const Astable a = runAstable(1e3, 10e3, 10e-6, 1e-3, 3.0);
    REQUIRE(a.rises >= 15);
    CHECK(a.periodS == Approx(std::log(2.0) * 21e3 * 10e-6).epsilon(0.02));
    CHECK(a.highS == Approx(std::log(2.0) * 11e3 * 10e-6).epsilon(0.03));
}

TEST_CASE("Timer 555 astabil: Ereignisortung haelt die Frequenz auch bei grobem Schritt", "[timer555]") {
    // Schritt 5 ms bei einer Halbperiode von ca. 70 ms: ohne Ereignisortung fiele jedes Kippen auf das Raster
    // (im Mittel 2,5 ms zu spaet je Halbperiode, Periode ca. 3,4 % zu lang, zusaetzlich zum Verfahrensfehler)
    const Astable fine = runAstable(1e3, 10e3, 10e-6, 1e-4, 2.0);
    const Astable coarse = runAstable(1e3, 10e3, 10e-6, 5e-3, 6.0);
    REQUIRE(coarse.rises >= 30);
    CHECK(coarse.periodS == Approx(fine.periodS).epsilon(0.03)); // Rueckwaerts-Euler: tau + dt/2, ca. 2,3 %
}

TEST_CASE("Timer 555 monostabil: Ausgang fuer 1,1 R C nach einem Impuls an TR", "[referenz][timer555]") {
    Circuit c;
    const NodeId vcc = c.addNode(), tr = c.addNode(), cap = c.addNode(), out = c.addNode();
    c.add<Battery>("B1", vcc, kGround);
    c.add<Switch>("S1", tr, kGround); // Taster von TR nach GND
    c.add<Resistor>("RT", vcc, tr, ResistorParams{10e3});
    c.add<Resistor>("R", vcc, cap, ResistorParams{10e3});
    c.add<Capacitor>("C", cap, kGround, CapacitorParams::film(10e-6));
    c.add<Resistor>("RL", out, kGround, ResistorParams{1000.0});
    Timer555Pins p;
    p.gnd = kGround;
    p.trigger = tr;
    p.out = out;
    p.reset = vcc;
    p.control = c.addNode();
    p.threshold = cap;
    p.discharge = cap;
    p.vcc = vcc;
    addTimer555(c, "IC1", p);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& button = static_cast<Switch&>(sim.circuit().component(1));
    for (int k = 0; k < 50; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    CHECK(sim.probe().voltage(out) < 0.5); // Ruhe: aus, C entladen
    CHECK(sim.probe().voltage(cap) < 0.05);
    button.setClosed(true);
    REQUIRE(sim.step(1e-3) == Status::Ok);
    button.setClosed(false);
    double t = 1e-3;
    double fall = -1.0;
    CHECK(sim.probe().voltage(out) > 6.5);
    for (int k = 0; k < 300; ++k) {
        REQUIRE(sim.step(1e-3) == Status::Ok);
        t += 1e-3;
        if (fall < 0.0 && sim.probe().voltage(out) < 4.0) fall = t;
    }
    // Ausgang hoch ab dem Ende des Impulsschritts (1 ms): ln3 R C = 1,1 R C spaeter aus, gemessen auf dem Raster
    CHECK(fall == Approx(1e-3 + std::log(3.0) * 10e3 * 10e-6).margin(1.2e-3));
    CHECK(sim.probe().voltage(cap) < 0.05); // wieder entladen
    requireKirchhoff(sim);
}

TEST_CASE("Timer 555: Reset an GND haelt den Ausgang aus, unter 4,5 V arbeitet er nicht", "[timer555]") {
    for (int cells : {6, 2}) {
        Circuit c;
        const NodeId vcc = c.addNode(), out = c.addNode(), reset = c.addNode();
        c.add<Battery>("B1", vcc, kGround, BatteryParams{cells});
        c.add<Switch>("S1", reset, kGround, cells == 6); // 9 V: Reset gedrueckt; 3 V: Reset offen
        c.add<Resistor>("RL", out, kGround, ResistorParams{1000.0});
        Timer555Pins p;
        p.gnd = kGround;
        p.trigger = kGround; // TR an GND: wuerde dauernd setzen
        p.out = out;
        p.reset = reset;
        p.control = c.addNode();
        p.threshold = c.addNode();
        p.discharge = c.addNode();
        p.vcc = vcc;
        addTimer555(c, "IC1", p);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        for (int k = 0; k < 10; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
        CHECK(sim.probe().voltage(out) < 0.2);
        requireKirchhoff(sim);
        if (cells == 6) { // Reset loslassen (offen gilt als hoch): TR an GND setzt sofort
            static_cast<Switch&>(sim.circuit().component(1)).setClosed(false);
            for (int k = 0; k < 3; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
            CHECK(sim.probe().voltage(out) > 6.5);
            requireKirchhoff(sim);
        }
    }
}

