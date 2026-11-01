// Spannungsregler 5 V (78L05-artig, ab Volta 2): Ausgangsspannung, Lastregelung, Mindest-Spannungsabfall,
// Strombegrenzung und Uebertemperaturschutz.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/regulator.hpp"
#include "volta/core/components/resistor.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
// Batterie (Zellen) -> Regler -> Last; liefert Ausgangsspannung nach einigen Schritten
struct Bench {
    Circuit c;
    NodeId in = 0, out = 0;
    VoltageRegulator* reg = nullptr;
    Resistor* load = nullptr;
    Simulator sim;
    Bench(int cells, double loadOhm) {
        in = c.addNode();
        out = c.addNode();
        c.add<Battery>("B1", in, kGround, BatteryParams{cells});
        reg = &c.add<VoltageRegulator>("IC1", in, kGround, out);
        load = &c.add<Resistor>("R1", out, kGround, ResistorParams{loadOhm});
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        for (int k = 0; k < 10; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    }
    double vout() const { return sim.probe().voltage(out); }
    double vin() const { return sim.probe().voltage(in); }
};
} // namespace

TEST_CASE("Spannungsregler: 5 V, Lastregelung wie im Datenblatt", "[referenz][regler]") {
    Bench light(6, 1000.0); // 5 mA
    Bench heavy(6, 50.0);   // 100 mA
    CHECK(light.vout() == Approx(5.0).epsilon(0.02));
    CHECK(heavy.vout() == Approx(5.0).epsilon(0.02));
    CHECK(light.vout() - heavy.vout() > 0.0);
    CHECK(light.vout() - heavy.vout() < 0.06); // Lastregelung hoechstens 60 mV (1 bis 100 mA)
    CHECK(!light.reg->isOverloaded());
    requireKirchhoff(light.sim);
    requireKirchhoff(heavy.sim);
    // Eingangsstrom = Laststrom + Ruhestrom (3 mA) + innerer Teiler (1 mA)
    const ComponentReading& r = light.sim.probe().readings[1];
    CHECK(r.terminalCurrentA[0] == Approx(light.vout() / 1000.0 + 0.003 + light.vout() / 5000.0).epsilon(0.02));
    CHECK(r.displayValue == Approx(light.vout()));
}

TEST_CASE("Spannungsregler: unter 6,7 V folgt der Ausgang dem Eingang mit ca. 1,7 V Abstand", "[referenz][regler]") {
    Bench low(4, 100.0); // 6 V Batterie, 50 mA Last
    const double drop = low.vin() - low.vout();
    CHECK(low.vout() < 4.8);
    CHECK(drop > 1.5);
    CHECK(drop < 1.9);
    requireKirchhoff(low.sim);
    Bench tiny(2, 100.0); // 3 V: Ausgang weit unter 5 V, aber endlich
    CHECK(tiny.vout() < 1.5);
    requireFinite(tiny.sim.probe());
}

TEST_CASE("Spannungsregler: Kurzschluss begrenzt den Strom, Uebertemperaturschutz regelt ab", "[regler]") {
    Bench shorted(6, 0.1);
    const ComponentReading& r = shorted.sim.probe().readings[1];
    CHECK(-r.terminalCurrentA[2] == Approx(0.14).epsilon(0.05));
    CHECK(shorted.reg->isOverloaded());
    // 9 V * 0,14 A = 1,2 W bei 200 K/W: weit ueber 125 Grad C -> der Schutz nimmt den Strom zurueck
    for (int k = 0; k < 20000; ++k) REQUIRE(shorted.sim.step(1e-3) == Status::Ok);
    CHECK(shorted.reg->thermalScale() < 0.6);
    CHECK(shorted.reg->tempK() < 398.0 + 25.0);
    CHECK(!shorted.reg->isDestroyed());
    requireKirchhoff(shorted.sim);
}

TEST_CASE("Spannungsregler mit Elko am Ausgang: laeuft hoch ohne Pendeln", "[regler]") {
    Circuit c;
    const NodeId in = c.addNode();
    const NodeId out = c.addNode();
    c.add<Battery>("B1", in, kGround);
    c.add<VoltageRegulator>("IC1", in, kGround, out);
    c.add<Capacitor>("C1", out, kGround, CapacitorParams::electrolytic(100e-6, 16.0));
    c.add<Resistor>("R1", out, kGround, ResistorParams{220.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    for (int k = 0; k < 50; ++k) REQUIRE(sim.step(1e-3) == Status::Ok);
    CHECK(sim.probe().voltage(out) == Approx(5.0).epsilon(0.02));
    requireKirchhoff(sim);
}

