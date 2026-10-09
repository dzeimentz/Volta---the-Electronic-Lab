#include "build_guide.hpp"

#include <algorithm>
#include <cmath>

#include "anchor.hpp"
#include "board_edit.hpp"

namespace volta::model {

namespace {

bool unpolarized(const std::string& type) {
    return type == "widerstand" || type == "gluehlampe" || type == "taster" || type == "reedkontakt" ||
           type == "motor" || type == "kondensator";
}

// Modulanschluss -> Feld der Leiste (im jeweiligen Aufbau); andere Anschlusspunkte unveraendert
std::string canonical(const Board& board, const Catalog& catalog, const std::string& anchor) {
    if (classifyAnchor(anchor) != AnchorKind::ModulePin) return anchor;
    const std::string slot = moduleSlotOf(board, catalog, anchor);
    return slot.empty() ? anchor : slot;
}

std::vector<std::string> fieldsOf(const std::vector<std::pair<std::string, std::string>>& pins) {
    std::vector<std::string> f;
    for (const auto& p : pins) f.push_back(p.second);
    return f;
}

std::string fieldOf(const std::vector<std::pair<std::string, std::string>>& pins, const std::string& name) {
    for (const auto& p : pins)
        if (p.first == name) return p.second;
    return {};
}

bool samePart(const GuideStep& step, const PartPlacement& p) {
    if (p.type != step.type || p.pins.size() != step.pins.size()) return false;
    if (step.value > 0.0 && std::abs(p.value - step.value) > 1e-9 * std::max(1.0, step.value)) return false;
    if (unpolarized(step.type)) {
        std::vector<std::string> a = fieldsOf(step.pins), b = fieldsOf(p.pins);
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());
        return a == b;
    }
    for (const auto& [name, field] : step.pins) {
        if (step.type == "transistor_npn" && (name == "e" || name == "e2")) {
            // beide Emitterfuesse sind auf der Platine verbunden
            const std::string e = fieldOf(p.pins, "e"), e2 = fieldOf(p.pins, "e2");
            if (field != e && field != e2) return false;
            continue;
        }
        if (fieldOf(p.pins, name) != field) return false;
    }
    return true;
}

} // namespace

std::vector<GuideStep> buildGuide(const Board& tmpl, const Catalog& catalog) {
    std::vector<GuideStep> steps;
    for (const auto& m : tmpl.modules) {
        GuideStep s;
        s.kind = GuideStep::Kind::Module;
        s.id = m.id;
        s.type = m.type;
        s.slot = m.slot;
        const ModuleEntry* e = catalog.module(m.type);
        const int width = e ? static_cast<int>(e->pins.size()) : 1;
        for (int k = 0; k < width; ++k) s.targets.push_back(moduleSlot(m.slot + k));
        steps.push_back(std::move(s));
    }
    for (const auto& p : tmpl.parts) {
        GuideStep s;
        s.kind = GuideStep::Kind::Part;
        s.id = p.id;
        s.type = p.type;
        s.pins = p.pins;
        // Wert nur, wo das Kind ihn waehlt (Widerstand, Nennspannung der Gluehlampe)
        if (p.type == "widerstand" || p.type == "gluehlampe") s.value = p.value;
        s.targets = fieldsOf(p.pins);
        // Felder nur einmal (Platinen)
        std::sort(s.targets.begin(), s.targets.end());
        s.targets.erase(std::unique(s.targets.begin(), s.targets.end()), s.targets.end());
        steps.push_back(std::move(s));
    }
    for (const auto& w : tmpl.wires) {
        GuideStep s;
        s.kind = GuideStep::Kind::Wire;
        s.id = w.id;
        s.from = canonical(tmpl, catalog, w.from);
        s.to = canonical(tmpl, catalog, w.to);
        s.color = w.color;
        s.targets = {s.from, s.to};
        steps.push_back(std::move(s));
    }
    GuideStep on;
    on.kind = GuideStep::Kind::MainSwitch;
    steps.push_back(std::move(on));
    return steps;
}

bool stepDone(const GuideStep& step, const Board& board, const Catalog& catalog) {
    switch (step.kind) {
    case GuideStep::Kind::Module:
        return std::any_of(board.modules.begin(), board.modules.end(),
                           [&step](const ModulePlacement& m) { return m.type == step.type && m.slot == step.slot; });
    case GuideStep::Kind::Part:
        return std::any_of(board.parts.begin(), board.parts.end(),
                           [&step](const PartPlacement& p) { return samePart(step, p); });
    case GuideStep::Kind::Wire:
        return std::any_of(board.wires.begin(), board.wires.end(), [&](const WirePlacement& w) {
            const std::string a = canonical(board, catalog, w.from), b = canonical(board, catalog, w.to);
            return (a == step.from && b == step.to) || (a == step.to && b == step.from);
        });
    case GuideStep::Kind::MainSwitch:
        return board.desk.mainSwitch;
    }
    return false;
}

std::size_t currentStep(const std::vector<GuideStep>& steps, const Board& board, const Catalog& catalog) {
    for (std::size_t i = 0; i < steps.size(); ++i)
        if (!stepDone(steps[i], board, catalog)) return i;
    return steps.size();
}

} // namespace volta::model
