// Machbarkeitstest Volta 3 (docs/Konzept_Volta3.md, Kap. 4 und Umsetzungsplan Schritt 1).
// Prueft auf dem Zielgeraet (Tablet) die drei technischen Grundsatzentscheidungen:
//  A Frequenzgang: Wechselstromanalyse (Simulator::acSweep) mit 200 Punkten 1 Hz ... 10 MHz an typischen Aufbauten.
//  B Radio: Abstimmen eines Mittelwellen-Schwingkreises ueber die Wechselstromanalyse (drei Sender), danach die
//    Huellkurve durch Detektor, Verstaerker und Lautsprecher im Zeitbereich mit 20 us. Ehrlichkeit: Vergleich des
//    Huellkurvenmodells mit einer Rechnung mit echtem Traeger (100 kHz, 0,1 us Schrittweite).
//  C Schaltwandler und Klasse D mit 1-5 kHz Schaltfrequenz: 20 us gegen 1 us Schrittweite (Abweichung < 2 %),
//    Echtzeitfaktor.
// Die Prueflinge Huellkurvenquelle und Taktschalter sind Prototypen nur in diesem Test (wie der 555 im
// Audio-Leistungstest von Volta 2). Rueckgabe 0, wenn alle Vorgaben erfuellt sind, sonst 1.
// Nur im Release-Build aussagekraeftig. Argument: Faktor fuer die simulierte Dauer (Vorgabe 1).
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "volta/core/components/battery.hpp"
#include "volta/core/components/capacitor.hpp"
#include "volta/core/components/diode.hpp"
#include "volta/core/components/inductor.hpp"
#include "volta/core/components/resistor.hpp"
#include "volta/core/components/sources.hpp"
#include "volta/core/components/speaker.hpp"
#include "volta/core/components/transistor.hpp"
#include "volta/core/simulator.hpp"
#include "volta/core/version.hpp"

using namespace volta::core;
using Clock = std::chrono::steady_clock;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kAudioStepS = 20e-6;   // Schrittweite der hoerbaren Simulation (Volta 2)
constexpr double kMinRealtime = 3.0;    // Reserve fuer Oberflaeche und Ton wie im Audio-Leistungstest
constexpr double kMaxSweepMs = 50.0;    // Frequenzgang mit 200 Punkten: fluessig beim Drehen am Regler
constexpr double kMaxTuneMs = 20.0;     // Abstimmen: ein Reglerschritt am Drehkondensator
constexpr double kMaxDeviation = 0.02;  // Regel 3

double msSince(Clock::time_point t0) { return std::chrono::duration<double, std::milli>(Clock::now() - t0).count(); }

bool g_ok = true;
bool g_predictor = true; // Praediktor im Zeitbereich (Simulator::setPredictor)
double g_tolRel = 1e-4;   // relative Konvergenztoleranz (Simulator::setRelativeTolerance)
void verdict(bool pass, const char* what) {
    std::printf("  -> %s: %s\n", what, pass ? "erfuellt" : "NICHT erfuellt");
    g_ok = g_ok && pass;
}

// ---- Prueflinge ----

// Spannungsquelle mit Innenwiderstand, deren Leerlaufspannung eine Funktion der Zeit ist (eigene Zeit, in
// updateState fortgeschrieben). Steht fuer die Huellkurve am Schwingkreis bzw. fuer den echten Traeger.
class TimeSource final : public ComponentModel {
public:
    TimeSource(std::string id, NodeId a, NodeId b, double ohm, std::function<double(double)> emf)
        : ComponentModel(std::move(id), {a, b}), m_ohm(ohm), m_emf(std::move(emf)) {}
    std::size_t branchCount() const noexcept override { return 1; }
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override {
        sys.stampVoltageBranch(terminal(0), terminal(1), firstBranch(), m_emf(m_t + std::max(ctx.dtS, 0.0)), m_ohm);
    }
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override {
        const double i = x.branchCurrent(firstBranch());
        return t == 0 ? i : -i;
    }
    void updateState(const SolutionView& /*x*/, double dtS) noexcept override { m_t += dtS; }

private:
    double m_ohm;
    std::function<double(double)> m_emf;
    double m_t = 0.0;
};

// Taktschalter (Stellvertreter fuer MOSFET mit Ansteuerung): Durchlass 0,2 Ohm, gesperrt 1 MOhm, Takt mit
// Frequenz f und Tastverhaeltnis duty(t) (zu Beginn jeder Periode abgetastet). Schaltflanken werden ueber die
// Ereignisortung exakt getroffen.
class PwmSwitch final : public ComponentModel {
public:
    PwmSwitch(std::string id, NodeId a, NodeId b, double hz, std::function<double(double)> duty)
        : ComponentModel(std::move(id), {a, b}), m_period(1.0 / hz), m_duty(std::move(duty)) {
        m_d = std::clamp(m_duty(0.0), 0.0, 1.0);
        m_on = m_d > 0.0;
    }
    void stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept override {
        sys.stampConductance(terminal(0), terminal(1), g());
    }
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override {
        const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) * g();
        return t == 0 ? i : -i;
    }
    double eventFraction(const SolutionView&, const SolutionView&, double dtS) const noexcept override {
        const double edge = nextEdge();
        return edge < m_t + dtS ? std::max((edge - m_t) / dtS, 0.0) : 1.0;
    }
    void updateState(const SolutionView& /*x*/, double dtS) noexcept override {
        m_t += dtS;
        const double eps = 1e-9 * m_period;
        const double k = std::floor((m_t + eps) / m_period);
        const double start = k * m_period;
        if (start > m_periodStart + 0.5 * m_period) { // neue Periode: Tastverhaeltnis abtasten
            m_periodStart = start;
            m_d = std::clamp(m_duty(start), 0.0, 1.0);
        }
        m_on = (m_t + eps - m_periodStart) < m_d * m_period;
    }

private:
    double g() const noexcept { return m_on ? 1.0 / 0.2 : 1e-6; }
    double nextEdge() const noexcept {
        const double eps = 1e-9 * m_period;
        const double off = m_periodStart + m_d * m_period;
        if (m_on && off > m_t + eps) return off;
        return m_periodStart + m_period;
    }
    double m_period;
    std::function<double(double)> m_duty;
    double m_t = 0.0;
    double m_periodStart = 0.0;
    double m_d = 0.0;
    bool m_on = false;
};

// Diode mit Ortung des Sperrbeginns (Prototyp): faellt der Strom durch den Bahnwiderstand innerhalb des Schritts
// von Durchlass auf null, wird der Schritt dort geteilt (wie die Kippschwelle des 555). Noetig fuer lueckenden
// Betrieb von Schaltwandlern, in dem der Spulenstrom mitten im Schritt endet.
class LocatingDiode final : public Diode {
public:
    LocatingDiode(std::string id, NodeId a, NodeId k, DiodeParams p) : Diode(std::move(id), a, k, p) {}
    // Der Strom endet nicht mit Vorzeichenwechsel (die gesperrte Diode haelt ihn bei null): Sperrbeginn im Schritt
    // durch Halbieren eingrenzen, bis der Teilschritt kuerzer als kLocateS ist.
    double eventFraction(const SolutionView& a, const SolutionView& n, double dtS) const noexcept override {
        const double ia = current(a), in = current(n);
        if (ia > kOnA && in < 1e-3 * ia && dtS > kLocateS) return 0.5;
        return 1.0;
    }

private:
    static constexpr double kOnA = 1e-6;
    static constexpr double kLocateS = 0.2e-6;
    double current(const SolutionView& x) const noexcept {
        return (x.voltage(terminal(0)) - x.voltage(internalNode(0))) / params().rsOhm;
    }
};

// ---- A Frequenzgang ----

std::vector<double> logSweep(double f0, double f1, int n) {
    std::vector<double> f(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) f[static_cast<std::size_t>(i)] = f0 * std::pow(f1 / f0, i / double(n - 1));
    return f;
}

// Zweistufiger Transistorverstaerker mit Klangfilter (typischer groesserer Aufbau, ca. 20 Bauteile).
Circuit makeAmplifier(std::size_t& gen, NodeId& out) {
    Circuit c;
    const NodeId plus = c.addNode(), g = c.addNode();
    c.add<Battery>("B", plus, kGround);
    gen = c.componentCount();
    c.add<FunctionGenerator>("G", g, kGround);
    NodeId in = g;
    for (int s = 0; s < 2; ++s) {
        const std::string k = std::to_string(s);
        const NodeId b = c.addNode(), col = c.addNode(), e = c.addNode();
        // Elko mit Plus an der hoeheren Gleichspannung: Basis (1,4 V) gegen Generator, Kollektor (5 V) gegen Basis
        if (s == 0)
            c.add<Capacitor>("Cin" + k, b, in, CapacitorParams::electrolytic(10e-6));
        else
            c.add<Capacitor>("Cin" + k, in, b, CapacitorParams::electrolytic(10e-6));
        c.add<Resistor>("R1" + k, plus, b, ResistorParams{47e3});
        c.add<Resistor>("R2" + k, b, kGround, ResistorParams{10e3});
        c.add<Resistor>("Rc" + k, plus, col, ResistorParams{2.2e3});
        c.add<Npn>("T" + k, b, col, e);
        c.add<Resistor>("Re" + k, e, kGround, ResistorParams{470.0});
        c.add<Capacitor>("Ce" + k, e, kGround, CapacitorParams::electrolytic(100e-6));
        in = col;
    }
    // Klangfilter: Tiefpass und Hochpass
    const NodeId f1 = c.addNode(), f2 = c.addNode();
    c.add<Resistor>("Rf1", in, f1, ResistorParams{10e3});
    c.add<Capacitor>("Cf1", f1, kGround, CapacitorParams::film(10e-9));
    c.add<Capacitor>("Cf2", f1, f2, CapacitorParams::film(100e-9));
    c.add<Resistor>("Rf2", f2, kGround, ResistorParams{10e3});
    out = f2;
    return c;
}

void partA() {
    std::printf("\nA Frequenzgang (Wechselstromanalyse, 200 Punkte 1 Hz ... 10 MHz)\n");
    const std::vector<double> f = logSweep(1.0, 10e6, 200);
    std::vector<Complex> h;
    // Schwingkreis
    {
        Circuit c;
        const NodeId g = c.addNode(), t = c.addNode();
        c.add<FunctionGenerator>("G", g, kGround);
        c.add<Resistor>("R", g, t, ResistorParams{100e3});
        c.add<Inductor>("L", t, kGround, InductorParams{200e-6, 3.0, 1e6, 0.1});
        c.add<Capacitor>("C", t, kGround, CapacitorParams::film(220e-12));
        Simulator sim;
        sim.setCircuit(std::move(c));
        const auto t0 = Clock::now();
        const int reps = 20;
        for (int r = 0; r < reps; ++r) sim.acSweep(0, f, t, kGround, h);
        const double ms = msSince(t0) / reps;
        std::printf("  Schwingkreis (4 Bauteile):             %7.2f ms je Durchlauf\n", ms);
        verdict(ms < kMaxSweepMs, "Schwingkreis fluessig");
    }
    // zweistufiger Verstaerker mit Filter
    {
        std::size_t gen = 0;
        NodeId out = 0;
        Simulator sim;
        sim.setCircuit(makeAmplifier(gen, out));
        auto& g = static_cast<FunctionGenerator&>(sim.circuit().component(gen));
        g.setAmplitude(0.0); // ohne Signal einschwingen (sonst uebersteuert und verpolt die Elkos)
        for (int i = 0; i < 2000; ++i) sim.step(1e-3); // Kondensatoren laden
        const auto t0 = Clock::now();
        const int reps = 10;
        for (int r = 0; r < reps; ++r) sim.acSweep(gen, f, out, kGround, h);
        const double ms = msSince(t0) / reps;
        double peak = 0.0, fPeak = 0.0;
        for (std::size_t i = 0; i < f.size(); ++i)
            if (std::abs(h[i]) > peak) { peak = std::abs(h[i]); fPeak = f[i]; }
        std::printf("  Verstaerker mit Filter (%zu Bauteile): %7.2f ms je Durchlauf (Maximum %.0f-fach bei %.0f Hz)\n",
                    sim.circuit().componentCount(), ms, peak, fPeak);
        verdict(ms < kMaxSweepMs, "Verstaerker fluessig");
    }
}

// ---- B Radio ----

struct Station { double hz; double audioHz; };
const Station kStations[] = {{600e3, 400.0}, {1000e3, 1000.0}, {1400e3, 700.0}};

// Abstimmkreis: Antenne (Quelle 1 kOhm) -> Koppelkondensator 10 pF (lose Kopplung, hohe Guete) -> L 200 uH ||
// Drehkondensator
constexpr double kCouplingF = 10e-12;
Circuit makeTuner(double drehkoF, NodeId& tank) {
    Circuit c;
    const NodeId a = c.addNode();
    tank = c.addNode();
    GeneratorParams gp;
    gp.outputOhm = 1e3;
    c.add<FunctionGenerator>("ANT", a, kGround, gp);
    c.add<Capacitor>("Ck", a, tank, CapacitorParams::film(kCouplingF));
    c.add<Inductor>("L", tank, kGround, InductorParams{200e-6, 3.0, 1e6, 0.1});
    c.add<Capacitor>("DK", tank, kGround, CapacitorParams::film(drehkoF));
    return c;
}

// Empfaenger im Zeitbereich: Quelle (Huellkurve oder Traeger) -> Ge-Diode -> RC-Last -> Emitterschaltung ->
// Emitterfolger -> Lautsprecher. Liefert den Knoten am Detektorausgang.
// Gemittelter Huellkurvendetektor (Prototyp): Anschluesse src (Huellkurve E gegen ref), det (Ausgang Vo gegen ref),
// ref. Der Strom ist der Mittelwert des Diodenstroms ueber eine Traegerperiode,
//   I(E, Vo) = 1/N * sum_k g(E cos(phi_k) - Vo),
// mit g = Strom durch Quellwiderstand des Kreises, Bahnwiderstand und pn-Uebergang (Shockley, gleiche Kennwerte wie
// die Diode im Zeitbereich) bei der Gesamtspannung v. Annahme: der Traeger ist viel schneller als die RC-Last am
// Ausgang (Mittelwert-Modell); der Kreis wirkt als Quelle mit seinem Resonanzwiderstand.
class AveragedDetector final : public ComponentModel {
public:
    AveragedDetector(std::string id, NodeId src, NodeId det, NodeId ref, DiodeParams dp, double sourceOhm)
        : ComponentModel(std::move(id), {src, det, ref}) {
        const Diode probe("probe", 1, 0, dp);
        const double is = probe.saturationCurrentA(), nvt = dp.n * probe.thermalVoltageV(),
                     r = sourceOhm + dp.rsOhm;
        m_table.resize(kTableN);
        for (std::size_t i = 0; i < kTableN; ++i) {
            const double v = kVmin + kDv * double(i);
            // vd + r * Is (exp(vd/nVt) - 1) = v nach vd loesen (Newton, monoton)
            double vd = std::min(v, 0.5);
            for (int k = 0; k < 60; ++k) {
                const double e = std::exp(std::min(vd / nvt, 80.0));
                const double f = vd + r * is * (e - 1.0) - v;
                const double df = 1.0 + r * is * e / nvt;
                vd -= f / df;
            }
            m_table[i] = is * (std::exp(std::min(vd / nvt, 80.0)) - 1.0);
        }
        // Phasen (k + 0,5) / 64 einer Periode; cos ist symmetrisch, daher reichen 32 Stuetzstellen mit doppeltem Gewicht
        for (int k = 0; k < kPhases; ++k) m_cos[std::size_t(k)] = std::cos(kPi * (k + 0.5) / kPhases);
    }
    bool isNonlinear() const noexcept override { return true; }
    bool linearize(const SolutionView& x) noexcept override {
        evaluate(e(x), vo(x));
        return false;
    }
    void resetLinearization(const SolutionView& x) noexcept override { evaluate(e(x), vo(x)); }
    void stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept override {
        const NodeId s = terminal(0), d = terminal(1), r = terminal(2);
        const double ieq = m_i0 - m_ge * m_e0 - m_go * m_vo0;
        // Strom I verlaesst src und fliesst in det: I = ge*(Vs - Vr) + go*(Vd - Vr) + ieq
        sys.stampNodeMatrix(s, s, m_ge);
        sys.stampNodeMatrix(s, r, -m_ge);
        sys.stampNodeMatrix(s, d, m_go);
        sys.stampNodeMatrix(s, r, -m_go);
        sys.stampNodeRhs(s, -ieq);
        sys.stampNodeMatrix(d, s, -m_ge);
        sys.stampNodeMatrix(d, r, m_ge);
        sys.stampNodeMatrix(d, d, -m_go);
        sys.stampNodeMatrix(d, r, m_go);
        sys.stampNodeRhs(d, ieq);
    }
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override {
        const double i = m_i0 + m_ge * (e(x) - m_e0) + m_go * (vo(x) - m_vo0);
        return t == 0 ? i : (t == 1 ? -i : 0.0);
    }

private:
    static constexpr double kVmin = -5.0, kDv = 1e-3;
    static constexpr std::size_t kTableN = 20001; // -5 ... +15 V
    static constexpr int kPhases = 32;

    double e(const SolutionView& x) const noexcept { return x.voltage(terminal(0)) - x.voltage(terminal(2)); }
    double vo(const SolutionView& x) const noexcept { return x.voltage(terminal(1)) - x.voltage(terminal(2)); }
    // Strom und Steigung der Tabelle bei v (linear interpoliert, ausserhalb linear fortgesetzt)
    void g(double v, double& i, double& di) const noexcept {
        double u = (v - kVmin) / kDv;
        u = std::clamp(u, 0.0, double(kTableN - 1) - 1e-9);
        const std::size_t k = std::size_t(u);
        const double fr = u - double(k);
        di = (m_table[k + 1] - m_table[k]) / kDv;
        i = m_table[k] + fr * (m_table[k + 1] - m_table[k]) + di * (v - (kVmin + kDv * u));
    }
    void evaluate(double e0, double vo0) noexcept {
        double sum = 0.0, sumD = 0.0, sumDc = 0.0;
        for (double c : m_cos) {
            double i = 0.0, di = 0.0;
            g(e0 * c - vo0, i, di);
            sum += i;
            sumD += di;
            sumDc += di * c;
        }
        m_e0 = e0;
        m_vo0 = vo0;
        m_i0 = sum / kPhases;
        m_ge = sumDc / kPhases;
        m_go = -sumD / kPhases - kJunctionGmin;
    }
    std::vector<double> m_table;
    std::array<double, kPhases> m_cos{};
    double m_e0 = 0.0, m_vo0 = 0.0, m_i0 = 0.0, m_ge = 0.0, m_go = -kJunctionGmin;
};

Circuit makeReceiver(std::function<double(double)> emf, bool withAmplifier, NodeId& det, bool averaged = false) {
    Circuit c;
    const NodeId src = c.addNode();
    det = c.addNode();
    if (averaged) { // Huellkurve als ideale Quelle, Quellwiderstand des Kreises im gemittelten Detektor
        c.add<TimeSource>("Q", src, kGround, 1e-3, std::move(emf));
        c.add<AveragedDetector>("D", src, det, kGround, DiodeParams::germanium(), 5e3);
    } else {
        c.add<TimeSource>("Q", src, kGround, 5e3, std::move(emf)); // Resonanzwiderstand des Kreises (Groessenordnung)
        c.add<Diode>("D", src, det, DiodeParams::germanium());
    }
    c.add<Capacitor>("Cd", det, kGround, CapacitorParams::film(4.7e-9));
    c.add<Resistor>("Rd", det, kGround, ResistorParams{47e3});
    if (withAmplifier) {
        const NodeId plus = c.addNode(), b = c.addNode(), col = c.addNode(), e = c.addNode(), o = c.addNode(),
                     ls = c.addNode();
        c.add<Battery>("B", plus, kGround);
        c.add<Capacitor>("Cc", b, det, CapacitorParams::electrolytic(1e-6));
        c.add<Resistor>("R1", plus, b, ResistorParams{47e3});
        c.add<Resistor>("R2", b, kGround, ResistorParams{10e3});
        c.add<Resistor>("Rc", plus, col, ResistorParams{2.2e3});
        c.add<Npn>("T1", b, col, e);
        c.add<Resistor>("Re", e, kGround, ResistorParams{470.0});
        c.add<Capacitor>("Ce", e, kGround, CapacitorParams::electrolytic(100e-6));
        c.add<Npn>("T2", col, plus, o);
        c.add<Resistor>("Ro", o, kGround, ResistorParams{220.0});
        c.add<Capacitor>("Co", o, ls, CapacitorParams::electrolytic(100e-6));
        c.add<Speaker>("LS", ls, kGround);
    }
    return c;
}

// Mittelwert und Amplitude der Grundschwingung f in v (Abtastung dt), per Projektion auf sin/cos
std::pair<double, double> meanAndAmplitude(const std::vector<double>& v, double dt, double f) {
    double m = 0.0, s = 0.0, co = 0.0;
    for (std::size_t i = 0; i < v.size(); ++i) {
        const double ph = 2.0 * kPi * f * dt * double(i);
        m += v[i];
        s += v[i] * std::sin(ph);
        co += v[i] * std::cos(ph);
    }
    const double n = double(v.size());
    return {m / n, 2.0 * std::hypot(s, co) / n};
}

void partB(double scale) {
    std::printf("\nB Radio (Abstimmen ueber Wechselstromanalyse, Huellkurve im Zeitbereich)\n");
    // 1 Abstimmen: Drehkondensator 10 ... 500 pF in 400 Stufen, je Stufe Aufbau neu und drei Senderfrequenzen
    const int steps = 400;
    std::vector<double> fs;
    for (const Station& s : kStations) fs.push_back(s.hz);
    std::vector<std::vector<double>> gain(3);
    std::vector<double> caps;
    std::vector<Complex> h;
    const auto t0 = Clock::now();
    for (int k = 0; k < steps; ++k) {
        const double cap = 10e-12 * std::pow(50.0, k / double(steps - 1));
        NodeId tank = 0;
        Simulator sim;
        sim.setCircuit(makeTuner(cap, tank));
        sim.acSweep(0, fs, tank, kGround, h);
        caps.push_back(cap);
        for (int s = 0; s < 3; ++s) gain[std::size_t(s)].push_back(std::abs(h[std::size_t(s)]));
    }
    const double tuneMs = msSince(t0) / steps;
    std::printf("  Abstimmen: %.3f ms je Reglerschritt (Aufbau, Arbeitspunkt, 3 Frequenzen)\n", tuneMs);
    verdict(tuneMs < kMaxTuneMs, "Abstimmen fluessig");
    bool selective = true;
    for (int s = 0; s < 3; ++s) {
        const auto& gs = gain[std::size_t(s)];
        const std::size_t best = std::size_t(std::max_element(gs.begin(), gs.end()) - gs.begin());
        double other = 0.0;
        for (int o = 0; o < 3; ++o)
            if (o != s) other = std::max(other, gain[std::size_t(o)][best]);
        const double db = 20.0 * std::log10(gs[best] / std::max(other, 1e-12));
        const double fRes = 1.0 / (2.0 * kPi * std::sqrt(200e-6 * (caps[best] + kCouplingF)));
        std::printf("  Sender %4.0f kHz: abgestimmt bei %5.1f pF (Thomson mit Koppel-C %4.0f kHz), Trennung %5.1f dB\n",
                    kStations[s].hz / 1e3, caps[best] * 1e12, fRes / 1e3, db);
        selective = selective && db > 10.0 && std::fabs(fRes - kStations[s].hz) < 0.02 * kStations[s].hz;
    }
    verdict(selective, "Sender trennbar (> 10 dB) und Abstimmung nach Thomson");

    // 2 Ehrlichkeit des Huellkurvenmodells: Detektor mit echtem Traeger (250 kHz, 0,04 us) gegen Huellkurve (20 us),
    //   einmal einfach (Huellkurve als Spannung vor der Diode) und einmal mit gemitteltem Detektor
    const double fa = 1000.0, fc = 250e3;
    auto detect = [&](std::function<double(double)> emf, double dt, bool averaged, double& msOut) {
        NodeId det = 0;
        Simulator sim;
        sim.setCircuit(makeReceiver(std::move(emf), false, det, averaged));
        sim.setRelativeTolerance(g_tolRel);
        const int settle = int(std::lround(0.005 / dt)), n = int(std::lround(0.005 / dt)); // je 5 Tonperioden
        const auto t1 = Clock::now();
        for (int i = 0; i < settle; ++i) sim.step(dt);
        std::vector<double> v;
        v.reserve(std::size_t(n));
        for (int i = 0; i < n; ++i) {
            sim.step(dt);
            v.push_back(sim.probe().voltage(det));
        }
        msOut = msSince(t1);
        return meanAndAmplitude(v, dt, fa);
    };
    bool honest = true;
    for (double amp : {0.3, 0.8, 2.0}) {
        const double m = 0.5;
        auto envelope = [=](double t) { return amp * (1.0 + m * std::sin(2.0 * kPi * fa * t)); };
        auto carrier = [=](double t) { return envelope(t) * std::sin(2.0 * kPi * fc * t); };
        double msRef = 0.0, msNaive = 0.0, msAvg = 0.0;
        const auto ref = detect(carrier, 0.04e-6, false, msRef);
        const auto naive = detect(envelope, kAudioStepS, false, msNaive);
        const auto avg = detect(envelope, kAudioStepS, true, msAvg);
        auto dev = [&](const std::pair<double, double>& x) { return std::fabs(x.second - ref.second) / ref.second; };
        std::printf("  Traeger %.1f V: echter Traeger Ton %.4f V (Gleichanteil %.4f V, %.0f ms) | einfach %.4f V (%+.1f %%)"
                    " | gemittelt %.4f V (%+.1f %%, Gleichanteil %.4f V, %.1f ms)\n",
                    amp, ref.second, ref.first, msRef, naive.second, 100.0 * (naive.second / ref.second - 1.0),
                    avg.second, 100.0 * (avg.second / ref.second - 1.0), avg.first, msAvg);
        // Modell und Schrittweite getrennt bewerten: gemittelter Detektor mit 1 us gegen echten Traeger (Modell);
        // die Abweichung bei 20 us ist der Schrittweitenfehler des Rueckwaertsverfahrens an der RC-Last (Ton 1 kHz,
        // tau 0,22 ms), derselbe wie bei jeder hoerbaren Schaltung in Volta 2
        double msFine = 0.0;
        const auto fine = detect(envelope, 1e-6, true, msFine);
        std::printf("    Modell allein (gemittelt mit 1 us): Ton %.4f V (%+.1f %%); Schrittweitenfehler 20 us: %+.1f %%\n",
                    fine.second, 100.0 * (fine.second / ref.second - 1.0), 100.0 * (avg.second / fine.second - 1.0));
        honest = honest && dev(fine) < kMaxDeviation && std::fabs(avg.first / ref.first - 1.0) < kMaxDeviation;
    }
    verdict(honest, "gemittelter Detektor ehrlich (Modell < 2 % gegen echten Traeger)");
    const double amp = 0.8, m = 0.5;
    auto envelope = [=](double t) { return amp * (1.0 + m * std::sin(2.0 * kPi * fa * t)); };

    // 3 Echtzeit: ganzer Empfaenger mit Verstaerker und Lautsprecher bei 20 us
    {
        NodeId det = 0;
        Simulator sim;
        sim.setCircuit(makeReceiver(envelope, true, det, true));
        sim.setPredictor(g_predictor);
        sim.setRelativeTolerance(g_tolRel);
        const double simS = 1.0 * scale;
        const int n = int(simS / kAudioStepS);
        const auto t1 = Clock::now();
        long iters = 0;
        int depth = 0;
        for (int i = 0; i < n; ++i) {
            sim.step(kAudioStepS);
            iters += sim.lastStepIterations();
            depth = std::max(depth, sim.lastStepMaxDepth());
        }
        const double rt = simS / (msSince(t1) / 1000.0);
        std::printf("  Empfaenger mit Verstaerker und Lautsprecher (%zu Bauteile): %.1f-fache Echtzeit "
                    "(%.2f Newton-Schritte je Zeitschritt, Halbierungstiefe %d)\n",
                    sim.circuit().componentCount(), rt, double(iters) / n, depth);
        verdict(rt >= kMinRealtime, "Empfaenger in Echtzeit (>= 3-fach)");
    }
}

// ---- C Schaltwandler ----

struct ConvResult { double mean; double ripple; double toneAmp; double realtime; };

ConvResult runConverter(const std::function<Circuit(NodeId&)>& make, double dt, double simS, double toneHz) {
    NodeId out = 0;
    Simulator sim;
    sim.setCircuit(make(out));
    sim.setPredictor(g_predictor);
        sim.setRelativeTolerance(g_tolRel);
    const int n = int(std::lround(simS / dt));
    const int window = int(std::lround(0.05 / dt)); // letzte 50 ms
    std::vector<double> v;
    v.reserve(std::size_t(window));
    const auto t0 = Clock::now();
    for (int i = 0; i < n; ++i) {
        if (sim.step(dt) != Status::Ok) std::printf("    (Schritt %d: keine Konvergenz)\n", i);
        if (i >= n - window) v.push_back(sim.probe().voltage(out));
    }
    const double rt = simS / (msSince(t0) / 1000.0);
    ConvResult r{};
    r.realtime = rt;
    double lo = 1e9, hi = -1e9, sum = 0.0;
    for (double x : v) { lo = std::min(lo, x); hi = std::max(hi, x); sum += x; }
    r.mean = sum / double(v.size());
    r.ripple = hi - lo;
    if (toneHz > 0.0) {
        // Fenster beginnt bei simS - 0,05 s: Phase des Tons dort beruecksichtigen ueber Projektion mit Zeitbezug
        double s = 0.0, co = 0.0;
        for (std::size_t i = 0; i < v.size(); ++i) {
            const double t = simS - 0.05 + dt * double(i + 1);
            s += v[i] * std::sin(2.0 * kPi * toneHz * t);
            co += v[i] * std::cos(2.0 * kPi * toneHz * t);
        }
        r.toneAmp = 2.0 * std::hypot(s, co) / double(v.size());
    }
    return r;
}

void report(const char* name, const ConvResult& fine, const ConvResult& coarse, double ideal, bool tone,
            bool counts = true) {
    const double a = tone ? fine.toneAmp : fine.mean, b = tone ? coarse.toneAmp : coarse.mean;
    const double dev = std::fabs(b - a) / std::fabs(a);
    std::printf("  %-34s 1 us: %7.4f V   20 us: %7.4f V   (ideal %.2f V)  Abweichung %.2f %%, Welligkeit %.0f/%.0f mV, "
                "%.1f-fache Echtzeit\n",
                name, a, b, ideal, 100.0 * dev, 1e3 * fine.ripple, 1e3 * coarse.ripple, coarse.realtime);
    if (counts)
        verdict(dev < kMaxDeviation && coarse.realtime >= kMinRealtime, name);
    else
        std::printf("  -> %s: nur zur Information (siehe Lueckbetrieb)\n", name);
}

void partC(double scale) {
    std::printf("\nC Schaltwandler und Klasse D (Schaltfrequenz 1-5 kHz, 20 us gegen 1 us)\n");
    const double simS = 0.4 * scale;
    const DiodeParams schottky = DiodeParams::germanium(); // Stellvertreter fuer die Schottky-Diode (0,35 V)
    auto buck = [&](double hz, double duty) {
        return [=](NodeId& out) {
            Circuit c;
            const NodeId plus = c.addNode(), sw = c.addNode();
            out = c.addNode();
            c.add<Battery>("B", plus, kGround);
            c.add<PwmSwitch>("S", plus, sw, hz, [=](double) { return duty; });
            c.add<Diode>("D", kGround, sw, schottky);
            c.add<Inductor>("L", sw, out, InductorParams{10e-3, 5.0, 1e5, 0.1});
            c.add<Capacitor>("C", out, kGround, CapacitorParams::electrolytic(100e-6));
            c.add<Resistor>("R", out, kGround, ResistorParams{220.0});
            return c;
        };
    };
    for (double hz : {1000.0, 2000.0, 5000.0}) {
        const auto f = buck(hz, 0.5);
        const std::string name = "Abwaertswandler " + std::to_string(int(hz)) + " Hz, 50 %";
        report(name.c_str(), runConverter(f, 1e-6, simS, 0), runConverter(f, kAudioStepS, simS, 0), 4.5, false);
    }
    auto boost = [](double loadOhm) {
        return [=](NodeId& out) {
            Circuit c;
            const NodeId plus = c.addNode(), sw = c.addNode();
            out = c.addNode();
            BatteryParams bp;
            bp.cells = 1;
            c.add<Battery>("B", plus, kGround, bp);
            c.add<Inductor>("L", plus, sw, InductorParams{10e-3, 5.0, 1e5, 0.1});
            c.add<PwmSwitch>("S", sw, kGround, 2000.0, [](double) { return 0.6; });
            c.add<Diode>("D", sw, out, DiodeParams::germanium());
            c.add<Capacitor>("C", out, kGround, CapacitorParams::electrolytic(100e-6));
            c.add<Resistor>("R", out, kGround, ResistorParams{loadOhm});
            return c;
        };
    };
    // 1 kOhm: Spulenstrom lueckt (L_krit = R D (1-D)^2 / 2f = 24 mH > 10 mH); 220 Ohm: nicht lueckend
    for (double r : {1000.0, 220.0}) {
        const std::string name = "Aufwaertswandler 2 kHz, 60 %, " + std::to_string(int(r)) + " Ohm";
        report(name.c_str(), runConverter(boost(r), 1e-6, simS, 0), runConverter(boost(r), kAudioStepS, simS, 0),
               3.75, false, r < 500.0); // lueckend (1 kOhm) bei 20 us nur zur Information
    }
    {
        // Lueckender Betrieb: Das Rueckwaertsverfahren zaehlt die Ladung je Schritt mit dem Strom am Schrittende;
        // endet der Spulenstrom dreieckfoermig, fehlt je Periode etwa ein halber Schritt Ladung (Fehler erster
        // Ordnung, der sich ohne Luecken aufhebt). Abhilfe: kleinere Schritte, solange ein Schaltwandler steckt.
        const ConvResult fine = runConverter(boost(1000.0), 0.25e-6, simS, 0);
        std::printf("  Lueckbetrieb 1 kOhm, Kontrolle 0,25 us: %.4f V\n", fine.mean);
        for (double dt : {10e-6, 5e-6}) {
            const ConvResult r = runConverter(boost(1000.0), dt, simS, 0);
            const double dev = std::fabs(r.mean - fine.mean) / fine.mean;
            std::printf("  Lueckbetrieb 1 kOhm mit %2.0f us: %.4f V, Abweichung %.2f %%, %.1f-fache Echtzeit\n",
                        dt * 1e6, r.mean, 100.0 * dev, r.realtime);
            if (dt < 6e-6) verdict(dev < kMaxDeviation && r.realtime >= kMinRealtime, "Lueckbetrieb mit 5 us");
        }
    }
    // Klasse D: Tastverhaeltnis 50 % +- 40 % mit 200 Hz, Takt 5 kHz, LC-Tiefpass (fg ca. 500 Hz)
    auto classD = [](NodeId& out) {
        Circuit c;
        const NodeId plus = c.addNode(), sw = c.addNode();
        out = c.addNode();
        c.add<Battery>("B", plus, kGround);
        c.add<PwmSwitch>("S", plus, sw, 5000.0, [](double t) { return 0.5 + 0.4 * std::sin(2.0 * kPi * 200.0 * t); });
        c.add<Diode>("D", kGround, sw, DiodeParams::germanium());
        c.add<Inductor>("L", sw, out, InductorParams{10e-3, 5.0, 1e5, 0.1});
        c.add<Capacitor>("C", out, kGround, CapacitorParams::film(10e-6));
        c.add<Resistor>("R", out, kGround, ResistorParams{100.0});
        return c;
    };
    report("Klasse D 5 kHz, Ton 200 Hz", runConverter(classD, 1e-6, simS, 200.0),
           runConverter(classD, kAudioStepS, simS, 200.0), 0.4 * 9.0, true);
}

} // namespace

int main(int argc, char** argv) {
    const double scale = argc > 1 ? std::max(0.1, std::atof(argv[1])) : 1.0;
    g_predictor = !(argc > 2 && std::string(argv[2]) == "ohne");
    std::printf("Praediktor: %s\n", g_predictor ? "an" : "aus");
    std::printf("Volta 3 Machbarkeitstest (Kern %s)\n", volta::core::version());
    const auto t0 = Clock::now();
    partA();
    partB(scale);
    partC(scale);
    std::printf("\nGesamtdauer %.1f s. Ergebnis: %s\n", msSince(t0) / 1000.0,
                g_ok ? "alle Vorgaben erfuellt" : "Vorgaben NICHT alle erfuellt");
    return g_ok ? 0 : 1;
}
