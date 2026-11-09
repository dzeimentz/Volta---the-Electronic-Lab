// MOSFET (ab Volta 3, Stufe 5): Kennlinie, Schalter an der Batterie (N- und P-Kanal), Body-Diode, Ueberlast,
// Halbbruecke mit Drossel an einem Rechteck (Klasse D). Dazu der Kondensator mit sehr kurzen Teilschritten (BDF2):
// Die Ladung folgt dem Strom, auch wenn kMinOhm den Serienwiderstand begrenzt.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/inductor.hpp"
#include "volta/core/components/mosfet.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/sources.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

TEST_CASE("MOSFET: Kennlinie im Widerstands- und Saettigungsbereich", "[mosfet]") {
    Mosfet m("TN", 1, 2, kGround, MosfetParams::nChannel());
    // Widerstandsbereich: Rds ~ 1/(K*Uov) = 0,2 Ohm bei 7 V
    const double i = m.channelCurrent(7.0, 0.01);
    REQUIRE(0.01 / i == Approx(0.2).epsilon(0.02));
    // Saettigung: K/2 * Uov^2 * (1 + lambda Uds)
    REQUIRE(m.channelCurrent(3.0, 5.0) == Approx(0.5 * 1.0 * 1.05).epsilon(0.02));
    // deutlich unter der Schwelle praktisch aus
    REQUIRE(std::fabs(m.channelCurrent(1.0, 9.0)) < 1e-6);
    // rueckwaerts leitend, solange das Gate offen ist; gesperrt bleibt gesperrt
    REQUIRE(m.channelCurrent(7.0, -0.01) == Approx(-i).epsilon(0.02));
    REQUIRE(std::fabs(m.channelCurrent(0.0, -5.0)) < 1e-6);
}

TEST_CASE("MOSFET: N-Kanal schaltet einen Lastwiderstand, P-Kanal gespiegelt", "[mosfet]") {
    for (int pch = 0; pch < 2; ++pch) {
        for (double ug : {0.0, 9.0}) {
            Circuit c;
            const NodeId p = c.addNode(), d = c.addNode(), g = c.addNode();
            c.add<Battery>("B1", p, kGround);
            c.add<Resistor>("RG", g, ug > 0.0 ? p : kGround, ResistorParams{1000.0});
            const bool on = pch ? ug == 0.0 : ug > 0.0;
            const std::size_t im = 2, il = 3;
            if (pch) {
                c.add<Mosfet>("TP", g, d, p, MosfetParams::pChannelDefault());
                c.add<Resistor>("RL", d, kGround, ResistorParams{18.0});
            } else {
                c.add<Mosfet>("TN", g, d, kGround, MosfetParams::nChannel());
                c.add<Resistor>("RL", p, d, ResistorParams{18.0});
            }
            Simulator sim;
            sim.setCircuit(std::move(c));
            REQUIRE(sim.solveOperatingPoint() == Status::Ok);
            const auto& r = sim.probe().readings;
            requireKirchhoff(sim);
            // ein: Batterie 9 V mit 1,5 Ohm Innenwiderstand, Last 18 Ohm, Rds(on) ~ 0,2 bzw. 0,25 Ohm
            const double rOn = pch ? 1.0 / (0.6 * 7.0) : 1.0 / (1.0 * 7.0);
            const double iRl = std::fabs(r[il].currentA);
            if (on) {
                REQUIRE(iRl == Approx(9.0 / (1.5 + 18.0 + rOn)).epsilon(0.02));
                REQUIRE(std::fabs(r[im].voltV) < 0.15); // Uds klein
            } else {
                REQUIRE(iRl < 1e-6);
            }
        }
    }
}

TEST_CASE("MOSFET: Body-Diode leitet, wenn der Drain unter die Source gezogen wird", "[mosfet]") {
    Circuit c;
    const NodeId p = c.addNode(), d = c.addNode();
    c.add<Battery>("B1", p, kGround);
    c.add<Resistor>("RL", p, kGround, ResistorParams{1e6});
    // umgekehrt angeschlossen: Source an +9 V, Gate an Source (aus), Drain ueber 47 Ohm an Minus
    c.add<Mosfet>("TN", p, d, p, MosfetParams::nChannel()); // Gate = Source = +9 V
    c.add<Resistor>("R1", d, kGround, ResistorParams{47.0});
    Simulator sim;
    sim.setCircuit(std::move(c));
    REQUIRE(sim.solveOperatingPoint() == Status::Ok);
    requireKirchhoff(sim);
    const auto& r = sim.probe().readings;
    // Strom von Source nach Drain durch die Diode: etwa (9 - 0,8) / (47 + 1,5)
    REQUIRE(-r[2].currentA == Approx((9.0 - 0.75) / 48.5).epsilon(0.03));
}

TEST_CASE("MOSFET: Halbbruecke mit Drossel an einem Rechteck gibt den Mittelwert ab", "[mosfet]") {
    // Rechteck 0 ... 9 V (Funktionsgenerator mit Gleichanteil) an beiden Gates, Tiefpass 1 mH / 22 uF, Last 8 Ohm
    auto run = [](double dtS) {
        Circuit c;
        const NodeId p = c.addNode(), g = c.addNode(), sw = c.addNode(), y = c.addNode();
        c.add<Battery>("B1", p, kGround);
        c.add<Capacitor>("CB", p, kGround, CapacitorParams::electrolytic(1000e-6));
        GeneratorParams gp;
        gp.waveform = Waveform::Square;
        gp.frequencyHz = 4000.0;
        gp.amplitudeV = 4.0;
        gp.offsetV = 4.0;
        gp.outputOhm = 50.0;
        c.add<FunctionGenerator>("G", g, kGround, gp);
        c.add<Mosfet>("TN", g, sw, kGround, MosfetParams::nChannel());
        c.add<Mosfet>("TP", g, sw, p, MosfetParams::pChannelDefault());
        c.add<Inductor>("L1", sw, y, InductorParams::choke1mH());
        c.add<Capacitor>("CF", y, kGround, CapacitorParams::film(22e-6));
        const std::size_t load = c.componentCount();
        c.add<Resistor>("RL", y, kGround, ResistorParams{8.0});
        c.setSecondOrder(true);
        Simulator sim;
        sim.setCircuit(std::move(c));
        for (int k = 0; k < static_cast<int>(0.05 / dtS); ++k) sim.step(dtS);
        double u = 0.0;
        const int n = static_cast<int>(0.01 / dtS);
        for (int k = 0; k < n; ++k) {
            sim.step(dtS);
            u += sim.probe().readings[load].voltV;
        }
        requireKirchhoff(sim);
        return u / n;
    };
    const double u5 = run(5e-6);
    // Tastverhaeltnis 1/2: Ausgang etwa die halbe Batteriespannung (Innenwiderstand und Verluste abgezogen)
    REQUIRE(u5 > 3.8);
    REQUIRE(u5 < 4.5);
    REQUIRE(u5 == Approx(run(1e-6)).epsilon(0.02));
}

TEST_CASE("MOSFET: starke Ueberlast zerstoert ihn", "[mosfet]") {
    Circuit c;
    const NodeId p = c.addNode();
    c.add<Battery>("B1", p, kGround);
    c.add<Mosfet>("TN", p, p, kGround, MosfetParams::nChannel()); // Gate und Drain an +9 V, Drain direkt an der Batterie
    Simulator sim;
    sim.setCircuit(std::move(c));
    for (int k = 0; k < 2000; ++k) sim.step(1e-3);
    REQUIRE(sim.probe().readings[1].destroyed);
    requireFinite(sim.probe());
}

TEST_CASE("Kondensator: Ladung folgt dem Strom auch bei sehr kurzen Teilschritten (BDF2)", "[capacitor]") {
    // 1000 uF ueber 10 Ohm an 9 V bis 0,2 ms: in Schritten von 0,1 us (dt/C = 1e-4 Ohm liegt unter kMinOhm) gleich
    // wie in Schritten von 10 us; vor der Korrektur lud er etwa zehnmal zu schnell
    auto run = [](double dt, int n) {
        Circuit c;
        const NodeId p = c.addNode(), x = c.addNode();
        c.add<Battery>("B1", p, kGround);
        c.add<Resistor>("R1", p, x, ResistorParams{10.0});
        c.add<Capacitor>("C1", x, kGround, CapacitorParams::electrolytic(1000e-6));
        c.setSecondOrder(true);
        Simulator sim;
        sim.setCircuit(std::move(c));
        for (int k = 0; k < n; ++k) sim.step(dt);
        return static_cast<const Capacitor&>(sim.circuit().component(2)).voltageV();
    };
    const double coarse = run(1e-5, 20);
    REQUIRE(coarse > 0.1);
    REQUIRE(run(1e-7, 2000) == Approx(coarse).epsilon(0.01));
}
