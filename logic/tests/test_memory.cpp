// Speicher der Lernstufe 4: Merker mit Rueckkopplung, Speicherbit, Taktbit mit Loeschen, Umschalter und Zaehler.
// Einschalten (powerOn) legt unbestimmte Speicher auf 0; geprueft wird als Ablauf Schritt fuer Schritt.
#include <catch2/catch_test_macros.hpp>

#include "chip_builders.hpp"
#include "volta/logic/logic_sim.hpp"
#include "volta/logic/netlist.hpp"

using namespace volta::logic;
using namespace volta::logic::test;

namespace {
constexpr Level O = Level::Zero, I = Level::One, X = Level::X;

// Merker: Q = (S oder Q) und nicht R
ChipDef merker() {
    return chip("MERKER", {"S", "R"}, {"Q"}, {{"o1", "OR"}, {"n1", "NOT"}, {"a1", "AND"}},
                {{"S", "o1.a"}, {"a1.y", "o1.b"}, {"R", "n1.a"}, {"o1.y", "a1.a"}, {"n1.y", "a1.b"}, {"a1.y", "Q"}});
}

// Speicherbit: bei M = 1 uebernimmt Q den Wert von D, sonst bleibt Q
ChipDef speicherbit() {
    return chip("SBIT", {"D", "M"}, {"Q"}, {{"a1", "AND"}, {"n1", "NOT"}, {"a2", "AND"}, {"m1", "MERKER"}},
                {{"D", "a1.a"}, {"M", "a1.b"}, {"a1.y", "m1.S"}, {"D", "n1.a"}, {"n1.y", "a2.a"}, {"M", "a2.b"},
                 {"a2.y", "m1.R"}, {"m1.Q", "Q"}});
}

// Taktbit: zwei Speicherbits hintereinander, das erste offen solange T = 0, das zweite beim Takt; Loeschen oeffnet
// beide und gibt beiden eine 0 (sonst uebernaehme das zweite beim Loslassen einen alten Wert, wenn T noch anliegt)
ChipDef taktbit() {
    return chip("TBIT", {"D", "T", "L"}, {"Q"},
                {{"n1", "NOT"}, {"o1", "OR"}, {"n2", "NOT"}, {"a1", "AND"}, {"o2", "OR"}, {"a2", "AND"},
                 {"s1", "SBIT"}, {"s2", "SBIT"}},
                {{"T", "n1.a"}, {"n1.y", "o2.a"}, {"L", "o2.b"}, {"o2.y", "s1.M"}, {"T", "o1.a"}, {"L", "o1.b"},
                 {"o1.y", "s2.M"}, {"L", "n2.a"}, {"D", "a1.a"}, {"n2.y", "a1.b"}, {"a1.y", "s1.D"},
                 {"s1.Q", "a2.a"}, {"n2.y", "a2.b"}, {"a2.y", "s2.D"}, {"s2.Q", "Q"}});
}

// Umschalter: Taktbit, das sein eigenes Gegenteil uebernimmt
ChipDef umschalter() {
    return chip("TFF", {"T", "L"}, {"Q"}, {{"b1", "TBIT"}, {"n1", "NOT"}},
                {{"b1.Q", "n1.a"}, {"n1.y", "b1.D"}, {"T", "b1.T"}, {"L", "b1.L"}, {"b1.Q", "Q"}});
}

// Zaehler mit vier Umschaltern: die naechste Stelle springt, wenn die vorige von 1 auf 0 faellt
ChipDef zaehler() {
    return chip("ZAEHLER", {"T", "L"}, {"Q3", "Q2", "Q1", "Q0"},
                {{"t0", "TFF"}, {"t1", "TFF"}, {"t2", "TFF"}, {"t3", "TFF"}, {"n0", "NOT"}, {"n1", "NOT"},
                 {"n2", "NOT"}},
                {{"T", "t0.T"}, {"t0.Q", "n0.a"}, {"n0.y", "t1.T"}, {"t1.Q", "n1.a"}, {"n1.y", "t2.T"},
                 {"t2.Q", "n2.a"}, {"n2.y", "t3.T"}, {"t0.Q", "Q0"}, {"t1.Q", "Q1"}, {"t2.Q", "Q2"}, {"t3.Q", "Q3"},
                 {"L", "t0.L"}, {"L", "t1.L"}, {"L", "t2.L"}, {"L", "t3.L"}});
}

Library memoryLibrary() {
    Library lib;
    lib.add(merker());
    lib.add(speicherbit());
    lib.add(taktbit());
    lib.add(umschalter());
    return lib;
}

SequenceStep step(std::initializer_list<Level> in, std::initializer_list<Level> out) { return {in, out}; }
} // namespace

TEST_CASE("Merker: Einschalten legt den Speicher auf 0, Ausgang zurueck an den Eingang merkt sich das Klingeln",
          "[logik][speicher]") {
    Netlist net;
    REQUIRE(elaborate(merker(), Library{}, net).ok());
    LogicSim sim;
    sim.load(net);
    sim.setInput(0, O);
    sim.setInput(1, O);
    sim.settle();
    CHECK(sim.output(0) == X); // ohne Einschalten unbestimmt
    CHECK(sim.powerOn() > 0);
    CHECK(sim.output(0) == O);
    const CheckResult r = checkSequence(net, {step({O, I}, {O}), step({I, O}, {I}), step({O, O}, {I}),
                                              step({O, I}, {O}), step({O, O}, {O})});
    CHECK(r.passed);
}

TEST_CASE("Offener Eingang bleibt auch nach dem Einschalten sichtbar", "[logik][speicher]") {
    const ChipDef d = chip("UND", {"A", "B"}, {"Y"}, {{"a1", "AND"}}, {{"A", "a1.a"}, {"a1.y", "Y"}});
    Netlist net;
    REQUIRE(elaborate(d, Library{}, net).ok());
    LogicSim sim;
    sim.load(net);
    sim.setInput(0, I);
    CHECK(sim.powerOn() == 0); // keine Rueckkopplung: nichts wird festgelegt
    CHECK(sim.output(0) == X);
}

TEST_CASE("Speicherbit uebernimmt D nur beim Merken", "[logik][speicher]") {
    Netlist net;
    REQUIRE(elaborate(speicherbit(), memoryLibrary(), net).ok());
    const CheckResult r = checkSequence(net, {step({I, I}, {I}), step({I, O}, {I}), step({O, O}, {I}),
                                              step({O, I}, {O}), step({I, O}, {O}), step({I, I}, {I})});
    CHECK(r.passed);
}

TEST_CASE("Taktbit uebernimmt nur im Moment des Takts, Loeschen gewinnt", "[logik][speicher]") {
    Netlist net;
    REQUIRE(elaborate(taktbit(), memoryLibrary(), net).ok());
    const CheckResult r = checkSequence(net, {step({I, O, O}, {O}), step({I, I, O}, {I}), step({O, I, O}, {I}),
                                              step({O, O, O}, {I}), step({O, O, I}, {O}), step({O, O, O}, {O}),
                                              step({I, O, O}, {O}), step({I, I, O}, {I}), step({I, O, O}, {I}),
                                              step({I, O, I}, {O})});
    CHECK(r.passed);
}

TEST_CASE("Umschalter wechselt bei jedem Druck und haelt beim Loslassen", "[logik][speicher]") {
    Netlist net;
    REQUIRE(elaborate(umschalter(), memoryLibrary(), net).ok());
    const CheckResult r = checkSequence(net, {step({O, O}, {O}), step({I, O}, {I}), step({O, O}, {I}),
                                              step({I, O}, {O}), step({O, O}, {O}), step({I, O}, {I}),
                                              step({O, O}, {I}), step({O, I}, {O})});
    CHECK(r.passed);
    CHECK(!r.oscillating);
}

TEST_CASE("Zaehler aus vier Umschaltern zaehlt bis 15, beginnt wieder bei 0 und laesst sich loeschen", "[logik][speicher]") {
    Netlist net;
    REQUIRE(elaborate(zaehler(), memoryLibrary(), net).ok());
    std::vector<SequenceStep> steps;
    for (int k = 1; k <= 17; ++k) {
        const int v = k % 16;
        std::vector<Level> out;
        for (int b = 3; b >= 0; --b) out.push_back(((v >> b) & 1) ? I : O);
        steps.push_back({{I, O}, out});
        steps.push_back({{O, O}, out});
    }
    steps.push_back({{O, I}, {O, O, O, O}}); // Loeschen
    steps.push_back({{O, O}, {O, O, O, O}});
    steps.push_back({{I, O}, {O, O, O, I}});
    const CheckResult r = checkSequence(net, steps);
    CHECK(r.passed);
}
