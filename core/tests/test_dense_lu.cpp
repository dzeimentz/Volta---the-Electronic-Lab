// Dichte LU-Zerlegung: Loesung, Pivotsuche, Singularitaet, Wiederverwendung der Zerlegung.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "volta/core/dense_lu.hpp"

using namespace volta::core;
using Catch::Approx;

TEST_CASE("LU loest ein 3x3-System", "[lu]") {
    // 2x + y - z = 8; -3x - y + 2z = -11; -2x + y + 2z = -3  ->  x=2, y=3, z=-1
    DenseLu lu;
    lu.resize(3);
    REQUIRE(lu.factor({2, 1, -1, -3, -1, 2, -2, 1, 2}));
    std::vector<double> x(3);
    REQUIRE(lu.solve({8, -11, -3}, x));
    CHECK(x[0] == Approx(2.0));
    CHECK(x[1] == Approx(3.0));
    CHECK(x[2] == Approx(-1.0));
}

TEST_CASE("LU benoetigt Pivotsuche bei Null auf der Diagonale", "[lu]") {
    // Typisch fuer MNA: Zweigzeile einer Spannungsquelle hat 0 auf der Diagonale.
    DenseLu lu;
    lu.resize(2);
    REQUIRE(lu.factor({0, 1, 1, 0}));
    std::vector<double> x(2);
    REQUIRE(lu.solve({5, 7}, x));
    CHECK(x[0] == Approx(7.0));
    CHECK(x[1] == Approx(5.0));
}

TEST_CASE("LU erkennt singulaere Matrix", "[lu][robust]") {
    DenseLu lu;
    lu.resize(2);
    CHECK_FALSE(lu.factor({1, 2, 2, 4}));
    std::vector<double> x(2);
    CHECK_FALSE(lu.solve({1, 1}, x));
}

TEST_CASE("LU weist nicht endliche Eintraege zurueck", "[lu][robust]") {
    DenseLu lu;
    lu.resize(1);
    CHECK_FALSE(lu.factor({std::numeric_limits<double>::quiet_NaN()}));
}

TEST_CASE("LU-Zerlegung wird fuer mehrere rechte Seiten genutzt", "[lu]") {
    DenseLu lu;
    lu.resize(2);
    REQUIRE(lu.factor({4, 1, 1, 3}));
    std::vector<double> x(2);
    REQUIRE(lu.solve({1, 2}, x));
    CHECK(4 * x[0] + x[1] == Approx(1.0));
    CHECK(x[0] + 3 * x[1] == Approx(2.0));
    REQUIRE(lu.solve({5, 0}, x));
    CHECK(4 * x[0] + x[1] == Approx(5.0));
    CHECK(x[0] + 3 * x[1] == Approx(0.0).margin(1e-12));
}
