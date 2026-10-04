// Referenzschaltung 1 aus docs/testing.md: Spannungsteiler 9 V, 1 kOhm / 2 kOhm -> 6,000 V
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "volta/core/circuit.hpp"
#include "volta/core/version.hpp"

using namespace volta::core;

TEST_CASE("Spannungsteiler 9 V mit 1 kOhm und 2 kOhm", "[referenz][dc]") {
    Circuit c;
    const NodeId top = c.addNode();
    const NodeId mid = c.addNode();
    c.addVoltageSource({"B1", top, kGround, 9.0, 0.0});
    c.addResistor({"R1", top, mid, 1000.0});
    c.addResistor({"R2", mid, kGround, 2000.0});

    const DcSolution s = solveDc(c);
    REQUIRE(s.ok);
    CHECK(s.nodeVolt[top] == Catch::Approx(9.0).margin(1e-3));
    CHECK(s.nodeVolt[mid] == Catch::Approx(6.0).epsilon(0.02).margin(1e-3));
    // Quellenstrom: 9 V / 3 kOhm = 3 mA (MNA-Konvention: Strom in die Quelle hinein negativ)
    CHECK(std::abs(s.sourceCurrent[0]) == Catch::Approx(3e-3).epsilon(0.02));
}

TEST_CASE("Batterie mit Innenwiderstand unter Last", "[referenz][dc]") {
    Circuit c;
    const NodeId plus = c.addNode();
    c.addVoltageSource({"B1", plus, kGround, 9.0, 1.5});
    c.addResistor({"R1", plus, kGround, 100.0});
    const DcSolution s = solveDc(c);
    REQUIRE(s.ok);
    CHECK(s.nodeVolt[plus] == Catch::Approx(9.0 * 100.0 / 101.5).epsilon(0.02));
}

TEST_CASE("Offener Knoten bleibt loesbar (GMIN)", "[robust]") {
    Circuit c;
    const NodeId plus = c.addNode();
    const NodeId floating = c.addNode();
    c.addVoltageSource({"B1", plus, kGround, 9.0, 0.0});
    (void)floating;
    const DcSolution s = solveDc(c);
    REQUIRE(s.ok);
    CHECK(s.nodeVolt[floating] == Catch::Approx(0.0).margin(1e-6));
}

TEST_CASE("Kernversion ist gesetzt", "[info]") {
    CHECK(std::string(version()).size() > 0);
}
