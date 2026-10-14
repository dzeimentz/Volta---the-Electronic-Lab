#include "workbench_controller.hpp"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QUndoCommand>
#include <QVariantMap>
#include <algorithm>
#include <cctype>

#include "chip_json.hpp"

namespace zuse {

using namespace volta::logic;

namespace {

// Jede Aenderung der Werkbank: Zustand vorher und nachher (Chips sind klein)
class DefCommand : public QUndoCommand {
public:
    DefCommand(WorkbenchController* c, ChipDef before, ChipDef after, const QString& text)
        : QUndoCommand(text), m_c(c), m_before(std::move(before)), m_after(std::move(after)) {}
    void redo() override { m_c->applyDef(m_after); }
    void undo() override { m_c->applyDef(m_before); }

private:
    WorkbenchController* m_c;
    ChipDef m_before;
    ChipDef m_after;
};

QString levelChar(Level l) { return QString(QChar(toChar(l))); }

std::vector<Level> parseLevels(const QString& s) {
    std::vector<Level> out;
    for (QChar ch : s) out.push_back(ch == '1' ? Level::One : ch == '0' ? Level::Zero : Level::X);
    return out;
}

QString joinLevels(const std::vector<Level>& v) {
    QString s;
    for (Level l : v) s += QChar(toChar(l));
    return s;
}

QString localized(const QJsonValue& v, const QString& language) {
    const QJsonObject o = v.toObject();
    const QString text = o.value(language).toString();
    return text.isEmpty() ? o.value("de").toString() : text;
}

bool isEndpointOfPart(const std::string& endpoint, const std::string& partId) {
    return endpoint.size() > partId.size() && endpoint.compare(0, partId.size(), partId) == 0 &&
           endpoint[partId.size()] == '.';
}

} // namespace

WorkbenchController::WorkbenchController(const QString& stagesFile, const QString& dataDir, const QString& language,
                                         QObject* parent)
    : QObject(parent), m_dataDir(dataDir) {
    loadStages(stagesFile, language);
    load();
    connect(&m_undo, &QUndoStack::canUndoChanged, this, &WorkbenchController::historyChanged);
    connect(&m_undo, &QUndoStack::canRedoChanged, this, &WorkbenchController::historyChanged);
    int first = 0; // erste noch offene Stufe
    while (first + 1 < static_cast<int>(m_stages.size()) && m_done.contains(m_stages[size_t(first)].kuerzel)) ++first;
    startStage(first);
}

bool WorkbenchController::loadStages(const QString& file, const QString& language) {
    QFile f(file);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    for (const QJsonValue& v : root.value("stufen").toArray()) {
        const QJsonObject o = v.toObject();
        Stage s;
        s.kuerzel = o.value("kuerzel").toString();
        s.symbol = o.value("symbol").toString();
        s.inverted = o.value("invertiert").toBool();
        s.title = localized(o.value("titel"), language);
        s.question = localized(o.value("frage"), language);
        s.hint = localized(o.value("hilfe"), language);
        for (const QJsonValue& x : o.value("eingaenge").toArray()) s.inputs << x.toString();
        for (const QJsonValue& x : o.value("ausgaenge").toArray()) s.outputs << x.toString();
        const QJsonArray kinds = o.value("eingabearten").toArray();
        for (qsizetype i = 0; i < s.inputs.size(); ++i)
            s.inputKinds << (i < kinds.size() && kinds.at(i).toString() == "taster" ? QStringLiteral("taster")
                                                                                   : QStringLiteral("kipp"));
        s.makesChip = o.value("chip").toBool(true);
        for (const QJsonValue& x : o.value("bausteine").toArray()) s.palette << x.toString();
        for (const QJsonValue& x : o.value("wahrheitstabelle").toArray()) s.table.push_back(parseLevels(x.toString()));
        QJsonObject sol = o.value("loesung").toObject();
        sol.insert("kuerzel", s.kuerzel);
        chipFromJson(sol, s.solution);
        m_stages.push_back(s);
    }
    return !m_stages.empty();
}

void WorkbenchController::load() {
    QFile f(QDir(m_dataDir).filePath("werkbank.json"));
    if (!f.open(QIODevice::ReadOnly)) return;
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    for (const QJsonValue& v : root.value("chips").toArray()) {
        ChipDef d;
        if (chipFromJson(v.toObject(), d)) m_library.add(d);
    }
    for (const QJsonValue& v : root.value("geschafft").toArray()) m_done.insert(v.toString());
    if (root.value("schaltzeichen").toString() == "us") m_symbolStyle = QStringLiteral("us");
    const QJsonObject drafts = root.value("entwuerfe").toObject();
    for (auto it = drafts.begin(); it != drafts.end(); ++it) {
        ChipDef d;
        if (chipFromJson(it.value().toObject(), d)) m_drafts[it.key()] = d;
    }
}

void WorkbenchController::save() const {
    if (m_dataDir.isEmpty()) return;
    QDir().mkpath(m_dataDir);
    QJsonArray chips;
    for (const std::string& k : m_library.kuerzel()) chips.append(chipToJson(*m_library.find(k)));
    QJsonArray done;
    for (const QString& k : m_done) done.append(k);
    QJsonObject drafts;
    for (const auto& d : m_drafts) drafts.insert(d.first, chipToJson(d.second));
    QSaveFile f(QDir(m_dataDir).filePath("werkbank.json"));
    if (!f.open(QIODevice::WriteOnly)) return;
    f.write(QJsonDocument(QJsonObject{{"schema_version", 1}, {"chips", chips}, {"geschafft", done}, {"entwuerfe", drafts},
                                         {"schaltzeichen", m_symbolStyle}})
                .toJson());
    f.commit();
}

// ---- Stufen ----

bool WorkbenchController::unlocked(int index) const {
    if (index <= 0) return index == 0;
    return m_done.contains(m_stages[size_t(index - 1)].kuerzel);
}

QVariantList WorkbenchController::stages() const {
    QVariantList list;
    for (size_t i = 0; i < m_stages.size(); ++i) {
        const Stage& s = m_stages[i];
        list.append(QVariantMap{{"index", int(i)},
                                {"kuerzel", s.kuerzel},
                                {"symbol", s.symbol},
                                {"title", s.title},
                                {"unlocked", unlocked(int(i))},
                                {"done", m_done.contains(s.kuerzel)}});
    }
    return list;
}

QString WorkbenchController::stageTitle() const { return m_stages.empty() ? QString() : m_stages[size_t(m_stage)].title; }
QString WorkbenchController::stageQuestion() const {
    return m_stages.empty() ? QString() : m_stages[size_t(m_stage)].question;
}
QString WorkbenchController::stageHint() const { return m_stages.empty() ? QString() : m_stages[size_t(m_stage)].hint; }
QString WorkbenchController::stageKuerzel() const {
    return m_stages.empty() ? QString() : m_stages[size_t(m_stage)].kuerzel;
}

QVariantList WorkbenchController::palette() const {
    QVariantList list;
    if (m_stages.empty()) return list;
    for (const QString& type : m_stages[size_t(m_stage)].palette) {
        const bool chip = !primitiveFromName(type.toStdString());
        if (chip && !m_library.find(type.toStdString())) continue; // noch nicht gebaut
        bool inv = false;
        const QString sym = symbolOf(type, &inv);
        list.append(QVariantMap{{"type", type}, {"symbol", sym}, {"inverted", inv}, {"own", chip}});
    }
    return list;
}

bool WorkbenchController::selectStage(int index) {
    if (index < 0 || index >= int(m_stages.size()) || !unlocked(index)) return false;
    startStage(index);
    return true;
}

void WorkbenchController::startStage(int index) {
    if (m_stages.empty()) return;
    m_stage = index;
    const Stage& s = m_stages[size_t(index)];
    const auto draft = m_drafts.find(s.kuerzel);
    if (draft != m_drafts.end()) {
        m_def = draft->second;
    } else {
        m_def = ChipDef{};
        m_def.kuerzel = s.kuerzel.toStdString();
    }
    // Anschluesse gibt die Stufe vor (auch fuer aeltere Entwuerfe)
    m_def.ports.clear();
    for (const QString& n : s.inputs) m_def.ports.push_back({n.toStdString(), Direction::In});
    for (const QString& n : s.outputs) m_def.ports.push_back({n.toStdString(), Direction::Out});
    m_inputs.assign(size_t(s.inputs.size()), Level::Zero);
    m_path.clear();
    m_undo.clear();
    m_checkDone = false;
    m_check = CheckResult{};
    setMessage(QString());
    rebuild();
    emit stageChanged();
    emit stagesChanged();
    emit checkChanged();
    emit historyChanged();
}

// ---- Aenderungen (Befehle) ----

void WorkbenchController::applyDef(const ChipDef& def) {
    m_def = def;
    if (!m_stages.empty()) m_drafts[m_stages[size_t(m_stage)].kuerzel] = def;
    m_path.clear();
    m_checkDone = false; // geaenderter Aufbau muss neu geprueft werden
    rebuild();
    save();
    emit checkChanged();
}

void WorkbenchController::push(const ChipDef& after, const QString& text) {
    m_undo.push(new DefCommand(this, m_def, after, text));
}

QString WorkbenchController::addPart(const QString& type, qreal x, qreal y) {
    if (inside() || m_stages.empty() || !m_stages[size_t(m_stage)].palette.contains(type)) return {};
    const std::string t = type.toStdString();
    if (!primitiveFromName(t) && !m_library.find(t)) return {};
    // ID: Anfangsbuchstabe des Typs und laufende Nummer
    const std::string prefix(1, char(std::tolower(static_cast<unsigned char>(t[0]))));
    std::string id;
    for (int k = 1;; ++k) {
        id = prefix + std::to_string(k);
        if (!m_def.findPart(id)) break;
    }
    qreal w = 0, h = 0;
    partSize(t, w, h);
    ChipDef after = m_def;
    after.parts.push_back({id, t, std::clamp<qreal>(x, kPartMargin, kFrameWidth - kPartMargin - w),
                           std::clamp<qreal>(y, 8.0, kFrameHeight - 8.0 - h)});
    push(after, tr("Baustein setzen"));
    return QString::fromStdString(id);
}

void WorkbenchController::movePart(const QString& id, qreal x, qreal y) {
    if (inside()) return;
    ChipDef after = m_def;
    for (Part& p : after.parts) {
        if (p.id != id.toStdString()) continue;
        qreal w = 0, h = 0;
        partSize(p.type, w, h);
        x = std::clamp<qreal>(x, kPartMargin, kFrameWidth - kPartMargin - w);
        y = std::clamp<qreal>(y, 8.0, kFrameHeight - 8.0 - h);
        if (qFuzzyCompare(p.x, x) && qFuzzyCompare(p.y, y)) return;
        p.x = x;
        p.y = y;
        push(after, tr("Baustein verschieben"));
        return;
    }
}

void WorkbenchController::removePart(const QString& id) {
    if (inside()) return;
    const std::string pid = id.toStdString();
    ChipDef after = m_def;
    const auto it = std::find_if(after.parts.begin(), after.parts.end(), [&](const Part& p) { return p.id == pid; });
    if (it == after.parts.end()) return;
    after.parts.erase(it);
    after.connections.erase(std::remove_if(after.connections.begin(), after.connections.end(),
                                           [&](const Connection& c) {
                                               return isEndpointOfPart(c.from, pid) || isEndpointOfPart(c.to, pid);
                                           }),
                            after.connections.end());
    push(after, tr("Baustein entfernen"));
}

bool WorkbenchController::connectPins(const QString& a, const QString& b) {
    if (inside() || a == b) return false;
    const std::string from = a.toStdString(), to = b.toStdString();
    for (const Connection& c : m_def.connections)
        if ((c.from == from && c.to == to) || (c.from == to && c.to == from)) return false;
    ChipDef after = m_def;
    after.connections.push_back({from, to});
    const ElaborateResult r = validate(after, m_library);
    if (r.error == ElaborateError::MultipleDrivers) {
        setMessage(tr("Zwei Ausgänge an einer Leitung würden gegeneinander arbeiten. Verbinde einen Ausgang mit Eingängen."));
        return false;
    }
    if (!r.ok()) return false;
    push(after, tr("Leitung ziehen"));
    setMessage(QString());
    return true;
}

void WorkbenchController::removeWire(int index) {
    if (inside() || index < 0 || index >= int(m_def.connections.size())) return;
    ChipDef after = m_def;
    after.connections.erase(after.connections.begin() + index);
    push(after, tr("Leitung entfernen"));
}

void WorkbenchController::clearBench() {
    if (inside() || (m_def.parts.empty() && m_def.connections.empty())) return;
    ChipDef after = m_def;
    after.parts.clear();
    after.connections.clear();
    push(after, tr("Werkbank leeren"));
}

void WorkbenchController::showSolution() {
    if (inside() || m_stages.empty()) return;
    ChipDef after = m_def;
    after.parts = m_stages[size_t(m_stage)].solution.parts;
    after.connections = m_stages[size_t(m_stage)].solution.connections;
    push(after, tr("Musterlösung"));
    setMessage(tr("Das ist die Musterlösung. Prüfe sie – und baue sie danach gern selbst nach."));
}

void WorkbenchController::undo() {
    m_undo.undo();
}
void WorkbenchController::redo() {
    m_undo.redo();
}

// ---- Simulation (Bedienhandlungen) ----

void WorkbenchController::rebuild() {
    const ElaborateResult r = elaborate(m_def, m_library, m_netlist);
    m_netOk = r.ok();
    if (m_netOk) {
        m_sim.load(m_netlist);
        for (size_t i = 0; i < m_inputs.size(); ++i) m_sim.setInput(i, m_inputs[i]);
        m_sim.settle();
    }
    emit viewChanged();
}

void WorkbenchController::toggleInput(const QString& name) {
    if (inside() || m_stages.empty()) return;
    const qsizetype i = m_stages[size_t(m_stage)].inputs.indexOf(name);
    if (i < 0) return;
    setInput(name, m_inputs[size_t(i)] != Level::One);
}

void WorkbenchController::setInput(const QString& name, bool on) {
    if (inside() || m_stages.empty()) return;
    const qsizetype i = m_stages[size_t(m_stage)].inputs.indexOf(name);
    if (i < 0) return;
    Level& l = m_inputs[size_t(i)];
    const Level next = on ? Level::One : Level::Zero;
    if (l == next) return;
    l = next;
    if (m_netOk) {
        m_sim.setInput(size_t(i), l);
        m_sim.settle();
    }
    emit viewChanged();
}

Level WorkbenchController::levelAt(const QString& endpoint) const {
    const Scope* scope = shownScope();
    if (!m_netOk || !scope) return Level::X;
    const auto it = scope->endpoints.find(endpoint.toStdString());
    return it == scope->endpoints.end() ? Level::X : m_sim.net(it->second);
}

QString WorkbenchController::levelString(const std::string& endpoint) const {
    return levelChar(levelAt(QString::fromStdString(endpoint)));
}

void WorkbenchController::check() {
    if (m_stages.empty()) return;
    if (!m_netOk) {
        setMessage(tr("Der Aufbau lässt sich nicht simulieren."));
        return;
    }
    m_check = checkTruthTable(m_netlist, m_stages[size_t(m_stage)].table);
    m_checkDone = true;
    if (m_check.passed && !m_stages[size_t(m_stage)].makesChip)
        setMessage(tr("Alle Zeilen stimmen! Schalter und LEDs machen, was sie sollen."));
    else if (m_check.passed)
        setMessage(tr("Alle Zeilen stimmen! Du kannst jetzt einen Chip daraus machen."));
    else if (m_check.oscillating)
        setMessage(tr("Die Schaltung kommt nicht zur Ruhe – sie schwingt."));
    else
        setMessage(tr("Noch nicht ganz: Die markierten Zeilen weichen ab. Schalte die Eingänge so und sieh nach."));
    emit checkChanged();
}

QVariantList WorkbenchController::checkRows() const {
    QVariantList rows;
    if (m_stages.empty()) return rows;
    const Stage& s = m_stages[size_t(m_stage)];
    const size_t n = size_t(1) << s.inputs.size();
    for (size_t r = 0; r < n && r < s.table.size(); ++r) {
        QString in;
        for (qsizetype i = 0; i < s.inputs.size(); ++i) in += ((r >> (s.inputs.size() - 1 - i)) & 1u) ? '1' : '0';
        QVariantMap row{{"inputs", in}, {"expected", joinLevels(s.table[r])}};
        if (m_checkDone && r < m_check.rows.size()) {
            row.insert("actual", joinLevels(m_check.rows[r].actual));
            row.insert("ok", m_check.rows[r].ok);
        } else {
            row.insert("actual", QString());
            row.insert("ok", false);
        }
        rows.append(row);
    }
    return rows;
}

QString WorkbenchController::checkState() const {
    if (!m_checkDone) return QStringLiteral("none");
    return m_check.passed ? QStringLiteral("passed") : QStringLiteral("failed");
}

bool WorkbenchController::createChip() {
    if (!chipReady() || m_stages.empty()) return false;
    const Stage& s = m_stages[size_t(m_stage)];
    if (!s.makesChip) { // Stufe ohne Chip: nur geschafft
        m_done.insert(s.kuerzel);
        save();
        setMessage(tr("Geschafft! Weiter geht es mit der nächsten Stufe."));
        m_checkDone = false;
        emit checkChanged();
        emit stagesChanged();
        return true;
    }
    ChipDef chip = m_def;
    const ChipDef* old = m_library.find(chip.kuerzel);
    chip.version = old ? old->version + 1 : 1;
    m_library.add(chip);
    m_done.insert(s.kuerzel);
    save();
    setMessage(tr("Neuer Chip im Fach: %1 ★").arg(s.kuerzel));
    m_checkDone = false;
    emit checkChanged();
    emit stagesChanged();
    emit stageChanged();
    return true;
}

// ---- Blick hinein ----

const ChipDef* WorkbenchController::shownDef() const {
    if (m_path.empty()) return &m_def;
    const Scope* scope = shownScope();
    return scope ? m_library.find(scope->kuerzel) : nullptr;
}

const Scope* WorkbenchController::shownScope() const {
    if (!m_netOk || m_netlist.scopes.empty()) return nullptr;
    const Scope* scope = &m_netlist.scopes[0];
    for (const std::string& id : m_path) {
        const auto it = scope->children.find(id);
        if (it == scope->children.end()) return nullptr;
        scope = &m_netlist.scopes[it->second];
    }
    return scope;
}

bool WorkbenchController::lookInside(const QString& partId) {
    const Scope* scope = shownScope();
    if (!scope || !scope->children.count(partId.toStdString())) return false; // Grundgatter haben kein Inneres
    m_path.push_back(partId.toStdString());
    emit viewChanged();
    return true;
}

void WorkbenchController::leaveTo(int depth) {
    if (depth < 0 || size_t(depth) >= m_path.size()) return;
    m_path.resize(size_t(depth));
    emit viewChanged();
}

QStringList WorkbenchController::path() const {
    QStringList labels{stageKuerzel()};
    const ChipDef* def = &m_def;
    for (const std::string& id : m_path) {
        const Part* p = def ? def->findPart(id) : nullptr;
        labels << (p ? QString::fromStdString(p->type) : QString::fromStdString(id));
        def = p ? m_library.find(p->type) : nullptr;
    }
    return labels;
}

// ---- Geometrie ----

QString WorkbenchController::symbolOf(const QString& type, bool* inverted) const {
    *inverted = false;
    if (const auto p = primitiveFromName(type.toStdString())) {
        switch (*p) {
        case Primitive::Nand: *inverted = true; return QStringLiteral("&");
        case Primitive::Not: *inverted = true; return QStringLiteral("1");
        case Primitive::And: return QStringLiteral("&");
        case Primitive::Or: return QStringLiteral("≥1");
        case Primitive::Xor: return QStringLiteral("=1");
        default: return type;
        }
    }
    for (const Stage& s : m_stages)
        if (s.kuerzel == type) {
            *inverted = s.inverted;
            return s.symbol;
        }
    return type;
}

void WorkbenchController::partSize(const std::string& type, qreal& w, qreal& h) const {
    std::vector<std::string> ins, outs;
    m_library.pinsOf(type, ins, outs);
    const size_t pins = std::max<size_t>({ins.size(), outs.size(), 2});
    w = kPartWidth;
    h = qreal(pins) * kPinPitch;
}

bool WorkbenchController::pinPos(const ChipDef& def, const std::string& endpoint, qreal& x, qreal& y) const {
    const auto dot = endpoint.find('.');
    if (dot == std::string::npos) { // Anschluss am Rahmen
        const std::vector<std::string> ins = def.inputNames(), outs = def.outputNames();
        const auto inIt = std::find(ins.begin(), ins.end(), endpoint);
        if (inIt != ins.end()) {
            x = kPortInset;
            y = kFrameHeight * qreal(inIt - ins.begin() + 1) / qreal(ins.size() + 1);
            return true;
        }
        const auto outIt = std::find(outs.begin(), outs.end(), endpoint);
        if (outIt == outs.end()) return false;
        x = kFrameWidth - kPortInset;
        y = kFrameHeight * qreal(outIt - outs.begin() + 1) / qreal(outs.size() + 1);
        return true;
    }
    const Part* part = def.findPart(endpoint.substr(0, dot));
    if (!part) return false;
    const std::string pin = endpoint.substr(dot + 1);
    std::vector<std::string> ins, outs;
    if (!m_library.pinsOf(part->type, ins, outs)) return false;
    qreal w = 0, h = 0;
    partSize(part->type, w, h);
    auto place = [&](const std::vector<std::string>& pins, qreal px) {
        const auto it = std::find(pins.begin(), pins.end(), pin);
        if (it == pins.end()) return false;
        const qreal offset = (h - qreal(pins.size()) * kPinPitch) / 2.0;
        x = px;
        y = part->y + offset + kPinPitch * (qreal(it - pins.begin()) + 0.5);
        return true;
    };
    return place(ins, part->x) || place(outs, part->x + w);
}

QVariantList WorkbenchController::parts() const {
    QVariantList list;
    const ChipDef* def = shownDef();
    if (!def) return list;
    for (const Part& p : def->parts) {
        qreal w = 0, h = 0;
        partSize(p.type, w, h);
        std::vector<std::string> ins, outs;
        m_library.pinsOf(p.type, ins, outs);
        auto pins = [&](const std::vector<std::string>& names) {
            QVariantList l;
            for (const std::string& n : names) {
                const std::string ep = p.id + "." + n;
                qreal x = 0, y = 0;
                pinPos(*def, ep, x, y);
                const bool prim = primitiveFromName(p.type).has_value();
                l.append(QVariantMap{{"endpoint", QString::fromStdString(ep)},
                                     {"name", prim ? QString() : QString::fromStdString(n)},
                                     {"x", x},
                                     {"y", y},
                                     {"level", levelString(ep)}});
            }
            return l;
        };
        bool inv = false;
        const QString type = QString::fromStdString(p.type);
        const QString sym = symbolOf(type, &inv);
        list.append(QVariantMap{{"id", QString::fromStdString(p.id)},
                                {"type", type},
                                {"symbol", sym},
                                {"inverted", inv},
                                {"own", !primitiveFromName(p.type).has_value()},
                                {"x", p.x},
                                {"y", p.y},
                                {"w", w},
                                {"h", h},
                                {"inPins", pins(ins)},
                                {"outPins", pins(outs)}});
    }
    return list;
}

QVariantList WorkbenchController::ports() const {
    QVariantList list;
    const ChipDef* def = shownDef();
    if (!def) return list;
    for (const Port& p : def->ports) {
        qreal x = 0, y = 0;
        pinPos(*def, p.name, x, y);
        QString kind = QStringLiteral("led");
        if (p.direction == Direction::In) {
            kind = QStringLiteral("kipp");
            if (!inside() && !m_stages.empty()) {
                const Stage& s = m_stages[size_t(m_stage)];
                const qsizetype i = s.inputs.indexOf(QString::fromStdString(p.name));
                if (i >= 0) kind = s.inputKinds.value(i, kind);
            }
        }
        list.append(QVariantMap{{"name", QString::fromStdString(p.name)},
                                {"input", p.direction == Direction::In},
                                {"kind", kind},
                                {"x", x},
                                {"y", y},
                                {"level", levelString(p.name)}});
    }
    return list;
}

QVariantList WorkbenchController::wires() const {
    QVariantList list;
    const ChipDef* def = shownDef();
    if (!def) return list;
    for (size_t i = 0; i < def->connections.size(); ++i) {
        const Connection& c = def->connections[i];
        qreal x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        if (!pinPos(*def, c.from, x1, y1) || !pinPos(*def, c.to, x2, y2)) continue;
        list.append(QVariantMap{{"index", int(i)},
                                {"x1", x1},
                                {"y1", y1},
                                {"x2", x2},
                                {"y2", y2},
                                {"level", levelString(c.from)}});
    }
    return list;
}

void WorkbenchController::setSymbolStyle(const QString& style) {
    const QString next = style == "us" ? QStringLiteral("us") : QStringLiteral("din");
    if (next == m_symbolStyle) return;
    m_symbolStyle = next;
    save();
    emit symbolStyleChanged();
}

void WorkbenchController::setMessage(const QString& text) {
    if (m_message == text) return;
    m_message = text;
    emit messageChanged();
}

} // namespace zuse
