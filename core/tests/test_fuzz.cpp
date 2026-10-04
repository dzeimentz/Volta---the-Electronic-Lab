// Zufallsschaltungen (Risiko R5): Kinder stecken frei. Viele zufaellig gesteckte Aufbauten aus allen Katalogbauteilen,
// mit Bedienhandlungen waehrend der Simulation. Erwartung: keine Ausnahme, kein NaN, Kirchhoff erfuellt,
// Nichtkonvergenz hoechstens selten und dann gemeldet. Deterministisch (fester Startwert).
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <random>
#include <string>
#include <vector>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/buzzer.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/lamp.hpp"
#include "volta/core/components/meter.hpp"
#include "volta/core/components/motor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/components/transistor.hpp"
#include "volta/core/components/wire.hpp"
#include "volta/core/modules/blink_module.hpp"

using namespace volta::core;

namespace {

constexpr double kE12[] = {1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.9, 4.7, 5.6, 6.8, 8.2};

struct Fuzzer {
    std::mt19937 rng;
    explicit Fuzzer(unsigned seed) : rng(seed) {}

    int uniform(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); }
    double resistor() { return kE12[uniform(0, 11)] * std::pow(10.0, uniform(1, 6)); } // 10 Ohm .. 8,2 MOhm

    // Aufbau mit Batterie an Knoten 1 und count zufaellig gesteckten Bauteilen zwischen nodes Knoten (inkl. Masse).
    Circuit build(int nodes, int count) {
        Circuit c;
        for (int i = 1; i < nodes; ++i) c.addNode();
        auto node = [&] { return static_cast<NodeId>(uniform(0, nodes - 1)); };
        c.add<Battery>("B1", 1, kGround, BatteryParams{uniform(1, 6)});
        for (int i = 0; i < count; ++i) {
            const std::string id = "X" + std::to_string(i);
            switch (uniform(0, 14)) {
            case 0:
            case 1: c.add<Resistor>(id, node(), node(), ResistorParams{resistor()}); break;
            case 2: c.add<Wire>(id, node(), node()); break;
            case 3: {
                const DiodeParams led[] = {DiodeParams::ledRed(), DiodeParams::ledGreen(), DiodeParams::ledYellow(),
                                           DiodeParams::ledBlue()};
                c.add<Diode>(id, node(), node(), led[uniform(0, 3)]);
                break;
            }
            case 4: c.add<Diode>(id, node(), node(), uniform(0, 1) ? DiodeParams::silicon() : DiodeParams::germanium()); break;
            case 5: c.add<Capacitor>(id, node(), node(), CapacitorParams::electrolytic(kE12[uniform(0, 11)] * 1e-5)); break;
            case 6: c.add<Capacitor>(id, node(), node(), CapacitorParams::film(kE12[uniform(0, 11)] * 1e-7)); break;
            case 7: c.add<Lamp>(id, node(), node()); break;
            case 8: c.add<Motor>(id, node(), node()); break;
            case 9: c.add<Buzzer>(id, node(), node()); break;
            case 10: c.add<Switch>(id, node(), node(), uniform(0, 1) == 1); break;
            case 11: c.add<ChangeoverSwitch>(id, node(), node(), node(), uniform(0, 1) == 1); break;
            case 12: c.add<Npn>(id, node(), node(), node()); break;
            case 13: {
                Meter& m = c.add<Meter>(id, node(), node(), node());
                m.setMode(static_cast<MeterMode>(uniform(0, 2)));
                m.setAnalog(uniform(0, 1) == 1);
                break;
            }
            default: {
                const NodeId rate = c.addNode();
                addBlinkModule(c, id, {node(), node(), node(), rate});
                break;
            }
            }
        }
        return c;
    }

    // Zufaellige Bedienhandlung: Schalter umlegen, Widerstand aendern, Motor blockieren, Zellenzahl aendern.
    void operate(Circuit& c) {
        ComponentModel& m = c.component(static_cast<std::size_t>(uniform(0, static_cast<int>(c.componentCount()) - 1)));
        if (auto* s = dynamic_cast<Switch*>(&m)) s->setClosed(!s->isClosed());
        else if (auto* w = dynamic_cast<ChangeoverSwitch*>(&m)) w->setToB(!w->isToB());
        else if (auto* r = dynamic_cast<Resistor*>(&m)) r->setOhm(resistor());
        else if (auto* mo = dynamic_cast<Motor*>(&m)) mo->setBlocked(!mo->isBlocked());
        else if (auto* b = dynamic_cast<Battery*>(&m)) b->setCells(uniform(1, 6));
        else if (auto* me = dynamic_cast<Meter*>(&m)) me->setMode(static_cast<MeterMode>(uniform(0, 2)));
    }
};

// Kirchhoff mit relativer Toleranz (Stroeme bis in den Amperebereich)
bool kirchhoffHolds(const Simulator& sim, double& worst) {
    const Circuit& c = sim.circuit();
    const Probe& p = sim.probe();
    worst = 0.0;
    for (NodeId n = 1; n < c.nodeCount(); ++n) {
        double sum = kGmin * p.voltage(n);
        double scale = 0.0;
        for (std::size_t i = 0; i < c.componentCount(); ++i) {
            const ComponentModel& m = c.component(i);
            for (std::size_t t = 0; t < m.terminalCount(); ++t) {
                if (m.terminal(t) != n) continue;
                sum += p.readings[i].terminalCurrentA[t];
                scale += std::fabs(p.readings[i].terminalCurrentA[t]);
            }
        }
        const double excess = std::fabs(sum) - (1e-9 + 1e-9 * scale);
        worst = std::max(worst, excess);
    }
    return worst <= 0.0;
}

bool allFinite(const Probe& p) {
    for (double v : p.nodeVolt)
        if (!std::isfinite(v)) return false;
    for (const auto& r : p.readings) {
        if (!std::isfinite(r.voltV) || !std::isfinite(r.currentA) || !std::isfinite(r.powerW) ||
            !std::isfinite(r.tempK) || !std::isfinite(r.level) || !std::isfinite(r.frequencyHz))
            return false;
        for (double i : r.terminalCurrentA)
            if (!std::isfinite(i)) return false;
    }
    return true;
}

} // namespace

TEST_CASE("Zufallsschaltungen: kein Absturz, kein NaN, Kirchhoff, seltene Nichtkonvergenz", "[robust][zufall]") {
#ifdef NDEBUG
    constexpr int kCircuits = 400;
#else
    constexpr int kCircuits = 150; // Debug-Build ist ca. 10x langsamer
#endif
    constexpr int kSteps = 300;
    Fuzzer fz(20261004u);
    int opFailures = 0;
    int circuitsWithStepFailure = 0;
    int kirchhoffViolations = 0;
    std::size_t convergenceEvents = 0;
    std::size_t destroyedEvents = 0;
    for (int k = 0; k < kCircuits; ++k) {
        Circuit c = fz.build(fz.uniform(3, 10), fz.uniform(2, 14));
        Simulator sim;
        const Status op = sim.setCircuit(std::move(c));
        REQUIRE((op == Status::Ok || op == Status::NotConverged));
        if (op != Status::Ok) ++opFailures;
        bool stepFailure = false;
        for (int i = 0; i < kSteps; ++i) {
            if (fz.uniform(0, 49) == 0) fz.operate(sim.circuit());
            const Status s = sim.step();
            REQUIRE((s == Status::Ok || s == Status::NotConverged));
            stepFailure = stepFailure || s != Status::Ok;
            if (sim.probe().valid && !allFinite(sim.probe())) {
                INFO("Schaltung " << k << ", Schritt " << i);
                FAIL("NaN/Inf in der Probe");
            }
        }
        if (stepFailure) ++circuitsWithStepFailure;
        double worst = 0.0;
        if (sim.probe().valid && !kirchhoffHolds(sim, worst)) {
            ++kirchhoffViolations;
            UNSCOPED_INFO("Kirchhoff verletzt in Schaltung " << k << " um " << worst << " A");
        }
        SimEvent e;
        while (sim.events().pop(e)) {
            if (e.type == SimEventType::ConvergenceTrouble) ++convergenceEvents;
            if (e.type == SimEventType::Destroyed) ++destroyedEvents;
        }
    }
    INFO("Arbeitspunkt gescheitert: " << opFailures << ", Schaltungen mit gescheitertem Schritt: "
                                      << circuitsWithStepFailure << ", Konvergenzereignisse: " << convergenceEvents
                                      << ", zerstoerte Bauteile: " << destroyedEvents);
    CHECK(kirchhoffViolations == 0);
    CHECK(opFailures <= kCircuits / 200);             // hoechstens 0,5 % (Stand Phase 0: 0)
    CHECK(circuitsWithStepFailure <= kCircuits / 200); // hoechstens 0,5 % (Stand Phase 0: 0)
    CHECK(destroyedEvents > 0);                        // die Zufallsaufbauten ueberlasten auch Bauteile
}
