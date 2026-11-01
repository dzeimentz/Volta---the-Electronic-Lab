#include "board_json.hpp"

#include <QJsonArray>

#include "anchor.hpp"
#include "board_edit.hpp"

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
    QJsonObject desk{{QStringLiteral("zellen"), d.cells},
                           {QStringLiteral("schalter"), d.switchClosed},
                           {QStringLiteral("p1"), d.p1},
                           {QStringLiteral("p2"), d.p2},
                           {QStringLiteral("messart"), static_cast<int>(d.meterMode)},
                           {QStringLiteral("analog"), d.meterAnalog},
                           {QStringLiteral("messgeraet"), d.meterVisible},
                           {QStringLiteral("raumlicht"), d.roomLight},
                           {QStringLiteral("werkzeuge"), tools}};
    QJsonObject out{{QStringLiteral("bauteile"), parts},     {QStringLiteral("leitungen"), wires},
                       {QStringLiteral("module"), modules},     {QStringLiteral("anzeigen"), displays},
                       {QStringLiteral("spalten"), board.columns}, {QStringLiteral("reihen"), board.rows},
                       {QStringLiteral("pult"), desk}};
    if (board.deskParts) { // Pult nach Bedarf (ab Volta 2); Volta-1-Staende bleiben unveraendert
        QJsonArray groups;
        for (const auto& g : *board.deskParts) groups.append(qs(g));
        out.insert(QStringLiteral("pultteile"), groups);
        desk.insert(QStringLiteral("zellen2"), d.cells2);
        desk.insert(QStringLiteral("wechsel_v"), d.acRmsV);
        desk.insert(QStringLiteral("generator"), QJsonObject{{QStringLiteral("form"), d.genWaveform},
                                                             {QStringLiteral("frequenz_hz"), d.genFrequencyHz},
                                                             {QStringLiteral("amplitude_v"), d.genAmplitudeV},
                                                             {QStringLiteral("gleichanteil_v"), d.genOffsetV}});
        QJsonArray meters;
        for (const auto& s : d.display.meters)
            meters.append(QJsonObject{{QStringLiteral("messstelle"), qs(s.source)},
                                     {QStringLiteral("bereich"), s.range},
                                     {QStringLiteral("wechsel"), s.ac}});
        desk.insert(QStringLiteral("anzeigegeraet"), QJsonObject{{QStringLiteral("doppelt"), d.display.twin},
                                                                 {QStringLiteral("leistung"), d.display.power},
                                                                 {QStringLiteral("anzeigen"), meters}});
        out.insert(QStringLiteral("pult"), desk);
    }
    return out;
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
    const int rows = json.value(QStringLiteral("reihen")).toInt(5); // aeltere Staende: ganze Grundplatte
    b.columns = validBoardSize(5, cols) ? cols : 12;
    b.rows = validBoardSize(rows, 12) ? rows : 5;
    const QJsonObject desk = json.value(QStringLiteral("pult")).toObject();
    DeskState& d = b.desk;
    const int cells = desk.value(QStringLiteral("zellen")).toInt(6);
    d.cells = (cells == 2 || cells == 3 || cells == 6) ? cells : 6;
    const int cells2 = desk.value(QStringLiteral("zellen2")).toInt(6);
    d.cells2 = (cells2 == 3 || cells2 == 6) ? cells2 : 6;
    d.acRmsV = desk.value(QStringLiteral("wechsel_v")).toInt(9) == 6 ? 6 : 9;
    const QJsonObject gen = desk.value(QStringLiteral("generator")).toObject();
    d.genWaveform = std::clamp(gen.value(QStringLiteral("form")).toInt(0), 0, 2);
    d.genFrequencyHz = std::clamp(gen.value(QStringLiteral("frequenz_hz")).toDouble(100.0), 1.0, 10000.0);
    d.genAmplitudeV = std::clamp(gen.value(QStringLiteral("amplitude_v")).toDouble(1.0), 0.0, 4.0);
    d.genOffsetV = std::clamp(gen.value(QStringLiteral("gleichanteil_v")).toDouble(0.0), -4.0, 4.0);
    const QJsonObject disp = desk.value(QStringLiteral("anzeigegeraet")).toObject();
    d.display.twin = disp.value(QStringLiteral("doppelt")).toBool(false);
    d.display.power = disp.value(QStringLiteral("leistung")).toBool(false);
    const QJsonArray meters = disp.value(QStringLiteral("anzeigen")).toArray();
    for (int i = 0; i < 2 && i < meters.size(); ++i) {
        const QJsonObject o = meters.at(i).toObject();
        d.display.meters[i] = {ss(o.value(QStringLiteral("messstelle"))),
                              std::clamp(o.value(QStringLiteral("bereich")).toInt(-1), -1, 4),
                              o.value(QStringLiteral("wechsel")).toBool(false)};
    }
    if (json.contains(QStringLiteral("pultteile"))) {
        std::vector<std::string> groups;
        for (const QJsonValue& v : json.value(QStringLiteral("pultteile")).toArray()) {
            const std::string g = ss(v);
            const auto& names = deskGroupNames();
            if (std::find(names.begin(), names.end(), g) != names.end() &&
                std::find(groups.begin(), groups.end(), g) == groups.end())
                groups.push_back(g);
        }
        if (std::find(groups.begin(), groups.end(), "batterie") == groups.end()) groups.insert(groups.begin(), "batterie");
        b.deskParts = std::move(groups);
    }
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
