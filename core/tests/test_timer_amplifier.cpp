// Zeitmodul (monostabile Kippstufe) und Verstaerkermodul (Darlington) als Teilschaltungen; Feuchtesonde.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/sensors.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/modules/timer_module.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {
// Batterie, Modul, LED mit 470 Ohm von plus nach aus; Taster bzw. Widerstand an den Eingang
struct Bench {
    Simulator sim;
    std::size_t led = 0;
    Switch* button = nullptr;
    void run(double seconds) {
        const int steps = static_cast<int>(std::lround(seconds / kDefaultStepS));
        for (int k = 0; k < steps; ++k) {
            const Status st = sim.step();
            INFO("t = " << sim.timeS());
            REQUIRE(st == Status::Ok);
            if (k % 23 == 0) requireKirchhoff(sim);
        }
    }
    double ledmA() const { return sim.probe().readings[led].currentA * 1e3; }
};
} // namespace

TEST_CASE("Zeitmodul: Impuls an start schaltet den Ausgang fuer ca. 3,2 s ein", "[modul][zeit]") {
    Bench b;
    Circuit c;
    const NodeId plus = c.addNode(), start = c.addNode(), out = c.addNode(), led = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Switch>("S1", plus, start, false);
    const SubcircuitRange r = addTimerModule(c, "ZM1", {plus, kGround, start, out});
    REQUIRE(r.count == 11);
    c.add<Resistor>("RV", plus, led, ResistorParams{470.0});
    b.led = c.componentCount();
    c.add<Diode>("D1", led, out, DiodeParams::ledRed());
    REQUIRE(b.sim.setCircuit(std::move(c)) == Status::Ok);
    b.button = &static_cast<Switch&>(b.sim.circuit().component(1));

    b.run(3.0); // Ruhe: Ausgang aus (auch nach dem Einschalten)
    CHECK(b.ledmA() < 0.5);
    b.button->setClosed(true); // kurzer Druck
    b.run(0.1);
    b.button->setClosed(false);
    CHECK(b.ledmA() > 10.0);
    // ein- und ausgeschaltet nach ca. 0,69 * 47 kOhm * 100 uF = 3,24 s (Toleranz fuer U_BE und Kippverhalten)
    double onFor = 0.1;
    while (b.ledmA() > 5.0 && onFor < 10.0) {
        b.run(0.05);
        onFor += 0.05;
    }
    INFO("eingeschaltet " << onFor << " s");
    CHECK(onFor > 2.6);
    CHECK(onFor < 4.0);
    b.run(1.0);
    CHECK(b.ledmA() < 0.5); // wieder aus und bleibt aus
    for (const auto& reading : b.sim.probe().readings) CHECK_FALSE(reading.destroyed);
}

TEST_CASE("Verstaerkermodul: wenige Mikroampere am Eingang schalten die LED", "[modul][verstaerker]") {
    auto ledWith = [](double inputOhm) {
        Bench b;
        Circuit c;
        const NodeId plus = c.addNode(), in = c.addNode(), out = c.addNode(), led = c.addNode();
        c.add<Battery>("B1", plus, kGround);
        if (inputOhm > 0.0) c.add<Resistor>("RE", plus, in, ResistorParams{inputOhm});
        addAmplifierModule(c, "VM1", {plus, kGround, in, out});
        c.add<Resistor>("RV", plus, led, ResistorParams{470.0});
        b.led = c.componentCount();
        c.add<Diode>("D1", led, out, DiodeParams::ledRed());
        REQUIRE(b.sim.setCircuit(std::move(c)) == Status::Ok);
        b.run(0.1);
        return b.ledmA();
    };
    CHECK(ledWith(0.0) < 0.01);      // Eingang offen: aus
    CHECK(ledWith(1.0e6) > 12.0);    // ca. 7 uA (z. B. durch Wasser): voll an
    CHECK(ledWith(100.0e6) < 2.0);   // 70 nA: kaum
}

TEST_CASE("Feuchtesonde: trocken offen, nass leitend, trocknet langsam", "[sensor]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<MoistureProbe>("F1", plus, kGround);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    auto& f = static_cast<MoistureProbe&>(sim.circuit().component(1));
    for (int k = 0; k < 100; ++k) REQUIRE(sim.step() == Status::Ok);
    CHECK(f.resistanceOhm() == Approx(2.0e7));
    f.setWetness(1.0);
    for (int k = 0; k < 2000; ++k) REQUIRE(sim.step() == Status::Ok);
    CHECK(f.resistanceOhm() == Approx(2.0e4).epsilon(0.01));
    requireKirchhoff(sim);
    f.setWetness(0.0);
    for (int k = 0; k < 2000; ++k) REQUIRE(sim.step() == Status::Ok); // 2 s: noch feucht (tau 4 s)
    CHECK(f.resistanceOhm() < 1.0e6);
    CHECK(f.resistanceOhm() > 2.0e4);
}
