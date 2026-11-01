// OP-Modul (Makromodell, ab Volta 2): invertierender und nichtinvertierender Verstaerker, Komparator, Integrator,
// Aussteuergrenzen, Strombegrenzung und Verhalten ohne Versorgung.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/modules/opamp_module.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
// Symmetrische Versorgung +-9 V aus zwei Batterien, Mitte = Masse; Eingangsquelle 1,5 V (eine Zelle)
struct Bench {
    Circuit c;
    NodeId plus = 0, minus = 0, src = 0, inP = 0, inN = 0, out = 0;
    Simulator sim;
    Bench() {
        plus = c.addNode();
        minus = c.addNode();
        src = c.addNode();
        inP = c.addNode();
        inN = c.addNode();
        out = c.addNode();
        c.add<Battery>("B1", plus, kGround);
        c.add<Battery>("B2", kGround, minus);
        c.add<Battery>("B3", src, kGround, BatteryParams{1});
    }
    void start(int steps = 20, double dtS = 1e-3) {
        addOpAmpModule(c, "OP1", OpAmpModulePins{plus, minus, inP, inN, out});
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        for (int k = 0; k < steps; ++k) REQUIRE(sim.step(dtS) == Status::Ok);
    }
    double v(NodeId n) const { return sim.probe().voltage(n); }
};
} // namespace

TEST_CASE("OP-Modul: invertierender Verstaerker V = -Rf/Rin", "[referenz][opamp]") {
    Bench b;
    b.c.add<Resistor>("R1", b.src, b.inN, ResistorParams{10e3});
    b.c.add<Resistor>("R2", b.inN, b.out, ResistorParams{22e3});
    b.c.add<Resistor>("R3", b.inP, kGround, ResistorParams{1e3});
    b.c.add<Resistor>("RL", b.out, kGround, ResistorParams{10e3});
    b.start();
    CHECK(b.v(b.out) == Approx(-2.2 * b.v(b.src)).epsilon(0.01));
    CHECK(std::fabs(b.v(b.inN)) < 1e-3); // virtuelle Masse
    requireKirchhoff(b.sim);
}

TEST_CASE("OP-Modul: nichtinvertierender Verstaerker V = 1 + Rf/Rg", "[referenz][opamp]") {
    Bench b;
    const NodeId mid = b.inP;
    b.c.add<Resistor>("R1", b.src, mid, ResistorParams{20e3}); // Teiler 1,5 V -> 0,5 V
    b.c.add<Resistor>("R2", mid, kGround, ResistorParams{10e3});
    b.c.add<Resistor>("R3", b.out, b.inN, ResistorParams{47e3});
    b.c.add<Resistor>("R4", b.inN, kGround, ResistorParams{10e3});
    b.start();
    CHECK(b.v(b.out) == Approx(5.7 * b.v(mid)).epsilon(0.01));
    requireKirchhoff(b.sim);
}

TEST_CASE("OP-Modul: Komparator geht an die Aussteuergrenzen, nicht an die Versorgung", "[opamp]") {
    Bench hi;
    hi.c.add<Resistor>("R1", hi.src, hi.inP, ResistorParams{1e3});
    hi.c.add<Resistor>("R2", hi.inN, kGround, ResistorParams{1e3});
    hi.c.add<Resistor>("RL", hi.out, kGround, ResistorParams{10e3});
    hi.start();
    const double vp = hi.v(hi.plus);
    CHECK(hi.v(hi.out) < vp - 1.3);
    CHECK(hi.v(hi.out) > vp - 1.8);
    CHECK(hi.sim.probe().readings.back().level == 1.0);
    requireKirchhoff(hi.sim);

    Bench lo;
    lo.c.add<Resistor>("R1", lo.src, lo.inN, ResistorParams{1e3});
    lo.c.add<Resistor>("R2", lo.inP, kGround, ResistorParams{1e3});
    lo.c.add<Resistor>("RL", lo.out, kGround, ResistorParams{10e3});
    lo.start();
    CHECK(lo.v(lo.out) > lo.v(lo.minus) + 1.3);
    CHECK(lo.v(lo.out) < lo.v(lo.minus) + 1.8);
    requireKirchhoff(lo.sim);
}

TEST_CASE("OP-Modul: Integrator laeuft mit -Uein/(RC) und bleibt an der Grenze stehen", "[referenz][opamp]") {
    Bench b;
    b.c.add<Resistor>("R1", b.src, b.inN, ResistorParams{100e3});
    b.c.add<Capacitor>("C1", b.inN, b.out, CapacitorParams::film(1e-6));
    b.c.add<Resistor>("R3", b.inP, kGround, ResistorParams{1e3});
    b.start(200); // 0,2 s
    const double expected = -b.v(b.src) / (100e3 * 1e-6) * 0.2; // ca. -3 V
    CHECK(b.v(b.out) == Approx(expected).epsilon(0.03));
    for (int k = 0; k < 2000; ++k) REQUIRE(b.sim.step(1e-3) == Status::Ok);
    CHECK(b.v(b.out) < b.v(b.minus) + 1.8); // in der Begrenzung
    requireKirchhoff(b.sim);
    requireFinite(b.sim.probe());
}

TEST_CASE("OP-Modul: Kurzschluss am Ausgang begrenzt den Strom", "[opamp]") {
    Bench b;
    b.c.add<Resistor>("R1", b.src, b.inP, ResistorParams{1e3});
    b.c.add<Resistor>("R2", b.inN, kGround, ResistorParams{1e3});
    b.c.add<Resistor>("RS", b.out, kGround, ResistorParams{0.1});
    b.start();
    const ComponentReading& r = b.sim.probe().readings.back();
    CHECK(-r.terminalCurrentA[4] == Approx(0.025).epsilon(0.05));
    // der Strom kommt aus +U: Anschluss plus nimmt Laststrom und Ruhestrom auf
    CHECK(r.terminalCurrentA[0] == Approx(0.025 + 0.0015).epsilon(0.05));
    requireKirchhoff(b.sim);
}

TEST_CASE("OP-Modul: ohne Versorgung liefert der Ausgang nichts", "[opamp]") {
    Circuit c;
    const NodeId plus = c.addNode(), src = c.addNode(), inP = c.addNode(), inN = c.addNode(), out = c.addNode();
    c.add<Battery>("B1", plus, kGround, BatteryParams{2}); // 3 V: unter der Mindestversorgung
    c.add<Battery>("B3", src, kGround, BatteryParams{1});
    c.add<Resistor>("R1", src, inP, ResistorParams{1e3});
    c.add<Resistor>("R2", inN, kGround, ResistorParams{1e3});
    c.add<Resistor>("RL", out, kGround, ResistorParams{1e3});
    addOpAmpModule(c, "OP1", OpAmpModulePins{plus, kGround, inP, inN, out});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    for (int k = 0; k < 10; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    CHECK(std::fabs(sim.probe().voltage(out)) < 0.01);
    requireKirchhoff(sim);
}

TEST_CASE("OP-Modul: Spannungsfolger auch mit kleinen Zeitschritten", "[opamp]") {
    Bench b;
    b.c.add<Resistor>("R1", b.src, b.inP, ResistorParams{1e3});
    b.c.add<Resistor>("RF", b.out, b.inN, ResistorParams{1.0});
    b.c.add<Resistor>("RL", b.out, kGround, ResistorParams{10e3});
    b.start(50, 1e-6);
    CHECK(b.v(b.out) == Approx(b.v(b.inP)).epsilon(0.005));
    requireKirchhoff(b.sim);
}

TEST_CASE("OP-Modul: Rechteckgenerator (Schmitt-Trigger mit RC) schwingt mit T = 2 RC ln 3", "[referenz][opamp]") {
    // Mitkopplung R1 = R2 = 10 kOhm (Schwellen bei +-U_aus/2), Gegenkopplung R = 10 kOhm auf C = 10 uF:
    // T = 2 R C ln(1 + 2 R2/R1) = 0,2197 s. Symmetrische Aussteuergrenzen, darum gilt die Formel unabhaengig von h.
    Bench b;
    b.c.add<Resistor>("R1", b.out, b.inP, ResistorParams{10e3});
    b.c.add<Resistor>("R2", b.inP, kGround, ResistorParams{10e3});
    b.c.add<Resistor>("R", b.out, b.inN, ResistorParams{10e3});
    b.c.add<Capacitor>("C", b.inN, kGround, CapacitorParams::film(10e-6));
    b.start(1);
    double last = b.v(b.out), firstEdge = -1.0, lastEdge = -1.0;
    int edges = 0;
    for (int k = 1; k <= 3000; ++k) {
        REQUIRE(b.sim.step(1e-3) == Status::Ok);
        const double v = b.v(b.out);
        if (k > 300 && (v > 0.0) != (last > 0.0)) { // Anlauf abwarten
            if (firstEdge < 0.0) firstEdge = k * 1e-3;
            lastEdge = k * 1e-3;
            ++edges;
        }
        last = v;
    }
    REQUIRE(edges >= 10);
    const double period = 2.0 * (lastEdge - firstEdge) / (edges - 1);
    CHECK(period == Approx(2.0 * 10e3 * 10e-6 * std::log(3.0)).epsilon(0.02));
    requireKirchhoff(b.sim);
}
