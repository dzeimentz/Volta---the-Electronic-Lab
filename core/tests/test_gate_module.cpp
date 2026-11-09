// Gatter-Modul (vier NAND mit Schmitt-Trigger-Eingaengen, ab Volta 3) und prellender Taster: Wahrheitstabelle,
// Schaltschwelle mit Hysterese (RC-Ladung), Schmitt-Oszillator, RS-Flipflop aus zwei Gattern, Mindestversorgung,
// Prellfolge.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/modules/gate_module.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
constexpr double kDt = 20e-6;

void run(Simulator& sim, double seconds, double dtS = kDt) {
    const int steps = static_cast<int>(seconds / dtS + 0.5);
    for (int k = 0; k < steps; ++k) REQUIRE(sim.step(dtS) == Status::Ok);
}
} // namespace

TEST_CASE("Gattermodul: NAND-Wahrheitstabelle an 9 V", "[gate]") {
    for (int a = 0; a < 2; ++a) {
        for (int b = 0; b < 2; ++b) {
            Circuit circ;
            const NodeId vcc = circ.addNode(), y = circ.addNode();
            circ.add<Battery>("B1", vcc, kGround, BatteryParams{6});
            GateModulePins p;
            p.plus = vcc;
            p.minus = kGround;
            for (auto& n : p.io) n = circ.addNode();
            p.io[0] = a ? vcc : kGround;
            p.io[1] = b ? vcc : kGround;
            p.io[2] = y;
            circ.add<Resistor>("RL", y, kGround, ResistorParams{10000.0});
            const SubcircuitRange r = addGateModule(circ, "GM1", p);
            CHECK(r.count == 4);
            Simulator sim;
            REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
            run(sim, 1e-3);
            const double u = sim.probe().voltage(vcc);
            const double vy = sim.probe().voltage(y);
            if (a && b) CHECK(vy < 0.05 * u);
            else CHECK(vy == Approx(u * 10000.0 / 10200.0).epsilon(0.01));
            requireKirchhoff(sim);
            requireFinite(sim.probe());
        }
    }
}

TEST_CASE("Gattermodul: Schaltschwelle bei 60 Prozent der Versorgung (RC-Ladung)", "[gate]") {
    // Eingang a laedt ueber R von +U (b fest auf H): der Ausgang faellt bei U_C = 0,6 U, t = R C ln(1/0,4)
    const double R = 100e3, C = 100e-9;
    Circuit circ;
    const NodeId vcc = circ.addNode(), in = circ.addNode(), y = circ.addNode();
    circ.add<Battery>("B1", vcc, kGround, BatteryParams{6});
    circ.add<Resistor>("R1", vcc, in, ResistorParams{R});
    circ.add<Capacitor>("C1", in, kGround, CapacitorParams::film(C));
    GateModulePins p;
    p.plus = vcc;
    for (auto& n : p.io) n = circ.addNode();
    p.io[0] = in;
    p.io[1] = vcc;
    p.io[2] = y;
    addGateModule(circ, "GM1", p);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
    double t = 0.0, fall = -1.0;
    while (t < 30e-3) {
        REQUIRE(sim.step(kDt) == Status::Ok);
        t += kDt;
        if (fall < 0.0 && sim.probe().voltage(y) < 0.5 * sim.probe().voltage(vcc)) fall = t;
    }
    REQUIRE(fall > 0.0);
    CHECK(near(fall, R * C * std::log(1.0 / 0.4), kDt));
    requireKirchhoff(sim);
}

TEST_CASE("Gattermodul: Schmitt-Oszillator mit R und C", "[gate]") {
    // y -> R -> a, C von a nach -U, b auf H: Periode 2 R C ln(0,6/0,4) (Hysterese 40/60 Prozent)
    const double R = 100e3, C = 47e-9;
    Circuit circ;
    const NodeId vcc = circ.addNode(), in = circ.addNode(), y = circ.addNode();
    circ.add<Battery>("B1", vcc, kGround, BatteryParams{6});
    circ.add<Resistor>("R1", y, in, ResistorParams{R});
    circ.add<Capacitor>("C1", in, kGround, CapacitorParams::film(C));
    GateModulePins p;
    p.plus = vcc;
    for (auto& n : p.io) n = circ.addNode();
    p.io[0] = in;
    p.io[1] = vcc;
    p.io[2] = y;
    addGateModule(circ, "GM1", p);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
    double t = 0.0, first = -1.0, last = -1.0;
    int rises = 0;
    bool high = true;
    while (t < 60e-3) {
        REQUIRE(sim.step(kDt) == Status::Ok);
        t += kDt;
        const bool now = sim.probe().voltage(y) > 0.5 * sim.probe().voltage(vcc);
        if (now && !high && t > 10e-3) {
            if (first < 0.0) first = t;
            last = t;
            ++rises;
        }
        high = now;
    }
    REQUIRE(rises >= 3);
    const double period = (last - first) / (rises - 1);
    CHECK(near(period, 2.0 * (R + 200.0) * C * std::log(1.5), 2 * kDt));
    requireKirchhoff(sim);
    requireFinite(sim.probe());
}

TEST_CASE("Gattermodul: RS-Flipflop aus zwei NAND haelt seinen Zustand", "[gate]") {
    // /S und /R mit 10 kOhm nach +U, Taster nach -U; Q = y1, /Q = y2
    Circuit circ;
    const NodeId vcc = circ.addNode(), s = circ.addNode(), r = circ.addNode(), q = circ.addNode(),
                 nq = circ.addNode();
    circ.add<Battery>("B1", vcc, kGround, BatteryParams{6});
    circ.add<Resistor>("RS", vcc, s, ResistorParams{10000.0});
    circ.add<Resistor>("RR", vcc, r, ResistorParams{10000.0});
    const std::size_t ts = circ.componentCount();
    circ.add<Switch>("TS", s, kGround);
    const std::size_t tr = circ.componentCount();
    circ.add<Switch>("TR", r, kGround);
    GateModulePins p;
    p.plus = vcc;
    for (auto& n : p.io) n = circ.addNode();
    p.io[0] = s;
    p.io[1] = nq;
    p.io[2] = q;
    p.io[3] = r;
    p.io[4] = q;
    p.io[5] = nq;
    addGateModule(circ, "GM1", p);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
    auto& swS = static_cast<Switch&>(sim.circuit().component(ts));
    auto& swR = static_cast<Switch&>(sim.circuit().component(tr));
    run(sim, 1e-3);
    const auto qHigh = [&] { return sim.probe().voltage(q) > 0.5 * sim.probe().voltage(vcc); };
    const auto nqHigh = [&] { return sim.probe().voltage(nq) > 0.5 * sim.probe().voltage(vcc); };
    CHECK(qHigh() != nqHigh()); // eindeutiger Startzustand

    swS.setClosed(true); // setzen
    run(sim, 1e-3);
    swS.setClosed(false);
    run(sim, 2e-3);
    CHECK(qHigh());
    CHECK_FALSE(nqHigh());

    swR.setClosed(true); // ruecksetzen
    run(sim, 1e-3);
    swR.setClosed(false);
    run(sim, 2e-3);
    CHECK_FALSE(qHigh());
    CHECK(nqHigh());
    requireKirchhoff(sim);
}

TEST_CASE("Gattermodul: unter 3 V arbeitet das Gatter nicht", "[gate]") {
    Circuit circ;
    const NodeId vcc = circ.addNode(), y = circ.addNode();
    circ.add<Battery>("B1", vcc, kGround, BatteryParams{1});
    GateModulePins p;
    p.plus = vcc;
    for (auto& n : p.io) n = circ.addNode();
    p.io[0] = kGround;
    p.io[1] = kGround;
    p.io[2] = y;
    circ.add<Resistor>("RL", y, kGround, ResistorParams{10000.0});
    addGateModule(circ, "GM1", p);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
    run(sim, 1e-3);
    CHECK(sim.probe().voltage(y) < 0.01);
    requireKirchhoff(sim);
}

TEST_CASE("Taster: prellt in Volta 3 mehrfach, sonst sauber", "[gate][switch]") {
    for (int bounce = 0; bounce < 2; ++bounce) {
        Circuit circ;
        const NodeId vcc = circ.addNode(), n = circ.addNode();
        circ.add<Battery>("B1", vcc, kGround, BatteryParams{6});
        const std::size_t ti = circ.componentCount();
        circ.add<Switch>("T1", vcc, n);
        circ.add<Resistor>("R1", n, kGround, ResistorParams{10000.0});
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
        auto& sw = static_cast<Switch&>(sim.circuit().component(ti));
        sw.setBounce(bounce == 1);
        run(sim, 1e-3, 50e-6);
        int rises = 0, falls = 0;
        bool high = false;
        const auto count = [&](double seconds) {
            const int steps = static_cast<int>(seconds / 50e-6 + 0.5);
            for (int k = 0; k < steps; ++k) {
                REQUIRE(sim.step(50e-6) == Status::Ok);
                const bool now = sim.probe().voltage(n) > 4.0;
                if (now && !high) ++rises;
                if (!now && high) ++falls;
                high = now;
            }
        };
        sw.setClosed(true);
        count(6e-3);
        CHECK(high);
        if (bounce) CHECK(rises == 4);
        else CHECK(rises == 1);
        rises = falls = 0;
        sw.setClosed(false);
        count(6e-3);
        CHECK_FALSE(high);
        if (bounce) CHECK(falls == 3);
        else CHECK(falls == 1);
        requireKirchhoff(sim);
    }
}

TEST_CASE("Gattermodul: RC und Schmitt-Trigger entprellen den Taster", "[gate][switch]") {
    // Aufbau wie Experiment 7.7: 10 kOhm nach +U, Taster nach -U (Knoten x); 100 kOhm von x nach y, 100 nF an y.
    // Gatter 1 invertiert x (prellt mit), Gatter 2 invertiert y (ein einziger Sprung, etwa 0,92 R C nach dem Druck)
    Circuit circ;
    const NodeId vcc = circ.addNode(), x = circ.addNode(), y = circ.addNode(), y1 = circ.addNode(),
                 y2 = circ.addNode();
    circ.add<Battery>("B1", vcc, kGround, BatteryParams{6});
    circ.add<Resistor>("R1", vcc, x, ResistorParams{10000.0});
    const std::size_t ti = circ.componentCount();
    circ.add<Switch>("S1", x, kGround);
    circ.add<Resistor>("R2", x, y, ResistorParams{100e3});
    circ.add<Capacitor>("C1", y, kGround, CapacitorParams::film(100e-9));
    GateModulePins p;
    p.plus = vcc;
    for (auto& n : p.io) n = circ.addNode();
    p.io[0] = p.io[1] = x;
    p.io[2] = y1;
    p.io[3] = p.io[4] = y;
    p.io[5] = y2;
    addGateModule(circ, "GM1", p);
    circ.setSecondOrder(true);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(circ)) == Status::Ok);
    auto& sw = static_cast<Switch&>(sim.circuit().component(ti));
    sw.setBounce(true);
    run(sim, 50e-3);
    const auto high = [&](NodeId n) { return sim.probe().voltage(n) > 0.5 * sim.probe().voltage(vcc); };
    REQUIRE_FALSE(high(y1));
    REQUIRE_FALSE(high(y2));
    sw.setClosed(true);
    int rises1 = 0, rises2 = 0;
    double t = 0.0, first2 = -1.0;
    bool h1 = false, h2 = false;
    while (t < 40e-3) {
        REQUIRE(sim.step(kDt) == Status::Ok);
        t += kDt;
        if (high(y1) && !h1) ++rises1;
        if (high(y2) && !h2) {
            ++rises2;
            if (first2 < 0.0) first2 = t;
        }
        h1 = high(y1);
        h2 = high(y2);
    }
    CHECK(rises1 == 4); // Prellen kommt am rohen Eingang an
    CHECK(rises2 == 1); // entprellt: ein Sprung
    CHECK(first2 > 7e-3);
    CHECK(first2 < 11e-3);
    requireKirchhoff(sim);
}
