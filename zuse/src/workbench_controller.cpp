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
        s.kind = o.value("art").toString("bauen");
        s.story = localized(o.value("geschichte"), language);
        s.merksatz = localized(o.value("merksatz"), language);
        s.name = localized(o.value("name"), language);
        s.tool = o.value("werkzeug").toString();
        // Eingaenge: {"name", "bezeichnung", "art"} oder nur der Name (aeltere Stufen)
        for (const QJsonValue& x : o.value("eingaenge").toArray()) {
            const QJsonObject e = x.toObject();
            s.inputs << (x.isString() ? x.toString() : e.value("name").toString());
            s.inputLabels << (x.isString() ? QString() : localized(e.value("bezeichnung"), language));
            s.inputKinds << (e.value("art").toString() == "taster" ? QStringLiteral("taster") : QStringLiteral("kipp"));
        }
        for (const QJsonValue& x : o.value("ausgaenge").toArray()) {
            const QJsonObject e = x.toObject();
            s.outputs << (x.isString() ? x.toString() : e.value("name").toString());
            s.outputLabels << (x.isString() ? QString() : localized(e.value("bezeichnung"), language));
        }
        for (const QJsonValue& k : o.value("kisten").toArray()) {
            std::vector<std::vector<Level>> table;
            for (const QJsonValue& row : k.toObject().value("tabelle").toArray()) table.push_back(parseLevels(row.toString()));
            s.boxes.push_back(table);
        }
        s.rightBox = o.value("richtig").toInt(-1);
        s.makesChip = s.kind == "bauen" && o.value("chip").toBool(true);
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
    m_numbersVisible = root.value("zahlen").toBool(true);
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
                                         {"schaltzeichen", m_symbolStyle}, {"zahlen", m_numbersVisible}})
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
        const QString name = toolName(type); // leer: noch nicht entdeckt bzw. gebaut
        if (name.isEmpty()) continue;
        bool inv = false;
        const QString sym = symbolOf(type, &inv);
        list.append(QVariantMap{{"type", type},
                                {"name", name},
                                {"symbol", sym},
                                {"inverted", inv},
                                {"own", !primitiveFromName(type.toStdString()).has_value()}});
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
    m_box = -1;
    m_seen.assign(s.table.size(), 0);
    setMessage(QString(), s.kind == "entdecken" ? QStringLiteral("erklaerend") : QStringLiteral("zeigend"));
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
    if (toolName(type).isEmpty()) return {}; // Werkzeug noch nicht in der Kiste
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
    setMessage(tr("So könnte es gehen. Prüfe die Lösung – und bau sie danach gern selbst nach."),
               QStringLiteral("erklaerend"));
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
    if (isDiscover() && m_box >= 0 && currentRow() < m_seen.size()) m_seen[currentRow()] = 1; // ausprobiert
    emit viewChanged();
}

Level WorkbenchController::levelAt(const QString& endpoint) const {
    if (isDiscover()) { // keine Werkbank: Schalter und die eingesetzte Raetselkiste
        const Stage& s = m_stages[size_t(m_stage)];
        const qsizetype in = s.inputs.indexOf(endpoint), out = s.outputs.indexOf(endpoint);
        if (in >= 0) return m_inputs[size_t(in)];
        return out >= 0 ? boxOutput(size_t(out)) : Level::X;
    }
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
        setMessage(tr("Alles richtig! Schalter und LEDs machen, was sie sollen."), QStringLiteral("froehlich"));
    else if (m_check.passed)
        setMessage(tr("Alles richtig! Pack deine Erfindung ein, dann kannst du sie immer wieder benutzen."),
                   QStringLiteral("froehlich"));
    else if (m_check.oscillating)
        setMessage(tr("Die Schaltung kommt nicht zur Ruhe – sie schwingt."), QStringLiteral("ueberrascht"));
    else
        setMessage(tr("Noch nicht ganz: Die markierten Zeilen stimmen nicht. Stell die Schalter so ein und schau, "
                      "was passiert."),
                   QStringLiteral("gruebelnd"));
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
        if (s.kind == "entdecken") { // was die eingesetzte Kiste bisher gezeigt hat; das Soll findet das Kind selbst
            const bool seen = m_box >= 0 && r < m_seen.size() && m_seen[r];
            row.insert("expected", QString());
            row.insert("actual", seen ? joinLevels(s.boxes[size_t(m_box)][r]) : QString());
            row.insert("ok", true);
            row.insert("current", r == currentRow());
        } else if (m_checkDone && r < m_check.rows.size()) {
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
        setMessage(tr("Geschafft! Weiter geht es mit der nächsten Stufe."), QStringLiteral("begeistert"));
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
    setMessage(tr("Eingepackt! »%1« liegt jetzt in deiner Werkzeugkiste.").arg(s.name.isEmpty() ? s.kuerzel : s.name),
               QStringLiteral("begeistert"));
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
        QString label;
        if (!inside() && !m_stages.empty()) {
            const Stage& s = m_stages[size_t(m_stage)];
            const QString n = QString::fromStdString(p.name);
            label = p.direction == Direction::In ? s.inputLabels.value(s.inputs.indexOf(n))
                                                 : s.outputLabels.value(s.outputs.indexOf(n));
        }
        list.append(QVariantMap{{"name", QString::fromStdString(p.name)},
                                {"label", label},
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

QVariantMap WorkbenchController::numberOf(const std::vector<Level>& bits) {
    QString bin;
    bool known = true;
    int value = 0;
    for (Level l : bits) {
        bin += QChar(toChar(l));
        known = known && l != Level::X;
        value = value * 2 + (l == Level::One ? 1 : 0);
    }
    const QString q = QStringLiteral("?");
    return QVariantMap{{"bits", int(bits.size())},
                       {"bin", bin},
                       {"dec", known ? QString::number(value) : q},
                       {"hex", known ? QString::number(value, 16).toUpper() : q},
                       {"value", known ? value : -1}};
}

QVariantMap WorkbenchController::inputNumber() const { return numberOf(m_inputs); }

QVariantMap WorkbenchController::outputNumber() const {
    std::vector<Level> bits;
    if (!m_stages.empty())
        for (const QString& o : m_stages[size_t(m_stage)].outputs) bits.push_back(levelAt(o));
    return numberOf(bits);
}

void WorkbenchController::setNumbersVisible(bool on) {
    if (on == m_numbersVisible) return;
    m_numbersVisible = on;
    save();
    emit numbersVisibleChanged();
}

void WorkbenchController::setSymbolStyle(const QString& style) {
    const QString next = style == "us" ? QStringLiteral("us") : QStringLiteral("din");
    if (next == m_symbolStyle) return;
    m_symbolStyle = next;
    save();
    emit symbolStyleChanged();
}

void WorkbenchController::setMessage(const QString& text, const QString& pose) {
    if (m_message == text && (pose.isEmpty() || pose == m_pose)) return;
    m_message = text;
    if (!pose.isEmpty()) m_pose = pose;
    emit messageChanged();
}

// ---- Entdecken: Raetselkisten ----

size_t WorkbenchController::currentRow() const {
    size_t row = 0;
    for (Level l : m_inputs) row = (row << 1) | (l == Level::One ? 1u : 0u);
    return row;
}

Level WorkbenchController::boxOutput(size_t output) const {
    if (!isDiscover() || m_box < 0) return Level::X; // keine Kiste eingesetzt: nichts angeschlossen
    const auto& table = m_stages[size_t(m_stage)].boxes[size_t(m_box)];
    const size_t row = currentRow();
    return row < table.size() && output < table[row].size() ? table[row][output] : Level::X;
}

QVariantList WorkbenchController::boxes() const {
    QVariantList list;
    if (!isDiscover()) return list;
    const Stage& s = m_stages[size_t(m_stage)];
    const bool solved = m_done.contains(s.kuerzel);
    for (size_t i = 0; i < s.boxes.size(); ++i) {
        const bool right = int(i) == s.rightBox;
        list.append(QVariantMap{{"index", int(i)},
                                {"selected", int(i) == m_box},
                                {"revealed", solved && right}, // nach dem Loesen zeigt die richtige Kiste ihr Zeichen
                                {"symbol", right ? s.symbol : QString()},
                                {"name", right ? s.kuerzel : QString()},
                                {"inverted", right && s.inverted}});
    }
    return list;
}

void WorkbenchController::selectBox(int index) {
    if (!isDiscover() || index < 0 || index >= int(m_stages[size_t(m_stage)].boxes.size()) || index == m_box) return;
    m_box = index;
    std::fill(m_seen.begin(), m_seen.end(), 0); // neue Kiste: neu ausprobieren
    if (currentRow() < m_seen.size()) m_seen[currentRow()] = 1;
    setMessage(tr("Kiste %1 ist eingesetzt. Probier die Schalter aus!").arg(index + 1), QStringLiteral("losgehts"));
    emit viewChanged();
    emit checkChanged();
}

bool WorkbenchController::confirmBox() {
    if (!isDiscover()) return false;
    const Stage& s = m_stages[size_t(m_stage)];
    if (m_box < 0) {
        setMessage(tr("Setz zuerst eine Kiste ein."), QStringLiteral("nachdenklich"));
        return false;
    }
    if (m_box == s.rightBox) {
        m_done.insert(s.kuerzel);
        save();
        setMessage(tr("Richtig! Diese Kiste heißt %1. %2 Sie liegt jetzt in deiner Werkzeugkiste.")
                       .arg(s.kuerzel, s.merksatz),
                   QStringLiteral("begeistert"));
        emit stagesChanged();
        emit viewChanged();
        return true;
    }
    // erste Schalterstellung, in der die Kiste etwas anderes tut als verlangt: konkret erklaeren
    const auto& table = s.boxes[size_t(m_box)];
    for (size_t r = 0; r < s.table.size() && r < table.size(); ++r) {
        if (table[r] == s.table[r]) continue;
        QStringList setting;
        for (qsizetype i = 0; i < s.inputs.size(); ++i) {
            const bool on = (r >> (s.inputs.size() - 1 - i)) & 1u;
            setting << QStringLiteral("%1 %2").arg(s.inputLabels.value(i, s.inputs[i]), on ? tr("an") : tr("aus"));
        }
        const QString label = s.outputLabels.value(0, s.outputs.value(0));
        const QString does = table[r][0] == Level::One ? tr("an") : tr("aus");
        const QString should = s.table[r][0] == Level::One ? tr("an") : tr("aus");
        setMessage(tr("Fast! Bei »%1« ist %2 mit dieser Kiste %3 – es soll aber %4 sein. Probier eine andere Kiste.")
                       .arg(setting.join(", "), label, does, should),
                   QStringLiteral("nachdenklich"));
        return false;
    }
    return false;
}

QString WorkbenchController::toolName(const QString& type) const {
    if (primitiveFromName(type.toStdString())) {
        for (const Stage& s : m_stages)
            if (s.tool == type && m_done.contains(s.kuerzel)) return s.kuerzel; // entdeckt: UND, ODER, NICHT
        return {};
    }
    return m_library.find(type.toStdString()) ? type : QString();
}

QVariantList WorkbenchController::tools() const {
    QVariantList list;
    for (const Stage& s : m_stages) {
        if (!m_done.contains(s.kuerzel)) continue;
        const bool discovered = s.kind == "entdecken" && !s.tool.isEmpty();
        const bool invention = s.kind == "bauen" && s.makesChip && m_library.find(s.kuerzel.toStdString());
        if (!discovered && !invention) continue;
        list.append(QVariantMap{{"type", discovered ? s.tool : s.kuerzel},
                                {"name", discovered ? s.kuerzel : (s.name.isEmpty() ? s.kuerzel : s.name)},
                                {"merksatz", s.merksatz},
                                {"symbol", s.symbol},
                                {"inverted", s.inverted},
                                {"invention", invention}});
    }
    return list;
}

QVariantList WorkbenchController::panelInputs() const {
    QVariantList list;
    if (m_stages.empty()) return list;
    const Stage& s = m_stages[size_t(m_stage)];
    for (qsizetype i = 0; i < s.inputs.size(); ++i)
        list.append(QVariantMap{{"name", s.inputs[i]},
                                {"label", s.inputLabels.value(i)},
                                {"kind", s.inputKinds.value(i, QStringLiteral("kipp"))},
                                {"level", levelChar(m_inputs[size_t(i)])}});
    return list;
}

QVariantList WorkbenchController::panelOutputs() const {
    QVariantList list;
    if (m_stages.empty()) return list;
    const Stage& s = m_stages[size_t(m_stage)];
    for (qsizetype i = 0; i < s.outputs.size(); ++i)
        list.append(QVariantMap{{"name", s.outputs[i]},
                                {"label", s.outputLabels.value(i)},
                                {"level", levelChar(levelAt(s.outputs[i]))}});
    return list;
}

} // namespace zuse
