// NPN-Transistor: Referenzschaltung 5 (Schalter in Saettigung), aktiver Bereich, Sperren, Ueberlast.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/transistor.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {

constexpr double kVt = 1.380649e-23 * kAmbientK / 1.602176634e-19; // 25,26 mV

// Emitterschaltung: 9 V, Kollektorwiderstand rc, Basiswiderstand rb von +9 V.
struct Stage {
    Simulator sim;
    NodeId plus = kGround, base = kGround, coll = kGround;
};
void build(Stage& s, double rc, double rb, BjtParams p = {}) {
    Circuit c;
    s.plus = c.addNode();
    s.base = c.addNode();
    s.coll = c.addNode();
    c.add<Battery>("B1", s.plus, kGround);
    c.add<Resistor>("RC", s.plus, s.coll, ResistorParams{rc});
    c.add<Resistor>("RB", s.plus, s.base, ResistorParams{rb});
    c.add<Npn>("T1", s.base, s.coll, kGround, p);
    REQUIRE(s.sim.setCircuit(std::move(c)) == Status::Ok);
}

} // namespace

TEST_CASE("Referenz 5: Transistorschalter in Saettigung", "[referenz][transistor]") {
    Stage s;
    build(s, 1000.0, 10000.0);
    const ComponentReading& t = s.sim.probe().readings[3];
    const double ic = t.terminalCurrentA[1];
    const double ib = t.terminalCurrentA[0];
    const double vce = s.sim.probe().voltage(s.coll);
    // Handrechnung: Ib = (9 - 0,7) / 10 kOhm = 0,83 mA; beta*Ib = 166 mA >> 9 V / 1 kOhm -> gesaettigt
    CHECK(ib == Approx(0.83e-3).epsilon(0.03));
    CHECK(ic == Approx((9.0 - vce) / 1001.5).epsilon(1e-3)); // Ri der Batterie 1,5 Ohm teilt sich auf
    CHECK(ic > 8.7e-3);
    CHECK(vce < 0.2);
    // Unabhaengige Pruefung: Ebers-Moll-Saettigungsspannung
    // Uce,sat = Vt * ln[(1 + (1 + Ic/Ib)/BR) / (1 - Ic/(BF*Ib))]
    const double br = 7.0, bf = 200.0;
    const double vceSat = kVt * std::log((1.0 + (1.0 + ic / ib) / br) / (1.0 - ic / (bf * ib)));
    CHECK(vce == Approx(vceSat).margin(1e-3));
    CHECK(s.sim.probe().voltage(s.base) > 0.6);
    CHECK(s.sim.probe().voltage(s.base) < 0.8);
    requireKirchhoff(s.sim);
}

TEST_CASE("Transistor im aktiven Bereich: Ic = beta * Ib", "[referenz][transistor]") {
    Stage s;
    build(s, 1000.0, 1e6);
    const ComponentReading& t = s.sim.probe().readings[3];
    const double ic = t.terminalCurrentA[1];
    const double ib = t.terminalCurrentA[0];
    CHECK(ic / ib == Approx(200.0).epsilon(0.01));
    const double vbe = s.sim.probe().voltage(s.base);
    CHECK(ib == Approx((9.0 - vbe) / (1e6 + 1.5)).epsilon(1e-3));
    // Eingangskennlinie: Vbe = Vt * ln(Ic / Is)
    CHECK(vbe == Approx(kVt * std::log(ic / 1.8e-14)).margin(2e-3));
    CHECK(ic == Approx(1.67e-3).epsilon(0.02)); // 200 * (9 - 0,64) / 1 MOhm
    CHECK(t.currentA == Approx(ic));             // Anzeige: Ic
    CHECK(t.voltV == Approx(s.sim.probe().voltage(s.coll)).epsilon(1e-6)); // Anzeige: Uce
    requireKirchhoff(s.sim);
}

TEST_CASE("Transistor ohne Basisstrom sperrt", "[transistor]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId base = c.addNode();
    const NodeId coll = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("RC", plus, coll, ResistorParams{1000.0});
    c.add<Npn>("T1", base, coll, kGround); // Basis offen
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(std::fabs(sim.probe().readings[2].terminalCurrentA[1]) < 1e-6);
    CHECK(sim.probe().voltage(coll) == Approx(9.0).epsilon(1e-3));
    requireKirchhoff(sim);
}

TEST_CASE("Transistor verkehrt herum eingesetzt verstaerkt kaum", "[transistor][robust]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId base = c.addNode();
    const NodeId emit = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("RC", plus, emit, ResistorParams{1000.0});
    c.add<Resistor>("RB", plus, base, ResistorParams{1e6});
    c.add<Npn>("T1", base, kGround, emit); // Kollektor und Emitter vertauscht
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const ComponentReading& t = sim.probe().readings[3];
    // Rueckwaertsbetrieb: Strom aus dem Emitteranschluss heraus ca. (1 + BR) * Ib, also << BF * Ib
    const double ib = t.terminalCurrentA[0];
    CHECK(-t.terminalCurrentA[2] / ib < 10.0);
    requireFinite(sim.probe());
    requireKirchhoff(sim);
}

TEST_CASE("Transistor ohne Kollektorwiderstand: Ueberlast zerstoert ihn", "[transistor][thermik]") {
    Stage s;
    build(s, kMinOhm, 1000.0); // Ib ca. 8 mA -> Ic durch Ri begrenzt, weit ueber 100 mA
    REQUIRE(s.sim.step() == Status::Ok);
    CHECK(s.sim.probe().readings[3].overloaded);
    bool destroyed = false;
    for (int i = 0; i < 5000 && !destroyed; ++i) {
        REQUIRE(s.sim.step() == Status::Ok);
        destroyed = s.sim.probe().readings[3].destroyed;
    }
    CHECK(destroyed);
    REQUIRE(s.sim.step() == Status::Ok);
    for (double i : s.sim.probe().readings[3].terminalCurrentA) CHECK(std::fabs(i) < 1e-6);
    requireKirchhoff(s.sim);
}
