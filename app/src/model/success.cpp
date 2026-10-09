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
};

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
        if (const auto p = net.parts.find(c.part); p != net.parts.end())
            index = p->second;
        else if (const auto m = net.moduleMain.find(c.part); m != net.moduleMain.end())
            index = m->second;
        if (index >= sim.probe().readings.size())
            return fail("Erfolgsbedingung: Bauteil '" + c.part + "' nicht im Aufbau");
        const core::ComponentReading& r = sim.probe().readings[index];
        if (c.kind == Kind::State) {
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
        raw = (!c.hasMin || v >= c.min) && (!c.hasMax || v <= c.max);
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
