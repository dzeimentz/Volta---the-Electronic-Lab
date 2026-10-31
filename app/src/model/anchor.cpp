#include "anchor.hpp"

#include <algorithm>
#include <cctype>

namespace volta::model {

const BoardLayout& BoardLayout::volta1() {
    static const BoardLayout layout;
    return layout;
}

const std::vector<std::string>& deskFields() {
    static const std::vector<std::string> fields = {"P:+9V", "P:0V", "P:P1a", "P:P1s", "P:P1b", "P:P2a",
                                                    "P:P2s", "P:P2b", "P:S1", "P:S2", "P:LS1", "P:LS2"};
    return fields;
}

const std::vector<std::string>& extraDeskFields() {
    static const std::vector<std::string> fields = {"P:+B2", "P:-B2"};
    return fields;
}

const std::vector<std::string>& deskGroupNames() {
    static const std::vector<std::string> names = {"batterie", "batterie2", "p1", "p2", "schalter", "lautsprecher"};
    return names;
}

std::vector<std::string> deskGroupFields(const std::string& group) {
    if (group == "batterie") return {"P:+9V", "P:0V"};
    if (group == "batterie2") return {"P:+B2", "P:-B2"};
    if (group == "p1") return {"P:P1a", "P:P1s", "P:P1b"};
    if (group == "p2") return {"P:P2a", "P:P2s", "P:P2b"};
    if (group == "schalter") return {"P:S1", "P:S2"};
    if (group == "lautsprecher") return {"P:LS1", "P:LS2"};
    return {};
}

std::string deskGroupOf(const std::string& anchor) {
    for (const auto& g : deskGroupNames()) {
        const auto f = deskGroupFields(g);
        if (std::find(f.begin(), f.end(), anchor) != f.end()) return g;
    }
    return {};
}

const std::vector<std::string>& meterJacks() {
    static const std::vector<std::string> jacks = {"MESS:COM", "MESS:VOHM", "MESS:A"};
    return jacks;
}

std::string moduleSlot(int n) { return "M:" + std::to_string(n); }

std::vector<std::string> boardFields(const BoardLayout& layout) {
    std::vector<std::string> out;
    for (char r = layout.firstRow; r <= layout.lastRow; ++r)
        for (int c = 1; c <= layout.columns; ++c) out.push_back(std::string(1, r) + std::to_string(c));
    return out;
}

namespace {
bool parseInt(const std::string& s, int& out) {
    if (s.empty() || s.size() > 3) return false;
    for (char ch : s)
        if (!std::isdigit(static_cast<unsigned char>(ch))) return false;
    if (s.size() > 1 && s[0] == '0') return false;
    out = std::stoi(s);
    return true;
}

bool isIdentifier(const std::string& s, bool upper) {
    if (s.empty()) return false;
    for (char ch : s) {
        const auto u = static_cast<unsigned char>(ch);
        if (upper ? !(std::isupper(u) || std::isdigit(u)) : !(std::islower(u) || std::isdigit(u) || ch == '_'))
            return false;
    }
    return true;
}
} // namespace

AnchorKind classifyAnchor(const std::string& a, const BoardLayout& layout) {
    if (a.size() >= 2 && a[0] >= layout.firstRow && a[0] <= layout.lastRow) {
        int col = 0;
        if (parseInt(a.substr(1), col) && col >= 1 && col <= layout.columns) return AnchorKind::Field;
    }
    const auto& desk = deskFields();
    if (std::find(desk.begin(), desk.end(), a) != desk.end()) return AnchorKind::Desk;
    const auto& extra = extraDeskFields();
    if (std::find(extra.begin(), extra.end(), a) != extra.end()) return AnchorKind::Desk;
    const auto& jacks = meterJacks();
    if (std::find(jacks.begin(), jacks.end(), a) != jacks.end()) return AnchorKind::MeterJack;
    if (a.rfind("M:", 0) == 0) {
        int n = 0;
        if (parseInt(a.substr(2), n) && n >= 1 && n <= layout.moduleSlots) return AnchorKind::ModuleSlot;
        return AnchorKind::Invalid;
    }
    // Modulanschluss <Modul-ID>.<anschluss>, z. B. BL1.takt
    const auto dot = a.find('.');
    if (dot != std::string::npos && isIdentifier(a.substr(0, dot), true) && isIdentifier(a.substr(dot + 1), false))
        return AnchorKind::ModulePin;
    return AnchorKind::Invalid;
}

} // namespace volta::model
