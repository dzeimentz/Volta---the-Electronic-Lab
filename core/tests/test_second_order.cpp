// Zeitintegration zweiter Ordnung (BDF2, ab Volta 3): Ein freier Schwingkreis klingt nur so schnell ab, wie seine
// Widerstaende es verlangen. Rueckwaerts-Euler daempft zusaetzlich um etwa w^2 dt / 2 (bei 1 H, 1 uF und 20 us
// rund 20 % mehr Daempfung); BDF2 bleibt unter 1 %. Grenze: Der Phasenfehler von BDF2 waechst mit (w dt)^2;
// bei 20 us bleiben Schwingkreise bis etwa 1 kHz unter 2 % (1,6 kHz nach fuenf Perioden schon rund 12 %). Referenz: dieselbe Schaltung mit Runge-Kutta 4. Ordnung.
#include <algorithm>
#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/inductor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/simulator.hpp"

using namespace volta::core;
using namespace volta::core::test;

namespace {

struct Tank {
    double lH = 1.0, rOhm = 100.0, rpOhm = 1e5, cF = 1e-6, leakOhm = 1e9, u0V = 5.0;
};

// Kondensator geladen auf u0, Spule parallel: groesste Abweichung der Kondensatorspannung von der Referenz ueber
// `durationS`, bezogen auf den groessten Betrag der Referenz. (Der Arbeitspunkt zu Beginn rechnet die Spule als
// Wicklungswiderstand; der Strom darin ist der Anfangszustand der Referenz.)
double maxError(const Tank& t, bool secondOrder, double dtS, double durationS) {
    Circuit c;
    const NodeId n = c.addNode();
    c.add<Capacitor>("C", n, kGround, CapacitorParams{t.cF, t.leakOhm, false, 63.0, 0.0, 0.2}).setVoltage(t.u0V);
    c.add<Inductor>("L", n, kGround, InductorParams{t.lH, t.rOhm, t.rpOhm, 1.0});
    c.setSecondOrder(secondOrder);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    REQUIRE(sim.step(dtS) == Status::Ok);
    // Referenz startet beim ersten gerechneten Zustand
    double v = sim.probe().readings[0].voltV;
    double i = static_cast<const Inductor&>(sim.circuit().component(1)).currentA();
    const auto deriv = [&t](double vv, double ii, double& dv, double& di) {
        dv = (-ii - vv / t.rpOhm - vv / t.leakOhm - vv * kGmin) / t.cF;
        di = (vv - t.rOhm * ii) / t.lH;
    };
    double worst = 0.0, peak = 0.0;
    const int steps = static_cast<int>(durationS / dtS);
    constexpr int kSub = 200;
    const double h = dtS / kSub;
    for (int s = 1; s < steps; ++s) {
        for (int k = 0; k < kSub; ++k) {
            double dv1, di1, dv2, di2, dv3, di3, dv4, di4;
            deriv(v, i, dv1, di1);
            deriv(v + 0.5 * h * dv1, i + 0.5 * h * di1, dv2, di2);
            deriv(v + 0.5 * h * dv2, i + 0.5 * h * di2, dv3, di3);
            deriv(v + h * dv3, i + h * di3, dv4, di4);
            v += h / 6.0 * (dv1 + 2 * dv2 + 2 * dv3 + dv4);
            i += h / 6.0 * (di1 + 2 * di2 + 2 * di3 + di4);
        }
        REQUIRE(sim.step(dtS) == Status::Ok);
        worst = std::max(worst, std::fabs(sim.probe().readings[0].voltV - v));
        peak = std::max(peak, std::fabs(v));
    }
    return worst / peak;
}

} // namespace

TEST_CASE("BDF2: freier Schwingkreis 160 Hz klingt ab wie die Referenz", "[integration][volta3]") {
    const Tank t; // 1 H, 100 Ohm, 1 uF: 159 Hz, Guete 10
    const double be = maxError(t, false, 20e-6, 0.03);
    const double bdf2 = maxError(t, true, 20e-6, 0.03);
    INFO("Rueckwaerts-Euler " << be << ", BDF2 " << bdf2);
    CHECK(bdf2 < 0.005);
    CHECK(be > 5.0 * bdf2); // das alte Verfahren daempft sichtbar mehr
}

TEST_CASE("BDF2: Schwingkreis 500 Hz mit 20 us bleibt unter 2 %", "[integration][volta3]") {
    Tank t; // Spule 100 mH (30 Ohm) und 1 uF: 503 Hz, Guete 10, 100 Schritte je Periode
    t.lH = 0.1;
    t.rOhm = 30.0;
    const double bdf2 = maxError(t, true, 20e-6, 0.01);
    INFO("BDF2 " << bdf2);
    CHECK(bdf2 < 0.02);
}

TEST_CASE("BDF2: zwei verbundene Kondensatoren behalten ihre Ladung", "[integration][volta3]") {
    Circuit c;
    const NodeId a = c.addNode();
    const NodeId b = c.addNode();
    c.add<Capacitor>("C1", a, kGround, CapacitorParams{1e-6, 1e15, false, 63.0, 0.0, 0.2}).setVoltage(4.0);
    c.add<Capacitor>("C2", b, kGround, CapacitorParams{3e-6, 1e15, false, 63.0, 0.0, 0.2});
    c.add<Resistor>("R", a, b, ResistorParams{100.0});
    c.setSecondOrder(true);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    for (int s = 0; s < 500; ++s) {
        REQUIRE(sim.step(s % 7 == 3 ? 7e-6 : 20e-6) == Status::Ok); // auch bei wechselnder Schrittweite
        const double q = 1e-6 * sim.probe().readings[0].voltV + 3e-6 * sim.probe().readings[1].voltV;
        REQUIRE(q == Catch::Approx(4e-6).epsilon(1e-4));
    }
    CHECK(sim.probe().readings[0].voltV == Catch::Approx(1.0).epsilon(1e-4));
}
