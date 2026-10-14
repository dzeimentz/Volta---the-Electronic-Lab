// Chips aus Chips: Aufloesung, Hierarchie fuer »Blick hinein«, Pruefung mit Wahrheitstabelle, Fehler.
#include <catch2/catch_test_macros.hpp>

#include "chip_builders.hpp"
#include "volta/logic/logic_sim.hpp"
#include "volta/logic/netlist.hpp"

using namespace volta::logic;
using namespace volta::logic::test;

namespace {
constexpr Level O = Level::Zero, I = Level::One, X = Level::X;

std::vector<std::vector<Level>> tableOf(const ChipDef& d, const Library& lib) {
    Netlist net;
    REQUIRE(elaborate(d, lib, net).ok());
    return truthTable(net);
}
} // namespace

TEST_CASE("Stufen 1 bis 3: NICHT, UND, ODER, XODER und Halbaddierer aus NAND", "[logik][chip]") {
    const Library lib = stageLibrary();
    CHECK(tableOf(notFromNand(), lib) == std::vector<std::vector<Level>>{{I}, {O}});
    CHECK(tableOf(andFromNand(), lib) == std::vector<std::vector<Level>>{{O}, {O}, {O}, {I}});
    CHECK(tableOf(orFromNand(), lib) == std::vector<std::vector<Level>>{{O}, {I}, {I}, {I}});
    CHECK(tableOf(xorFromNand(), lib) == std::vector<std::vector<Level>>{{O}, {I}, {I}, {O}});
    // Halbaddierer: S, C
    CHECK(tableOf(halfAdder(), lib) == std::vector<std::vector<Level>>{{O, O}, {I, O}, {I, O}, {O, I}});
}

TEST_CASE("Halbaddierer ist bis zum NAND aufgeloest, die Hierarchie bleibt erhalten", "[logik][chip]") {
    const Library lib = stageLibrary();
    Netlist net;
    REQUIRE(elaborate(halfAdder(), lib, net).ok());
    // XODER: 4 NAND; UND: 1 NAND + NICHT (1 NAND) = 6 Gatter, keine Durchverbindungen
    CHECK(net.gates.size() == 6);
    for (const Gate& g : net.gates) CHECK(g.kind == Primitive::Nand);
    REQUIRE(net.scopes.size() == 4); // HA, XODER, UND, NICHT im UND
    const Scope& top = net.scopes[0];
    CHECK(top.kuerzel == "HA");
    const Scope& inner = net.scopes.at(top.children.at("x1"));
    CHECK(inner.kuerzel == "XODER");
    // Blick hinein: Anschluss A im XODER ist dieselbe Leitung wie A des Halbaddierers
    CHECK(inner.endpoints.at("A") == top.endpoints.at("A"));
    const Scope& und = net.scopes.at(top.children.at("u1"));
    CHECK(net.scopes.at(und.children.at("i1")).kuerzel == "NICHT");

    LogicSim sim;
    sim.load(net);
    sim.setInput(0, I);
    sim.setInput(1, O);
    sim.settle();
    CHECK(sim.net(inner.endpoints.at("n1.y")) == I); // NAND(1, 0) im Inneren
    CHECK(sim.output(0) == I);
}

TEST_CASE("Pruefung markiert falsche Zeilen", "[logik][pruefung]") {
    const Library lib = stageLibrary();
    Netlist net;
    REQUIRE(elaborate(orFromNand(), lib, net).ok());
    // Aufgabe war XODER: Zeile 11 weicht ab
    const CheckResult r = checkTruthTable(net, {{O}, {I}, {I}, {O}});
    CHECK_FALSE(r.passed);
    REQUIRE(r.rows.size() == 4);
    CHECK(r.rows[0].ok);
    CHECK(r.rows[2].ok);
    CHECK_FALSE(r.rows[3].ok);
    CHECK(r.rows[3].inputs == std::vector<Level>{I, I});
    CHECK(r.rows[3].actual == std::vector<Level>{I});

    REQUIRE(elaborate(xorFromNand(), lib, net).ok());
    CHECK(checkTruthTable(net, {{O}, {I}, {I}, {O}}).passed);
}

TEST_CASE("Unvollstaendiger Aufbau besteht die Pruefung nicht", "[logik][pruefung]") {
    const ChipDef d = chip("NICHT", {"A"}, {"Y"}, {{"n1", "NAND"}}, {{"A", "n1.a"}, {"n1.y", "Y"}});
    Netlist net;
    REQUIRE(elaborate(d, Library{}, net).ok());
    const CheckResult r = checkTruthTable(net, {{I}, {O}});
    CHECK_FALSE(r.passed);
    CHECK(r.rows[0].actual == std::vector<Level>{I}); // 0 entscheidet
    CHECK(r.rows[1].actual == std::vector<Level>{X}); // b offen
}

TEST_CASE("Durchverbindung und mehrfach genutzter Ausgang", "[logik][chip]") {
    Library lib;
    lib.add(chip("DURCH", {"A"}, {"Y", "Z"}, {}, {{"A", "Y"}, {"A", "Z"}}));
    const ChipDef outer = chip("AUSSEN", {"E"}, {"P", "Q"}, {{"d1", "DURCH"}},
                               {{"E", "d1.A"}, {"d1.Y", "P"}, {"d1.Z", "Q"}});
    CHECK(tableOf(outer, lib) == std::vector<std::vector<Level>>{{O, O}, {I, I}});
}

TEST_CASE("Fehler beim Aufloesen", "[logik][fehler]") {
    Library lib = stageLibrary();
    Netlist net;
    SECTION("unbekannter Typ") {
        const auto r = elaborate(chip("T", {"A"}, {"Y"}, {{"z1", "GIBTSNICHT"}}, {}), lib, net);
        CHECK(r.error == ElaborateError::UnknownType);
        CHECK(r.where == "GIBTSNICHT");
    }
    SECTION("unbekannter Anschluss") {
        const auto r = elaborate(chip("T", {"A"}, {"Y"}, {{"n1", "NAND"}}, {{"A", "n1.c"}}), lib, net);
        CHECK(r.error == ElaborateError::UnknownEndpoint);
        CHECK(r.where == "n1.c");
    }
    SECTION("zwei Ausgaenge auf einer Leitung") {
        const auto r = elaborate(chip("T", {"A"}, {"Y"}, {{"n1", "NAND"}, {"n2", "NAND"}}, {{"n1.y", "n2.y"}}),
                                 lib, net);
        CHECK(r.error == ElaborateError::MultipleDrivers);
    }
    SECTION("Eingang auf Ausgang eines Gatters") {
        const auto r = elaborate(chip("T", {"A"}, {"Y"}, {{"n1", "NAND"}}, {{"A", "n1.y"}}), lib, net);
        CHECK(r.error == ElaborateError::MultipleDrivers);
    }
    SECTION("Chip enthaelt sich selbst") {
        lib.add(chip("SELBST", {"A"}, {"Y"}, {{"s1", "SELBST"}}, {{"A", "s1.A"}, {"s1.Y", "Y"}}));
        const auto r = elaborate(*lib.find("SELBST"), lib, net);
        CHECK(r.error == ElaborateError::Recursion);
    }
    SECTION("Grundgatter-Kuerzel sind vergeben") {
        CHECK_FALSE(lib.add(chip("NAND", {"A"}, {"Y"}, {}, {})));
    }
}
