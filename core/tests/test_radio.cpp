// Radio (Volta 3, Konzept Kap. 4): Abstimmen ueber die Wechselstromanalyse, gemittelter Huellkurvendetektor mit
// Belastung des Kreises. Referenz ist eine Rechnung mit echtem Traeger. Das Rueckwaertsverfahren des Kerns daempft
// einen Schwingkreis bei endlicher Schrittweite kuenstlich (Guete etwa 1 / (w dt)), deshalb rechnet die Referenz
// dieselbe Schaltung hier mit dem klassischen Runge-Kutta-Verfahren (4. Ordnung, praktisch ohne Daempfung).
#include <algorithm>
#include <cmath>
#include <vector>

#include "test_support.hpp"
#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/inductor.hpp"
#include "volta/core/components/radio.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/simulator.hpp"

using namespace volta::core;
using namespace volta::core::test;

namespace {
constexpr double kPi = 3.14159265358979323846;

// Detektorempfaenger: Antenne (1 kOhm) -> Koppelkondensator -> Schwingkreis (L || Drehkondensator) -> Ge-Diode ->
// Ladekondensator || Lastwiderstand
struct Receiver {
    double antOhm = 1e3, ckF = 10e-12, lH = 200e-6, lOhm = 3.0, lParOhm = 1e6, dkF = 340e-12;
    double cdF = 1e-9, rdOhm = 47e3;
};

struct Built {
    NodeId tank = 0, det = 0;
    std::size_t antenna = 0, detector = 0;
};

Circuit build(const Receiver& r, Built& b) {
    Circuit c;
    const NodeId a = c.addNode();
    b.tank = c.addNode();
    b.det = c.addNode();
    b.antenna = c.componentCount();
    c.add<AntennaFeed>("ANT", a, kGround, r.antOhm);
    c.add<Capacitor>("Ck", a, b.tank, CapacitorParams::film(r.ckF));
    c.add<Inductor>("L", b.tank, kGround, InductorParams{r.lH, r.lOhm, r.lParOhm, 0.1});
    c.add<Capacitor>("DK", b.tank, kGround, CapacitorParams::film(r.dkF));
    b.detector = c.componentCount();
    c.add<DetectorDiode>("D", b.tank, b.det, DiodeParams::germanium());
    c.add<Capacitor>("Cd", b.det, kGround, CapacitorParams::film(r.cdF));
    c.add<Resistor>("Rd", b.det, kGround, ResistorParams{r.rdOhm});
    return c;
}

// Referenz mit echtem Traeger: Zustandsgleichungen der Schaltung, RK4. Diodenstrom aus Shockley mit Bahnwiderstand
// (gleiche Kennwerte wie im Kern), Leckwiderstaende und GMIN wie im Kern.
struct Reference {
    Receiver r;
    double is = 0.0, nvt = 0.0, rs = 0.0;
    double emfV = 1.0, depth = 0.5, toneHz = 5000.0, carrierHz = 600e3;

    double diode(double v) const {
        // vd + rs * Is (exp(vd / nVt) - 1) = v
        double vd = std::min(v, 0.3);
        for (int k = 0; k < 60; ++k) {
            const double e = std::exp(std::min(vd / nvt, 80.0));
            const double f = vd + rs * is * (e - 1.0) - v, df = 1.0 + rs * is * e / nvt;
            const double s = f / df;
            vd -= s;
            if (std::fabs(s) < 1e-14) break;
        }
        return is * (std::exp(std::min(vd / nvt, 80.0)) - 1.0);
    }
    // Zustand: q = Spannung am Koppelkondensator (a - t), vt = Schwingkreis, il = Spulenstrom (durch L und RL), vd
    void deriv(double t, const double* s, double* ds) const {
        const double q = s[0], vt = s[1], il = s[2], vd = s[3];
        const double es = emfV * (1.0 + depth * std::sin(2.0 * kPi * toneHz * t)) * std::sin(2.0 * kPi * carrierHz * t);
        const double va = vt + q;
        const double ia = (es - va) / r.antOhm - kGmin * va;         // in Knoten a von der Antenne
        const double ick = ia;                                        // durch Ck (mit Leckwiderstand parallel)
        const double ickCap = ick - q / 1e9;
        const double id = diode(vt - vd);
        ds[0] = ickCap / r.ckF;
        ds[1] = (ick - il - vt / r.lParOhm - vt / 1e9 - id - kGmin * vt) / r.dkF;
        ds[2] = (vt - r.lOhm * il) / r.lH;
        ds[3] = (id - vd / r.rdOhm - vd / 1e9 - kGmin * vd) / r.cdF;
    }
};

// Mittelwert und Amplitude der Grundschwingung f (Projektion), Abtastung dt
std::pair<double, double> meanAndAmplitude(const std::vector<double>& v, double dt, double f, double t0) {
    double m = 0.0, s = 0.0, c = 0.0;
    for (std::size_t i = 0; i < v.size(); ++i) {
        const double ph = 2.0 * kPi * f * (t0 + dt * double(i));
        m += v[i];
        s += v[i] * std::sin(ph);
        c += v[i] * std::cos(ph);
    }
    const double n = double(v.size());
    return {m / n, 2.0 * std::hypot(s, c) / n};
}
} // namespace

TEST_CASE("Melodien spielen ihre Toene in der richtigen Reihenfolge", "[radio][volta3]") {
    // Grundfrequenz eines Tonabschnitts aus den Nulldurchgaengen
    auto pitchAt = [](StationProgram p, double t0) {
        int crossings = 0;
        double prev = stationSignal(p, 0.0, t0);
        for (int i = 1; i <= 1000; ++i) { // 0,1 s in 0,1 ms
            const double s = stationSignal(p, 0.0, t0 + i * 1e-4);
            if ((prev < 0.0) != (s < 0.0)) ++crossings;
            prev = s;
        }
        return crossings / 2.0 / 0.1;
    };
    // Volta-Melodie: G5 (784 Hz), dann C5 (523 Hz), Einheit 0,2 s
    CHECK(pitchAt(StationProgram::Melody, 0.1) == Catch::Approx(784.0).epsilon(0.03));
    CHECK(pitchAt(StationProgram::Melody, 0.5) == Catch::Approx(523.25).epsilon(0.03));
    // "O Holy Night": E5 (659 Hz) auf "O ho-", der Spitzenton C6 (1047 Hz) auf "shin-", Einheit 0,28 s
    CHECK(pitchAt(StationProgram::Carol, 0.6) == Catch::Approx(659.3).epsilon(0.03));
    CHECK(pitchAt(StationProgram::Carol, 25 * 0.28 - 1.0) == Catch::Approx(1046.5).epsilon(0.03));
    // Pause am Ende, dann von vorn (55 Einheiten)
    CHECK(stationSignal(StationProgram::Carol, 0.0, 52 * 0.28) == 0.0);
    CHECK(pitchAt(StationProgram::Carol, 0.6 + 55 * 0.28) == Catch::Approx(659.3).epsilon(0.03));
}

TEST_CASE("Sendersignale bleiben im Bereich und haben Pausen", "[radio][volta3]") {
    for (StationProgram p : {StationProgram::Tone, StationProgram::Morse, StationProgram::TimeSignal,
                             StationProgram::Melody, StationProgram::Carol}) {
        double lo = 0.0, hi = 0.0;
        int silent = 0;
        for (int i = 0; i < 200000; ++i) { // 20 s in 0,1 ms
            const double s = stationSignal(p, 800.0, i * 1e-4);
            REQUIRE(std::isfinite(s));
            lo = std::min(lo, s);
            hi = std::max(hi, s);
            if (s == 0.0) ++silent;
        }
        CHECK(lo >= -1.0);
        CHECK(hi <= 1.0);
        CHECK(hi > 0.99);
        if (p != StationProgram::Tone) CHECK(silent > 1000); // Morse, Zeitzeichen und Melodie haben Pausen
    }
}

TEST_CASE("Detektordiode ohne Traeger verhaelt sich wie die Diode", "[radio][volta3]") {
    // 1,5 V und 9 V ueber 1 kOhm, in Durchlass- und Sperrrichtung
    for (int cells : {1, 6})
        for (bool reverse : {false, true}) {
            double iRef = 0.0, iDet = 0.0;
            for (int kind = 0; kind < 2; ++kind) {
                Circuit c;
                const NodeId p = c.addNode(), a = c.addNode();
                if (reverse)
                    c.add<Battery>("B", kGround, p, BatteryParams{cells});
                else
                    c.add<Battery>("B", p, kGround, BatteryParams{cells});
                c.add<Resistor>("R", p, a, ResistorParams{1e3});
                if (kind == 0)
                    c.add<Diode>("D", a, kGround, DiodeParams::germanium());
                else
                    c.add<DetectorDiode>("D", a, kGround, DiodeParams::germanium());
                Simulator sim;
                REQUIRE(sim.setCircuit(std::move(c)) == Status::Ok);
                REQUIRE(sim.step(1e-3) == Status::Ok);
                (kind == 0 ? iRef : iDet) = sim.probe().readings[2].currentA;
                requireKirchhoff(sim);
            }
            INFO(cells << " Zellen, Sperrrichtung " << reverse << ": Diode " << iRef << " A, Detektordiode " << iDet);
            CHECK(std::fabs(iDet - iRef) <= 1e-4 * std::fabs(iRef) + 1e-9);
        }
}

TEST_CASE("Abstimmen: der Traeger an der Diode ist bei der Thomson-Frequenz am groessten", "[radio][volta3]") {
    Receiver r;
    const RadioStation st{1e6, 1.0, 0.5, StationProgram::Tone, 1000.0, true};
    double best = 0.0, bestC = 0.0;
    for (int k = 0; k <= 400; ++k) {
        r.dkF = 100e-12 + k * 0.5e-12; // 100 ... 300 pF
        Built b;
        Simulator sim;
        REQUIRE(sim.setCircuit(build(r, b)) == Status::Ok);
        REQUIRE(tuneRadio(sim, {st}) == Status::Ok);
        const auto& det = static_cast<const DetectorDiode&>(sim.circuit().component(b.detector));
        REQUIRE(det.driveCount() == 1);
        if (det.drive(0).e0V > best) {
            best = det.drive(0).e0V;
            bestC = r.dkF;
        }
    }
    // Resonanz mit Koppelkondensator (Antenne ueber 1 kOhm praktisch an Masse): C = DK + Ck
    const double f = 1.0 / (2.0 * kPi * std::sqrt(r.lH * (bestC + r.ckF)));
    CHECK(near(f, 1e6, 0.0));
    CHECK(best > 1.0); // Spannungsueberhoehung im Kreis

    // ohne Antenneneinspeisung oder ohne Sender: kein Traeger
    Built b;
    Simulator sim;
    REQUIRE(sim.setCircuit(build(r, b)) == Status::Ok);
    RadioStation off = st;
    off.on = false;
    REQUIRE(tuneRadio(sim, {off}) == Status::Ok);
    CHECK(static_cast<const DetectorDiode&>(sim.circuit().component(b.detector)).driveCount() == 0);
}

TEST_CASE("Drehkondensator: Verstellen und neu Abstimmen wie ein neu gebauter Kreis", "[radio][volta3]") {
    const RadioStation st{1e6, 1.0, 0.5, StationProgram::Tone, 1000.0, true};
    Receiver r;
    r.dkF = 150e-12;
    Built b;
    Simulator sim;
    REQUIRE(sim.setCircuit(build(r, b)) == Status::Ok);
    // Drehkondensator ist das vierte Bauteil (ANT, Ck, L, DK)
    auto& dk = static_cast<Capacitor&>(sim.circuit().component(3));
    REQUIRE(dk.params().capacitanceF == 150e-12);
    dk.setCapacitance(250e-12);
    dk.setCapacitance(-1.0); // ungueltig: bleibt
    CHECK(dk.params().capacitanceF == 250e-12);
    REQUIRE(tuneRadio(sim, {st}) == Status::Ok);
    const double e0 = static_cast<const DetectorDiode&>(sim.circuit().component(b.detector)).drive(0).e0V;

    Receiver r2 = r;
    r2.dkF = 250e-12;
    Built b2;
    Simulator sim2;
    REQUIRE(sim2.setCircuit(build(r2, b2)) == Status::Ok);
    REQUIRE(tuneRadio(sim2, {st}) == Status::Ok);
    CHECK(exact(e0, static_cast<const DetectorDiode&>(sim2.circuit().component(b2.detector)).drive(0).e0V));
}

TEST_CASE("Antennenspule 220 uH: Drehkondensator 10 ... 500 pF erreicht alle drei Sender", "[radio][volta3]") {
    const InductorParams l = InductorParams::uH220();
    const auto f = [&](double c) { return 1.0 / (2.0 * kPi * std::sqrt(l.inductanceH * c)); };
    CHECK(f(500e-12) < 600e3);
    CHECK(f(10e-12 + 10e-12) > 1400e3); // mit kleinem Koppelkondensator
    // Guete bei 1 MHz (Reihen- und Parallelverluste): um 90, mindestens 50
    const double w = 2.0 * kPi * 1e6, xl = w * l.inductanceH;
    const double q = 1.0 / (l.windingOhm / xl + xl / l.parallelOhm);
    CHECK(q > 50.0);
    CHECK(q < 150.0);
}

TEST_CASE("Gemittelter Detektor mit Belastung des Kreises stimmt mit dem echten Traeger ueberein", "[radio][volta3]") {
    Receiver r;
    const double fc = 600e3, fa = 5000.0, depth = 0.5;
    r.dkF = 1.0 / (std::pow(2.0 * kPi * fc, 2.0) * r.lH) - r.ckF; // abgestimmt (ohne Belastung)
    const double settle = 0.5e-3, measure = 0.4e-3;               // Kreis, Ladekondensator; dann 2 Tonperioden

    for (double emf : {0.5, 2.0}) {
        // Referenz mit Traeger (RK4, 2000 Schritte je Traegerperiode)
        Reference ref;
        ref.r = r;
        const Diode probe("p", 1, 0, DiodeParams::germanium());
        ref.is = probe.saturationCurrentA();
        ref.nvt = probe.params().n * probe.thermalVoltageV();
        ref.rs = probe.params().rsOhm;
        ref.emfV = emf;
        ref.depth = depth;
        ref.toneHz = fa;
        ref.carrierHz = fc;
        const double h = 1.0 / (fc * 2000.0);
        double s[4] = {0, 0, 0, 0}, k1[4], k2[4], k3[4], k4[4], tmp[4];
        std::vector<double> refDet;
        const long nSettle = std::lround(settle / h), nMeasure = std::lround(measure / h);
        const long every = 50; // Abtastung 1/40 der Traegerperiode
        for (long i = 0; i < nSettle + nMeasure; ++i) {
            const double t = i * h;
            ref.deriv(t, s, k1);
            for (int j = 0; j < 4; ++j) tmp[j] = s[j] + 0.5 * h * k1[j];
            ref.deriv(t + 0.5 * h, tmp, k2);
            for (int j = 0; j < 4; ++j) tmp[j] = s[j] + 0.5 * h * k2[j];
            ref.deriv(t + 0.5 * h, tmp, k3);
            for (int j = 0; j < 4; ++j) tmp[j] = s[j] + h * k3[j];
            ref.deriv(t + h, tmp, k4);
            for (int j = 0; j < 4; ++j) s[j] += h / 6.0 * (k1[j] + 2.0 * k2[j] + 2.0 * k3[j] + k4[j]);
            if (i + 1 >= nSettle && (i + 1 - nSettle) % every == 0) refDet.push_back(s[3]);
        }
        const auto refMA = meanAndAmplitude(refDet, h * every, fa, settle);

        // Modell: Abstimmen, gemittelter Detektor im Zeitbereich (1 us)
        Built b;
        Simulator sim;
        REQUIRE(sim.setCircuit(build(r, b)) == Status::Ok);
        REQUIRE(tuneRadio(sim, {RadioStation{fc, emf, depth, StationProgram::Tone, fa, true}}) == Status::Ok);
        const double dt = 1e-6;
        std::vector<double> model;
        const int mSettle = int(std::lround(settle / dt)), mMeasure = int(std::lround(measure / dt));
        for (int i = 0; i < mSettle + mMeasure; ++i) {
            REQUIRE(sim.step(dt) == Status::Ok);
            if (i + 1 > mSettle) model.push_back(sim.probe().voltage(b.det));
        }
        requireKirchhoff(sim);
        const auto modMA = meanAndAmplitude(model, dt, fa, settle + dt);
        INFO("Traeger " << emf << " V: Referenz Gleichanteil " << refMA.first << " V, Ton " << refMA.second
                        << " V; Modell " << modMA.first << " V, " << modMA.second << " V");
        CHECK(near(modMA.first, refMA.first, 1e-3));
        CHECK(near(modMA.second, refMA.second, 1e-3));
    }
}
