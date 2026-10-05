// Dreifach-Messgeraet: Referenzschaltung 7 (Belastung durch den Eingangswiderstand), Strommessung, Sicherung,
// Widerstandsmessung mit Fremdspannungserkennung.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/meter.hpp"
#include "volta/core/components/resistor.hpp"

using namespace volta::core;
using namespace volta::core::test;
using Catch::Approx;

namespace {

void runFor(Simulator& sim, double seconds) {
    const int steps = static_cast<int>(std::lround(seconds / kDefaultStepS));
    for (int i = 0; i < steps; ++i) REQUIRE(sim.step() == Status::Ok);
}

std::size_t countEvents(Simulator& sim, SimEventType type) {
    std::size_t n = 0;
    SimEvent e;
    while (sim.events().pop(e))
        if (e.type == type) ++n;
    return n;
}

// Spannungsteiler 9 V aus r/r, Messgeraet im V-Bereich am unteren Widerstand.
Circuit dividerWithMeter(double r, bool analog) {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId mid = c.addNode();
    const NodeId ampJack = c.addNode(); // A-Buchse ohne Messleitung
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, mid, ResistorParams{r});
    c.add<Resistor>("R2", mid, kGround, ResistorParams{r});
    Meter& m = c.add<Meter>("MESS", kGround, mid, ampJack);
    m.setAnalog(analog);
    return c;
}

double parallel(double a, double b) { return a * b / (a + b); }

} // namespace

TEST_CASE("Referenz 7: Messgeraet im V-Bereich (10 MOhm) belastet einen 1-MOhm-Teiler messbar", "[referenz][messgeraet]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(dividerWithMeter(1e6, false)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    const ComponentReading& m = sim.probe().readings[3];
    // Von Hand: 1 MOhm || 10 MOhm = 909,09 kOhm -> U = 9 V * 909,09 / (1000 + 909,09) = 4,2857 V statt 4,5 V
    const double low = parallel(1e6, 10e6);
    const double expected = 9.0 * low / (1e6 + low + 1.5);
    CHECK(m.displayValue == Approx(expected).epsilon(2e-3));
    CHECK(m.displayValue == Approx(4.2857).epsilon(2e-3));
    CHECK((4.5 - m.displayValue) / 4.5 > 0.04); // Belastung deutlich sichtbar
    CHECK(m.displayFlags == 0);
    requireKirchhoff(sim);
}

TEST_CASE("Messgeraet digital am niederohmigen Teiler: Belastung vernachlaessigbar", "[messgeraet]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(dividerWithMeter(1000.0, false)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[3].displayValue == Approx(4.5).epsilon(1e-3));
}

TEST_CASE("Messgeraet analog: 20 kOhm/V im selbsttaetig gewaehlten Bereich", "[messgeraet]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(dividerWithMeter(10000.0, true)) == Status::Ok);
    runFor(sim, 0.01);
    const auto& meter = static_cast<const Meter&>(sim.circuit().component(3));
    CHECK(meter.analogRangeV() == 10.0); // ca. 4,4 V -> 10-V-Bereich
    CHECK(meter.inputOhm() == Approx(200e3));
    // 10 kOhm || 200 kOhm = 9,524 kOhm -> U = 9 V * 9,524 / 19,524 = 4,390 V
    const double low = parallel(10000.0, 200e3);
    CHECK(sim.probe().readings[3].displayValue == Approx(9.0 * low / (10000.0 + low + 1.5)).epsilon(2e-3));
    CHECK(sim.probe().readings[3].level == Approx(sim.probe().readings[3].displayValue / 10.0)); // Zeigerausschlag
    requireKirchhoff(sim);
}

TEST_CASE("Strommessung in Reihe: Anzeige gleich LED-Strom", "[messgeraet]") {
    // +9V -> 470 Ohm -> A-Buchse | COM -> LED -> Masse
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId a = c.addNode();
    const NodeId com = c.addNode();
    const NodeId vo = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Resistor>("R1", plus, a, ResistorParams{470.0});
    c.add<Meter>("MESS", com, vo, a).setMode(MeterMode::Ampere);
    c.add<Diode>("D1", com, kGround, DiodeParams::ledRed());
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    const double iLed = sim.probe().readings[3].currentA;
    CHECK(iLed == Approx(15.14e-3).epsilon(2e-3)); // Shunt 0,1 Ohm aendert praktisch nichts
    CHECK(sim.probe().readings[2].displayValue == Approx(iLed).epsilon(1e-6));
    CHECK(sim.probe().readings[2].displayFlags == 0);
    requireKirchhoff(sim);
}

TEST_CASE("Messgeraet im A-Bereich parallel zur Batterie: Sicherung loest aus", "[messgeraet][grenzwert]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId vo = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Meter>("MESS", kGround, vo, plus).setMode(MeterMode::Ampere);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[1].displayValue == Approx(9.0 / 1.6).epsilon(1e-3)); // 5,6 A
    CHECK(sim.probe().readings[1].overloaded);
    runFor(sim, 0.02);
    const auto& meter = static_cast<const Meter&>(sim.circuit().component(1));
    CHECK(meter.isFuseBlown());
    CHECK(countEvents(sim, SimEventType::FuseBlown) == 1);
    CHECK(sim.probe().readings[1].displayFlags & kDisplayFuseBlown);
    CHECK(std::fabs(sim.probe().readings[1].displayValue) < 1e-6);
    CHECK(std::fabs(sim.probe().readings[0].currentA) < 1e-6);
    requireKirchhoff(sim);
}

TEST_CASE("Undo stellt die Sicherung wieder her", "[messgeraet][undo]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId vo = c.addNode();
    c.add<Battery>("B1", plus, kGround);
    c.add<Meter>("MESS", kGround, vo, plus).setMode(MeterMode::Ampere);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    const ComponentState intact = sim.circuit().component(1).saveState();
    runFor(sim, 0.05);
    REQUIRE(static_cast<const Meter&>(sim.circuit().component(1)).isFuseBlown());
    sim.circuit().component(1).restoreState(intact);
    REQUIRE(sim.solveOperatingPoint() == Status::Ok);
    CHECK_FALSE(static_cast<const Meter&>(sim.circuit().component(1)).isFuseBlown());
}

TEST_CASE("Messgeraet analog im A- und Ohm-Bereich: Zeigerausschlag und Bereich", "[messgeraet]") {
    // A: +9V -> 470 Ohm -> A-Buchse, COM -> Masse: ca. 19 mA -> Bereich 30 mA
    {
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId a = c.addNode();
        const NodeId vo = c.addNode();
        c.add<Battery>("B1", plus, kGround);
        c.add<Resistor>("R1", plus, a, ResistorParams{470.0});
        Meter& m = c.add<Meter>("MESS", kGround, vo, a);
        m.setMode(MeterMode::Ampere);
        m.setAnalog(true);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        REQUIRE(sim.step() == Status::Ok);
        const ComponentReading& r = sim.probe().readings[2];
        CHECK(r.displayRange == 30e-3);
        CHECK(r.level == Approx(r.displayValue / 30e-3).epsilon(1e-9));
        CHECK(r.level == Approx(9.0 / 471.6 / 30e-3).epsilon(2e-2));
        requireKirchhoff(sim);
    }
    // Ohm: Zeiger-Ohmmeter, Skalenmitte in Zehnerstufen; 0 Ohm rechts, offen links
    for (double rx : {47.0, 470.0, 3300.0, 1e6}) {
        Circuit c;
        const NodeId vo = c.addNode();
        const NodeId ampJack = c.addNode();
        c.add<Resistor>("R1", vo, kGround, ResistorParams{rx});
        Meter& m = c.add<Meter>("MESS", kGround, vo, ampJack);
        m.setMode(MeterMode::Ohm);
        m.setAnalog(true);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        REQUIRE(sim.step() == Status::Ok);
        const ComponentReading& r = sim.probe().readings[1];
        INFO(rx << " Ohm");
        const double center = std::pow(10.0, std::round(std::log10(rx)));
        CHECK(r.displayRange == Approx(center));
        CHECK(r.level == Approx(center / (center + rx)).epsilon(5e-3));
        CHECK(r.level > 0.2);
        CHECK(r.level < 0.8);
    }
}

TEST_CASE("Widerstandsmessung: Anzeige R, offen OL, unter Spannung Warnung", "[messgeraet]") {
    for (double r : {10.0, 470.0, 10e3, 1e6}) {
        Circuit c;
        const NodeId vo = c.addNode();
        const NodeId ampJack = c.addNode();
        c.add<Resistor>("R1", vo, kGround, ResistorParams{r});
        c.add<Meter>("MESS", kGround, vo, ampJack).setMode(MeterMode::Ohm);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        REQUIRE(sim.step() == Status::Ok);
        INFO(r << " Ohm");
        // GMIN (1 GOhm) parallel zum Messobjekt verfaelscht 1 MOhm um 0,1 %
        CHECK(sim.probe().readings[1].displayValue == Approx(r).epsilon(2e-3));
        CHECK(sim.probe().readings[1].displayFlags == 0);
        CHECK(sim.probe().readings[0].currentA <= 1e-3 + 1e-9); // Messstrom hoechstens 1 mA
        requireKirchhoff(sim);
    }
    {
        Circuit c;
        const NodeId vo = c.addNode();
        const NodeId ampJack = c.addNode();
        c.add<Meter>("MESS", kGround, vo, ampJack).setMode(MeterMode::Ohm);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        REQUIRE(sim.step() == Status::Ok);
        CHECK(sim.probe().readings[0].displayFlags & kDisplayOverrange);
    }
    {
        // Widerstand in einer eingeschalteten Schaltung messen: fremde Spannung
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId mid = c.addNode();
        const NodeId ampJack = c.addNode();
        c.add<Battery>("B1", plus, kGround);
        c.add<Resistor>("R1", plus, mid, ResistorParams{1000.0});
        c.add<Resistor>("R2", mid, kGround, ResistorParams{1000.0});
        c.add<Meter>("MESS", mid, plus, ampJack).setMode(MeterMode::Ohm); // ueber R1
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        REQUIRE(sim.step() == Status::Ok);
        CHECK(sim.probe().readings[3].displayFlags & kDisplayForeignVoltage);
        requireKirchhoff(sim);
    }
}

TEST_CASE("Messart umschalten ist eine Bedienhandlung ohne Topologiewechsel", "[messgeraet]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(dividerWithMeter(1000.0, false)) == Status::Ok);
    auto& meter = static_cast<Meter&>(sim.circuit().component(3));
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[3].displayValue == Approx(4.5).epsilon(1e-3));
    meter.setMode(MeterMode::Ohm); // Teiler steht unter Spannung
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[3].displayFlags & kDisplayForeignVoltage);
    meter.setMode(MeterMode::Volt);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[3].displayValue == Approx(4.5).epsilon(1e-3));
}

TEST_CASE("Sicherung: 0,45 A haelt, 0,6 A loest aus", "[messgeraet][grenzwert]") {
    for (double ohm : {9.0 / 0.45 - 1.6, 9.0 / 0.6 - 1.6}) {
        Circuit c;
        const NodeId plus = c.addNode();
        const NodeId a = c.addNode();
        const NodeId vo = c.addNode();
        c.add<Battery>("B1", plus, kGround);
        c.add<Resistor>("R1", plus, a, ResistorParams{ohm});
        c.add<Meter>("MESS", kGround, vo, a).setMode(MeterMode::Ampere);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        runFor(sim, 0.05);
        const double current = 9.0 / (ohm + 1.6);
        INFO(current << " A");
        CHECK(static_cast<const Meter&>(sim.circuit().component(2)).isFuseBlown() == (current > 0.5));
    }
}

TEST_CASE("Widerstandsmessung an einer LED: Durchlass ergibt einige kOhm, Sperrrichtung OL", "[messgeraet]") {
    for (bool forward : {true, false}) {
        Circuit c;
        const NodeId vo = c.addNode();
        const NodeId ampJack = c.addNode();
        if (forward)
            c.add<Diode>("D1", vo, kGround, DiodeParams::ledRed());
        else
            c.add<Diode>("D1", kGround, vo, DiodeParams::ledRed());
        c.add<Meter>("MESS", kGround, vo, ampJack).setMode(MeterMode::Ohm);
        Simulator sim;
        REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
        REQUIRE(sim.step() == Status::Ok);
        const ComponentReading& m = sim.probe().readings[1];
        INFO((forward ? "Durchlass: " : "Sperr: ") << m.displayValue << " Ohm, Flags " << m.displayFlags);
        if (forward) {
            // Messquelle 3 V / 3 kOhm, LED ca. 1,7 V bei ca. 0,43 mA -> R = U / I ca. 4 kOhm (nichtlinear!)
            CHECK(m.displayFlags == 0);
            CHECK(m.displayValue > 2000.0);
            CHECK(m.displayValue < 8000.0);
        } else {
            CHECK(m.displayFlags & kDisplayOverrange);
        }
        requireKirchhoff(sim);
    }
}

TEST_CASE("Ohm-Bereich: gleichgepolte Fremdspannung unter 3 V bleibt unbemerkt (dokumentierte Grenze)", "[messgeraet]") {
    // Thevenin-Quelle 1,5 V (eine Zelle) ueber 1 kOhm: Messwert verfaelscht, keine Warnung - wie bei realen Geraeten
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId vo = c.addNode();
    const NodeId ampJack = c.addNode();
    c.add<Battery>("B1", plus, kGround, BatteryParams{1});
    c.add<Resistor>("R1", plus, vo, ResistorParams{1000.0});
    c.add<Meter>("MESS", kGround, vo, ampJack).setMode(MeterMode::Ohm);
    Simulator sim;
    REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
    REQUIRE(sim.step() == Status::Ok);
    CHECK(sim.probe().readings[2].displayFlags == 0);
    CHECK(sim.probe().readings[2].displayValue > 1500.0); // statt 1000 Ohm
}

TEST_CASE("Analoges Messgeraet an hochohmiger Quelle: Bereichswahl verstaerkt die Belastung", "[messgeraet]") {
    Simulator sim;
    REQUIRE(sim.setCircuit(dividerWithMeter(1e6, true)) == Status::Ok);
    const auto& meter = static_cast<const Meter&>(sim.circuit().component(3));
    CHECK(meter.analogRangeV() == 30.0); // ohne Messwert: groesster Bereich (Ri 600 kOhm)
    runFor(sim, 0.01);
    // Im Gleichgewicht passt der Bereich zum Messwert, und der Messwert zum Bereich (Ri = 20 kOhm/V * Bereich)
    const double r = meter.analogRangeV();
    const double low = parallel(1e6, 20e3 * r);
    const double expected = 9.0 * low / (1e6 + low + 1.5);
    INFO("Bereich " << r << " V, Anzeige " << sim.probe().readings[3].displayValue << " V");
    CHECK(sim.probe().readings[3].displayValue == Approx(expected).epsilon(2e-3));
    CHECK(sim.probe().readings[3].displayValue < 1.0); // weit unter 4,5 V
    CHECK(std::fabs(sim.probe().readings[3].displayValue) <= 0.95 * r);
}
