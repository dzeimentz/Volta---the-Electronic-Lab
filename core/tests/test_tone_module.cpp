// Tonmodul als Verhaltensmodell: Eingang mit Hysterese, Melodie in Schleife, Endstufe als Mittelwert, Kirchhoff.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <set>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/speaker.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/modules/tone_module.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
// Batterie, Tonmodul (plus/minus an der Batterie), Schalter von plus nach ein, Lautsprecher von plus nach aus.
struct ToneBench {
    Simulator sim;
    Switch* sw = nullptr;
    ToneModule* tm = nullptr;
    std::size_t tmIndex = 0;
    std::size_t lsIndex = 0;
    explicit ToneBench(int cells = 6) {
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId in = c.addNode();
        const NodeId out = c.addNode();
        BatteryParams bp;
        bp.cells = cells;
        c.add<Battery>("B1", plus, kGround, bp);
        c.add<Switch>("S1", plus, in, false);
        const SubcircuitRange r = addToneModule(c, "TM1", {plus, kGround, in, out});
        REQUIRE(r.count == 1);
        tmIndex = r.first;
        lsIndex = c.componentCount();
        c.add<Speaker>("LS", plus, out);
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        sw = &static_cast<Switch&>(sim.circuit().component(1));
        tm = &static_cast<ToneModule&>(sim.circuit().component(tmIndex));
    }
    void run(double seconds) {
        const int steps = static_cast<int>(std::lround(seconds / kDefaultStepS));
        for (int k = 0; k < steps; ++k) {
            REQUIRE(sim.step() == Status::Ok);
            if (k % 13 == 0) requireKirchhoff(sim);
        }
    }
    const ComponentReading& reading() const { return sim.probe().readings[tmIndex]; }
};
} // namespace

TEST_CASE("Tonmodul schweigt ohne Eingang und zieht nur Ruhestrom", "[tonmodul]") {
    ToneBench b;
    b.run(0.5);
    const ComponentReading& r = b.reading();
    CHECK_FALSE(b.tm->playing());
    CHECK(r.frequencyHz == 0.0);
    CHECK(r.level == 0.0);
    CHECK(r.displayValue == -1.0);
    CHECK(std::fabs(r.currentA) < 1e-6); // Endstufe gesperrt
    // Ruhestrom: ca. 9 V / 4,7 kOhm
    const double vSupply = r.voltV;
    CHECK(nearA(r.terminalCurrentA[0], vSupply / 4700.0));
}

TEST_CASE("Tonmodul spielt die Melodie, solange der Eingang aktiv ist", "[tonmodul]") {
    ToneBench b;
    b.run(0.1);
    b.sw->setClosed(true);
    b.run(0.05);
    REQUIRE(b.tm->playing());
    const ComponentReading& r = b.reading();
    CHECK(r.frequencyHz == Approx(ToneModule::kMelodyHz[0]));
    CHECK(r.level > 0.9);
    // Endstufe als Mittelwert: I = U / (R_LS + R_aus / Tastverhaeltnis) = U / (8 + 200)
    const double vSupply = r.voltV;
    CHECK(near(r.currentA, vSupply / 208.0, 1e-5));
    CHECK(near(b.sim.probe().readings[b.lsIndex].currentA, r.currentA, 1e-5)); // derselbe Strom

    // Melodie: alle Toene einer Schleife werden durchlaufen, die Pause eingeschlossen
    std::set<int> seen;
    for (int k = 0; k < 1700; ++k) {
        REQUIRE(b.sim.step() == Status::Ok);
        seen.insert(static_cast<int>(b.reading().displayValue));
    }
    CHECK(seen.size() == ToneModule::kNotes);

    // Eingang aus: still, Endstufe gesperrt
    b.sw->setClosed(false);
    b.run(0.05);
    CHECK_FALSE(b.tm->playing());
    CHECK(b.reading().frequencyHz == 0.0);
    CHECK(std::fabs(b.reading().currentA) < 1e-6);
}

TEST_CASE("Tonmodul braucht mindestens 3 V Versorgung", "[tonmodul]") {
    ToneBench low(1); // eine Zelle: 1,5 V
    low.sw->setClosed(true);
    low.run(0.2);
    CHECK_FALSE(low.tm->playing());
    ToneBench mid(3); // 4,5 V: spielt, leiser
    mid.sw->setClosed(true);
    mid.run(0.2);
    CHECK(mid.tm->playing());
    CHECK(mid.reading().level == Approx(mid.reading().voltV / 9.0));
}

TEST_CASE("Tonmodul: Eingang mit Hysterese ueber einen Spannungsteiler", "[tonmodul]") {
    // ein liegt an einem Teiler aus R_oben (plus -> ein) und dem Eingangswiderstand 100 kOhm
    auto playsWith = [](double rTop) {
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId in = c.addNode();
        const NodeId out = c.addNode();
        c.add<Battery>("B1", plus, kGround);
        c.add<Resistor>("RO", plus, in, ResistorParams{rTop});
        addToneModule(c, "TM1", {plus, kGround, in, out});
        c.add<Resistor>("RL", plus, out, ResistorParams{1000.0});
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        for (int k = 0; k < 50; ++k) REQUIRE(sim.step() == Status::Ok);
        requireKirchhoff(sim);
        return static_cast<const ToneModule&>(sim.circuit().component(2)).playing();
    };
    CHECK(playsWith(100000.0));   // ca. 4,5 V am Eingang
    CHECK_FALSE(playsWith(1e6));  // ca. 0,8 V: unter der Schwelle
}
