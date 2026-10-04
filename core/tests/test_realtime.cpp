// Echtzeitregel aus core/CLAUDE.md: keine Speicheranforderung in Simulator::step().
// Der globale operator new wird fuer dieses Testprogramm ersetzt und zaehlt Anforderungen, solange
// die Zaehlung eingeschaltet ist.
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstdlib>
#include <new>
#include <string>

#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/wire.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/simulator.hpp"
#include "test_support.hpp"

namespace {
std::atomic<bool> g_counting{false};
std::atomic<long> g_allocations{0};
} // namespace

void* operator new(std::size_t size) {
    if (g_counting.load(std::memory_order_relaxed)) g_allocations.fetch_add(1, std::memory_order_relaxed);
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

using namespace volta::core;

TEST_CASE("Zaehler fuer Speicheranforderungen ist wirksam", "[echtzeit]") {
    g_allocations = 0;
    g_counting = true;
    auto* probe = new int(42);
    g_counting = false;
    delete probe;
    CHECK(g_allocations.load() == 1);
}

TEST_CASE("Simulator::step fordert keinen Speicher an", "[echtzeit]") {
    Circuit c;
    NodeId prev = c.addNode();
    c.add<Battery>("B1", prev, kGround);
    for (int i = 0; i < 20; ++i) { // Widerstandskette mit Abgriffen gegen Masse
        const NodeId next = c.addNode();
        c.add<Resistor>("R" + std::to_string(i), prev, next, ResistorParams{100.0});
        c.add<Resistor>("G" + std::to_string(i), next, kGround, ResistorParams{10000.0});
        prev = next;
    }
    const NodeId led = c.addNode();
    c.add<Resistor>("RV", prev, led, ResistorParams{100.0});
    c.add<Diode>("D1", led, kGround, DiodeParams::ledRed()); // nichtlinearer Pfad mit innerem Knoten
    const NodeId cap = c.addNode();
    c.add<Wire>("W1", prev, cap);
    c.add<Capacitor>("C1", cap, kGround, CapacitorParams::electrolytic(100e-6)); // Begleitmodell, Thermik
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);

    g_allocations = 0;
    g_counting = true;
    bool ok = true;
    for (int i = 0; i < 1000; ++i) ok = (sim.step() == Status::Ok) && ok;
    const Status op = sim.solveOperatingPoint();
    g_counting = false;

    CHECK(ok);
    CHECK(op == Status::Ok);
    CHECK(g_allocations.load() == 0);
}

TEST_CASE("Newton, Halbierung und Quellenrampe fordern keinen Speicher an", "[echtzeit][newton]") {
    using volta::core::test::StubbornResistor;
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<StubbornResistor>("X1", plus, kGround);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& stub = static_cast<StubbornResistor&>(sim.circuit().component(1));
    SimEvent e;

    g_allocations = 0;
    g_counting = true;
    stub.failIterations = kMaxNewtonIter + 3; // Halbierung, dann Erfolg
    const Status halved = sim.step();
    stub.failIterations = -1;                 // Halbierung bis 1/16 scheitert, Ereignis
    const Status failed = sim.step();
    stub.failIterations = kMaxNewtonIter + 3; // Arbeitspunkt nur ueber die Quellenrampe
    const Status ramped = sim.solveOperatingPoint();
    while (sim.events().pop(e)) {
    }
    g_counting = false;

    CHECK(halved == Status::Ok);
    CHECK(failed == Status::NotConverged);
    CHECK(ramped == Status::Ok);
    CHECK(g_allocations.load() == 0);
}
