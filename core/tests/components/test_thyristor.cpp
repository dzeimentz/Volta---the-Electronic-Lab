// Thyristor, Triac und Diac (ab Volta 3, Stufe 4): Zuenden, Halten, Loeschen an der Batterie, Loeschen je Halbwelle
// an Wechselspannung und Phasenanschnitt mit Diac und Triac; Zuendzeitpunkt unabhaengig von der Schrittweite.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/sources.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/components/thyristor.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
constexpr double kPiConst = 3.14159265358979323846;
void run(Simulator& sim, double seconds, double dt) {
    const int n = static_cast<int>(std::lround(seconds / dt));
    for (int k = 0; k < n; ++k) REQUIRE(sim.step(dt) == Status::Ok);
}
} // namespace

TEST_CASE("Thyristor: einmal gezuendet bleibt er an", "[referenz][transient][thyristor]") {
    Circuit c;
    const NodeId plus = c.addNode(), sw = c.addNode(), anode = c.addNode(), gate = c.addNode(), top = c.addNode();
    c.add<Battery>("B1", plus, kGround);                       // 0
    c.add<Switch>("S2", plus, top, true);                       // 1 (Loeschtaster in der Anodenleitung)
    c.add<Resistor>("R1", top, anode, ResistorParams{180.0});   // 2
    c.add<Thyristor>("TY1", anode, kGround, gate);              // 3
    c.add<Switch>("S1", plus, sw, false);                       // 4 (Zuendtaster)
    c.add<Resistor>("R2", sw, gate, ResistorParams{4700.0});    // 5
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& s1 = static_cast<Switch&>(sim.circuit().component(4));
    auto& s2 = static_cast<Switch&>(sim.circuit().component(1));
    auto& ty = static_cast<Thyristor&>(sim.circuit().component(3));
    const double dt = 1e-4;
    run(sim, 0.01, dt);
    CHECK_FALSE(ty.isOn());
    CHECK(std::fabs(sim.probe().readings[3].currentA) < 1e-6);
    s1.setClosed(true);
    run(sim, 0.01, dt);
    REQUIRE(ty.isOn());
    const double uBatt = sim.probe().voltage(plus);
    CHECK(sim.probe().readings[3].currentA == Approx((uBatt - 0.8) / 180.5).epsilon(0.02));
    s1.setClosed(false); // Gate offen: bleibt gezuendet
    run(sim, 0.05, dt);
    CHECK(ty.isOn());
    CHECK(sim.probe().readings[3].currentA > 0.04);
    s2.setClosed(false); // Anodenstrom unterbrochen: geloescht
    run(sim, 0.01, dt);
    CHECK_FALSE(ty.isOn());
    s2.setClosed(true);
    run(sim, 0.01, dt);
    CHECK_FALSE(ty.isOn());
    CHECK(std::fabs(sim.probe().readings[3].currentA) < 1e-6);
    requireKirchhoff(sim);
}

TEST_CASE("Thyristor: unter dem Haltestrom loescht er", "[referenz][transient][thyristor]") {
    Circuit c;
    const NodeId plus = c.addNode(), sw = c.addNode(), anode = c.addNode(), gate = c.addNode(), top = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, anode, ResistorParams{4700.0}); // allein etwa 1,7 mA < 3 mA Haltestrom
    c.add<Thyristor>("TY1", anode, kGround, gate);              // 2
    c.add<Switch>("S1", plus, sw, true);                        // 3
    c.add<Resistor>("R2", sw, gate, ResistorParams{4700.0});
    c.add<Switch>("S2", plus, top, true);                       // 5
    c.add<Resistor>("R3", top, anode, ResistorParams{1000.0});  // zusammen etwa 9 mA
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& ty = static_cast<Thyristor&>(sim.circuit().component(2));
    run(sim, 0.01, 1e-4);
    REQUIRE(ty.isOn());
    static_cast<Switch&>(sim.circuit().component(3)).setClosed(false);
    run(sim, 0.01, 1e-4);
    CHECK(ty.isOn()); // ueber dem Haltestrom bleibt er an
    static_cast<Switch&>(sim.circuit().component(5)).setClosed(false);
    run(sim, 0.01, 1e-4);
    CHECK_FALSE(ty.isOn()); // 1,7 mA reichen nicht
    CHECK(sim.probe().readings[2].currentA < 1e-6);
    requireKirchhoff(sim);
}

// Thyristor an 9 V~ mit Gate ueber 10 kOhm an der Anode: leitet nur in der positiven Halbwelle
static double halfWaveMeanA(double dt, double* negPeak) {
    Circuit c;
    const NodeId a = c.addNode(), anode = c.addNode(), gate = c.addNode();
    c.add<AcSource>("W1", a, kGround);
    c.add<Resistor>("R1", a, anode, ResistorParams{100.0});
    c.add<Thyristor>("TY1", anode, kGround, gate); // 2
    c.add<Resistor>("R2", anode, gate, ResistorParams{10000.0});
    c.setSecondOrder(true);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    run(sim, 0.04, dt);
    const int n = static_cast<int>(std::lround(0.1 / dt));
    double sum = 0.0, neg = 0.0;
    for (int k = 0; k < n; ++k) {
        REQUIRE(sim.step(dt) == Status::Ok);
        const double i = sim.probe().readings[2].currentA;
        sum += i;
        neg = std::min(neg, i);
    }
    requireKirchhoff(sim);
    *negPeak = neg;
    return sum / n;
}

TEST_CASE("Thyristor: an Wechselspannung loescht er in jeder Halbwelle", "[referenz][transient][thyristor]") {
    double negFine = 0.0, negApp = 0.0;
    const double fine = halfWaveMeanA(1e-5, &negFine);
    const double app = halfWaveMeanA(1e-4, &negApp);
    // Einweg, spaet gezuendet: Gatestrom ueber 10 kOhm erreicht den Zuendstrom (0,2 mA im Uebergang plus 0,6 mA im
    // inneren Gatewiderstand) erst bei etwa 8,7 V an der Anode
    const double peak = 9.0 * std::sqrt(2.0) / (100.0 + 2.0);
    CHECK(fine < 0.95 * peak / kPiConst);
    CHECK(fine > 0.6 * peak / kPiConst);
    CHECK(app == Approx(fine).epsilon(0.02));
    CHECK(negFine > -1e-4); // in der negativen Halbwelle sperrt er
}

// Phasenanschnitt beider Halbwellen: Last 100 Ohm, RC 4,7 kOhm / 1 uF an MT2, Diac vom Kondensator zum Gate
static double triacRmsA(double dt, double ohm, double* posPeak, double* negPeak) {
    Circuit c;
    const NodeId a = c.addNode(), mt2 = c.addNode(), cap = c.addNode(), gate = c.addNode();
    c.add<AcSource>("W1", a, kGround);
    c.add<Resistor>("R1", a, mt2, ResistorParams{100.0});
    c.add<Thyristor>("TR1", mt2, kGround, gate, ThyristorParams::triac()); // 2
    c.add<Resistor>("R2", mt2, cap, ResistorParams{ohm});
    c.add<Capacitor>("C1", cap, kGround, CapacitorParams{1e-6});
    c.add<Thyristor>("DI1", cap, gate, ThyristorParams::diac()); // 5
    c.setSecondOrder(true);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    run(sim, 0.06, dt);
    const int n = static_cast<int>(std::lround(0.1 / dt));
    double sq = 0.0, pos = 0.0, neg = 0.0;
    for (int k = 0; k < n; ++k) {
        REQUIRE(sim.step(dt) == Status::Ok);
        const double i = sim.probe().readings[2].currentA;
        sq += i * i;
        pos = std::max(pos, i);
        neg = std::min(neg, i);
    }
    requireKirchhoff(sim);
    *posPeak = pos;
    *negPeak = neg;
    return std::sqrt(sq / n);
}

TEST_CASE("Triac und Diac: Phasenanschnitt beider Halbwellen", "[referenz][transient][thyristor]") {
    double p1 = 0.0, n1 = 0.0, p2 = 0.0, n2 = 0.0, p3 = 0.0, n3 = 0.0;
    const double fine = triacRmsA(1e-5, 4700.0, &p1, &n1);
    const double app = triacRmsA(1e-4, 4700.0, &p2, &n2);
    const double later = triacRmsA(1e-5, 2.0 * 4700.0, &p3, &n3);
    const double full = 9.0 / 102.0;
    CHECK(p1 > 0.05);   // positive Halbwelle leitet
    CHECK(n1 < -0.05);  // negative auch
    CHECK(p1 == Approx(-n1).epsilon(0.05)); // symmetrisch
    CHECK(fine < 0.95 * full); // angeschnitten
    CHECK(fine > 0.2 * full);
    CHECK(app == Approx(fine).epsilon(0.02));
    CHECK(later < fine); // groesseres R: spaeter gezuendet, weniger Strom
}
