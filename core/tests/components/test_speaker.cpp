// Lautsprecher des Bedienpults: ohmsch, Klanganzeige aus dem Stromverlauf (Tonhoehe, Signalstaerke, Impulse).
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/speaker.hpp"
#include "volta/core/components/switch.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
// Batterie -> Schalter -> Vorwiderstand -> Lautsprecher -> Masse; der Schalter wird im Takt bedient.
struct SpeakerBench {
    Simulator sim;
    Switch* sw = nullptr;
    Speaker* ls = nullptr;
    explicit SpeakerBench(double seriesOhm = 100.0, bool closed = false) {
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId a = c.addNode();
        const NodeId b = c.addNode();
        c.add<Battery>("B1", plus, kGround);
        c.add<Switch>("S1", plus, a, closed);
        c.add<Resistor>("R1", a, b, ResistorParams{seriesOhm, 5.0});
        c.add<Speaker>("LS", b, kGround);
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        sw = &static_cast<Switch&>(sim.circuit().component(1));
        ls = &static_cast<Speaker&>(sim.circuit().component(3));
    }
    // halfSteps Schritte ein, halfSteps Schritte aus, ueber seconds
    void square(int halfSteps, double seconds) {
        const int steps = static_cast<int>(std::lround(seconds / kDefaultStepS));
        for (int k = 0; k < steps; ++k) {
            sw->setClosed((k / halfSteps) % 2 == 0);
            REQUIRE(sim.step() == Status::Ok);
        }
    }
    void run(double seconds) {
        const int steps = static_cast<int>(std::lround(seconds / kDefaultStepS));
        for (int k = 0; k < steps; ++k) REQUIRE(sim.step() == Status::Ok);
    }
    const ComponentReading& reading() const { return sim.probe().readings[3]; }
};
} // namespace

TEST_CASE("Lautsprecher: ohmsch 8 Ohm, Gleichstrom bleibt stumm", "[bauteil][lautsprecher]") {
    SpeakerBench b(100.0, true);
    b.run(2.0);
    const double i = 9.0 / (100.0 + 8.0 + 1.5 + kContactClosedOhm); // Batterie-Innenwiderstand 1,5 Ohm
    CHECK(b.reading().currentA == Approx(i).epsilon(1e-6));
    CHECK(b.reading().frequencyHz == 0.0);
    CHECK(b.reading().level < 0.01);
    requireKirchhoff(b.sim);
}

TEST_CASE("Lautsprecher: Rechteck 50 Hz wird als 50 Hz erkannt", "[bauteil][lautsprecher][referenz]") {
    SpeakerBench b;
    b.square(10, 2.0); // 10 ms ein, 10 ms aus
    CHECK(b.reading().frequencyHz == Approx(50.0).epsilon(0.02));
    CHECK(b.reading().level > 0.1);
    CHECK(b.ls->pulses() == Approx(100.0).margin(3.0));
    requireKirchhoff(b.sim);
}

TEST_CASE("Lautsprecher: Impulse unter 20 Hz werden einzeln gezaehlt", "[bauteil][lautsprecher]") {
    SpeakerBench b;
    b.square(250, 3.0); // 2 Hz
    CHECK(b.reading().frequencyHz == Approx(2.0).epsilon(0.02));
    CHECK(b.ls->pulses() == Approx(6.0).margin(1.0));
}

TEST_CASE("Lautsprecher: nach dem Signal wieder Stille", "[bauteil][lautsprecher]") {
    SpeakerBench b;
    b.square(10, 1.0);
    REQUIRE(b.reading().frequencyHz > 40.0);
    b.sw->setClosed(true);
    b.run(2.0);
    CHECK(b.reading().frequencyHz == 0.0);
    CHECK(b.reading().level < 0.05);
}

TEST_CASE("Lautsprecher: direkt an 9 V ueberlastet und brennt durch", "[bauteil][lautsprecher][thermik]") {
    SpeakerBench b(0.0001, true); // praktisch ohne Vorwiderstand: ca. 7 W
    b.run(0.1);
    CHECK(b.reading().overloaded);
    b.run(10.0);
    CHECK(b.reading().destroyed);
    CHECK(std::fabs(b.reading().currentA) < 1e-6); // danach offen
}
