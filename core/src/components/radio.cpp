#include "volta/core/components/radio.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "volta/core/simulator.hpp"

namespace volta::core {

namespace {
constexpr double kPi = 3.14159265358979323846;

// Morsezeichen "VOLTA" in Einheiten (1 = Ton, 0 = Pause): Punkt 1, Strich 3, Pause im Zeichen 1, zwischen
// Buchstaben 3, danach Wortpause und eine laengere Pause bis zur Wiederholung.
constexpr char kMorse[] = "1010101110"   // V ...-
                          "00"
                          "11101110111" // O ---
                          "000"
                          "101110101"   // L .-..
                          "000"
                          "111"         // T -
                          "000"
                          "10111"       // A .-
                          "00000000000000000000";
constexpr std::size_t kMorseUnits = sizeof(kMorse) - 1;
constexpr double kMorseUnitS = 0.08; // 15 Woerter je Minute

// Tonfolgen: Halbtoene ueber C5 = 523,25 Hz (kRest = Pause) und Dauer in Einheiten
constexpr int kRest = -100;
struct Note {
    int semitone;
    int units;
};

// Volta-Erkennungsmelodie (eigene Komposition): "Vol-ta, Vol-ta" als Quintsprung abwaerts, dann die Tonleiter
// hinauf ("das E-lek-tro-nik-La-bor"); Einheit = Achtel zu 0,2 s, 6,4 s mit Pause
constexpr Note kMelody[] = {{7, 2}, {0, 2}, {7, 2}, {0, 2}, {kRest, 1}, {0, 1}, {2, 1}, {4, 1}, {5, 1},
                            {7, 1}, {9, 1}, {7, 4}, {kRest, 4}};
constexpr double kMelodyUnitS = 0.2;

// "Minuit, chretiens" / "O Holy Night" (Adolphe Adam 1847, gemeinfrei), erste Zeile bis "... Saviour's birth",
// abgelesen aus der Ausgabe G. Schirmer 1871 ("Social Evenings", Wikimedia Commons); eine Oktave hoeher wie auf der
// Geige. Einheit = Triolenachtel (der 4/4-Satz mit punktierten Sechzehnteln entspricht dem ueblichen 12/8), 0,28 s;
// 15,4 s mit Pause
constexpr Note kCarol[] = {{4, 1},  {4, 5},  {7, 1}, {7, 5}, {7, 1}, {9, 2}, {9, 1},  {5, 2}, {9, 1},
                           {12, 6}, {7, 2},  {7, 1}, {4, 2}, {2, 1}, {0, 2}, {0, 1},  {4, 2}, {5, 1},
                           {7, 3},  {5, 2},  {2, 1}, {0, 6}, {kRest, 6}};
constexpr double kCarolUnitS = 0.28;

template <std::size_t N>
double playNotes(const Note (&notes)[N], double unitS, double timeS) noexcept {
    int total = 0;
    for (const Note& n : notes) total += n.units;
    const double periodS = total * unitS;
    double t = std::fmod(timeS, periodS);
    for (const Note& n : notes) {
        const double durS = n.units * unitS;
        if (t < durS) {
            if (n.semitone == kRest || t > durS - 0.02) return 0.0; // kurze Luecke zwischen den Toenen
            return std::sin(2.0 * kPi * 523.25 * std::pow(2.0, n.semitone / 12.0) * timeS);
        }
        t -= durS;
    }
    return 0.0;
}

double tone(double hz, double t) noexcept { return std::sin(2.0 * kPi * hz * t); }
} // namespace

double stationSignal(StationProgram program, double toneHz, double timeS) noexcept {
    if (!std::isfinite(timeS) || timeS < 0.0) timeS = 0.0;
    if (!(toneHz > 0.0) || !std::isfinite(toneHz)) toneHz = 1000.0;
    switch (program) {
    case StationProgram::Tone:
        return tone(toneHz, timeS);
    case StationProgram::Morse: {
        const auto unit = static_cast<std::size_t>(std::floor(timeS / kMorseUnitS)) % kMorseUnits;
        return kMorse[unit] == '1' ? tone(toneHz, timeS) : 0.0;
    }
    case StationProgram::TimeSignal: {
        const double sec = std::floor(timeS);
        const double frac = timeS - sec;
        const bool full = std::fmod(sec, 10.0) < 0.5;
        return frac < (full ? 0.5 : 0.1) ? tone(toneHz, timeS) : 0.0;
    }
    case StationProgram::Melody:
        return playNotes(kMelody, kMelodyUnitS, timeS);
    case StationProgram::Carol:
        return playNotes(kCarol, kCarolUnitS, timeS);
    }
    return 0.0;
}

// ---------------------------------------------------------------- Antenneneinspeisung

AntennaFeed::AntennaFeed(std::string id, NodeId ant, NodeId ref, double sourceOhm)
    : ComponentModel(std::move(id), {ant, ref}),
      m_ohm(std::isfinite(sourceOhm) && sourceOhm >= kMinOhm ? sourceOhm : 1e3) {}

void AntennaFeed::stamp(MnaSystem& sys, const StampContext& /*ctx*/) const noexcept {
    sys.stampConductance(terminal(0), terminal(1), 1.0 / m_ohm);
}

bool AntennaFeed::stampAc(ComplexMna& sys, const AcContext& ctx) const noexcept {
    // Quelle 1 V hinter dem Innenwiderstand als Ersatzstromquelle (1 V / R parallel zu 1 / R)
    sys.stampAdmittance(terminal(0), terminal(1), 1.0 / m_ohm);
    if (ctx.excite) {
        auto& b = sys.rhs();
        if (terminal(0) != kGround) b[terminal(0) - 1] += 1.0 / m_ohm;
        if (terminal(1) != kGround) b[terminal(1) - 1] -= 1.0 / m_ohm;
    }
    return true;
}

double AntennaFeed::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    const double i = (x.voltage(terminal(0)) - x.voltage(terminal(1))) / m_ohm;
    return t == 0 ? i : -i;
}

// ---------------------------------------------------------------- Detektordiode

DetectorDiode::DetectorDiode(std::string id, NodeId anode, NodeId cathode, DiodeParams params)
    : ComponentModel(std::move(id), {anode, cathode}) {
    // Kennwerte wie die Diode im Zeitbereich (gleiche Pruefung der Parameter, gleicher Saettigungsstrom)
    const Diode probe("probe", 1, 0, params);
    m_params = probe.params();
    const double is = probe.saturationCurrentA(), nvt = m_params.n * probe.thermalVoltageV();
    const double r = std::max(m_params.rsOhm, kMinOhm);
    m_table.resize(kTableN);
    for (std::size_t i = 0; i < kTableN; ++i) {
        const double v = kVmin + kDv * double(i);
        // vd + r * Is (exp(vd / nVt) - 1) = v nach vd loesen (Newton, monoton; Startwert unterhalb der Loesung)
        double vd = std::min(v, 0.3);
        for (int k = 0; k < 80; ++k) {
            const double e = std::exp(std::min(vd / nvt, 80.0));
            const double f = vd + r * is * (e - 1.0) - v;
            const double df = 1.0 + r * is * e / nvt;
            const double step = f / df;
            vd -= step;
            if (std::fabs(step) < 1e-15) break;
        }
        m_table[i] = is * (std::exp(std::min(vd / nvt, 80.0)) - 1.0);
    }
    // Phasen (k + 0,5) / 64 einer Periode; cos ist symmetrisch: 32 Stuetzstellen genuegen
    for (int k = 0; k < kPhases; ++k) m_cos[std::size_t(k)] = std::cos(kPi * (k + 0.5) / kPhases);
    evaluate(0.0, 0.0);
}

void DetectorDiode::setDrive(const Drive* drives, std::size_t count, double sourceOhm, double cEnvF) noexcept {
    m_driveCount = 0;
    for (std::size_t i = 0; i < count && m_driveCount < kMaxStations; ++i) {
        Drive d = drives[i];
        if (!(d.e0V > 0.0) || !std::isfinite(d.e0V)) continue;
        d.depth = std::isfinite(d.depth) ? std::clamp(d.depth, 0.0, 1.0) : 0.0;
        m_drives[m_driveCount++] = d;
    }
    m_sourceOhm = std::isfinite(sourceOhm) && sourceOhm >= kMinOhm ? sourceOhm : 1.0;
    m_cEnvF = m_driveCount > 0 && std::isfinite(cEnvF) && cEnvF > 0.0 ? cEnvF : 0.0;
}

void DetectorDiode::beginSolve(const StampContext& ctx) noexcept {
    double sum = 0.0;
    for (std::size_t i = 0; i < m_driveCount; ++i) {
        const Drive& d = m_drives[i];
        const double env = d.e0V * (1.0 + d.depth * stationSignal(d.program, d.toneHz, ctx.timeS));
        sum += env * env;
    }
    m_e0Now = std::sqrt(sum);
    m_rNow = m_driveCount > 0 ? m_sourceOhm : 1.0;
}

void DetectorDiode::g(double v, double& i, double& di) const noexcept {
    double u = (v - kVmin) / kDv;
    u = std::clamp(u, 0.0, double(kTableN - 1) - 1e-9);
    const std::size_t k = std::size_t(u);
    const double fr = u - double(k);
    di = (m_table[k + 1] - m_table[k]) / kDv;
    // innerhalb linear interpoliert, ausserhalb mit der Randsteigung fortgesetzt
    i = m_table[k] + fr * (m_table[k + 1] - m_table[k]) + di * (v - (kVmin + kDv * u));
}

void DetectorDiode::evaluate(double e, double v) noexcept {
    if (!std::isfinite(e)) e = 0.0;
    if (!std::isfinite(v)) v = 0.0;
    double s0 = 0.0, s0d = 0.0, s1 = 0.0, s1d = 0.0, s2d = 0.0;
    for (double c : m_cos) {
        double i = 0.0, di = 0.0;
        g(v + e * c, i, di);
        s0 += i;
        s0d += di;
        s1 += i * c;
        s1d += di * c;
        s2d += di * c * c;
    }
    constexpr double n = kPhases;
    m_eLin = e;
    m_vLin = v;
    m_i0 = s0 / n;
    m_i0dv = s0d / n + kJunctionGmin;
    m_i0de = s1d / n;
    m_i1 = 2.0 * s1 / n;
    m_i1dv = 2.0 * s1d / n;
    m_i1de = 2.0 * s2d / n;
}

bool DetectorDiode::linearize(const SolutionView& x) noexcept {
    evaluate(amp(x), vak(x));
    return false;
}

void DetectorDiode::resetLinearization(const SolutionView& x) noexcept { evaluate(amp(x), vak(x)); }

void DetectorDiode::stamp(MnaSystem& sys, const StampContext& ctx) const noexcept {
    const NodeId a = terminal(0), k = terminal(1), e = internalNode(0);
    // Traegeramplitude: (E0 - E) / R = I1 + C_h dE/dt (Kreis als Quelle mit Innenwiderstand und Traegheit, Diode
    // entnimmt die Grundschwingung); C_h im Rueckwaertsverfahren wie ein Kondensator, im Arbeitspunkt offen
    sys.stampConductance(e, kGround, 1.0 / m_rNow);
    sys.stampNodeRhs(e, m_e0Now / m_rNow);
    if (m_cEnvF > 0.0 && !ctx.isDc()) {
        const double gc = m_cEnvF / ctx.dtS;
        sys.stampConductance(e, kGround, gc);
        sys.stampNodeRhs(e, gc * m_ePrev);
    }
    if (m_destroyed) return; // zerstoert: offen, kein Strom
    const double ieq1 = m_i1 - m_i1dv * m_vLin - m_i1de * m_eLin;
    sys.stampNodeMatrix(e, a, m_i1dv);
    sys.stampNodeMatrix(e, k, -m_i1dv);
    sys.stampNodeMatrix(e, e, m_i1de);
    sys.stampNodeRhs(e, -ieq1);
    // Gleichstrom Anode -> Kathode: I0 = i0dv * vak + i0de * E + ieq0
    const double ieq0 = m_i0 - m_i0dv * m_vLin - m_i0de * m_eLin;
    sys.stampNodeMatrix(a, a, m_i0dv);
    sys.stampNodeMatrix(a, k, -m_i0dv);
    sys.stampNodeMatrix(a, e, m_i0de);
    sys.stampNodeRhs(a, -ieq0);
    sys.stampNodeMatrix(k, a, -m_i0dv);
    sys.stampNodeMatrix(k, k, m_i0dv);
    sys.stampNodeMatrix(k, e, -m_i0de);
    sys.stampNodeRhs(k, ieq0);
}

bool DetectorDiode::stampAc(ComplexMna& sys, const AcContext& ctx) const noexcept {
    sys.stampAdmittance(internalNode(0), kGround, 1.0); // innerer Knoten bleibt bestimmt
    if (ctx.excite) {
        auto& b = sys.rhs();
        if (terminal(0) != kGround) b[terminal(0) - 1] += 1.0;
        if (terminal(1) != kGround) b[terminal(1) - 1] -= 1.0;
    }
    return true;
}

double DetectorDiode::terminalCurrent(const SolutionView& x, std::size_t t) const noexcept {
    if (m_destroyed) return 0.0;
    const double i = m_i0 + m_i0dv * (vak(x) - m_vLin) + m_i0de * (amp(x) - m_eLin);
    return t == 0 ? i : -i;
}

void DetectorDiode::updateState(const SolutionView& x, double /*dtS*/) noexcept {
    const double e = amp(x);
    m_ePrev = std::isfinite(e) ? e : 0.0;
    m_lastE = std::max(m_ePrev, 0.0);
}

void DetectorDiode::thermal(const SolutionView& x, double dtS) noexcept {
    m_lastDtS = dtS;
    const double i = terminalCurrent(x, 0);
    m_lastCurrentA = std::fabs(i);
    // mittlere Leistung naeherungsweise aus Gleichanteil und Grundschwingung
    const double p = vak(x) * i + 0.5 * amp(x) * (m_destroyed ? 0.0 : m_i1);
    integrateThermal(m_destroyed ? 0.0 : std::max(p, 0.0), dtS, m_params.thermal);
}

void DetectorDiode::checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept {
    if (isDestroyed()) return;
    m_overloaded = m_lastCurrentA > m_params.imaxA;
    const bool severe = m_lastCurrentA > m_params.overloadFactor * m_params.imaxA;
    m_overloadS = severe ? m_overloadS + m_lastDtS : 0.0;
    if (severe && m_overloadS >= m_params.overloadFailS - 1e-9) destroy(events, index, timeS);
    checkThermalLimit(m_params.thermal, events, index, timeS);
}

ComponentReading DetectorDiode::read(const SolutionView& x) const noexcept {
    ComponentReading r = ComponentModel::read(x);
    r.level = (m_destroyed || r.currentA <= 0.0) ? 0.0 : r.currentA / m_params.ifRefA;
    r.displayValue = m_destroyed ? 0.0 : std::max(amp(x), 0.0); // Traegeramplitude an der Diode
    return r;
}

ComponentState DetectorDiode::saveState() const noexcept {
    ComponentState s = ComponentModel::saveState();
    s.internal[0] = m_overloadS;
    s.internal[1] = m_ePrev;
    return s;
}

void DetectorDiode::restoreState(const ComponentState& state) noexcept {
    ComponentModel::restoreState(state);
    m_overloadS = std::isfinite(state.internal[0]) ? state.internal[0] : 0.0;
    m_ePrev = std::isfinite(state.internal[1]) ? state.internal[1] : 0.0;
    m_lastE = std::max(m_ePrev, 0.0);
}

// ---------------------------------------------------------------- Abstimmen

Status tuneRadio(Simulator& sim, const std::vector<RadioStation>& stations) {
    if (!sim.hasCircuit()) return Status::NoCircuit;
    Circuit& c = sim.circuit();
    std::size_t antenna = c.componentCount();
    std::vector<std::size_t> detectors;
    for (std::size_t i = 0; i < c.componentCount(); ++i) {
        ComponentModel& m = c.component(i);
        if (dynamic_cast<AntennaFeed*>(&m) != nullptr && antenna == c.componentCount()) antenna = i;
        if (dynamic_cast<DetectorDiode*>(&m) != nullptr) detectors.push_back(i);
    }
    std::vector<const RadioStation*> on;
    std::vector<double> freqs;
    for (const RadioStation& s : stations)
        if (s.on && s.carrierHz > 0.0 && std::isfinite(s.carrierHz) && s.emfV > 0.0 && std::isfinite(s.emfV)) {
            on.push_back(&s);
            freqs.push_back(s.carrierHz);
        }
    for (std::size_t d : detectors) static_cast<DetectorDiode&>(c.component(d)).clearDrive();
    if (antenna == c.componentCount() || on.empty()) return Status::Ok;

    // je Sender zusaetzlich eine Frequenz knapp daneben: Steigung der Admittanz fuer die Traegheit des Kreises
    constexpr double kRel = 1e-4;
    std::vector<double> freqs2;
    for (double f : freqs) {
        freqs2.push_back(f);
        freqs2.push_back(f * (1.0 + kRel));
    }
    std::vector<Complex> h, z;
    std::vector<DetectorDiode::Drive> drives;
    for (std::size_t d : detectors) {
        auto& det = static_cast<DetectorDiode&>(c.component(d));
        const NodeId a = det.terminal(0), k = det.terminal(1);
        Status st = sim.acSweep(antenna, freqs, a, k, h);
        if (st != Status::Ok) return st;
        st = sim.acSweep(d, freqs2, a, k, z);
        if (st != Status::Ok) return st;
        drives.clear();
        double strongest = 0.0, r = 1.0, cEnv = 0.0;
        for (std::size_t i = 0; i < on.size(); ++i) {
            DetectorDiode::Drive dr;
            dr.e0V = on[i]->emfV * std::abs(h[i]);
            dr.depth = on[i]->depth;
            dr.program = on[i]->program;
            dr.toneHz = on[i]->toneHz;
            drives.push_back(dr);
            if (dr.e0V > strongest) {
                const Complex z0 = z[2 * i], z1 = z[2 * i + 1];
                strongest = dr.e0V;
                r = std::max(z0.real(), kMinOhm);
                if (std::abs(z0) > 0.0 && std::abs(z1) > 0.0) {
                    const double dw = 2.0 * 3.14159265358979323846 * freqs[i] * kRel;
                    cEnv = std::max(((1.0 / z1 - 1.0 / z0) / dw).imag(), 0.0);
                }
            }
        }
        det.setDrive(drives.data(), drives.size(), r, cEnv);
    }
    return Status::Ok;
}

} // namespace volta::core
