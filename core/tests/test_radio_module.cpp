// MW-Modul (Volta 3) als Teilschaltung: rechnet genau wie derselbe Empfaenger aus Einzelteilen (Experiment 2.5) und
// treibt abgestimmt einen Lautsprecher mit einigen Milliampere Tonstrom.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/inductor.hpp"
#include "volta/core/components/radio.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/transistor.hpp"
#include "volta/core/modules/radio_module.hpp"
#include "volta/core/simulator.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {

// Empfaenger mit Batterie, Antenne, Drehkondensator (dkF) und 8-Ohm-Last als Lautsprecher; modul = MW-Modul,
// sonst dieselben Bauteile einzeln. speaker = Index der Last.
Circuit receiver(bool module, double dkF, std::size_t& speaker) {
    Circuit c;
    const NodeId plus = c.addNode(), ant = c.addNode(), tank = c.addNode(), out = c.addNode();
    c.add<Battery>("B", plus, kGround);
    c.add<AntennaFeed>("ANT", ant, kGround);
    c.add<Capacitor>("DK", tank, kGround, CapacitorParams::film(dkF));
    speaker = c.componentCount();
    c.add<Resistor>("LS", plus, out, ResistorParams{8.0});
    if (module) {
        addMwModule(c, "MW1", {plus, kGround, ant, tank, out});
    } else {
        const NodeId det = c.addNode(), base = c.addNode();
        c.add<Capacitor>("C1", ant, tank, CapacitorParams::film(10e-12));
        c.add<Inductor>("SP1", tank, kGround, InductorParams::uH220());
        c.add<DetectorDiode>("D1", tank, det, DiodeParams::germanium());
        c.add<Resistor>("R1", det, kGround, ResistorParams{22000.0});
        c.add<Capacitor>("C2", base, det, CapacitorParams::electrolytic(10e-6));
        c.add<Resistor>("R2", plus, base, ResistorParams{150000.0});
        c.add<Npn>("T1", base, out, kGround);
    }
    return c;
}

struct Run {
    double meanA = 0.0, ppA = 0.0;
};

Run listen(bool module, double dkF) {
    const RadioStation st{1e6, 1.0, 0.5, StationProgram::Tone, 1000.0, true};
    std::size_t ls = 0;
    Simulator sim;
    REQUIRE(sim.setCircuit(receiver(module, dkF, ls)) == Status::Ok);
    REQUIRE(tuneRadio(sim, {st}) == Status::Ok);
    double lo = 1e9, hi = -1e9, sum = 0.0;
    int n = 0;
    for (int i = 0; i < 15000; ++i) { // 20 us, 0,3 s; ausgewertet ab 0,2 s (Elko eingeschwungen)
        REQUIRE(sim.step(20e-6) == Status::Ok);
        if (i < 10000) continue;
        const double a = sim.probe().readings[ls].currentA;
        lo = std::min(lo, a);
        hi = std::max(hi, a);
        sum += a;
        ++n;
    }
    requireKirchhoff(sim);
    return {sum / n, hi - lo};
}

} // namespace

TEST_CASE("MW-Modul rechnet wie der Empfaenger aus Einzelteilen", "[radio][modul][volta3]") {
    // 1 MHz mit 220 uH: 115 pF im Kreis, davon etwa 10 pF ueber die Antennenkopplung
    const double tuned = 105e-12;
    const Run mod = listen(true, tuned), ref = listen(false, tuned);
    CHECK(mod.meanA == Approx(ref.meanA).epsilon(1e-9));
    CHECK(mod.ppA == Approx(ref.ppA).epsilon(1e-9));
    // Arbeitspunkt: (9 V - 0,65 V) / 150 kOhm * 200 = 11 mA; Tonstrom einige mA
    CHECK(mod.meanA > 8e-3);
    CHECK(mod.meanA < 14e-3);
    CHECK(mod.ppA > 2e-3);
    // verstimmt (300 pF, etwa 560 kHz; Guete um 8): deutlich leiser
    const Run off = listen(true, 300e-12);
    CHECK(off.ppA < 0.25 * mod.ppA);
}
