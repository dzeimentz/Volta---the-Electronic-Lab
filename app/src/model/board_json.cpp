#include "board_json.hpp"

#include <QJsonArray>

#include <algorithm>

namespace volta::model {

namespace {
QString qs(const std::string& s) { return QString::fromStdString(s); }
std::string ss(const QJsonValue& v) { return v.toString().toStdString(); }
} // namespace

QJsonObject boardToJson(const Board& board) {
    QJsonArray parts;
    for (const auto& p : board.parts) {
        QJsonArray pins;
        for (const auto& [name, field] : p.pins) pins.append(QJsonArray{qs(name), qs(field)});
        parts.append(QJsonObject{{QStringLiteral("id"), qs(p.id)},
                                 {QStringLiteral("typ"), qs(p.type)},
                                 {QStringLiteral("anschluesse"), pins},
                                 {QStringLiteral("wert"), p.value},
                                 {QStringLiteral("geschlossen"), p.closed},
                                 {QStringLiteral("stellung"), p.position}});
    }
    QJsonArray wires;
    for (const auto& w : board.wires)
        wires.append(QJsonObject{{QStringLiteral("id"), qs(w.id)},
                                 {QStringLiteral("von"), qs(w.from)},
                                 {QStringLiteral("nach"), qs(w.to)},
                                 {QStringLiteral("farbe"), qs(w.color)},
                                 {QStringLiteral("farbe_fest"), w.colorFixed}});
    QJsonArray modules;
    for (const auto& m : board.modules)
        modules.append(QJsonObject{{QStringLiteral("id"), qs(m.id)},
                                   {QStringLiteral("typ"), qs(m.type)},
                                   {QStringLiteral("feld"), m.slot}});
    QJsonArray displays;
    for (const auto& d : board.voltageDisplays) displays.append(qs(d));
    QJsonArray tools;
    for (const auto& t : board.desk.tools)
        tools.append(QJsonObject{{QStringLiteral("typ"), qs(t.type)},
                                 {QStringLiteral("spalte"), t.col},
                                 {QStringLiteral("zeile"), t.row},
                                 {QStringLiteral("an"), t.on}});
    const DeskState& d = board.desk;
    const QJsonObject desk{{QStringLiteral("zellen"), d.cells},
                           {QStringLiteral("schalter"), d.switchClosed},
                           {QStringLiteral("p1"), d.p1},
                           {QStringLiteral("p2"), d.p2},
                           {QStringLiteral("messart"), static_cast<int>(d.meterMode)},
                           {QStringLiteral("analog"), d.meterAnalog},
                           {QStringLiteral("messgeraet"), d.meterVisible},
                           {QStringLiteral("raumlicht"), d.roomLight},
                           {QStringLiteral("werkzeuge"), tools}};
    return QJsonObject{{QStringLiteral("bauteile"), parts},     {QStringLiteral("leitungen"), wires},
                       {QStringLiteral("module"), modules},     {QStringLiteral("anzeigen"), displays},
                       {QStringLiteral("spalten"), board.columns}, {QStringLiteral("pult"), desk}};
}

bool boardFromJson(const QJsonObject& json, Board& out) {
    if (!json.contains(QStringLiteral("bauteile"))) return false;
    Board b;
    for (const QJsonValue& v : json.value(QStringLiteral("bauteile")).toArray()) {
        const QJsonObject o = v.toObject();
        PartPlacement p;
        p.id = ss(o.value(QStringLiteral("id")));
        p.type = ss(o.value(QStringLiteral("typ")));
        for (const QJsonValue& pin : o.value(QStringLiteral("anschluesse")).toArray()) {
            const QJsonArray a = pin.toArray();
            if (a.size() == 2) p.pins.emplace_back(ss(a.at(0)), ss(a.at(1)));
        }
        p.value = o.value(QStringLiteral("wert")).toDouble(0.0);
        p.closed = o.value(QStringLiteral("geschlossen")).toBool(false);
        p.position = o.value(QStringLiteral("stellung")).toDouble(0.5);
        if (p.id.empty() || p.type.empty() || p.pins.empty()) return false;
        b.parts.push_back(std::move(p));
    }
    for (const QJsonValue& v : json.value(QStringLiteral("leitungen")).toArray()) {
        const QJsonObject o = v.toObject();
        WirePlacement w;
        w.id = ss(o.value(QStringLiteral("id")));
        w.from = ss(o.value(QStringLiteral("von")));
        w.to = ss(o.value(QStringLiteral("nach")));
        w.color = o.value(QStringLiteral("farbe")).toString(QStringLiteral("rot")).toStdString();
        w.colorFixed = o.value(QStringLiteral("farbe_fest")).toBool(false);
        if (w.id.empty() || w.from.empty() || w.to.empty()) return false;
        b.wires.push_back(std::move(w));
    }
    for (const QJsonValue& v : json.value(QStringLiteral("module")).toArray()) {
        const QJsonObject o = v.toObject();
        b.modules.push_back({ss(o.value(QStringLiteral("id"))), ss(o.value(QStringLiteral("typ"))),
                             o.value(QStringLiteral("feld")).toInt(1)});
    }
    for (const QJsonValue& v : json.value(QStringLiteral("anzeigen")).toArray()) b.voltageDisplays.push_back(ss(v));
    const int cols = json.value(QStringLiteral("spalten")).toInt(12);
    b.columns = (cols == 8 || cols == 10 || cols == 12) ? cols : 12;
    const QJsonObject desk = json.value(QStringLiteral("pult")).toObject();
    DeskState& d = b.desk;
    const int cells = desk.value(QStringLiteral("zellen")).toInt(6);
    d.cells = (cells == 2 || cells == 3 || cells == 6) ? cells : 6;
    d.mainSwitch = false; // Start im Steckmodus
    d.switchClosed = desk.value(QStringLiteral("schalter")).toBool(false);
    d.p1 = std::clamp(desk.value(QStringLiteral("p1")).toDouble(0.5), 0.0, 1.0);
    d.p2 = std::clamp(desk.value(QStringLiteral("p2")).toDouble(0.5), 0.0, 1.0);
    d.meterMode = static_cast<core::MeterMode>(std::clamp(desk.value(QStringLiteral("messart")).toInt(0), 0, 2));
    d.meterAnalog = desk.value(QStringLiteral("analog")).toBool(false);
    d.meterVisible = desk.value(QStringLiteral("messgeraet")).toBool(false);
    d.roomLight = std::clamp(desk.value(QStringLiteral("raumlicht")).toInt(2), 0, 2);
    for (const QJsonValue& v : desk.value(QStringLiteral("werkzeuge")).toArray()) {
        const QJsonObject o = v.toObject();
        d.tools.push_back({ss(o.value(QStringLiteral("typ"))), o.value(QStringLiteral("spalte")).toDouble(1.0),
                           o.value(QStringLiteral("zeile")).toDouble(0.0), o.value(QStringLiteral("an")).toBool(true)});
    }
    out = std::move(b);
    return true;
}

} // namespace volta::model
