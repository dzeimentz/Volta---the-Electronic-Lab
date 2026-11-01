#include "success.hpp"

#include <algorithm>
#include <cmath>

#include "volta/core/components/motor.hpp"

namespace volta::model {

struct SuccessEvaluator::Node {
    SuccessCondition cond;
    std::vector<Node> children;
    double sinceS = -1.0; // Zeitpunkt, seit dem die Rohbedingung gilt (Haltedauer); < 0: gilt nicht
    bool reportedError = false;
    // Zeitfenster (Statistic != Instant)
    struct Window {
        double startS = -1.0, lastS = -1.0;
        double min = 0.0, max = 0.0, sum = 0.0, sumSq = 0.0, dur = 0.0;
        int crossings = 0;
        bool above = false;
        double mid = 0.0, hyst = 0.0; // aus dem vorigen Fenster
        bool haveMid = false;
        bool result = false;
        void clear() { *this = Window{}; }
    } win;
};

namespace {
// Fenster fortschreiben; true, wenn es gerade abgeschlossen wurde (value = Ergebnis der Auswertung)
bool accumulate(SuccessEvaluator::Node::Window& w, SuccessCondition::Statistic stat, double windowS, double t,
                double v, double& value) {
    using S = SuccessCondition::Statistic;
    if (w.startS < 0.0 || t < w.lastS) { // Beginn (oder Zeitsprung zurueck)
        const bool keepMid = w.haveMid && t >= w.lastS;
        const double mid = w.mid, hyst = w.hyst;
        w = SuccessEvaluator::Node::Window{};
        if (keepMid) { w.mid = mid; w.hyst = hyst; w.haveMid = true; }
        w.startS = w.lastS = t;
        w.min = w.max = v;
        w.above = v > w.mid;
        return false;
    }
    const double dt = t - w.lastS;
    w.lastS = t;
    w.min = std::min(w.min, v);
    w.max = std::max(w.max, v);
    w.sum += v * dt;
    w.sumSq += v * v * dt;
    w.dur += dt;
    if (w.haveMid) {
        if (!w.above && v > w.mid + w.hyst) {
            w.above = true;
            ++w.crossings;
        } else if (w.above && v < w.mid - w.hyst) {
            w.above = false;
        }
    }
    const double need = stat == S::Frequency ? std::max(windowS, 1.0) : windowS;
    if (w.dur < need - 1e-12) return false;
    const double mean = w.sum / w.dur;
    switch (stat) {
    case S::Peak: value = std::max(std::fabs(w.min), std::fabs(w.max)); break;
    case S::PeakToPeak: value = w.max - w.min; break;
    case S::Mean: value = mean; break;
    case S::Rms: value = std::sqrt(std::max(0.0, w.sumSq / w.dur - mean * mean)); break;
    case S::Frequency: value = w.haveMid ? w.crossings / w.dur : -1.0; break;
    case S::Instant: value = v; break;
    }
    // naechstes Fenster: Mitte und Hysterese aus diesem; ohne Schwankung zaehlt kein Durchgang
    const double pp = w.max - w.min;
    w.mid = 0.5 * (w.max + w.min);
    w.hyst = pp > 1e-9 ? 0.1 * pp : 1e300;
    w.haveMid = true;
    w.startS = t;
    w.min = w.max = v;
    w.sum = w.sumSq = w.dur = 0.0;
    w.crossings = 0;
    w.above = v > w.mid;
    return true;
}
} // namespace

void SuccessEvaluator::build(Node& node, const SuccessCondition& c) {
    node.cond = c;
    node.cond.children.clear();
    node.children.resize(c.children.size());
    for (std::size_t i = 0; i < c.children.size(); ++i) build(node.children[i], c.children[i]);
}

SuccessEvaluator::SuccessEvaluator(SuccessCondition condition) : m_root(std::make_unique<Node>()) {
    build(*m_root, condition);
}

SuccessEvaluator::~SuccessEvaluator() = default;
SuccessEvaluator::SuccessEvaluator(SuccessEvaluator&&) noexcept = default;
SuccessEvaluator& SuccessEvaluator::operator=(SuccessEvaluator&&) noexcept = default;

void SuccessEvaluator::reset() {
    m_reached = false;
    m_errors.clear();
    struct Reset {
        static void run(Node& n) {
            n.sinceS = -1.0;
            n.reportedError = false;
            n.win.clear();
            for (auto& ch : n.children) run(ch);
        }
    };
    Reset::run(*m_root);
}

bool SuccessEvaluator::update(const core::Simulator& sim, const Netlist& net) {
    if (!sim.probe().valid) return false;
    const bool ok = evaluate(*m_root, sim, net);
    if (ok) m_reached = true;
    return ok;
}

bool SuccessEvaluator::evaluate(Node& node, const core::Simulator& sim, const Netlist& net) {
    using Kind = SuccessCondition::Kind;
    const SuccessCondition& c = node.cond;
    switch (c.kind) {
    case Kind::All: {
        bool all = true;
        for (auto& ch : node.children) all = evaluate(ch, sim, net) && all; // alle auswerten (Haltezeiten)
        return all;
    }
    case Kind::Any: {
        bool any = false;
        for (auto& ch : node.children) any = evaluate(ch, sim, net) || any;
        return any;
    }
    case Kind::Not:
        return node.children.empty() ? false : !evaluate(node.children.front(), sim, net);
    default:
        break;
    }

    auto fail = [&](const std::string& msg) {
        if (!node.reportedError) {
            m_errors.push_back(msg);
            node.reportedError = true;
        }
        return false;
    };

    bool raw = false;
    if (!c.part.empty()) {
        // Bauteil oder Modul (Messwerte des Moduls: Netlist::moduleMain)
        std::size_t index = sim.probe().readings.size();
        if (c.part == "MESS") { // Dreifach-Messgeraet des Pults
            if (const auto d = net.desk.find(desk_id::kMeter); d != net.desk.end()) index = d->second;
        } else if (const auto p = net.parts.find(c.part); p != net.parts.end())
            index = p->second;
        else if (const auto m = net.moduleMain.find(c.part); m != net.moduleMain.end())
            index = m->second;
        if (index >= sim.probe().readings.size())
            return fail("Erfolgsbedingung: Bauteil '" + c.part + "' nicht im Aufbau");
        const core::ComponentReading& r = sim.probe().readings[index];
        if (c.kind == Kind::Quantity && c.statistic != SuccessCondition::Statistic::Instant) {
            double v = 0.0; // vorzeichenrichtig
            switch (c.quantity) {
            case SuccessCondition::Quantity::CurrentMilliA: v = r.currentA * 1e3; break;
            case SuccessCondition::Quantity::VoltageV: v = r.voltV; break;
            case SuccessCondition::Quantity::PowerMilliW: v = r.powerW * 1e3; break;
            case SuccessCondition::Quantity::Brightness: v = r.level; break;
            case SuccessCondition::Quantity::FrequencyHz: v = r.frequencyHz; break;
            case SuccessCondition::Quantity::TemperatureC: v = r.tempK - 273.15; break;
            case SuccessCondition::Quantity::Display: v = r.displayValue; break;
            case SuccessCondition::Quantity::Rpm: v = 0.0; break;
            }
            double s = 0.0;
            if (accumulate(node.win, c.statistic, c.windowS, sim.timeS(), v, s))
                node.win.result = s >= 0.0 && (!c.hasMin || s >= c.min) && (!c.hasMax || s <= c.max);
            raw = node.win.result;
        } else if (c.kind == Kind::State) {
            switch (c.state) {
            case SuccessCondition::State::Intact: raw = !r.destroyed && !r.overloaded; break;
            case SuccessCondition::State::Overloaded: raw = r.overloaded && !r.destroyed; break;
            case SuccessCondition::State::Destroyed: raw = r.destroyed; break;
            }
        } else {
            double v = 0.0;
            switch (c.quantity) {
            case SuccessCondition::Quantity::CurrentMilliA: v = std::fabs(r.currentA) * 1e3; break;
            case SuccessCondition::Quantity::VoltageV: v = std::fabs(r.voltV); break;
            case SuccessCondition::Quantity::PowerMilliW: v = std::fabs(r.powerW) * 1e3; break;
            case SuccessCondition::Quantity::Brightness: v = r.level; break;
            case SuccessCondition::Quantity::FrequencyHz: v = r.frequencyHz; break;
            case SuccessCondition::Quantity::TemperatureC: v = r.tempK - 273.15; break;
            case SuccessCondition::Quantity::Display: v = std::fabs(r.displayValue); break;
            case SuccessCondition::Quantity::Rpm: {
                const auto* motor = dynamic_cast<const core::Motor*>(&sim.circuit().component(index));
                v = motor ? std::fabs(motor->rpm()) : 0.0;
                break;
            }
            }
            raw = (!c.hasMin || v >= c.min) && (!c.hasMax || v <= c.max);
        }
    } else if (!c.field.empty()) {
        core::NodeId n = 0;
        if (!net.nodeOf(c.field, n)) return fail("Erfolgsbedingung: Feld '" + c.field + "' nicht im Aufbau");
        const double v = sim.probe().voltage(n);
        if (c.statistic != SuccessCondition::Statistic::Instant) {
            double s = 0.0;
            if (accumulate(node.win, c.statistic, c.windowS, sim.timeS(), v, s))
                node.win.result = s >= 0.0 && (!c.hasMin || s >= c.min) && (!c.hasMax || s <= c.max);
            raw = node.win.result;
        } else {
            raw = (!c.hasMin || v >= c.min) && (!c.hasMax || v <= c.max);
        }
    } else {
        return fail("Erfolgsbedingung ohne Bauteil oder Feld");
    }

    // Haltedauer
    const double t = sim.timeS();
    if (!raw) {
        node.sinceS = -1.0;
        return false;
    }
    if (node.sinceS < 0.0) node.sinceS = t;
    return t - node.sinceS >= c.holdS - 1e-9;
}

} // namespace volta::model
