// Spule (ab Volta 2): RL-Einschaltvorgang, Blindwiderstand bei Wechselspannung, Abschaltspitze, Ueberhitzung, Undo.
// Speicherdrossel (ab Volta 3): Kennwerte und Abwaertswandler mit 5 us Schrittweite.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <complex>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/inductor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/sources.hpp"
#include "volta/core/components/switch.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
constexpr double kPi = 3.14159265358979323846;

struct RlCircuit {
    Simulator sim;
    Switch* sw = nullptr;
    Inductor* coil = nullptr;
    NodeId plus = kGround, mid = kGround;
};

// 9-V-Batterie, Schalter, Spule 1 H gegen Masse
void buildRl(RlCircuit& rl, InductorParams p = InductorParams::h1()) {
    Circuit c;
    rl.plus = c.addNode();
    rl.mid = c.addNode();
    c.add<Battery>("B1", rl.plus, kGround);
    rl.sw = &c.add<Switch>("S1", rl.plus, rl.mid, false);
    rl.coil = &c.add<Inductor>("L1", rl.mid, kGround, p);
    REQUIRE(rl.sim.setCircuit(std::move(c)) == Status::Ok);
}
} // namespace

TEST_CASE("Spule 1 H: Strom erreicht nach tau = L / R 63,2 %", "[referenz][transient][spule]") {
    RlCircuit rl;
    buildRl(rl);
    rl.sw->setClosed(true);
    // Handrechnung: R = 100 Ohm Wicklung + 1,5 Ohm Batterie (+ Schalter); I_end = 9 / 101,5 = 88,7 mA
    const double rTotal = 101.5 + 0.01;
    const double iEnd = 9.0 / rTotal;
    const double tau = 1.0 / rTotal;
    const double dt = 1e-5;
    const int steps = static_cast<int>(std::lround(tau / dt));
    for (int k = 0; k < steps; ++k) REQUIRE(rl.sim.step(dt) == Status::Ok);
    CHECK(rl.coil->currentA() == Approx(iEnd * (1.0 - std::exp(-1.0))).epsilon(0.02));
    requireKirchhoff(rl.sim);
    for (int k = 0; k < 10 * steps; ++k) REQUIRE(rl.sim.step(dt) == Status::Ok);
    CHECK(rl.coil->currentA() == Approx(iEnd).epsilon(0.01));
    CHECK(rl.coil->isOverloaded()); // 88,7 mA > 50 mA
}

TEST_CASE("Spule 100 mH am Generator: Strom nach dem Blindwiderstand", "[referenz][transient][spule]") {
    Circuit c;
    const NodeId out = c.addNode();
    c.add<FunctionGenerator>("G1", out, kGround, GeneratorParams{Waveform::Sine, 1000.0, 4.0, 0.0, 50.0});
    const auto p = InductorParams::mH100();
    c.add<Inductor>("L1", out, kGround, p);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    // Unabhaengig komplex gerechnet: Z = 50 + ((30 + j w L) || Rp)
    const std::complex<double> zl(p.windingOhm, 2.0 * kPi * 1000.0 * p.inductanceH);
    const std::complex<double> z = 50.0 + zl * p.parallelOhm / (zl + p.parallelOhm);
    const double iPeak = 4.0 / std::abs(z); // ca. 6,3 mA (bei 1 kHz: X_L = 628 Ohm)
    const double dt = 1e-6;
    for (int k = 0; k < 30000; ++k) REQUIRE(sim.step(dt) == Status::Ok); // Einschwingen (tau = 1,25 ms)
    double peak = 0.0;
    for (int k = 0; k < 1000; ++k) { // eine Periode
        REQUIRE(sim.step(dt) == Status::Ok);
        peak = std::max(peak, std::fabs(sim.probe().readings[0].terminalCurrentA[0]));
    }
    CHECK(peak == Approx(iPeak).epsilon(0.02));
    requireKirchhoff(sim);
    // bei der zehnfachen Frequenz etwa ein Zehntel des Stroms (Konzept, Experiment 2.6)
    CHECK(iPeak > 6e-3);
    CHECK(iPeak < 6.5e-3);
}

TEST_CASE("Spule: Abschaltspitze bleibt endlich", "[spule][robust]") {
    RlCircuit rl;
    buildRl(rl, InductorParams::mH100());
    rl.sw->setClosed(true);
    for (int k = 0; k < 200; ++k) REQUIRE(rl.sim.step(1e-4) == Status::Ok);
    const double i0 = rl.coil->currentA();
    rl.sw->setClosed(false);
    REQUIRE(rl.sim.step(1e-5) == Status::Ok);
    const double v = rl.sim.probe().voltage(rl.mid);
    requireFinite(rl.sim.probe());
    // Der Strom sucht sich den Weg ueber Rp: Spannung umgekehrt, hoechstens I * Rp
    CHECK(v < -100.0);
    CHECK(std::fabs(v) <= i0 * rl.coil->params().parallelOhm * 1.01);
    for (int k = 0; k < 100; ++k) REQUIRE(rl.sim.step(1e-5) == Status::Ok);
    CHECK(std::fabs(rl.coil->currentA()) < 1e-3 * i0); // klingt mit L / Rp = 1 us ab
}

TEST_CASE("Spule: Ueberhitzung zerstoert die Wicklung, Undo stellt sie her", "[spule][thermik][undo]") {
    RlCircuit rl;
    buildRl(rl, InductorParams::mH100()); // 9 V / 31,5 Ohm = 0,29 A, 2,5 W in der Wicklung
    rl.sw->setClosed(true);
    REQUIRE(rl.sim.step(1e-3) == Status::Ok);
    const ComponentState fresh = rl.coil->saveState();
    for (int k = 0; k < 60000 && !rl.coil->isDestroyed(); ++k) rl.sim.step(1e-3);
    CHECK(rl.coil->isDestroyed());
    REQUIRE(rl.sim.step(1e-3) == Status::Ok);
    CHECK(std::fabs(rl.sim.probe().readings[2].currentA) < 1e-3); // offen, nur Rp
    rl.coil->restoreState(fresh);
    for (int k = 0; k < 20; ++k) REQUIRE(rl.sim.step(1e-3) == Status::Ok); // tau = 3,2 ms
    CHECK(!rl.coil->isDestroyed());
    CHECK(rl.coil->currentA() > 0.2);
}

// Abwaertswandler wie in Volta 3, Experiment 3.4: Rechteck 0 ... 8 V mit 3,8 kHz ueber eine Diode als Schalter,
// Freilaufdiode, Speicherdrossel, Elko 100 uF, Last 82 Ohm. Mit 5 us (Schrittweite der App mit Drossel) weicht die
// Ausgangsspannung hoechstens 2 % von der Rechnung mit 0,5 us ab (Konzept Volta 3, Kap. 4).
static double buckOutput(double dt) {
    Circuit c;
    const NodeId g = c.addNode(), sw = c.addNode(), out = c.addNode();
    c.add<FunctionGenerator>("G1", g, kGround, GeneratorParams{Waveform::Square, 3800.0, 4.0, 4.0, 0.5});
    c.add<Diode>("D1", g, sw, DiodeParams::silicon());
    c.add<Diode>("D2", kGround, sw, DiodeParams::silicon());
    c.add<Inductor>("L1", sw, out, InductorParams::choke10mH());
    c.add<Capacitor>("C1", out, kGround, CapacitorParams::electrolytic(100e-6));
    c.add<Resistor>("R1", out, kGround, ResistorParams{82.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const int n = static_cast<int>(std::lround(0.2 / dt));
    for (int k = 0; k < n; ++k) REQUIRE(sim.step(dt) == Status::Ok); // Einschwingen
    double sum = 0.0;
    for (int k = 0; k < n / 4; ++k) {
        REQUIRE(sim.step(dt) == Status::Ok);
        sum += sim.probe().voltage(out);
    }
    requireKirchhoff(sim);
    return sum / (n / 4);
}

TEST_CASE("Speicherdrossel: Kennwerte und Abwaertswandler mit 5 us", "[referenz][transient][spule][drossel]") {
    const auto p = InductorParams::choke10mH();
    CHECK(p.inductanceH == Approx(0.01));
    CHECK(p.windingOhm == Approx(2.0));
    CHECK(p.imaxA == Approx(0.5));
    const double fine = buckOutput(0.5e-6);
    const double app = buckOutput(5e-6);
    CHECK(fine > 3.0); // etwa 0,5 * (8 V - 0,7 V) abzueglich Freilaufdiode
    CHECK(fine < 4.5);
    CHECK(app == Approx(fine).epsilon(0.02));
}
