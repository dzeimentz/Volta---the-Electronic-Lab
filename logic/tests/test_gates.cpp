// Grundgatter mit unbestimmten Pegeln, Einheitsverzoegerung, Stoerimpuls, Speicher und Schwingen.
#include <catch2/catch_test_macros.hpp>

#include "chip_builders.hpp"
#include "volta/logic/logic_sim.hpp"
#include "volta/logic/netlist.hpp"

using namespace volta::logic;
using namespace volta::logic::test;

namespace {
constexpr Level O = Level::Zero, I = Level::One, X = Level::X;
}

TEST_CASE("Grundgatter: Wahrheitstabellen und unbestimmte Eingaenge", "[logik][gatter]") {
    CHECK(evaluate(Primitive::Nand, I, I) == O);
    CHECK(evaluate(Primitive::Nand, O, I) == I);
    CHECK(evaluate(Primitive::Nand, O, X) == I); // eine 0 entscheidet
    CHECK(evaluate(Primitive::Nand, I, X) == X);
    CHECK(evaluate(Primitive::And, X, O) == O);
    CHECK(evaluate(Primitive::Or, X, I) == I);
    CHECK(evaluate(Primitive::Or, X, O) == X);
    CHECK(evaluate(Primitive::Xor, I, O) == I);
    CHECK(evaluate(Primitive::Xor, I, I) == O);
    CHECK(evaluate(Primitive::Xor, X, O) == X);
    CHECK(evaluate(Primitive::Not, O, O) == I);
    CHECK(evaluate(Primitive::Not, X, X) == X);
    CHECK(toChar(X) == '?');
    CHECK(primitiveFromName("NAND") == Primitive::Nand);
    CHECK_FALSE(primitiveFromName("XODER"));
}

TEST_CASE("Unverbundener Eingang bleibt unbestimmt", "[logik][gatter]") {
    const ChipDef d = chip("T", {"A"}, {"Y"}, {{"n1", "NAND"}}, {{"A", "n1.a"}, {"n1.y", "Y"}});
    Netlist net;
    REQUIRE(elaborate(d, Library{}, net).ok());
    LogicSim sim;
    sim.load(net);
    sim.setInput(0, I);
    REQUIRE(sim.settle() == SettleStatus::Stable);
    CHECK(sim.output(0) == X);
    sim.setInput(0, O); // 0 am angeschlossenen Eingang entscheidet trotzdem
    sim.settle();
    CHECK(sim.output(0) == I);
}

TEST_CASE("Einheitsverzoegerung: A und nicht A erzeugt einen kurzen Stoerimpuls", "[logik][laufzeit]") {
    // Y = AND(A, NOT A) ist logisch immer 0; beim Wechsel 0 -> 1 ist der Inverter eine Laufzeit zu spaet
    const ChipDef d = chip("G", {"A"}, {"Y"}, {{"i1", "NOT"}, {"u1", "AND"}},
                           {{"A", "i1.a"}, {"A", "u1.a"}, {"i1.y", "u1.b"}, {"u1.y", "Y"}});
    Netlist net;
    REQUIRE(elaborate(d, Library{}, net).ok());
    LogicSim sim;
    sim.load(net);
    sim.setInput(0, O);
    sim.settle();
    REQUIRE(sim.output(0) == O);
    sim.setInput(0, I);
    bool sawPulse = false;
    while (sim.round()) sawPulse = sawPulse || sim.output(0) == I;
    CHECK(sawPulse);
    CHECK(sim.output(0) == O);
}

TEST_CASE("RS-Flipflop aus zwei NAND speichert", "[logik][speicher]") {
    // /S und /R aktiv 0; Q und /Q ueber Kreuz
    const ChipDef d = chip("RS", {"S", "R"}, {"Q", "NQ"}, {{"n1", "NAND"}, {"n2", "NAND"}},
                           {{"S", "n1.a"}, {"n2.y", "n1.b"}, {"R", "n2.a"}, {"n1.y", "n2.b"}, {"n1.y", "Q"},
                            {"n2.y", "NQ"}});
    Netlist net;
    REQUIRE(elaborate(d, Library{}, net).ok());
    LogicSim sim;
    sim.load(net);
    sim.setInput(0, I);
    sim.setInput(1, I);
    REQUIRE(sim.settle() == SettleStatus::Stable);
    CHECK(sim.output(0) == X); // nach dem Einschalten unbestimmt
    sim.setInput(0, O);        // setzen
    sim.settle();
    CHECK(sim.output(0) == I);
    sim.setInput(0, I); // loslassen: Zustand bleibt
    sim.settle();
    CHECK(sim.output(0) == I);
    CHECK(sim.output(1) == O);
    sim.setInput(1, O); // ruecksetzen
    sim.settle();
    sim.setInput(1, I);
    sim.settle();
    CHECK(sim.output(0) == O);
}

TEST_CASE("Rueckgekoppelter Inverter schwingt und wird erkannt", "[logik][schwingen]") {
    // Y = NAND(EN, NOT NOT Y): mit EN = 1 ungerade Zahl von Invertierungen im Kreis
    const ChipDef d = chip("OSZ", {"EN"}, {"Y"}, {{"n1", "NAND"}, {"i1", "NOT"}, {"i2", "NOT"}},
                           {{"EN", "n1.a"}, {"n1.y", "i1.a"}, {"i1.y", "i2.a"}, {"i2.y", "n1.b"}, {"n1.y", "Y"}});
    Netlist net;
    REQUIRE(elaborate(d, Library{}, net).ok());
    LogicSim sim;
    sim.load(net);
    sim.setInput(0, O);
    REQUIRE(sim.settle() == SettleStatus::Stable);
    CHECK(sim.output(0) == I);
    sim.setInput(0, I);
    CHECK(sim.settle(200) == SettleStatus::Oscillating);
}
