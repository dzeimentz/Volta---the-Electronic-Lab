// Ereignis-Ringpuffer und Kernversion.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "volta/core/sim_event.hpp"
#include "volta/core/version.hpp"

using namespace volta::core;

TEST_CASE("Ereignisse kommen in Reihenfolge heraus", "[events]") {
    EventQueue q;
    q.reserve(4);
    for (std::size_t i = 0; i < 3; ++i) REQUIRE(q.push({SimEventType::Destroyed, i, 0.1 * double(i)}));
    SimEvent e;
    for (std::size_t i = 0; i < 3; ++i) {
        REQUIRE(q.pop(e));
        CHECK(e.component == i);
    }
    CHECK_FALSE(q.pop(e));
}

TEST_CASE("Voller Ereignispuffer verwirft und zaehlt", "[events][robust]") {
    EventQueue q;
    q.reserve(2);
    CHECK(q.push({}));
    CHECK(q.push({}));
    CHECK_FALSE(q.push({}));
    CHECK(q.size() == 2);
    CHECK(q.dropped() == 1);
    SimEvent e;
    REQUIRE(q.pop(e));
    CHECK(q.push({SimEventType::FuseBlown, 7, 1.0})); // Ringpuffer laeuft um
    REQUIRE(q.pop(e));
    REQUIRE(q.pop(e));
    CHECK(e.type == SimEventType::FuseBlown);
    CHECK(e.component == 7);
}

TEST_CASE("Kernversion ist gesetzt", "[info]") { CHECK(std::string(version()).size() > 0); }
