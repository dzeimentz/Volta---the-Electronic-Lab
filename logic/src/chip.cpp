#include "volta/logic/chip.hpp"

#include <utility>

namespace volta::logic {

char toChar(Level level) noexcept {
    switch (level) {
    case Level::Zero: return '0';
    case Level::One: return '1';
    default: return '?';
    }
}

std::optional<Primitive> primitiveFromName(const std::string& name) noexcept {
    if (name == "NAND") return Primitive::Nand;
    if (name == "AND") return Primitive::And;
    if (name == "OR") return Primitive::Or;
    if (name == "NOT") return Primitive::Not;
    if (name == "XOR") return Primitive::Xor;
    return std::nullopt;
}

const char* primitiveName(Primitive p) noexcept {
    switch (p) {
    case Primitive::Nand: return "NAND";
    case Primitive::And: return "AND";
    case Primitive::Or: return "OR";
    case Primitive::Not: return "NOT";
    case Primitive::Xor: return "XOR";
    default: return "";
    }
}

Level evaluate(Primitive p, Level a, Level b) noexcept {
    const bool a0 = a == Level::Zero, a1 = a == Level::One;
    const bool b0 = b == Level::Zero, b1 = b == Level::One;
    switch (p) {
    case Primitive::Buf: return a;
    case Primitive::Not: return a0 ? Level::One : a1 ? Level::Zero : Level::X;
    case Primitive::And:
        if (a0 || b0) return Level::Zero; // eine 0 entscheidet, auch wenn der andere Eingang unbestimmt ist
        return a1 && b1 ? Level::One : Level::X;
    case Primitive::Nand:
        if (a0 || b0) return Level::One;
        return a1 && b1 ? Level::Zero : Level::X;
    case Primitive::Or:
        if (a1 || b1) return Level::One;
        return a0 && b0 ? Level::Zero : Level::X;
    case Primitive::Xor:
        if ((a0 || a1) && (b0 || b1)) return a1 != b1 ? Level::One : Level::Zero;
        return Level::X;
    }
    return Level::X;
}

std::vector<std::string> ChipDef::inputNames() const {
    std::vector<std::string> names;
    for (const Port& p : ports)
        if (p.direction == Direction::In) names.push_back(p.name);
    return names;
}

std::vector<std::string> ChipDef::outputNames() const {
    std::vector<std::string> names;
    for (const Port& p : ports)
        if (p.direction == Direction::Out) names.push_back(p.name);
    return names;
}

const Part* ChipDef::findPart(const std::string& id) const noexcept {
    for (const Part& p : parts)
        if (p.id == id) return &p;
    return nullptr;
}

bool Library::add(ChipDef def) {
    if (def.kuerzel.empty() || primitiveFromName(def.kuerzel)) return false;
    const std::string key = def.kuerzel;
    m_chips[key] = std::make_shared<const ChipDef>(std::move(def));
    return true;
}

bool Library::remove(const std::string& kuerzel) { return m_chips.erase(kuerzel) > 0; }

const ChipDef* Library::find(const std::string& kuerzel) const noexcept {
    const auto it = m_chips.find(kuerzel);
    return it == m_chips.end() ? nullptr : it->second.get();
}

std::vector<std::string> Library::kuerzel() const {
    std::vector<std::string> names;
    for (const auto& entry : m_chips) names.push_back(entry.first);
    return names;
}

bool Library::pinsOf(const std::string& type, std::vector<std::string>& inputs,
                     std::vector<std::string>& outputs) const {
    inputs.clear();
    outputs.clear();
    if (const auto p = primitiveFromName(type)) {
        inputs = *p == Primitive::Not ? std::vector<std::string>{"a"} : std::vector<std::string>{"a", "b"};
        outputs = {"y"};
        return true;
    }
    const ChipDef* def = find(type);
    if (!def) return false;
    inputs = def->inputNames();
    outputs = def->outputNames();
    return true;
}

} // namespace volta::logic
