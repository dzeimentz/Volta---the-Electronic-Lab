#include "parts_list.hpp"

#include <algorithm>
#include <cmath>

namespace volta::model {

namespace {
PartsLine& lineFor(std::vector<PartsLine>& lines, PartsLine::Kind kind, const std::string& type, double value) {
    for (auto& l : lines)
        if (l.kind == kind && l.type == type && std::abs(l.value - value) <= 1e-12 * std::max(1.0, std::abs(value)))
            return l;
    lines.push_back({kind, type, value, 0, {}});
    return lines.back();
}
} // namespace

std::vector<PartsLine> partsList(const Board& board) {
    std::vector<PartsLine> modules, parts, wires, tools;
    for (const auto& m : board.modules) {
        auto& l = lineFor(modules, PartsLine::Kind::Module, m.type, 0.0);
        ++l.count;
        l.ids.push_back(m.id);
    }
    for (const auto& p : board.parts) {
        auto& l = lineFor(parts, PartsLine::Kind::Part, p.type, p.value);
        ++l.count;
        l.ids.push_back(p.id);
    }
    for (const auto& w : board.wires) {
        auto& l = lineFor(wires, PartsLine::Kind::Wire, w.color, 0.0);
        ++l.count;
        l.ids.push_back(w.id);
    }
    for (const auto& t : board.desk.tools) ++lineFor(tools, PartsLine::Kind::Tool, t.type, 0.0).count;
    std::stable_sort(parts.begin(), parts.end(), [](const PartsLine& a, const PartsLine& b) {
        return a.type != b.type ? a.type < b.type : a.value < b.value;
    });
    std::vector<PartsLine> out;
    for (auto* group : {&modules, &parts, &wires, &tools}) out.insert(out.end(), group->begin(), group->end());
    return out;
}

} // namespace volta::model
