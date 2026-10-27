#include "wire_router.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace volta::model {

namespace {

struct Grid {
    double ox = 0.0, oy = 0.0, cell = 8.0;
    int w = 0, h = 0;
    std::vector<unsigned char> blocked; // Bauteilkoerper samt Abstand
    std::vector<float> extra;           // Zusatzkosten: Naehe zu Bauteilen, schon verlegte Leitungen

    int index(int cx, int cy) const { return cy * w + cx; }
    bool inside(int cx, int cy) const { return cx >= 0 && cy >= 0 && cx < w && cy < h; }
    int cellX(double x) const { return std::clamp(static_cast<int>(std::floor((x - ox) / cell)), 0, w - 1); }
    int cellY(double y) const { return std::clamp(static_cast<int>(std::floor((y - oy) / cell)), 0, h - 1); }
    RouterPoint center(int cx, int cy) const { return {ox + (cx + 0.5) * cell, oy + (cy + 0.5) * cell}; }
};

bool inRect(const RouterRect& r, double x, double y, double grow) {
    return x >= r.x - grow && x <= r.x + r.w + grow && y >= r.y - grow && y <= r.y + r.h + grow;
}

double dist(const RouterPoint& a, const RouterPoint& b) { return std::hypot(b.x - a.x, b.y - a.y); }

// Endpunkte liegen in Feldern, oft dicht neben dem Bein eines Bauteils: dort darf die Leitung beginnen
bool nearEnds(const RouterPoint& p, const RouterPoint& a, const RouterPoint& b, double radius) {
    return dist(p, a) <= radius || dist(p, b) <= radius;
}

bool pointFree(const std::vector<RouterRect>& obstacles, const RouterPoint& p, double grow) {
    for (const auto& r : obstacles)
        if (inRect(r, p.x, p.y, grow)) return false;
    return true;
}

// Strecke frei von Bauteilen (in kleinen Schritten abgetastet), ausgenommen die Umgebung der Endpunkte
bool segmentFree(const std::vector<RouterRect>& obstacles, const RouterPoint& p, const RouterPoint& q, double grow,
                 const RouterPoint& a, const RouterPoint& b, double endRadius) {
    const double len = dist(p, q);
    const int n = std::max(1, static_cast<int>(len / 3.0));
    for (int i = 0; i <= n; ++i) {
        const double t = static_cast<double>(i) / n;
        const RouterPoint s{p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t};
        if (nearEnds(s, a, b, endRadius)) continue;
        if (!pointFree(obstacles, s, grow)) return false;
    }
    return true;
}

RouterPoint arcPoint(const RouterPoint& a, const RouterPoint& b, double lift, double t) {
    const double u = 1.0 - t;
    const double c1y = a.y - lift, c2y = b.y - lift;
    return {u * u * u * a.x + 3 * u * u * t * a.x + 3 * u * t * t * b.x + t * t * t * b.x,
            u * u * u * a.y + 3 * u * u * t * c1y + 3 * u * t * t * c2y + t * t * t * b.y};
}

void markUsage(Grid& g, const std::vector<RouterPoint>& pts) {
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        const double len = dist(pts[i], pts[i + 1]);
        const int n = std::max(1, static_cast<int>(len / (g.cell / 2)));
        for (int k = 0; k <= n; ++k) {
            const double t = static_cast<double>(k) / n;
            const int cx = g.cellX(pts[i].x + (pts[i + 1].x - pts[i].x) * t);
            const int cy = g.cellY(pts[i].y + (pts[i + 1].y - pts[i].y) * t);
            g.extra[g.index(cx, cy)] += 1.5f;
        }
    }
}

constexpr int kDx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
constexpr int kDy[8] = {0, 1, 1, 1, 0, -1, -1, -1};

// A* ueber (Zelle, Richtung): Knicke kosten extra, damit die Leitung ruhig verlaeuft
std::vector<RouterPoint> searchPath(const Grid& g, const RouterPoint& a, const RouterPoint& b, double endRadius) {
    const int sx = g.cellX(a.x), sy = g.cellY(a.y), tx = g.cellX(b.x), ty = g.cellY(b.y);
    const int states = g.w * g.h * 8;
    std::vector<float> cost(states, std::numeric_limits<float>::infinity());
    std::vector<int> from(states, -1);
    using Item = std::pair<float, int>;
    std::priority_queue<Item, std::vector<Item>, std::greater<>> open;
    auto heuristic = [&](int cx, int cy) {
        const double dx = std::abs(cx - tx), dy = std::abs(cy - ty);
        return static_cast<float>(std::max(dx, dy) + (std::sqrt(2.0) - 1.0) * std::min(dx, dy));
    };
    auto passable = [&](int cx, int cy) {
        if (!g.blocked[g.index(cx, cy)]) return true;
        return nearEnds(g.center(cx, cy), a, b, endRadius);
    };
    for (int d = 0; d < 8; ++d) {
        const int s = g.index(sx, sy) * 8 + d;
        cost[s] = 0.0f;
        open.push({heuristic(sx, sy), s});
    }
    int goal = -1;
    while (!open.empty()) {
        const auto [f, s] = open.top();
        open.pop();
        const int c = s / 8, d = s % 8, cx = c % g.w, cy = c / g.w;
        if (f - heuristic(cx, cy) > cost[s] + 1e-3f) continue; // veraltet
        if (cx == tx && cy == ty) {
            goal = s;
            break;
        }
        for (int nd = 0; nd < 8; ++nd) {
            const int nx = cx + kDx[nd], ny = cy + kDy[nd];
            if (!g.inside(nx, ny) || !passable(nx, ny)) continue;
            const int turn = std::min((nd - d + 8) % 8, (d - nd + 8) % 8);
            if (turn > 2) continue; // keine Spitzkehren
            const float step = (nd % 2 ? 1.4142f : 1.0f) + 0.6f * turn + g.extra[g.index(nx, ny)];
            const int ns = g.index(nx, ny) * 8 + nd;
            const float nc = cost[s] + step;
            if (nc < cost[ns]) {
                cost[ns] = nc;
                from[ns] = s;
                open.push({nc + heuristic(nx, ny), ns});
            }
        }
    }
    if (goal < 0) return {};
    std::vector<RouterPoint> cells;
    for (int s = goal; s >= 0; s = from[s]) cells.push_back(g.center((s / 8) % g.w, (s / 8) / g.w));
    std::reverse(cells.begin(), cells.end());
    cells.front() = a;
    cells.back() = b;
    return cells;
}

} // namespace

std::vector<RoutedWire> WireRouter::route(const RouterRect& bounds, const std::vector<RouterRect>& obstacles,
                                          const std::vector<std::pair<RouterPoint, RouterPoint>>& wires,
                                          const RouterOptions& o) {
    Grid g;
    g.ox = bounds.x;
    g.oy = bounds.y;
    g.cell = o.cell;
    g.w = std::max(1, static_cast<int>(std::ceil(bounds.w / o.cell)));
    g.h = std::max(1, static_cast<int>(std::ceil(bounds.h / o.cell)));
    g.blocked.assign(static_cast<size_t>(g.w) * g.h, 0);
    g.extra.assign(static_cast<size_t>(g.w) * g.h, 0.0f);
    for (int cy = 0; cy < g.h; ++cy)
        for (int cx = 0; cx < g.w; ++cx) {
            const RouterPoint p = g.center(cx, cy);
            if (!pointFree(obstacles, p, o.clearance)) g.blocked[g.index(cx, cy)] = 1;
            else if (!pointFree(obstacles, p, 2.5 * o.clearance)) g.extra[g.index(cx, cy)] = 0.8f; // Abstand halten
        }

    const double endRadius = o.clearance + 1.5 * o.cell;
    std::vector<RoutedWire> out;
    out.reserve(wires.size());
    for (const auto& [a, b] : wires) {
        RoutedWire r;
        // gewohnter Bogen, wenn er kein Bauteil streift
        bool arcFree = true;
        for (int i = 1; i < 40 && arcFree; ++i) {
            const RouterPoint p = arcPoint(a, b, o.arcLift, i / 40.0);
            arcFree = nearEnds(p, a, b, endRadius) || pointFree(obstacles, p, o.clearance);
        }
        if (arcFree) {
            r.arc = true;
            r.points = {a, b};
            std::vector<RouterPoint> samples;
            for (int i = 0; i <= 20; ++i) samples.push_back(arcPoint(a, b, o.arcLift, i / 20.0));
            markUsage(g, samples);
            out.push_back(r);
            continue;
        }
        std::vector<RouterPoint> cells = searchPath(g, a, b, endRadius);
        if (cells.empty()) { // kein Weg: Bogen wie bisher
            r.arc = true;
            r.points = {a, b};
            out.push_back(r);
            continue;
        }
        // straffen: vom aktuellen Punkt so weit wie moeglich in gerader Linie
        std::vector<RouterPoint> pts{cells.front()};
        size_t i = 0;
        while (i + 1 < cells.size()) {
            size_t j = cells.size() - 1;
            while (j > i + 1 && !segmentFree(obstacles, cells[i], cells[j], o.clearance, a, b, endRadius)) --j;
            pts.push_back(cells[j]);
            i = j;
        }
        markUsage(g, pts);
        r.points = std::move(pts);
        out.push_back(std::move(r));
    }
    return out;
}

} // namespace volta::model
