#include "chip_json.hpp"

#include <QJsonArray>

namespace zuse {

using namespace volta::logic;

QJsonObject chipToJson(const ChipDef& def) {
    QJsonArray ports;
    for (const Port& p : def.ports)
        ports.append(QJsonObject{{"name", QString::fromStdString(p.name)},
                                 {"richtung", p.direction == Direction::In ? "ein" : "aus"},
                                 {"breite", 1}});
    QJsonArray parts;
    for (const Part& p : def.parts)
        parts.append(QJsonObject{{"id", QString::fromStdString(p.id)},
                                 {"typ", QString::fromStdString(p.type)},
                                 {"x", p.x},
                                 {"y", p.y}});
    QJsonArray wires;
    for (const Connection& c : def.connections)
        wires.append(QJsonArray{QString::fromStdString(c.from), QString::fromStdString(c.to)});
    return QJsonObject{{"schema_version", 1},
                       {"kuerzel", QString::fromStdString(def.kuerzel)},
                       {"version", def.version},
                       {"anschluesse", ports},
                       {"bausteine", parts},
                       {"verbindungen", wires}};
}

bool chipFromJson(const QJsonObject& obj, ChipDef& def) {
    def = ChipDef{};
    def.kuerzel = obj.value("kuerzel").toString().toStdString();
    def.version = obj.value("version").toInt(1);
    if (def.kuerzel.empty()) return false;
    for (const QJsonValue& v : obj.value("anschluesse").toArray()) {
        const QJsonObject p = v.toObject();
        const QString name = p.value("name").toString();
        if (name.isEmpty()) return false;
        def.ports.push_back({name.toStdString(), p.value("richtung").toString() == "aus" ? Direction::Out : Direction::In});
    }
    for (const QJsonValue& v : obj.value("bausteine").toArray()) {
        const QJsonObject p = v.toObject();
        Part part{p.value("id").toString().toStdString(), p.value("typ").toString().toStdString(),
                  p.value("x").toDouble(), p.value("y").toDouble()};
        if (part.id.empty() || part.type.empty()) return false;
        def.parts.push_back(part);
    }
    for (const QJsonValue& v : obj.value("verbindungen").toArray()) {
        const QJsonArray w = v.toArray();
        if (w.size() != 2) return false;
        def.connections.push_back({w.at(0).toString().toStdString(), w.at(1).toString().toStdString()});
    }
    return true;
}

} // namespace zuse
