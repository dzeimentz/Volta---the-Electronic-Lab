// Kondensator und Elko: Referenzschaltung 3 (RC-Ladung), Entladung, Ladungserhalt bei Topologiewechsel und Undo,
// Ladungserhaltung, Grenzwerte.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/wire.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {

void runFor(Simulator& sim, double seconds) {
    const int steps = static_cast<int>(std::lround(seconds / kDefaultStepS));
    for (int i = 0; i < steps; ++i) REQUIRE(sim.step() == Status::Ok);
}

std::size_t countEvents(Simulator& sim, SimEventType type, std::size_t component) {
    std::size_t n = 0;
    SimEvent e;
    while (sim.events().pop(e))
        if (e.type == type && e.component == component) ++n;
    return n;
}

// RC-Ladeschaltung: 9 V (Ri 1,5 Ohm) -> 10 kOhm -> Elko 100 uF (Leck 10 MOhm) -> Masse.
struct RcCircuit {
    Circuit circuit;
    NodeId cap = kGround;
};
RcCircuit makeRc() {
    RcCircuit rc;
    const NodeId plus = rc.circuit.addNode();
    rc.cap = rc.circuit.addNode();
    rc.circuit.add<Battery>("B1", plus, kGround);
    rc.circuit.add<Resistor>("R1", plus, rc.cap, ResistorParams{10000.0});
    rc.circuit.add<Capacitor>("C1", rc.cap, kGround, CapacitorParams::electrolytic(100e-6));
    return rc;
}

// Thevenin-Ersatz der RC-Ladung inkl. Ri und Leckwiderstand: U(t) = Uinf * (1 - exp(-t / tau)).
constexpr double kRs = 10000.0 + 1.5;
constexpr double kLeak = 1e7;
constexpr double kUinf = 9.0 * kLeak / (kLeak + kRs);
constexpr double kTau = (kRs * kLeak / (kRs + kLeak)) * 100e-6; // ca. 0,999 s
double charging(double t) { return kUinf * (1.0 - std::exp(-t / kTau)); }

} // namespace

TEST_CASE("Referenz 3: RC-Ladung 10 kOhm / 100 uF erreicht nach tau 63,2 %", "[referenz][transient][kondensator]") {
    RcCircuit rc = makeRc();
    const NodeId cap = rc.cap;
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(rc.circuit)) == Status::Ok);
    // Ungeladener Kondensator im Arbeitspunkt: 0 V, voller Ladestrom 9 V / 10 kOhm
    CHECK(sim.probe().voltage(cap) == Approx(0.0).margin(1e-3)); // 10 us / C im Arbeitspunkt: ca. 0,1 mV
    CHECK(sim.probe().readings[2].currentA == Approx(9.0 / kRs).epsilon(1e-3));

    runFor(sim, 1.0);
    CHECK(nearV(sim.probe().voltage(cap), 0.632 * 9.0));             // Lehrbuchwert (2 %)
    CHECK(sim.probe().voltage(cap) == Approx(charging(1.0)).epsilon(2e-3)); // inkl. Ri und Leck, Euler-Fehler < 0,1 %
    requireKirchhoff(sim);

    runFor(sim, 4.0); // 5 tau: > 99 %
    CHECK(sim.probe().voltage(cap) == Approx(charging(5.0)).epsilon(2e-3));
    CHECK(sim.probe().voltage(cap) > 0.99 * kUinf);
}

TEST_CASE("Kondensator entlaedt sich exponentiell", "[referenz][transient][kondensator]") {
    Circuit c;
    const NodeId a = c.addNode();
    c.add<Capacitor>("C1", a, kGround, CapacitorParams::electrolytic(100e-6)).setVoltage(9.0);
    c.add<Resistor>("R1", a, kGround, ResistorParams{10000.0});
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(sim.probe().voltage(a) == Approx(9.0).epsilon(1e-3)); // Arbeitspunkt behaelt die Ladung
    runFor(sim, 1.0);
    const double tau = (10000.0 * kLeak / (10000.0 + kLeak)) * 100e-6;
    CHECK(sim.probe().voltage(a) == Approx(9.0 * std::exp(-1.0 / tau)).epsilon(2e-3)); // ca. 3,31 V
    requireKirchhoff(sim);
}

TEST_CASE("Ladung bleibt beim Topologiewechsel erhalten", "[kondensator][undo]") {
    RcCircuit rc = makeRc();
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(rc.circuit)) == Status::Ok);
    runFor(sim, 0.5);
    const ComponentState charged = sim.circuit().component(2).saveState();
    const double v = charged.internal[0];
    CHECK(v == Approx(charging(0.5)).epsilon(2e-3));

    // Neuer Aufbau: Batterie abgezogen, Kondensator allein (Zustand vom Modell der App uebernommen)
    Circuit c;
    const NodeId a = c.addNode();
    c.add<Capacitor>("C1", a, kGround, CapacitorParams::electrolytic(100e-6)).restoreState(charged);
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    CHECK(sim.probe().voltage(a) == Approx(v).epsilon(1e-4));
    runFor(sim, 1.0); // Selbstentladung ueber 10 MOhm: tau = 1000 s
    CHECK(sim.probe().voltage(a) == Approx(v * std::exp(-1.0 / 1000.0)).epsilon(1e-4));
}

TEST_CASE("Undo stellt die Kondensatorladung her und die Ladung laeuft korrekt weiter", "[kondensator][undo]") {
    RcCircuit rc = makeRc();
    const NodeId cap = rc.cap;
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(rc.circuit)) == Status::Ok);
    runFor(sim, 0.2);
    const ComponentState at02 = sim.circuit().component(2).saveState();
    runFor(sim, 0.8);
    CHECK(sim.probe().voltage(cap) == Approx(charging(1.0)).epsilon(2e-3));

    sim.circuit().component(2).restoreState(at02);
    REQUIRE(sim.solveOperatingPoint() == Status::Ok);
    CHECK(sim.probe().voltage(cap) == Approx(charging(0.2)).epsilon(2e-3));
    runFor(sim, 0.3);
    CHECK(sim.probe().voltage(cap) == Approx(charging(0.5)).epsilon(2e-3));
}

TEST_CASE("Zwei Kondensatoren gleichen ihre Ladung aus (Ladungserhaltung)", "[kondensator][transient]") {
    Circuit c;
    const NodeId a = c.addNode();
    const NodeId b = c.addNode();
    c.add<Capacitor>("C1", a, kGround, CapacitorParams::film(100e-6)).setVoltage(9.0);
    c.add<Capacitor>("C2", b, kGround, CapacitorParams::film(47e-6));
    c.add<Wire>("W1", a, b);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    runFor(sim, 0.01);
    const double expected = 9.0 * 100e-6 / (100e-6 + 47e-6); // Q1 / (C1 + C2) = 6,12 V
    CHECK(sim.probe().voltage(a) == Approx(expected).epsilon(1e-3));
    CHECK(sim.probe().voltage(b) == Approx(expected).epsilon(1e-3));
    requireKirchhoff(sim);
}

TEST_CASE("Kondensator sperrt Gleichstrom nach dem Laden", "[kondensator][transient]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{1000.0});
    c.add<Capacitor>("C1", a, kGround, CapacitorParams::film(10e-6)); // tau = 10 ms
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    runFor(sim, 0.2);
    CHECK(std::fabs(sim.probe().readings[2].currentA) < 1e-7); // nur Leckstrom
    CHECK(sim.probe().voltage(a) == Approx(9.0).epsilon(1e-3));
}

TEST_CASE("Elko verpolt: Defekt nach 0,2 s, danach offen", "[kondensator][grenzwert]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{100.0});
    c.add<Capacitor>("C1", kGround, a, CapacitorParams::electrolytic(100e-6)); // Plus an Masse: verpolt
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    runFor(sim, 0.1);
    CHECK(sim.probe().readings[2].overloaded);
    CHECK_FALSE(sim.probe().readings[2].destroyed);
    runFor(sim, 0.3);
    CHECK(sim.probe().readings[2].destroyed);
    CHECK(countEvents(sim, SimEventType::Destroyed, 2) == 1);
    runFor(sim, 0.1);
    CHECK(std::fabs(sim.probe().readings[2].currentA) < 1e-9);
    requireKirchhoff(sim);
}

TEST_CASE("Elko richtig gepolt innerhalb der Nennspannung bleibt heil", "[kondensator][grenzwert]") {
    RcCircuit rc = makeRc();
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(rc.circuit)) == Status::Ok);
    runFor(sim, 5.0);
    CHECK_FALSE(sim.probe().readings[2].destroyed);
    CHECK_FALSE(sim.probe().readings[2].overloaded);
}

TEST_CASE("Elko 6,3 V an 9 V: Ueberspannung zerstoert ihn", "[kondensator][grenzwert]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{100.0});
    c.add<Capacitor>("C1", a, kGround, CapacitorParams::electrolytic(100e-6, 6.3));
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    runFor(sim, 1.0);
    CHECK(sim.probe().readings[2].destroyed);
    CHECK(countEvents(sim, SimEventType::Destroyed, 2) == 1);
}

TEST_CASE("Kondensator: unsinnige Kennwerte werden abgefangen", "[kondensator][robust]") {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    Capacitor cap("C1", 1, 0, CapacitorParams{nan, -1.0, true, nan, nan, nan});
    CHECK(cap.params().capacitanceF > 0.0);
    CHECK(cap.params().leakOhm > 0.0);
    cap.setVoltage(nan);
    CHECK(cap.voltageV() == 0.0);
}

TEST_CASE("Arbeitspunkt: verschieden geladene parallele Kondensatoren ergeben den ladungsgewichteten Wert", "[kondensator][undo]") {
    Circuit c;
    const NodeId a = c.addNode();
    c.add<Capacitor>("C1", a, kGround, CapacitorParams::film(100e-6)).setVoltage(9.0);
    c.add<Capacitor>("C2", a, kGround, CapacitorParams::electrolytic(2200e-6)).setVoltage(0.0);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const double expected = 9.0 * 100e-6 / (100e-6 + 2200e-6); // 0,391 V
    CHECK(sim.probe().voltage(a) == Approx(expected).epsilon(1e-3));
    requireFinite(sim.probe());
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().voltage(a) == Approx(expected).epsilon(1e-3));
    requireKirchhoff(sim);
}

TEST_CASE("Undo eines zerstoerten Elkos stellt ihn samt Ladung wieder her", "[kondensator][undo]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{100.0});
    c.add<Capacitor>("C1", kGround, a, CapacitorParams::electrolytic(100e-6)); // verpolt
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    runFor(sim, 0.1);
    const ComponentState before = sim.circuit().component(2).saveState();
    CHECK(before.internal[1] > 0.05); // Verletzungsdauer laeuft
    runFor(sim, 0.3);
    REQUIRE(sim.probe().readings[2].destroyed);

    sim.circuit().component(2).restoreState(before);
    REQUIRE(sim.solveOperatingPoint() == Status::Ok);
    CHECK_FALSE(sim.probe().readings[2].destroyed);
    CHECK(sim.probe().voltage(a) == Approx(-before.internal[0]).epsilon(1e-3));
    runFor(sim, 0.15); // Verletzungsdauer zaehlt weiter: insgesamt > 0,2 s
    CHECK(sim.probe().readings[2].destroyed);
}

TEST_CASE("Folienkondensator: Ueberspannung zerstoert ihn", "[kondensator][grenzwert]") {
    CapacitorParams p = CapacitorParams::film(1e-6);
    p.umaxV = 5.0;
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{100.0});
    c.add<Capacitor>("C1", a, kGround, p);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    runFor(sim, 0.5);
    CHECK(sim.probe().readings[2].destroyed);
}
