// Frequenzgang-Schreiber (Volta 3): Auswertung der Wechselstromanalyse gegen die Formeln.
#include <cmath>
#include <vector>

#include "test_support.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/inductor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/sources.hpp"
#include "volta/core/frequency_response.hpp"
#include "volta/core/simulator.hpp"

using namespace volta::core;

namespace {
constexpr double kPi = 3.14159265358979323846;
}

TEST_CASE("Frequenzgang: logarithmische Stufen von 1 Hz bis 10 MHz", "[bode]") {
    const auto f = logFrequencies(1.0, 1e7, 20);
    REQUIRE(f.size() == 141);
    CHECK(f.front() == 1.0);
    CHECK(f.back() == 1e7);
    CHECK(f[20] == Catch::Approx(10.0).epsilon(1e-12));
}

TEST_CASE("Frequenzgang: RC-Tiefpass zeigt die Grenzfrequenz 1/(2 pi R C)", "[bode]") {
    // 10 kOhm (plus 50 Ohm Generator) und 10 nF: fg = 1583,6 Hz
    Circuit c;
    const NodeId g = c.addNode(), out = c.addNode();
    c.add<FunctionGenerator>("G", g, kGround);
    c.add<Resistor>("R", g, out, ResistorParams{10e3});
    c.add<Capacitor>("C", out, kGround, CapacitorParams::film(10e-9));
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const auto f = logFrequencies(1.0, 1e7, 20);
    std::vector<Complex> h;
    REQUIRE(sim.acSweep(0, f, out, kGround, h) == Status::Ok);
    const FrequencyResponse r = analyzeResponse(f, h);
    const double fg = 1.0 / (2.0 * kPi * 10050.0 * 10e-9);
    CHECK(r.kind == FrequencyResponse::Kind::LowPass);
    CHECK(r.cutHighHz == Catch::Approx(fg).epsilon(0.01));
    CHECK(r.cutLowHz == 0.0);
    CHECK(r.resonanceHz == 0.0);
    CHECK(r.maxDb == Catch::Approx(0.0).margin(0.01));
    CHECK(r.phaseDeg.back() == Catch::Approx(-90.0).margin(1.0));
}

TEST_CASE("Frequenzgang: CR-Hochpass zeigt die untere Grenzfrequenz", "[bode]") {
    Circuit c;
    const NodeId g = c.addNode(), out = c.addNode();
    c.add<FunctionGenerator>("G", g, kGround);
    c.add<Capacitor>("C", g, out, CapacitorParams::film(100e-9));
    c.add<Resistor>("R", out, kGround, ResistorParams{1e3});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const auto f = logFrequencies(1.0, 1e7, 20);
    std::vector<Complex> h;
    REQUIRE(sim.acSweep(0, f, out, kGround, h) == Status::Ok);
    const FrequencyResponse r = analyzeResponse(f, h);
    CHECK(r.kind == FrequencyResponse::Kind::HighPass);
    CHECK(r.cutLowHz == Catch::Approx(1.0 / (2.0 * kPi * 1050.0 * 100e-9)).epsilon(0.01));
}

TEST_CASE("Frequenzgang: Parallelschwingkreis zeigt Resonanz, Bandbreite und Guete", "[bode]") {
    // 10 kOhm vor L || C (1 H, 1 uF): f0 = 159,15 Hz; B = 1 / (2 pi R C) mit R = 10 kOhm || Verluste
    Circuit c;
    const NodeId g = c.addNode(), t = c.addNode();
    c.add<FunctionGenerator>("G", g, kGround);
    c.add<Resistor>("R", g, t, ResistorParams{10e3});
    c.add<Inductor>("L", t, kGround, InductorParams{1.0, 1e-3, 1e9, 0.1});
    c.add<Capacitor>("C", t, kGround, CapacitorParams::film(1e-6));
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const auto f = logFrequencies(1.0, 1e7, 100);
    std::vector<Complex> h;
    REQUIRE(sim.acSweep(0, f, t, kGround, h) == Status::Ok);
    const FrequencyResponse r = analyzeResponse(f, h);
    const double f0 = 1.0 / (2.0 * kPi * std::sqrt(1.0 * 1e-6));
    const double b = 1.0 / (2.0 * kPi * 10050.0 * 1e-6);
    CHECK(r.kind == FrequencyResponse::Kind::BandPass);
    CHECK(r.resonanceHz == Catch::Approx(f0).epsilon(0.01));
    CHECK(r.bandwidthHz == Catch::Approx(b).epsilon(0.02));
    CHECK(r.quality == Catch::Approx(f0 / b).epsilon(0.02));
    CHECK(std::sqrt(r.cutLowHz * r.cutHighHz) == Catch::Approx(f0).epsilon(0.01));
}

TEST_CASE("Frequenzgang: ohne Uebertragung keine Marken", "[bode]") {
    const std::vector<double> f{1.0, 10.0, 100.0};
    const std::vector<Complex> h(3, Complex{});
    const FrequencyResponse r = analyzeResponse(f, h);
    CHECK(r.kind == FrequencyResponse::Kind::None);
    CHECK(r.db.front() == kFloorDb);
}

TEST_CASE("Frequenzgang: der feine Durchlauf trifft auch einen Kreis hoher Guete", "[bode]") {
    // 1 MOhm vor L || C (100 uH, 253,3 pF): f0 = 1 MHz, Guete ca. 1590 ohne Verluste der Spule; mit 2 Ohm Wicklung
    // und 1 MOhm parallel deutlich kleiner - die Marke muss trotzdem auf 1 % stimmen
    Circuit c;
    const NodeId g = c.addNode(), t = c.addNode();
    c.add<FunctionGenerator>("G", g, kGround);
    c.add<Resistor>("R", g, t, ResistorParams{1e6});
    c.add<Inductor>("L", t, kGround, InductorParams{100e-6, 2.0, 1e6, 0.1});
    c.add<Capacitor>("C", t, kGround, CapacitorParams::film(253.3e-12));
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    std::vector<double> f;
    std::vector<Complex> h;
    FrequencyResponse r;
    REQUIRE(sweepResponse(sim, 0, t, kGround, 1.0, 1e7, 20, f, h, r) == Status::Ok);
    const double f0 = 1.0 / (2.0 * kPi * std::sqrt(100e-6 * 253.3e-12));
    CHECK(r.kind == FrequencyResponse::Kind::BandPass);
    CHECK(r.resonanceHz == Catch::Approx(f0).epsilon(0.01));
    // Bandbreite gegen eine sehr feine Rechnung um die Resonanz
    const auto ref = logFrequencies(0.9 * f0, 1.1 * f0, 20000);
    std::vector<Complex> hr;
    REQUIRE(sim.acSweep(0, ref, t, kGround, hr) == Status::Ok);
    const FrequencyResponse rr = analyzeResponse(ref, hr);
    CHECK(r.bandwidthHz == Catch::Approx(rr.bandwidthHz).epsilon(0.02));
    CHECK(f.size() > 141);
}

TEST_CASE("Frequenzgang: offene Felder stoeren die Rechnung bei 1 Hz nicht", "[bode]") {
    // offene Pultfelder haengen nur ueber kGmin an 0V; der Kondensator bringt bei 1 Hz 16 MOhm in die Matrix
    Circuit c;
    const NodeId g = c.addNode(), out = c.addNode();
    c.addNode(); // offenes Feld
    const NodeId a = c.addNode(), b = c.addNode();
    c.add<Capacitor>("Cx", a, b, CapacitorParams::film(1e-12)); // zwischen zwei offenen Feldern
    c.add<FunctionGenerator>("G", g, kGround);
    c.add<Resistor>("R", g, out, ResistorParams{10e3});
    c.add<Capacitor>("C", out, kGround, CapacitorParams::film(10e-9));
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    std::vector<double> f;
    std::vector<Complex> h;
    FrequencyResponse r;
    REQUIRE(sweepResponse(sim, 1, out, kGround, 1.0, 1e7, 20, f, h, r) == Status::Ok);
    CHECK(r.kind == FrequencyResponse::Kind::LowPass);
    CHECK(r.cutHighHz == Catch::Approx(1.0 / (2.0 * kPi * 10050.0 * 10e-9)).epsilon(0.01));
}

// Wobbeln (ab Volta 3): logarithmischer Durchlauf, aus = bitgleich
TEST_CASE("Generator wobbelt logarithmisch und beginnt von vorn", "[bode]") {
    using namespace volta::core;
    GeneratorParams gp;
    gp.frequencyHz = 100.0;
    FunctionGenerator off("G", 1, 0, gp);
    CHECK(!off.sweeping());
    CHECK(off.currentFrequency() == 100.0);
    gp.sweepFactor = 100.0;
    gp.sweepTimeS = 2.0;
    FunctionGenerator g("G", 1, 0, gp);
    CHECK(g.sweeping());
    CHECK(g.frequencyAt(0.0) == Catch::Approx(100.0));
    CHECK(g.frequencyAt(0.5) == Catch::Approx(1000.0));
    CHECK(g.frequencyAt(1.0) == Catch::Approx(10000.0));
    gp.frequencyHz = 1000.0; // Endwert hoechstens 10 kHz
    FunctionGenerator capped("G", 1, 0, gp);
    CHECK(capped.frequencyAt(1.0) == Catch::Approx(10000.0));
    CHECK(capped.frequencyAt(0.5) == Catch::Approx(std::sqrt(1000.0 * 10000.0)));
}
