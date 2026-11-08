// Wechselquelle und Funktionsgenerator (ab Volta 2): Spitzen- und Effektivwert, Signalformen, Phase, Ueberlast.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/sources.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

TEST_CASE("Wechselquelle 9 V~ an 1 kOhm: Spitze und Effektivwert", "[referenz][transient][quelle]") {
    Circuit c;
    const NodeId a = c.addNode();
    auto& src = c.add<AcSource>("W1", a, kGround);
    c.add<Resistor>("R1", a, kGround, ResistorParams{1000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(src.params().frequencyHz == 50.0); // fest 50 Hz (entschieden 01.11.2026)
    // Handrechnung: Spitze 9 * sqrt(2) = 12,728 V; Teiler 1000 / 1002 Ohm
    const double dt = 1e-4; // 200 Punkte je Periode
    double peak = 0.0, sumSq = 0.0;
    const int steps = 2000; // 0,2 s = 10 Perioden
    for (int k = 0; k < steps; ++k) {
        REQUIRE(sim.step(dt) == Status::Ok);
        const double v = sim.probe().voltage(a);
        peak = std::max(peak, std::fabs(v));
        sumSq += v * v;
    }
    CHECK(peak == Approx(12.728 * 1000.0 / 1002.0).epsilon(1e-3));
    CHECK(std::sqrt(sumSq / steps) == Approx(9.0 * 1000.0 / 1002.0).epsilon(1e-3));
    CHECK(src.phase() == Approx(0.0).margin(1e-6)); // ganze Perioden
    CHECK(!src.isOverloaded());
    requireKirchhoff(sim);
}

TEST_CASE("Wechselquelle: 6 V~ und Ueberlast bei Kurzschluss", "[quelle]") {
    Circuit c;
    const NodeId a = c.addNode();
    auto& src = c.add<AcSource>("W1", a, kGround);
    src.setRms(6.0);
    CHECK(src.peakV() == Approx(8.485).epsilon(1e-3));
    c.add<Resistor>("R1", a, kGround, ResistorParams{0.1});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    for (int k = 0; k < 1000; ++k) REQUIRE(sim.step(1e-4) == Status::Ok);
    // I_eff = 6 / 2,1 = 2,86 A >> 0,33 A; der gleitende Effektivwert hat nach 0,1 s (tau 0,2 s) etwa 63 % erreicht
    CHECK(src.isOverloaded());
    CHECK(src.rmsCurrentA() > 1.5);
    CHECK(!src.isDestroyed());
    requireFinite(sim.probe());
}

TEST_CASE("Funktionsgenerator: Signalformen bei der Phase", "[quelle]") {
    FunctionGenerator g("G1", 1, 0, {Waveform::Sine, 100.0, 2.0, 1.0, 50.0});
    CHECK(g.emfAt(0.0) == Approx(1.0));
    CHECK(g.emfAt(0.25) == Approx(3.0));
    CHECK(g.emfAt(0.75) == Approx(-1.0));
    g.setWaveform(Waveform::Triangle);
    CHECK(g.emfAt(0.125) == Approx(2.0)); // halbe Amplitude, steigend
    CHECK(g.emfAt(0.25) == Approx(3.0));
    CHECK(g.emfAt(0.5) == Approx(1.0).margin(1e-12));
    CHECK(g.emfAt(0.75) == Approx(-1.0));
    g.setWaveform(Waveform::Square);
    CHECK(g.emfAt(0.1) == Approx(3.0));
    CHECK(g.emfAt(0.6) == Approx(-1.0));
    // Grenzen des Geraets
    g.setFrequency(1e6);
    CHECK(g.params().frequencyHz == 10000.0);
    g.setFrequency(0.0);
    CHECK(g.params().frequencyHz == 1.0);
    g.setAmplitude(9.0);
    CHECK(g.params().amplitudeV == 4.0);
    g.setOffset(-7.0);
    CHECK(g.params().offsetV == -4.0);
}

TEST_CASE("Funktionsgenerator 1 kHz an 10 kOhm: Spitzen und Mittelwert", "[referenz][transient][quelle]") {
    Circuit c;
    const NodeId out = c.addNode();
    auto& gen = c.add<FunctionGenerator>("G1", out, kGround, GeneratorParams{Waveform::Sine, 1000.0, 2.0, 1.0, 50.0});
    c.add<Resistor>("R1", out, kGround, ResistorParams{10000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const double k = 10000.0 / 10050.0; // Ausgangswiderstand 50 Ohm
    double hi = -1e9, lo = 1e9, sum = 0.0;
    const int steps = 1000; // 10 us: 100 Punkte je Periode (Spitzen werden genau getroffen), 10 Perioden
    for (int i = 0; i < steps; ++i) {
        REQUIRE(sim.step(1e-5) == Status::Ok);
        const double v = sim.probe().voltage(out);
        hi = std::max(hi, v);
        lo = std::min(lo, v);
        sum += v;
    }
    CHECK(hi == Approx(3.0 * k).epsilon(1e-3));
    CHECK(lo == Approx(-1.0 * k).epsilon(1e-3));
    CHECK(sum / steps == Approx(1.0 * k).epsilon(1e-3));
    CHECK(sim.probe().readings[0].frequencyHz == 1000.0);
    requireKirchhoff(sim);
    // Frequenzwechsel ohne Sprung: die Phase laeuft weiter
    const double before = gen.emfAt(gen.phase());
    gen.setFrequency(250.0);
    CHECK(gen.emfAt(gen.phase()) == Approx(before));
}

TEST_CASE("Funktionsgenerator aus: Ausgang 0 V, Phase laeuft weiter", "[transient][quelle]") {
    Circuit c;
    const NodeId out = c.addNode();
    auto& gen = c.add<FunctionGenerator>("G1", out, kGround, GeneratorParams{Waveform::Sine, 100.0, 2.0, 1.0, 50.0});
    c.add<Resistor>("R1", out, kGround, ResistorParams{1000.0});
    gen.setEnabled(false);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    for (int i = 0; i < 100; ++i) {
        REQUIRE(sim.step(1e-4) == Status::Ok);
        CHECK(std::fabs(sim.probe().voltage(out)) < 1e-9);
    }
    CHECK(gen.phase() == Approx(0.0).margin(1e-6)); // 100 Schritte zu 0,1 ms = eine Periode
    gen.setEnabled(true);
    REQUIRE(sim.step(1e-4) == Status::Ok);
    CHECK(std::fabs(sim.probe().voltage(out)) > 0.5); // Gleichanteil 1 V plus Sinus
    requireKirchhoff(sim);
}

TEST_CASE("Kennlinienschreiber: Dreieck, Grenzen", "[quelle]") {
    CurveTracer t("KS", 1, 0, {-10.0, 10.0, 500.0, 2.0});
    CHECK(t.emfAt(0.0) == Approx(-10.0));
    CHECK(t.emfAt(0.25) == Approx(0.0).margin(1e-12));
    CHECK(t.emfAt(0.5) == Approx(10.0));
    CHECK(t.emfAt(0.75) == Approx(0.0).margin(1e-12));
    t.setRange(30.0, -1.0); // vertauscht und zu gross
    CHECK(t.params().minV == -1.0);
    CHECK(t.params().maxV == 20.0);
    t.setSeriesOhm(0.0);
    CHECK(t.params().seriesOhm == 10.0);
}

TEST_CASE("Kennlinienschreiber an 1 kOhm: Gerade durch 0 mit Steigung 1/R", "[referenz][transient][quelle]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<CurveTracer>("KS", plus, kGround, TracerParams{-10.0, 10.0, 500.0, 2.0});
    c.add<Resistor>("R1", plus, kGround, ResistorParams{1000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    double lo = 1e9, hi = -1e9;
    for (int i = 0; i < 2000; ++i) { // 1 ms: ein ganzer Durchlauf
        REQUIRE(sim.step(1e-3) == Status::Ok);
        const auto& r = sim.probe().readings[0];
        const double u = r.voltV, current = -r.currentA;
        CHECK(current == Approx(u / 1000.0).margin(1e-9)); // Klemmenmessung: I = U / R
        lo = std::min(lo, u);
        hi = std::max(hi, u);
    }
    // Handrechnung: Teiler 1000 / 1500 Ohm -> +-6,667 V am Widerstand
    CHECK(hi == Approx(10.0 * 1000.0 / 1500.0).epsilon(2e-3));
    CHECK(lo == Approx(-10.0 * 1000.0 / 1500.0).epsilon(2e-3));
    requireKirchhoff(sim);
}

TEST_CASE("Funktionsgenerator: Rechteck schaltet auf die Schrittgrenze", "[transient][quelle]") {
    Circuit c;
    const NodeId out = c.addNode();
    c.add<FunctionGenerator>("G1", out, kGround, GeneratorParams{Waveform::Square, 100.0, 4.0, 0.0, 50.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    int highs = 0;
    for (int i = 0; i < 1000; ++i) { // 0,1 s = 10 Perioden zu je 100 Schritten
        REQUIRE(sim.step(1e-4) == Status::Ok);
        const double v = sim.probe().voltage(out);
        CHECK(std::fabs(std::fabs(v) - 4.0) < 1e-6); // im Leerlauf genau +-4 V
        if (v > 0.0) ++highs;
    }
    CHECK(highs == 500); // Tastverhaeltnis 50 %
}

TEST_CASE("Funktionsgenerator mit Amplitudenmodulation: Huellkurve folgt dem Ton", "[referenz][transient][quelle]") {
    // Traeger 2 kHz, 1 V Spitze, Ton 100 Hz, Tiefe 50 %: Huellkurve schwankt zwischen 0,5 und 1,5 V
    GeneratorParams p{Waveform::Sine, 2000.0, 1.0, 0.0, 50.0};
    p.amDepth = 0.5;
    p.amToneHz = 100.0;
    {
        FunctionGenerator g("G1", 1, 0, p);
        CHECK(g.emfAt(0.25, 0.25) == Approx(1.5));
        CHECK(g.emfAt(0.25, 0.75) == Approx(0.5));
        CHECK(g.emfAt(0.25, 0.0) == Approx(1.0));
        g.setAmDepth(2.0);
        CHECK(g.params().amDepth == 1.0);
        g.setAmTone(5.0);
        CHECK(g.params().amToneHz == 20.0);
        g.setAmDepth(0.0); // aus: wie ohne Modulation
        CHECK(g.emfAt(0.25, 0.75) == 1.0);
    }
    Circuit c;
    const NodeId out = c.addNode();
    c.add<FunctionGenerator>("G1", out, kGround, p);
    c.add<Resistor>("R1", out, kGround, ResistorParams{10000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const double k = 10000.0 / 10050.0;
    // je Tonperiode (10 ms) die groesste Spitze in den beiden Haelften: oben 1,5 V, unten (zweite Haelfte) 0,5 V
    double hiFirst = 0.0, hiSecond = 0.0;
    for (int i = 0; i < 10000; ++i) { // 2 us, 20 ms = 2 Tonperioden; die zweite wird ausgewertet
        REQUIRE(sim.step(2e-6) == Status::Ok);
        if (i < 5000) continue;
        const double v = std::fabs(sim.probe().voltage(out));
        const double tone = std::fmod(i * 2e-6, 0.01);
        if (std::fabs(tone - 0.0025) < 0.00025) hiFirst = std::max(hiFirst, v);  // um das Tonmaximum
        if (std::fabs(tone - 0.0075) < 0.00025) hiSecond = std::max(hiSecond, v); // um das Tonminimum
    }
    CHECK(hiFirst == Approx(1.5 * k).epsilon(5e-3));
    CHECK(hiSecond == Approx(0.5 * k).epsilon(5e-3));
    requireKirchhoff(sim);
}
