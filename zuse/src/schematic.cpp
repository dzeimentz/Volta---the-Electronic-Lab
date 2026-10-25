// Schaltplan der Werkbank (wie bei Volta parallel zum Aufbau): automatisch aus dem gezeigten Aufbau angeordnet –
// Eingaenge links, Bausteine in Spalten nach ihrer Logiktiefe (Signalfluss von links nach rechts), Ausgaenge rechts.
// Leitungen nur mit rechten Winkeln (DIN EN 61082-1), Verbindungspunkte an Abzweigungen; Rueckfuehrungen (Speicher)
// laufen unter dem Plan zurueck. Schaltzeichen wie auf der Werkbank (DIN EN 60617-12 oder IEEE Std 91).
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <set>

#include <QPointF>

#include "workbench_controller.hpp"

using namespace volta::logic;

namespace zuse {

namespace {
constexpr qreal kPitch = 28.0;    // Abstand der Anschluesse eines Bausteins
constexpr qreal kGateW = 48.0;    // Breite eines Schaltzeichens
constexpr qreal kColumn = 104.0;  // Spaltenbreite
constexpr qreal kRowGap = 32.0;   // senkrechter Abstand zwischen Bausteinen
constexpr qreal kMargin = 24.0;
constexpr qreal kLaneStep = 8.0;  // Abstand paralleler Leitungen

std::string partOf(const std::string& endpoint) {
    const auto dot = endpoint.find('.');
    return dot == std::string::npos ? std::string() : endpoint.substr(0, dot);
}
} // namespace

QVariantMap WorkbenchController::schematic() const {
    const ChipDef* def = shownDef();
    if (!def) return {};

    // ---- Netze: verbundene Endpunkte zusammenfassen (Union-Find)
    std::map<std::string, std::string> parent;
    std::function<std::string(const std::string&)> find = [&](const std::string& e) -> std::string {
        auto it = parent.find(e);
        if (it == parent.end()) parent[e] = e;
        if (parent[e] == e) return e;
        return parent[e] = find(parent[e]);
    };
    for (const Connection& c : def->connections) parent[find(c.from)] = find(c.to);

    // Treiber eines Endpunkts: Eingang des Chips oder Ausgang eines Bausteins
    std::set<std::string> drivers;
    for (const Port& p : def->ports)
        if (p.direction == Direction::In) drivers.insert(p.name);
    std::map<std::string, std::pair<std::vector<std::string>, std::vector<std::string>>> pins; // Teil -> ein, aus
    for (const Part& p : def->parts) {
        auto& io = pins[p.id];
        m_library.pinsOf(p.type, io.first, io.second);
        for (const std::string& o : io.second) drivers.insert(p.id + "." + o);
    }
    std::map<std::string, std::string> netDriver; // Netz -> Treiber
    for (const auto& e : parent)
        if (drivers.count(e.first)) netDriver[find(e.first)] = e.first;
    auto driverOf = [&](const std::string& endpoint) -> std::string {
        if (!parent.count(endpoint)) return {};
        const auto it = netDriver.find(find(endpoint));
        return it == netDriver.end() ? std::string() : it->second;
    };

    // ---- Logiktiefe: 1 + groesste Tiefe der treibenden Bausteine (Rueckkopplungen werden uebersprungen)
    std::map<std::string, int> depth;
    std::set<std::string> onStack;
    std::function<int(const std::string&)> depthOf = [&](const std::string& id) -> int {
        if (const auto it = depth.find(id); it != depth.end()) return it->second;
        onStack.insert(id);
        int d = 1;
        for (const std::string& in : pins[id].first) {
            const std::string src = partOf(driverOf(id + "." + in));
            if (!src.empty() && !onStack.count(src)) d = std::max(d, depthOf(src) + 1);
        }
        onStack.erase(id);
        return depth[id] = d;
    };
    int maxDepth = 0;
    for (const Part& p : def->parts) maxDepth = std::max(maxDepth, depthOf(p.id));

    // ---- Spalten: Bausteine nach ihrer Hoehe auf der Werkbank (so, wie das Kind sie angeordnet hat)
    std::map<int, std::vector<const Part*>> columns;
    for (const Part& p : def->parts) columns[depth[p.id]].push_back(&p);
    auto heightOf = [&](const std::string& id) {
        const auto& io = pins[id];
        return qreal(std::max<size_t>({io.first.size(), io.second.size(), 2})) * kPitch;
    };
    qreal height = 0;
    for (auto& col : columns) {
        std::stable_sort(col.second.begin(), col.second.end(), [](const Part* a, const Part* b) { return a->y < b->y; });
        qreal h = 0;
        for (const Part* p : col.second) h += heightOf(p->id) + kRowGap;
        height = std::max(height, h - kRowGap);
    }
    const std::vector<std::string> ins = def->inputNames(), outs = def->outputNames();
    height = std::max({height, qreal(ins.size()) * 2 * kPitch - kPitch, qreal(outs.size()) * 2 * kPitch - kPitch, kPitch});

    std::map<std::string, QPointF> at; // Endpunkt -> Lage im Plan
    std::map<std::string, int> colOf;  // Endpunkt -> Spalte (0 = Schalter, maxDepth + 1 = Lampen)
    std::map<int, std::vector<std::pair<qreal, qreal>>> boxes; // Spalte -> von Bausteinen belegte Hoehen
    QVariantList elements;
    auto portColumn = [&](const std::vector<std::string>& names, qreal x, bool input, int column) {
        const qreal top = kMargin + (height - (qreal(names.size()) * 2 - 1) * kPitch) / 2;
        for (size_t i = 0; i < names.size(); ++i) {
            const qreal y = top + qreal(i) * 2 * kPitch;
            at[names[i]] = QPointF(x, y);
            colOf[names[i]] = column;
            const QString n = QString::fromStdString(names[i]);
            const Stage* s = m_stages.empty() || inside() ? nullptr : &m_stages[size_t(m_stage)];
            const QString label = s ? (input ? s->inputLabels.value(s->inputs.indexOf(n)) : s->outputLabels.value(s->outputs.indexOf(n)))
                                    : QString();
            elements.append(QVariantMap{{"kind", input ? "in" : "out"}, {"id", n}, {"label", label}, {"x", x}, {"y", y},
                                        {"level", levelString(names[i])}});
        }
    };
    const qreal inX = kMargin + 28; // links davon der Name des Schalters
    portColumn(ins, inX, true, 0);
    for (const auto& col : columns) {
        const qreal x = inX + qreal(col.first) * kColumn - kGateW;
        qreal total = 0;
        for (const Part* p : col.second) total += heightOf(p->id) + kRowGap;
        qreal y = kMargin + (height - (total - kRowGap)) / 2;
        for (const Part* p : col.second) {
            const qreal h = heightOf(p->id);
            boxes[col.first].emplace_back(y, y + h);
            const auto& io = pins[p->id];
            QVariantList inPins, outPins;
            auto place = [&](const std::vector<std::string>& names, qreal px, QVariantList& out) {
                for (size_t i = 0; i < names.size(); ++i) {
                    const qreal py = y + h * (qreal(i) + 0.5) / qreal(names.size());
                    const std::string ep = p->id + "." + names[i];
                    at[ep] = QPointF(px, py);
                    colOf[ep] = col.first;
                    const bool prim = primitiveFromName(p->type).has_value();
                    out.append(QVariantMap{{"name", prim ? QString() : QString::fromStdString(names[i])},
                                           {"x", px}, {"y", py}});
                }
            };
            place(io.first, x, inPins);
            place(io.second, x + kGateW, outPins);
            bool inv = false;
            const QString type = QString::fromStdString(p->type);
            const QString sym = symbolOf(type, &inv);
            elements.append(QVariantMap{{"kind", "part"}, {"id", QString::fromStdString(p->id)}, {"type", type},
                                        {"symbol", sym}, {"inverted", inv},
                                        {"own", !primitiveFromName(p->type).has_value()},
                                        {"x", x}, {"y", y}, {"w", kGateW}, {"h", h},
                                        {"inPins", inPins}, {"outPins", outPins}});
            y += h + kRowGap;
        }
    }
    const qreal outX = inX + qreal(maxDepth + 1) * kColumn - kGateW + 12;
    portColumn(outs, outX, false, maxDepth + 1);

    // ---- Leitungen. Jedes Netz bekommt in der Gasse hinter seinem Treiber eine senkrechte Spur. Ziele in der
    // naechsten Spalte erreicht es von dort waagerecht; weiter entfernte Ziele ueber eine freie Waagerechte zwischen
    // den Bausteinen der Spalten dazwischen und eine zweite Spur in der Gasse vor dem Ziel. Rueckfuehrungen
    // (Speicher) laufen unter dem Plan zurueck.
    struct Lane {
        qreal x;
        std::vector<qreal> ys; // Abzweigungen
        QString level;
    };
    std::map<std::pair<std::string, int>, Lane> lanes; // (Netz, Gasse hinter Spalte) -> Spur
    std::map<int, int> laneCount;
    const int lanesPerGap = int((kColumn - kGateW - 16) / kLaneStep) + 1;
    auto laneFor = [&](const std::string& net, int gap, const QString& level) -> Lane& {
        const auto key = std::make_pair(net, gap);
        if (const auto it = lanes.find(key); it != lanes.end()) return it->second;
        const int n = laneCount[gap]++;
        return lanes[key] = Lane{inX + qreal(gap) * kColumn + 8 + kLaneStep * qreal(n % lanesPerGap), {}, level};
    };
    struct Track {
        qreal y, x0, x1;
    };
    std::vector<Track> tracks; // belegte Waagerechte
    auto freeY = [&](int c1, int c2, qreal want, qreal x0, qreal x1) {
        auto blocked = [&](qreal y) {
            for (int c = c1; c <= c2; ++c)
                for (const auto& b : boxes[c])
                    if (y > b.first - 6 && y < b.second + 6) return true;
            for (const Track& t : tracks)
                if (std::abs(t.y - y) < kLaneStep - 1 && t.x0 < x1 && x0 < t.x1) return true;
            return false;
        };
        std::vector<qreal> cands{want, kMargin - kRowGap / 2};
        for (int c = c1; c <= c2; ++c)
            for (const auto& b : boxes[c]) {
                cands.push_back(b.first - kRowGap / 2);
                cands.push_back(b.second + kRowGap / 2);
            }
        std::stable_sort(cands.begin(), cands.end(),
                         [want](qreal a, qreal b) { return std::abs(a - want) < std::abs(b - want); });
        for (const qreal y : cands)
            for (const qreal off : {0.0, kLaneStep, -kLaneStep, 2 * kLaneStep, -2 * kLaneStep})
                if (!blocked(y + off)) return y + off;
        return want;
    };
    QVariantList lines, dots;
    const qreal bottom = kMargin + height + 20;
    int backLanes = 0;
    qreal topMost = kMargin;
    for (const auto& net : netDriver) {
        const std::string& drv = net.second;
        if (!at.count(drv)) continue;
        const QPointF d = at[drv];
        const int c = colOf[drv];
        std::vector<std::string> sinks;
        for (const auto& e : parent)
            if (e.first != drv && find(e.first) == net.first && at.count(e.first) && !drivers.count(e.first))
                sinks.push_back(e.first);
        if (sinks.empty()) continue;
        const QString level = levelString(drv);
        auto line = [&](std::initializer_list<QPointF> pts) {
            QVariantList l;
            for (const QPointF& p : pts) l << p.x() << p.y();
            lines.append(QVariantMap{{"points", l}, {"level", level}});
        };
        Lane& main = laneFor(net.first, c, level);
        line({d, QPointF(main.x, d.y())});
        main.ys.push_back(d.y());
        for (const std::string& ep : sinks) {
            const QPointF s = at[ep];
            const int k = colOf[ep];
            if (k == c + 1) {
                line({QPointF(main.x, s.y()), s});
                main.ys.push_back(s.y());
                continue;
            }
            Lane& last = laneFor(net.first, k - 1, level);
            qreal y;
            if (k > c + 1) {
                y = freeY(c + 1, k - 1, s.y(), main.x, last.x);
                tracks.push_back(Track{y, std::min(main.x, last.x), std::max(main.x, last.x)});
                topMost = std::min(topMost, y);
            } else {
                y = bottom + kLaneStep * qreal(backLanes++);
            }
            line({QPointF(main.x, y), QPointF(last.x, y)});
            main.ys.push_back(y);
            last.ys.push_back(y);
            last.ys.push_back(s.y());
            line({QPointF(last.x, s.y()), s});
        }
    }
    for (auto& entry : lanes) {
        Lane& lane = entry.second;
        std::sort(lane.ys.begin(), lane.ys.end());
        lane.ys.erase(std::unique(lane.ys.begin(), lane.ys.end(), [](qreal a, qreal b) { return std::abs(a - b) < 0.5; }),
                      lane.ys.end());
        if (lane.ys.size() < 2) continue;
        lines.append(QVariantMap{{"points", QVariantList{lane.x, lane.ys.front(), lane.x, lane.ys.back()}},
                                 {"level", lane.level}});
        for (size_t i = 1; i + 1 < lane.ys.size(); ++i) // Verbindungspunkt an jeder Abzweigung
            dots.append(QVariantMap{{"x", lane.x}, {"y", lane.ys[i]}, {"level", lane.level}});
    }
    // Waagerechte oberhalb aller Bausteine: Plan nach unten schieben
    const qreal shift = topMost < kMargin / 2 ? kMargin / 2 - topMost : 0;
    if (shift > 0) {
        auto move = [shift](QVariantList& list) {
            for (QVariant& v : list) {
                QVariantMap m = v.toMap();
                if (m.contains("points")) {
                    QVariantList pts = m["points"].toList();
                    for (int i = 1; i < pts.size(); i += 2) pts[i] = pts[i].toReal() + shift;
                    m["points"] = pts;
                } else {
                    m["y"] = m["y"].toReal() + shift;
                    for (const char* key : {"inPins", "outPins"}) {
                        if (!m.contains(key)) continue;
                        QVariantList list2 = m.value(key).toList();
                        for (QVariant& pin : list2) {
                            QVariantMap pm = pin.toMap();
                            pm["y"] = pm["y"].toReal() + shift;
                            pin = pm;
                        }
                        m[key] = list2;
                    }
                }
                v = m;
            }
        };
        move(elements);
        move(lines);
        move(dots);
    }
    const qreal totalHeight = shift + std::max(kMargin + height + kMargin, bottom + kLaneStep * qreal(backLanes) + kMargin);
    return QVariantMap{{"elements", elements}, {"lines", lines}, {"dots", dots},
                       {"width", outX + 28 + kMargin}, {"height", totalHeight}};
}

// Fehlersuche wie bei Volta, fuer Logik: offene Eingaenge, unbenutzte Ausgaenge, Lampen ohne Leitung
QVariantList WorkbenchController::faultFindings() const {
    QVariantList list;
    const ChipDef* def = shownDef();
    if (!def || isDiscover()) return list;
    std::set<std::string> used;
    for (const Connection& c : def->connections) {
        used.insert(c.from);
        used.insert(c.to);
    }
    for (const Part& p : def->parts) {
        std::vector<std::string> ins, outs;
        m_library.pinsOf(p.type, ins, outs);
        const QString name = toolName(QString::fromStdString(p.type));
        const QString shown = name.isEmpty() ? QString::fromStdString(p.type) : name;
        QStringList open;
        for (const std::string& i : ins)
            if (!used.count(p.id + "." + i)) open << QString::fromStdString(i);
        if (!open.isEmpty())
            list.append(QVariantMap{{"part", QString::fromStdString(p.id)}, {"kind", "offen"},
                                    {"text", primitiveFromName(p.type).has_value()
                                                 ? tr("%1: Ein Eingang hat keine Leitung. Ein offener Eingang ist "
                                                      "weder 0 noch 1 – die Lampe zeigt dann »?«.").arg(shown)
                                                 : tr("%1: Die Eingänge %2 haben keine Leitung. Offene Eingänge "
                                                      "sind weder 0 noch 1.").arg(shown, open.join(", "))}});
        bool anyOut = false;
        for (const std::string& o : outs) anyOut = anyOut || used.count(p.id + "." + o);
        if (!anyOut)
            list.append(QVariantMap{{"part", QString::fromStdString(p.id)}, {"kind", "unbenutzt"},
                                    {"text", tr("%1: Der Ausgang führt nirgendwohin. Wolltest du ihn mit einer "
                                                "Lampe oder einem anderen Baustein verbinden?").arg(shown)}});
    }
    for (const std::string& o : def->outputNames())
        if (!used.count(o)) {
            const QString n = QString::fromStdString(o);
            QString label = n;
            if (!inside() && !m_stages.empty()) {
                const Stage& s = m_stages[size_t(m_stage)];
                label = s.outputLabels.value(s.outputs.indexOf(n), n);
            }
            list.append(QVariantMap{{"part", QString()}, {"kind", "lampe"},
                                    {"text", tr("Die Lampe »%1« hat keine Leitung.").arg(label.isEmpty() ? n : label)}});
        }
    return list;
}

// Teileliste wie bei Volta: was im gezeigten Aufbau steckt, je Werkzeug mit Anzahl
QVariantList WorkbenchController::partsList() const {
    QVariantList list;
    const ChipDef* def = shownDef();
    if (!def) return list;
    std::map<std::string, int> count;
    std::vector<std::string> order;
    for (const Part& p : def->parts)
        if (count[p.type]++ == 0) order.push_back(p.type);
    for (const std::string& t : order) {
        bool inv = false;
        const QString type = QString::fromStdString(t);
        const QString sym = symbolOf(type, &inv);
        const QString name = toolName(type);
        list.append(QVariantMap{{"type", type}, {"name", name.isEmpty() ? type : name}, {"symbol", sym},
                                {"inverted", inv}, {"own", !primitiveFromName(t).has_value()}, {"count", count[t]}});
    }
    return list;
}

} // namespace zuse
