// Leistungsmessung des Simulationskerns (docs/testing.md, Leistung):
//  1. Aufbau mit mindestens 40 Bauteilen inkl. Blinkmodul: 1000 Schritte muessen < 50 ms dauern (Desktop-Referenz).
//  2. Blinkmodul: mittlere und schlechteste Schrittdauer (Umkippen mit tiefer Halbierung).
// Rueckgabe 0, wenn die Vorgabe eingehalten ist, sonst 1. Nur im Release-Build aussagekraeftig.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "volta/core/components/battery.hpp"
#include "volta/core/components/buzzer.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/lamp.hpp"
#include "volta/core/components/motor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/components/transistor.hpp"
#include "volta/core/components/wire.hpp"
#include "volta/core/modules/blink_module.hpp"
#include "volta/core/simulator.hpp"
#include "volta/core/version.hpp"

using namespace volta::core;
using Clock = std::chrono::steady_clock;

namespace {

constexpr double kLimitMs = 50.0; // 40 Bauteile, 1000 Schritte

double msSince(Clock::time_point t0) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

// Typischer grosser Aufbau aus Volta 1 (Stufen 1-4) mit mindestens 40 Bauteilen.
Circuit makeLargeCircuit() {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);

    // Blinkmodul mit LED (Auftakt)
    const NodeId out = c.addNode();
    const NodeId rate = c.addNode();
    const NodeId led = c.addNode();
    addBlinkModule(c, "BL1", {plus, kGround, out, rate});
    c.add<Resistor>("RV", plus, led, ResistorParams{470.0});
    c.add<Diode>("D0", led, out, DiodeParams::ledRed());

    // vier LED-Zweige
    const DiodeParams colors[] = {DiodeParams::ledRed(), DiodeParams::ledGreen(), DiodeParams::ledYellow(),
                                  DiodeParams::ledBlue()};
    for (int i = 0; i < 4; ++i) {
        const NodeId a = c.addNode();
        c.add<Resistor>("RL" + std::to_string(i), plus, a, ResistorParams{470.0});
        c.add<Diode>("DL" + std::to_string(i), a, kGround, colors[i]);
    }
    // zwei Transistorschalter mit LED im Kollektorkreis
    for (int i = 0; i < 2; ++i) {
        const NodeId b = c.addNode();
        const NodeId k = c.addNode();
        const NodeId a = c.addNode();
        c.add<Resistor>("RB" + std::to_string(i), plus, b, ResistorParams{i == 0 ? 10000.0 : 1e6});
        c.add<Resistor>("RC" + std::to_string(i), plus, a, ResistorParams{470.0});
        c.add<Diode>("DT" + std::to_string(i), a, k, DiodeParams::ledGreen());
        c.add<Npn>("T" + std::to_string(i), b, k, kGround);
    }
    // RC-Glied
    const NodeId rc = c.addNode();
    c.add<Resistor>("R_RC", plus, rc, ResistorParams{10000.0});
    c.add<Capacitor>("C_RC", rc, kGround, CapacitorParams::electrolytic(100e-6));
    // Lampe, Motor, Summer mit Schaltern
    const NodeId l = c.addNode();
    c.add<Switch>("S_L", plus, l, true);
    c.add<Lamp>("L1", l, kGround);
    const NodeId m = c.addNode();
    c.add<Switch>("S_M", plus, m, true);
    const NodeId m2 = c.addNode();
    c.add<Resistor>("R_M", m, m2, ResistorParams{22.0});
    c.add<Motor>("M1", m2, kGround);
    const NodeId s = c.addNode();
    c.add<Switch>("S_S", plus, s, true);
    c.add<Buzzer>("SU1", s, kGround);
    // Leitungen
    const NodeId w = c.addNode();
    c.add<Wire>("W1", plus, w);
    c.add<Resistor>("R_W", w, kGround, ResistorParams{10000.0});
    return c;
}

} // namespace

int main() {
    std::printf("Volta Simulationskern %s - Leistungsmessung\n\n", version());
    bool ok = true;

    // 1. Grosser Aufbau: bestes von 5 Durchlaeufen zu je 1000 Schritten
    {
        Simulator sim;
        Circuit c = makeLargeCircuit();
        const std::size_t components = c.componentCount();
        const std::size_t nodes = c.nodeCount();
        if (sim.setCircuit(std::move(c)) != Status::Ok) {
            std::printf("FEHLER: Arbeitspunkt des grossen Aufbaus nicht gefunden\n");
            return 1;
        }
        double best = 1e9;
        double worstStep = 0.0;
        int failed = 0;
        for (int run = 0; run < 5; ++run) {
            const auto t0 = Clock::now();
            for (int i = 0; i < 1000; ++i) {
                const auto s0 = Clock::now();
                if (sim.step() != Status::Ok) ++failed;
                worstStep = std::max(worstStep, msSince(s0));
            }
            best = std::min(best, msSince(t0));
        }
        const bool pass = best < kLimitMs && failed == 0;
        ok = ok && pass;
        std::printf("Grosser Aufbau: %zu Bauteile, %zu Knoten\n", components, nodes);
        std::printf("  1000 Schritte: %.2f ms (Vorgabe < %.0f ms)  -> %s\n", best, kLimitMs, pass ? "OK" : "VERFEHLT");
        std::printf("  schlechtester Einzelschritt: %.3f ms, gescheiterte Schritte: %d\n\n", worstStep, failed);
    }

    // 2. Blinkmodul allein: Verteilung der Schrittdauern ueber 10 s
    {
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId out = c.addNode();
        const NodeId rate = c.addNode();
        const NodeId led = c.addNode();
        c.add<Battery>("B1", plus, kGround);
        addBlinkModule(c, "BL1", {plus, kGround, out, rate});
        c.add<Resistor>("RV", plus, led, ResistorParams{470.0});
        c.add<Diode>("D1", led, out, DiodeParams::ledRed());
        Simulator sim;
        sim.setCircuit(std::move(c));
        std::vector<double> times;
        times.reserve(10000);
        int maxIter = 0;
        int maxDepth = 0;
        const auto t0 = Clock::now();
        for (int i = 0; i < 10000; ++i) {
            const auto s0 = Clock::now();
            sim.step();
            times.push_back(msSince(s0));
            maxIter = std::max(maxIter, sim.lastStepIterations());
            maxDepth = std::max(maxDepth, sim.lastStepMaxDepth());
        }
        const double total = msSince(t0);
        std::sort(times.begin(), times.end());
        std::printf("Blinkmodul, 10 s Simulation: %.1f ms gesamt (Echtzeitfaktor %.0f)\n", total, 10000.0 / total);
        std::printf("  Schrittdauer Median %.4f ms, 99. Perzentil %.4f ms, Maximum %.3f ms\n", times[times.size() / 2],
                    times[times.size() * 99 / 100], times.back());
        std::printf("  hoechstens %d Newton-Iterationen je Schritt, Halbierungstiefe %d\n", maxIter, maxDepth);
    }

    std::printf("\nErgebnis: %s\n", ok ? "Vorgabe eingehalten" : "Vorgabe VERFEHLT");
    return ok ? 0 : 1;
}
