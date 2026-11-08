// Uebertrager (ab Volta 3): Spannungsuebersetzung gegen die komplexe Rechnung und Sperrschwinger an einer Zelle
// (Experiment 3.5) mit 5 us gegen 1 us.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <complex>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/sources.hpp"
#include "volta/core/components/transformer.hpp"
#include "volta/core/components/transistor.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
constexpr double kPi = 3.14159265358979323846;
}

TEST_CASE("Uebertrager: Sekundaerspannung wie die komplexe Rechnung", "[referenz][transient][uebertrager]") {
    Circuit c;
    const NodeId g = c.addNode(), s = c.addNode();
    c.add<FunctionGenerator>("G1", g, kGround, GeneratorParams{Waveform::Sine, 1000.0, 4.0, 0.0, 50.0});
    const TransformerParams p;
    c.add<Transformer>("UE1", g, kGround, s, kGround, p);
    c.add<Resistor>("R1", s, kGround, ResistorParams{1000.0});
    c.setSecondOrder(true);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    // Unabhaengig komplex gerechnet (Rp vernachlaessigbar: 200 kOhm gegen 1 kOhm und 50 Ohm)
    const std::complex<double> jw(0.0, 2.0 * kPi * 1000.0);
    const double m = p.coupling * std::sqrt(p.inductance1H * p.inductance2H);
    const std::complex<double> z1 = 50.0 + p.winding1Ohm + jw * p.inductance1H;
    const std::complex<double> z2 = p.winding2Ohm + jw * p.inductance2H + 1000.0;
    const std::complex<double> i1 = 4.0 / (z1 - jw * m * jw * m / z2);
    const double vPeak = std::abs(jw * m * i1 / z2) * 1000.0;
    const double dt = 1e-6;
    for (int k = 0; k < 20000; ++k) REQUIRE(sim.step(dt) == Status::Ok);
    double peak = 0.0;
    for (int k = 0; k < 1000; ++k) {
        REQUIRE(sim.step(dt) == Status::Ok);
        peak = std::max(peak, std::fabs(sim.probe().voltage(s)));
    }
    CHECK(peak == Approx(vPeak).epsilon(0.02));
    requireKirchhoff(sim);
}

// Sperrschwinger wie Experiment 3.5: Zelle 1,5 V, 15 kOhm, NPN, weisse LED vom Kollektor nach Masse
static double jouleThiefLedMa(double dt, bool withLed, double* peakV = nullptr) {
    Circuit c;
    const NodeId plus = c.addNode(), col = c.addNode(), e2 = c.addNode(), base = c.addNode();
    BatteryParams bp;
    bp.cells = 1;
    c.add<Battery>("B1", plus, kGround, bp);
    c.add<Resistor>("R1", plus, e2, ResistorParams{15000.0});
    c.add<Transformer>("UE1", plus, col, base, e2);
    c.add<Npn>("T1", base, col, kGround);
    if (withLed) c.add<Diode>("D1", col, kGround, DiodeParams::ledWhite());
    c.setSecondOrder(true);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const int n = static_cast<int>(std::lround(0.1 / dt));
    for (int k = 0; k < n; ++k) REQUIRE(sim.step(dt) == Status::Ok);
    double sum = 0.0, vmax = 0.0;
    for (int k = 0; k < n; ++k) {
        REQUIRE(sim.step(dt) == Status::Ok);
        if (withLed) sum += sim.probe().readings[4].currentA;
        vmax = std::max(vmax, sim.probe().voltage(col));
    }
    requireFinite(sim.probe());
    if (peakV) *peakV = vmax;
    return sum / n * 1e3;
}

TEST_CASE("Uebertrager: Sperrschwinger an einer Zelle", "[referenz][transient][uebertrager]") {
    double vLed = 0.0, vOpen = 0.0;
    const double fine = jouleThiefLedMa(1e-6, true, &vLed);
    const double app = jouleThiefLedMa(5e-6, true);
    CHECK(fine > 4.0); // LED leuchtet an 1,5 V (etwa 6 mA im Mittel)
    CHECK(fine < 9.0);
    CHECK(app == Approx(fine).epsilon(0.03));
    CHECK(vLed > 2.8); // Kollektor ueber der Zellenspannung
    jouleThiefLedMa(5e-6, false, &vOpen);
    CHECK(vOpen > 100.0); // ohne LED: Abschaltspitze, nur durch Rp begrenzt
}
