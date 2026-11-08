// UKW-Modul (Volta 3) als Verhaltensmodell: Abstimmspannung waehlt die Frequenz, abgestimmt kommt das Programm
// des Senders, zwischen den Sendern nur leises Rauschen; Lautstaerke und Antenne wirken, Kirchhoff gilt.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/modules/fm_module.hpp"
#include "volta/core/simulator.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {

struct Run {
    double mhz = 0.0, quality = 0.0, ppA = 0.0, meanA = 0.0;
    int station = -1;
};

// 9-V-Batterie, Spannungsteiler (10 kOhm gesamt) fuer abst und laut, 8-Ohm-Last zwischen plus und aus.
Run listen(double tune, double volume, bool antenna, unsigned stationsOn = 0b111u) {
    Circuit c;
    const NodeId plus = c.addNode(), ant = c.addNode(), ab = c.addNode(), la = c.addNode(), out = c.addNode();
    c.add<Battery>("B", plus, kGround);
    auto divider = [&](const char* a, const char* b, NodeId mid, double k) {
        k = std::clamp(k, 1e-6, 1.0 - 1e-6);
        c.add<Resistor>(a, plus, mid, ResistorParams{10000.0 * (1.0 - k)});
        c.add<Resistor>(b, mid, kGround, ResistorParams{10000.0 * k});
    };
    divider("P1a", "P1b", ab, tune);
    divider("P2a", "P2b", la, volume);
    const std::size_t ls = c.componentCount();
    c.add<Resistor>("LS", plus, out, ResistorParams{8.0});
    const std::size_t fm = c.componentCount();
    addFmModule(c, "UKW1", {plus, kGround, ant, ab, la, out}, antenna, stationsOn);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    double lo = 1e9, hi = -1e9, sum = 0.0;
    int n = 0;
    for (int i = 0; i < 30000; ++i) { // 20 us, 0,6 s (mehrere Toene der Melodie)
        REQUIRE(sim.step(20e-6) == Status::Ok);
        const double a = sim.probe().readings[ls].currentA;
        lo = std::min(lo, a);
        hi = std::max(hi, a);
        sum += a;
        ++n;
    }
    requireKirchhoff(sim);
    const ComponentReading& r = sim.probe().readings[fm];
    Run run;
    run.mhz = r.displayValue;
    run.quality = r.level;
    run.station = r.displayFlags - 1;
    run.ppA = hi - lo;
    run.meanA = sum / n;
    return run;
}

double tuneFor(double mhz) { return (mhz - 87.5) / 20.5; }

} // namespace

TEST_CASE("UKW-Modul empfaengt abgestimmt den Sender, dazwischen rauscht es", "[radio][modul][volta3]") {
    // Teiler 10 kOhm an 100 kOhm Eingang: die Spannung sinkt um bis zu 2,5 %; die Abstimmung liest sie ehrlich ab
    const Run on = listen(tuneFor(96.8), 0.7, true);
    CHECK(on.mhz > 96.3);
    CHECK(on.mhz < 97.0);
    REQUIRE(on.station == 1);
    CHECK(on.quality > 0.0);
    // voller Empfang genau auf dem Sender: der Teiler wird so nachgestellt, dass die Anzeige 96,8 MHz zeigt
    double k = tuneFor(96.8);
    Run exact = on;
    for (int i = 0; i < 4; ++i) {
        k += (96.8 - exact.mhz) / 20.5;
        exact = listen(k, 0.7, true);
    }
    CHECK(exact.mhz == Approx(96.8).margin(0.02));
    CHECK(exact.quality == Approx(1.0));
    CHECK(exact.ppA > 30e-3);

    // zwischen 96,8 und 104,5 MHz: kein Sender, nur Rauschen mit weniger als der halben Aussteuerung
    const Run between = listen(tuneFor(100.6), 0.7, true);
    CHECK(between.station == -1);
    CHECK(between.quality == 0.0);
    CHECK(between.ppA > 1e-3);
    CHECK(between.ppA < 0.5 * exact.ppA);

    // ohne Antenne nur Rauschen, Lautstaerke 0 schweigt
    const Run noAnt = listen(k, 0.7, false);
    CHECK(noAnt.quality == 0.0);
    CHECK(noAnt.ppA < 0.5 * exact.ppA);
    const Run silent = listen(k, 0.0, true);
    CHECK(silent.ppA < 1e-4);
    CHECK(silent.meanA < 1e-4);

    // Sender 2 im Senderfenster ausgeschaltet: auf 96,8 MHz nur Rauschen, die anderen Sender bleiben
    const Run off = listen(k, 0.7, true, 0b101u);
    CHECK(off.station == -1);
    CHECK(off.quality == 0.0);
    CHECK(off.ppA < 0.5 * exact.ppA);
    const Run other = listen(tuneFor(104.5), 0.7, true, 0b101u);
    CHECK(other.station == 2);
}
