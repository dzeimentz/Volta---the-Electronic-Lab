// Leistungstest Volta 2: hoerbare Simulation (docs/volta2-planung.md, »Technisches Hauptrisiko: Audio-Schrittweite«).
// Rechnet der Kern Tongeneratoren um 1 kHz mit 20 us Schrittweite (50 000 Schritte je Sekunde) schneller als in
// Echtzeit? Zwei Bauarten, je 1, 2 und 4 Generatoren an einer 9-V-Batterie mit Hauptschalter (eingeschaltet im
// ersten Schritt wie in der App), Lautsprecher 8 Ohm:
// - Kippstufe: astabiler Multivibrator aus zwei NPN, Emitterfolger als Endstufe. An jeder Kippflanke wird der
//   Zeitschritt bis in den Nanosekundenbereich geteilt (Mitkopplung am Faltungspunkt), das kostet Rechenzeit.
// - Timer 555 (Prototyp eines Verhaltensmodells, nur hier im Leistungstest): Spannungsteiler, Entladeschalter und
//   Ausgang als Leitwerte, Komparatoren und Flipflop als diskreter Zustand, fortgeschrieben in angenommenen Schritten.
//   Zeitglied R/C ist echte Schaltung; die Schaltung bleibt linear (eine Loesung je Schritt).
// Pruefung der Physik: Tonhoehe am Ausgang gegen die Formel. Rueckgabe 0, wenn vier 555-Generatoren bei 20 us
// mindestens dreifache Echtzeit erreichen (Reserve fuer Oberflaeche und Ton) und schwingen, sonst 1.
// Nur im Release-Build aussagekraeftig. Argument: simulierte Sekunden je Messung (Vorgabe 1).
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/speaker.hpp"
#include "volta/core/components/switch.hpp"
#include "volta/core/components/transistor.hpp"
#include "volta/core/simulator.hpp"
#include "volta/core/version.hpp"

using namespace volta::core;
using Clock = std::chrono::steady_clock;

namespace {

constexpr double kR = 10e3;   // Kippstufe: Basiswiderstaende (R_B2 = 1,22 * R_B1 wie im Blinkmodul)
constexpr double kC = 68e-9;  // Kippstufe: Koppelkondensatoren; 555: Zeitkondensator
constexpr double kRc = 1e3;   // Kippstufe: Kollektorwiderstaende
constexpr double kRa = 1e3;   // 555: Ladewiderstand plus -> Entladung
constexpr double kRb = 10e3;  // 555: Entladung -> Kondensator

// Prototyp Timer 555, Kern: Teiler 3 x 5 kOhm (Versorgungsstrom), Schwellen 1/3 und 2/3 der Versorgung an einem
// gemeinsamen Eingang (Trigger und Schwelle verbunden), Entladetransistor als Schalter 20 Ohm.
// Schaltzeitpunkt innerhalb des Schritts: sagt die Steigung der Kondensatorspannung das Erreichen einer Schwelle im
// naechsten Schritt voraus, wirkt der Schritt anteilig in beiden Zustaenden (Leitwerte mit dem Zeitanteil
// gewichtet, ladungserhaltend wie das Tastverhaeltnis im Tonmodul); danach gilt der neue Zustand.
class Timer555Core final : public ComponentModel {
public:
    Timer555Core(std::string id, NodeId vcc, NodeId gnd, NodeId cap, NodeId dis)
        : ComponentModel(std::move(id), {vcc, gnd, cap, dis}) {}
    double qShare() const noexcept { return m_qShare; } // Zeitanteil von Q = 1 im laufenden Schritt
    void stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept override {
        sys.stampConductance(terminal(0), terminal(1), 1.0 / kDividerOhm);
        sys.stampConductance(terminal(3), terminal(1), disG());
    }
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override {
        const double vg = x.voltage(terminal(1));
        const double iDiv = (x.voltage(terminal(0)) - vg) / kDividerOhm;
        const double iDis = (x.voltage(terminal(3)) - vg) * disG();
        switch (t) {
        case 0: return iDiv;
        case 1: return -(iDiv + iDis);
        case 2: return 0.0;
        default: return iDis;
        }
    }
    void updateState(const SolutionView& x, double dtS) noexcept override {
        const double vg = x.voltage(terminal(1));
        const double vcc = x.voltage(terminal(0)) - vg;
        const double vc = x.voltage(terminal(2)) - vg;
        const double slope = dtS > 0.0 ? vc - m_vcOld : 0.0; // Aenderung je Schritt
        m_vcOld = vc;
        const double lo = vcc / 3.0, hi = 2.0 * vcc / 3.0;
        if (vcc < kSupplyMinV) {
            m_q = false;
        } else if (vc <= lo) {
            m_q = true; // Trigger setzt
        } else if (vc >= hi) {
            m_q = false; // Schwelle setzt zurueck
        }
        m_qShare = m_q ? 1.0 : 0.0;
        if (vcc < kSupplyMinV) return;
        if (m_q && slope > 0.0 && vc + slope > hi) { // Schwelle wird im naechsten Schritt erreicht
            m_qShare = (hi - vc) / slope;
            m_q = false;
        } else if (!m_q && slope < 0.0 && vc + slope < lo) {
            m_qShare = 1.0 - (vc - lo) / -slope;
            m_q = true;
        }
    }

private:
    static constexpr double kDividerOhm = 15e3;
    static constexpr double kDischargeOhm = 20.0;
    static constexpr double kSupplyMinV = 3.0;
    double disG() const noexcept { return m_qShare * kGmin + (1.0 - m_qShare) / kDischargeOhm; }
    bool m_q = false;        // Zustand am Ende des laufenden Schritts
    double m_qShare = 0.0;
    double m_vcOld = 0.0;
};

// Prototyp Timer 555, Ausgang: 10 Ohm nach plus (Q) bzw. nach minus
class Timer555Out final : public ComponentModel {
public:
    Timer555Out(std::string id, NodeId vcc, NodeId gnd, NodeId out, const Timer555Core& core)
        : ComponentModel(std::move(id), {vcc, gnd, out}), m_core(core) {}
    void stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept override {
        const double share = m_core.qShare();
        sys.stampConductance(terminal(2), terminal(0), share / kOutOhm);
        sys.stampConductance(terminal(2), terminal(1), (1.0 - share) / kOutOhm);
    }
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override {
        const double share = m_core.qShare();
        const double vo = x.voltage(terminal(2));
        const double iHigh = (vo - x.voltage(terminal(0))) * share / kOutOhm;
        const double iLow = (vo - x.voltage(terminal(1))) * (1.0 - share) / kOutOhm;
        if (t == 2) return iHigh + iLow;
        return t == 0 ? -iHigh : -iLow;
    }

private:
    static constexpr double kOutOhm = 10.0;
    const Timer555Core& m_core;
};

enum class Kind { Multivibrator, Timer555 };

struct Built {
    Circuit circuit;
    std::vector<Speaker*> speakers;
    Switch* mainSwitch = nullptr;
    NodeId probe = kGround; // Kollektor T2 bzw. Ausgang des ersten Generators
};

// n Tongeneratoren; der i-te mit etwas groesserem C (Tonhoehen unterscheidbar)
Built build(Kind kind, int n) {
    Built b;
    Circuit& c = b.circuit;
    const NodeId bat = c.addNode(), plus = c.addNode();
    c.add<Battery>("B1", bat, kGround);
    b.mainSwitch = &c.add<Switch>("S1", bat, plus, false);
    for (int i = 0; i < n; ++i) {
        const std::string s = std::to_string(i);
        const double cap = kC * (1.0 + 0.15 * i);
        if (kind == Kind::Multivibrator) {
            const NodeId c1 = c.addNode(), c2 = c.addNode(), b1 = c.addNode(), b2 = c.addNode();
            c.add<Resistor>("RC1_" + s, plus, c1, ResistorParams{kRc});
            c.add<Resistor>("RC2_" + s, plus, c2, ResistorParams{kRc});
            c.add<Resistor>("RB1_" + s, plus, b1, ResistorParams{kR});
            // unsymmetrisch, sonst bleibt die Kippstufe im (instabilen) Gleichgewicht stehen
            c.add<Resistor>("RB2_" + s, plus, b2, ResistorParams{kR * 1.22});
            c.add<Capacitor>("C1_" + s, c1, b2, CapacitorParams::film(cap));
            c.add<Capacitor>("C2_" + s, c2, b1, CapacitorParams::film(cap));
            c.add<Npn>("T1_" + s, b1, c1, kGround);
            c.add<Npn>("T2_" + s, b2, c2, kGround);
            const NodeId b3 = c.addNode(), e3 = c.addNode();
            c.add<Resistor>("RB3_" + s, c2, b3, ResistorParams{22e3});
            c.add<Npn>("T3_" + s, b3, plus, e3);
            b.speakers.push_back(&c.add<Speaker>("LS_" + s, e3, kGround));
            if (i == 0) b.probe = c2;
        } else {
            const NodeId dis = c.addNode(), th = c.addNode(), out = c.addNode(), ls = c.addNode();
            c.add<Resistor>("RA_" + s, plus, dis, ResistorParams{kRa});
            c.add<Resistor>("RB_" + s, dis, th, ResistorParams{kRb});
            c.add<Capacitor>("CT_" + s, th, kGround, CapacitorParams::film(cap));
            const Timer555Core& core = c.add<Timer555Core>("IC_" + s, plus, kGround, th, dis);
            c.add<Timer555Out>("ICO_" + s, plus, kGround, out, core);
            const NodeId k = c.addNode();
            c.add<Resistor>("RL_" + s, out, k, ResistorParams{100.0});
            c.add<Capacitor>("CK_" + s, k, ls, CapacitorParams::electrolytic(10e-6));
            b.speakers.push_back(&c.add<Speaker>("LS_" + s, ls, kGround));
            if (i == 0) b.probe = th; // Zeitkondensator pendelt zwischen 3 und 6 V
        }
    }
    return b;
}

struct Result {
    std::size_t components = 0;
    double simS = 0.0;
    double wallMs = 0.0;
    double worstStepMs = 0.0;
    int failed = 0;
    long long totalIter = 0; // Newton-Iterationen insgesamt
    int splitSteps = 0;      // Schritte, die geteilt werden mussten
    int maxDepth = 0;        // feinste Teilung (dt / 2^Tiefe)
    double freqHz = 0.0;     // aus den steigenden Flanken am Messpunkt
};

Result run(Kind kind, int n, double dtS, double simS) {
    Built b = build(kind, n);
    Result r;
    r.components = b.circuit.componentCount();
    const NodeId probe = b.probe;
    Switch* mainSwitch = b.mainSwitch;
    Simulator sim;
    if (sim.setCircuit(std::move(b.circuit)) != Status::Ok) {
        r.failed = -1;
        return r;
    }
    const int steps = static_cast<int>(std::lround(simS / dtS));
    int edges = 0;
    double firstEdgeS = -1.0, lastEdgeS = 0.0;
    bool high = false;
    mainSwitch->setClosed(true);
    const auto t0 = Clock::now();
    for (int i = 0; i < steps; ++i) {
        const auto s0 = Clock::now();
        if (sim.step(dtS) != Status::Ok) ++r.failed;
        r.worstStepMs = std::max(r.worstStepMs, std::chrono::duration<double, std::milli>(Clock::now() - s0).count());
        r.totalIter += sim.lastStepIterations();
        if (sim.lastStepMaxDepth() > 0) ++r.splitSteps;
        r.maxDepth = std::max(r.maxDepth, sim.lastStepMaxDepth());
        // Tonhoehe: steigende Flanken (Hysterese 3,5 / 5,5 V) nach 0,1 s Einschwingen
        const double v = sim.probe().voltage(probe);
        if (!high && v > 5.5) {
            high = true;
            if (sim.timeS() > 0.1) {
                ++edges;
                if (firstEdgeS < 0) firstEdgeS = sim.timeS();
                lastEdgeS = sim.timeS();
            }
        } else if (high && v < 3.5) {
            high = false;
        }
    }
    r.wallMs = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
    r.simS = sim.timeS();
    if (edges > 1) r.freqHz = (edges - 1) / (lastEdgeS - firstEdgeS);
    return r;
}

} // namespace

int main(int argc, char** argv) {
    const double simS = argc > 1 ? std::atof(argv[1]) : 1.0;
    const double fMulti = 1.0 / (0.693 * kC * (kR + 1.22 * kR));
    const double f555 = 1.0 / (0.693 * kC * (kRa + 2.0 * kRb));
    std::printf("Volta Simulationskern %s - Leistungstest Audio (Volta 2)\n", version());
    std::printf("Kippstufe: R = 10 / 12,2 kOhm, C = 68 nF -> Formel %.0f Hz; Emitterfolger, Lautsprecher 8 Ohm\n", fMulti);
    std::printf("Timer 555 (Prototyp): Ra = 1 kOhm, Rb = 10 kOhm, C = 68 nF -> Formel %.0f Hz; Lautsprecher ueber 100 Ohm und 10 uF\n",
                f555);
    std::printf("Generator i mit C * (1 + 0,15 i); Tonhoehe gemessen an Generator 1. Simulierte Zeit je Messung: %.1f s\n\n",
                simS);
    std::printf("%-11s %-4s %-9s %-8s %-12s %-15s %-12s %-16s %-14s %s\n", "Bauart", "Gen.", "Bauteile", "Schritt",
                "Rechenzeit", "Echtzeitfaktor", "max. Schritt", "Newton/Schritt", "geteilt", "Ton");
    bool ok = true;
    for (Kind kind : {Kind::Multivibrator, Kind::Timer555}) {
        const bool is555 = kind == Kind::Timer555;
        for (double dt : {5e-6, 20e-6, 50e-6}) {
            if (!is555 && dt < 10e-6) continue; // Referenz nur fuer den 555 (Kippstufe waere zu langsam)
            for (int n : {1, 2, 4}) {
                const Result r = run(kind, n, dt, simS);
                const char* name = is555 ? "Timer 555" : "Kippstufe";
                if (r.failed < 0) {
                    std::printf("%-11s %-4d Arbeitspunkt nicht gefunden\n", name, n);
                    ok = false;
                    continue;
                }
                const double factor = r.simS * 1000.0 / r.wallMs;
                const double formula = is555 ? f555 : fMulti;
                const int stepCount = static_cast<int>(r.simS / dt + 0.5);
                std::printf("%-11s %-4d %-9zu %3.0f us   %7.0f ms   %9.1f x     %7.3f ms   %8.2f         %5.1f %% (%2d)  "
                            "%.0f Hz (%+.1f %%)%s\n",
                            name, n, r.components, dt * 1e6, r.wallMs, factor, r.worstStepMs,
                            double(r.totalIter) / stepCount, 100.0 * r.splitSteps / stepCount, r.maxDepth, r.freqHz,
                            r.freqHz > 0 ? 100.0 * (r.freqHz / formula - 1.0) : -100.0,
                            r.failed ? "  (Schritte ohne Konvergenz!)" : "");
                if (is555 && n == 4 && dt == 20e-6) ok = ok && factor >= 3.0 && r.failed == 0 && r.freqHz > 300.0;
            }
        }
    }
    std::printf("\nVorgabe: vier 555-Tongeneratoren bei 20 us mindestens 3-fache Echtzeit -> %s\n",
                ok ? "eingehalten" : "VERFEHLT");
    return ok ? 0 : 1;
}
