// Signalstatistik je Messfenster: Gleichanteil, Effektivwert, Wechselerkennung und Frequenz.
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "volta/core/signal_stats.hpp"

using namespace volta::core;
using Catch::Approx;

namespace {
constexpr double kPi = 3.14159265358979323846;

// Kanal 0: Sinus amp, f, mit Gleichanteil dc; Kanal 1: konstant -3 V
void feed(SignalWindow& w, double amp, double f, double dc, double seconds, double dtS) {
    for (int k = 1; k <= static_cast<int>(std::lround(seconds / dtS)); ++k) {
        const double t = k * dtS;
        const double v[2] = {dc + amp * std::sin(2.0 * kPi * f * t), -3.0};
        w.add(v, 2, dtS);
    }
}
} // namespace

TEST_CASE("Signalstatistik: Sinus 50 Hz mit 1-ms-Schritten", "[signal]") {
    SignalWindow w;
    w.reset(2, 0.1);
    feed(w, 12.73, 50.0, 0.0, 0.3, 1e-3); // 9 V Effektivwert
    const SignalSummary& s = w.latest()[0];
    REQUIRE(s.valid);
    CHECK(std::fabs(s.mean) < 0.05);
    CHECK(s.acRms == Approx(9.0).epsilon(0.01));
    CHECK(s.isAc());
    CHECK(s.max == Approx(12.73).epsilon(0.01));
    CHECK(s.frequencyHz == Approx(50.0).epsilon(0.01));
    const SignalSummary& d = w.latest()[1];
    CHECK(d.mean == Approx(-3.0));
    CHECK(d.acRms < 1e-6);
    CHECK(!d.isAc());
    CHECK(d.frequencyHz == 0.0);
}

TEST_CASE("Signalstatistik: Gleichanteil ueberwiegt -> keine Wechselgroesse", "[signal]") {
    SignalWindow w;
    w.reset(2, 0.1);
    feed(w, 0.5, 100.0, 5.0, 0.3, 1e-3); // Brummen auf 5 V
    const SignalSummary& s = w.latest()[0];
    CHECK(s.mean == Approx(5.0).epsilon(0.01));
    CHECK(s.acRms == Approx(0.5 / std::sqrt(2.0)).epsilon(0.02));
    CHECK(!s.isAc());
    CHECK(s.frequencyHz == Approx(100.0).epsilon(0.02));
}

TEST_CASE("Signalstatistik: Fenster wird erst nach Ablauf veroeffentlicht", "[signal]") {
    SignalWindow w;
    w.reset(1, 0.1);
    const double v = 2.0;
    for (int k = 0; k < 99; ++k) w.add(&v, 1, 1e-3);
    CHECK(!w.latest()[0].valid);
    w.add(&v, 1, 1e-3);
    CHECK(w.latest()[0].valid);
    CHECK(w.latest()[0].mean == Approx(2.0));
}
