#include "lab_session.hpp"

#include <QCoreApplication>

#include <algorithm>
#include <map>
#include <cmath>

#include "anchor.hpp"
#include "environment.hpp"

namespace volta::model {

namespace {
constexpr double kBarHysteresis = 0.02; // 2 % der Batteriespannung
constexpr int kUndoLimit = 0;           // Sitzung: ganzer Verlauf (gespeichert werden spaeter die letzten 200)

QString q(const std::string& s) { return QString::fromStdString(s); }
} // namespace

// Gemeinsamer Zustand der Erfolgspruefung; der Bewerter wird nur im Simulations-Thread benutzt.
struct LabSession::SuccessTracker {
    explicit SuccessTracker(SuccessCondition c) : eval(std::move(c)) {}
    SuccessEvaluator eval;
    std::atomic<bool> reached{false};
};

namespace {

// Netzliste ohne Schaltung (nur Zuordnungen), mit den Kennungen der Vorlage als zusaetzliche Namen
Netlist successIndex(const Netlist& net, const Board& board, const Board& tmpl, bool colourFixed) {
    Netlist idx;
    idx.nodes = net.nodes;
    idx.parts = net.parts;
    idx.desk = net.desk;
    idx.modules = net.modules;
    idx.moduleMain = net.moduleMain;
    std::vector<std::string> taken;
    auto isTaken = [&](const std::string& id) { return std::find(taken.begin(), taken.end(), id) != taken.end(); };
    // 1. gleiche Kennung und gleicher Typ
    std::vector<const PartPlacement*> open;
    for (const auto& t : tmpl.parts) {
        const PartPlacement* p = board.findPart(t.id);
        if (p && sameKind(p->type, t.type, colourFixed))
            taken.push_back(p->id);
        else
            open.push_back(&t);
    }
    // Module: gleiche Kennung und gleicher Typ, sonst das naechste freie Modul gleichen Typs; ihre Messwerte
    // (moduleMain) sind in Bedingungen wie Bauteile ansprechbar
    std::vector<std::string> takenModules;
    for (const auto& t : tmpl.modules) {
        std::string match;
        for (const auto& m : board.modules)
            if (m.id == t.id && m.type == t.type) match = m.id;
        if (match.empty())
            for (const auto& m : board.modules)
                if (m.type == t.type &&
                    std::find(takenModules.begin(), takenModules.end(), m.id) == takenModules.end()) {
                    match = m.id;
                    break;
                }
        if (match.empty()) continue;
        takenModules.push_back(match);
        const auto it = net.moduleMain.find(match);
        if (it != net.moduleMain.end()) idx.parts[t.id] = it->second;
    }
    for (const auto& m : board.modules) // eigene Module des Kindes unter ihrer Kennung
        if (const auto it = net.moduleMain.find(m.id); it != net.moduleMain.end() && !idx.parts.count(m.id))
            idx.parts[m.id] = it->second;
    // 2. uebrige Vorlagenbauteile: naechstes freies Bauteil gleichen Typs
    for (const PartPlacement* t : open) {
        idx.parts.erase(t->id);
        for (const auto& p : board.parts) {
            if (!sameKind(p.type, t->type, colourFixed) || isTaken(p.id)) continue;
            const auto it = net.parts.find(p.id);
            if (it == net.parts.end()) continue;
            idx.parts[t->id] = it->second;
            taken.push_back(p.id);
            break;
        }
    }
    return idx;
}

class SuccessObserver : public sim::StepObserver {
public:
    SuccessObserver(std::shared_ptr<void> keep, SuccessEvaluator* eval, std::atomic<bool>* reached, Netlist index)
        : m_keep(std::move(keep)), m_eval(eval), m_reached(reached), m_index(std::move(index)) {}
    void afterStep(const core::Simulator& sim) override {
        if (m_first) {
            m_eval->reset(); // neue Topologie: Haltedauern beginnen neu (Simulationszeit kann neu starten)
            m_first = false;
        }
        m_eval->update(sim, m_index);
        if (m_eval->reached()) m_reached->store(true);
    }

private:
    std::shared_ptr<void> m_keep;
    SuccessEvaluator* m_eval;
    std::atomic<bool>* m_reached;
    Netlist m_index;
    bool m_first = true;
};

} // namespace

void LabSession::setSuccess(const SuccessCondition& condition, const Board& templateBoard, bool colourFixed) {
    m_success = std::make_shared<SuccessTracker>(condition);
    m_successTemplate = templateBoard;
    m_colourFixed = colourFixed;
    rebuild({});
}

void LabSession::clearSuccess() {
    m_success.reset();
    m_successTemplate = {};
    rebuild({});
}

bool LabSession::successReached() const { return m_success && m_success->reached.load(); }

bool LabSession::templateComplete() const {
    if (!m_success) return false;
    std::map<std::string, int> need;
    const auto kind = [this](const std::string& t) { return !m_colourFixed && isLed(t) ? std::string("led") : t; };
    for (const auto& p : m_successTemplate.parts) ++need[kind(p.type)];
    for (const auto& m : m_successTemplate.modules) ++need[m.type];
    for (const auto& p : m_board.parts) --need[kind(p.type)];
    for (const auto& m : m_board.modules) --need[m.type];
    return std::all_of(need.begin(), need.end(), [](const auto& n) { return n.second <= 0; });
}

LabSession::LabSession(Catalog catalog) : m_catalog(std::move(catalog)) {
    m_undo.setUndoLimit(kUndoLimit);
    rebuild({});
}

LabSession::~LabSession() {
    m_undo.clear(); // Befehle verweisen auf diese Sitzung
    m_bridge.stop();
}

void LabSession::loadBoard(const Board& board) {
    m_undo.clear();
    m_board = board;
    m_bars.clear();
    rebuild({});
}

void LabSession::applyBoard(const Board& board, const std::vector<sim::StateOverride>& states) {
    const DeskState desk = m_board.desk; // Bedienhandlungen gehoeren nicht zum Verlauf
    m_board = board;
    m_board.desk = desk;
    // Leitung an einer Messbuchse (z. B. durch Rueckgaengig): Messgeraet sichtbar machen
    for (const auto& w : m_board.wires)
        if (classifyAnchor(w.from) == AnchorKind::MeterJack || classifyAnchor(w.to) == AnchorKind::MeterJack)
            m_board.desk.meterVisible = true;
    rebuild(states);
}

void LabSession::rebuild(const std::vector<sim::StateOverride>& states) {
    m_net = buildNetlist(m_board, m_catalog);
    std::shared_ptr<sim::StepObserver> observer;
    if (m_success) m_success->reached.store(false); // neuer Aufbau: muss erneut funktionieren
    if (m_success)
        observer = std::make_shared<SuccessObserver>(m_success, &m_success->eval, &m_success->reached,
                                                     successIndex(m_net, m_board, m_successTemplate, m_colourFixed));
    m_topology = m_bridge.setCircuit(std::move(m_net.circuit), states, std::move(observer));
    updateStations();
    m_idHistory.emplace_back(m_topology, m_net.componentIds);
    while (m_idHistory.size() > 32) m_idHistory.pop_front();
    if (!states.empty()) m_pendingStates.emplace_back(m_topology, states);
    updateScope();
    if (onBoardChanged) onBoardChanged();
}

std::array<std::string, kScopeChannels> LabSession::scopeProbes() const {
    std::array<std::string, kScopeChannels> out;
    for (const auto& p : m_board.parts) {
        if (p.type != "tastkopf" || p.id.size() < 3) continue;
        const int n = std::atoi(p.id.c_str() + 2); // TK1 -> Kanal 1
        if (n >= 1 && n <= static_cast<int>(kScopeChannels)) out[static_cast<std::size_t>(n - 1)] = p.id;
    }
    return out;
}

void LabSession::setScopeActive(bool active) {
    m_scopeActive = active;
    updateScope();
}

void LabSession::setScopeSettings(const ScopeSettings& s) {
    m_board.desk.scope = s;
    updateScope();
}

// Abtastung der Bruecke nach Tastkoepfen und Zeitbasis; ohne Oszilloskop aus (Schrittweite wieder 1 ms)
void LabSession::updateScope() {
    sim::ScopeConfig c;
    if (hasDeskPart(m_board, "kennlinienschreiber")) {
        c.tracerId = desk_id::kTracer;
        c.tracerIntervalS = kTracerPeriodS / static_cast<double>(kTracerPoints);
        c.tracerCapacity = kTracerPoints;
        c.tracerEpoch = m_tracerEpoch;
    }
    if (m_scopeActive) {
        const ScopeSettings& s = m_board.desk.scope;
        c.probeIds = scopeProbes();
        c.intervalS = s.sampleIntervalS();
        c.capacity = 2 * kScopePointsPerScreen + 200; // zwei Bilder: Trigger mit Vorlauf findet immer ein ganzes
        c.stepS = s.simStepS();
    }
    // Schrittweite nach dem Aufbau (Generator, Lautsprecher), unabhaengig davon, ob der Ton laeuft
    c.stepS = std::min(c.stepS, requiredStepS(m_board, m_timeScale == 1.0));
    if (needsAudioStep(m_board) && m_timeScale == 1.0 && m_audible && m_audio) {
        c.audio = m_audio;
        c.audioId = desk_id::kSpeaker;
        // ohne verdrahteten Lautsprecher klingt der Kristallhoerer (ab Volta 3): seine Spannung, volle Aussteuerung
        // bei u_voll_V
        const std::string ear = speakerWired(m_board) ? std::string() : earpieceId(m_board);
        if (const CatalogEntry* e = ear.empty() ? nullptr : m_catalog.part("kristallhoerer")) {
            c.audioId = ear;
            c.audioVoltScale = AudioStream::kFullScaleA / e->param("u_voll_V", 0.5);
        }
    }
    if (m_timeScale < 1.0) // Zeitlupe: etwa gleich viele Schritte je Sekunde wie in Echtzeit, hoechstens 20 us fein
        c.stepS = std::min(c.stepS, std::clamp(core::kDefaultStepS * m_timeScale, kAudioStepS, core::kDefaultStepS));
    if (c.probeIds == m_scopeConfig.probeIds && c.intervalS == m_scopeConfig.intervalS &&
        c.capacity == m_scopeConfig.capacity && c.stepS == m_scopeConfig.stepS &&
        c.tracerId == m_scopeConfig.tracerId && c.tracerCapacity == m_scopeConfig.tracerCapacity &&
        c.tracerEpoch == m_scopeConfig.tracerEpoch && c.audio == m_scopeConfig.audio &&
        c.audioId == m_scopeConfig.audioId && c.audioVoltScale == m_scopeConfig.audioVoltScale)
        return;
    m_scopeConfig = c;
    m_bridge.setScope(c);
}

bool LabSession::poll() {
    const bool fresh = m_bridge.latest(m_snap);
    // Vorgaben, die der Schnappschuss bereits enthaelt, werden nicht mehr gebraucht
    m_pendingStates.erase(std::remove_if(m_pendingStates.begin(), m_pendingStates.end(),
                                         [this](const auto& p) { return p.first <= m_snap.topology; }),
                          m_pendingStates.end());
    return fresh;
}

double LabSession::voltage(const std::string& anchor) const {
    core::NodeId n = 0;
    if (!snapshotCurrent() || !m_net.nodeOf(anchor, n) || n >= m_snap.nodeVolt.size()) return 0.0;
    return m_snap.nodeVolt[n];
}

const core::ComponentReading* LabSession::reading(const std::string& partId) const {
    const auto it = m_net.parts.find(partId);
    if (!snapshotCurrent() || it == m_net.parts.end() || it->second >= m_snap.readings.size()) return nullptr;
    return &m_snap.readings[it->second];
}

const core::SignalSummary* LabSession::readingStats(const std::string& partId, bool current) const {
    const auto it = m_net.parts.find(partId);
    if (!snapshotCurrent() || it == m_net.parts.end()) return nullptr;
    const std::size_t k = 2 * it->second + (current ? 1 : 0);
    return k < m_snap.readingStats.size() ? &m_snap.readingStats[k] : nullptr;
}

double LabSession::voltageScale() const {
    double ref = 1.5 * m_board.desk.cells;
    if (hasDeskPart(m_board, "batterie2")) ref = std::max(ref, 1.5 * m_board.desk.cells2);
    if (hasDeskPart(m_board, "wechselquelle")) ref = std::max(ref, std::sqrt(2.0) * m_board.desk.acRmsV);
    if (hasDeskPart(m_board, "generator"))
        ref = std::max(ref, std::fabs(m_board.desk.genOffsetV) + m_board.desk.genAmplitudeV);
    if (hasDeskPart(m_board, "kennlinienschreiber")) ref = std::max(ref, m_board.desk.tracer.maxV());
    return ref;
}

LabSession::VoltageDisplay LabSession::voltageDisplay(const std::string& anchor) {
    const double ref = voltageScale();
    // Wechselspannung: Effektivwert des Wechselanteils aus dem letzten Messfenster; sonst der Augenblickswert
    // (wie Volta 1, damit ladende Kondensatoren fluessig angezeigt werden)
    core::NodeId n = 0;
    const core::SignalSummary* stats = nullptr;
    if (snapshotCurrent() && m_net.nodeOf(anchor, n) && n < m_snap.nodeStats.size()) stats = &m_snap.nodeStats[n];
    const bool ac = stats && stats->isAc();
    const double v = ac ? stats->acRms : voltage(anchor);
    const bool negative = !ac && v < 0.0;
    const double ratio = ref > 0.0 ? std::fabs(v) / ref : 0.0;
    auto barsFor = [](double r) {
        if (r < 1.0 / 8) return 0;
        if (r < 3.0 / 8) return 1;
        if (r < 5.0 / 8) return 2;
        if (r < 7.0 / 8) return 3;
        return 4;
    };
    const int candidate = barsFor(ratio);
    const auto it = m_bars.find(anchor);
    if (it == m_bars.end() || it->second.negative != negative || it->second.ac != ac) {
        m_bars[anchor] = VoltageDisplay{candidate, negative, ac}; // Vorzeichen roh, Minuszeichen erst ab 1 Balken
        return VoltageDisplay{candidate, negative && candidate > 0, ac};
    }
    if (candidate != it->second.bars) {
        const int lo = std::min(candidate, it->second.bars);
        const double threshold = (2.0 * lo + 1.0) / 8.0; // Schwelle zwischen lo und lo + 1
        if (std::fabs(ratio - threshold) > kBarHysteresis) it->second.bars = candidate;
    }
    VoltageDisplay d = it->second;
    d.negative = d.negative && d.bars > 0;
    return d;
}

std::vector<sim::StateOverride> LabSession::captureStates() const {
    std::vector<sim::StateOverride> out;
    if (!m_snap.valid) return out;
    // Kennungen der Topologie, zu der der Schnappschuss gehoert (auch aeltere Versionen)
    const std::vector<std::string>* ids = nullptr;
    for (auto it = m_idHistory.rbegin(); it != m_idHistory.rend() && !ids; ++it)
        if (it->first == m_snap.topology) ids = &it->second;
    if (!ids) return out;
    const std::size_t n = std::min(m_snap.states.size(), ids->size());
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) out.push_back({(*ids)[i], m_snap.states[i]});
    // Vorgaben juengerer Topologien (Undo, Reparieren), die der Schnappschuss noch nicht zeigt, haben Vorrang
    for (const auto& [topology, states] : m_pendingStates) {
        if (topology <= m_snap.topology) continue;
        for (const auto& o : states) {
            const auto it = std::find_if(out.begin(), out.end(),
                                         [&o](const sim::StateOverride& x) { return x.componentId == o.componentId; });
            if (it != out.end()) it->state = o.state;
            else out.push_back(o);
        }
    }
    return out;
}

bool LabSession::fail(Message message) {
    m_error = std::move(message);
    return false;
}

bool LabSession::setColumns(int columns) {
    if (columns < kMinColumns || columns > kMaxColumns || (columns - kMinColumns) % kColumnStep != 0)
        return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Die Grundplatte hat 8, 10 oder 12 Felder je Reihe.")));
    if (columns == m_board.columns) return true;
    if (highestUsedColumn(m_board) > columns)
        return fail(msg(QT_TRANSLATE_NOOP("Meldung", "In den Feldern %1–%2 steckt noch etwas. Nimm dort zuerst Bauteile und Leitungen heraus."), {std::to_string(columns + 1), std::to_string(m_board.columns)}));
    return setBoardSize(m_board.rows, columns);
}

bool LabSession::setBoardSize(int rows, int columns) {
    if (!validBoardSize(rows, columns)) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Diese Größe hat die Grundplatte nicht.")));
    if (rows == m_board.rows && columns == m_board.columns) return true;
    if (highestUsedColumn(m_board) > columns || highestUsedRow(m_board) > rows)
        return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Auf den wegfallenden Feldern steckt noch etwas. Nimm dort zuerst Bauteile und Leitungen heraus.")));
    Board after = m_board;
    after.rows = rows;
    after.columns = columns;
    // Spannungsanzeigen auf wegfallenden Feldern entfallen (kommen mit Rueckgaengig zurueck)
    after.voltageDisplays.erase(std::remove_if(after.voltageDisplays.begin(), after.voltageDisplays.end(),
                                               [&after](const std::string& f) { return !withinBoard(after, f); }),
                                after.voltageDisplays.end());
    return push(QCoreApplication::translate("Befehl", "Grundplatte 1–%1").arg(columns), std::move(after));
}

bool LabSession::replaceBoard(const Board& after, const QString& text) {
    Board b = after;
    b.desk = m_board.desk;
    // Pult nach Bedarf: was schon im Pult steht, bleibt; dazu, was der neue Aufbau benutzt
    if (m_board.deskParts && !b.deskParts) b.deskParts = m_board.deskParts;
    if (b.deskParts)
        for (const auto& g : deskPartsUsed(b))
            if (std::find(b.deskParts->begin(), b.deskParts->end(), g) == b.deskParts->end()) b.deskParts->push_back(g);
    // Groesse bleibt (kleines Steckbrett der Stufe), solange der neue Aufbau hineinpasst
    b.rows = std::max(m_board.rows, highestUsedRow(b));
    b.columns = m_board.columns;
    if (highestUsedColumn(b) > b.columns) b.columns = after.columns;
    return push(text, std::move(b));
}

bool LabSession::addDeskPart(const std::string& group) { return addDeskParts({group}); }

bool LabSession::addDeskParts(const std::vector<std::string>& groups) {
    const auto& names = deskGroupNames();
    if (!m_board.deskParts) return false;
    bool missing = false;
    for (const auto& g : groups) {
        if (std::find(names.begin(), names.end(), g) == names.end()) return false;
        if (!hasDeskPart(m_board, g)) missing = true;
    }
    if (!missing) return true;
    Board after = m_board;
    std::vector<std::string> parts;
    for (const auto& g : names) // Anzeigereihenfolge
        if (hasDeskPart(m_board, g) || std::find(groups.begin(), groups.end(), g) != groups.end()) parts.push_back(g);
    after.deskParts = std::move(parts);
    return push(QCoreApplication::translate("Befehl", "Pultteil einsetzen"), std::move(after));
}

bool LabSession::removeDeskPart(const std::string& group) {
    if (!m_board.deskParts || group == "batterie") return false;
    if (!hasDeskPart(m_board, group)) return true;
    Board after = m_board;
    after.deskParts->erase(std::remove(after.deskParts->begin(), after.deskParts->end(), group), after.deskParts->end());
    // Leitungen und Spannungsanzeigen an seinen Feldern gehen mit (kommen mit Rueckgaengig zurueck)
    after.wires.erase(std::remove_if(after.wires.begin(), after.wires.end(),
                                     [&group](const WirePlacement& w) {
                                         return deskGroupOf(w.from) == group || deskGroupOf(w.to) == group;
                                     }),
                      after.wires.end());
    after.voltageDisplays.erase(std::remove_if(after.voltageDisplays.begin(), after.voltageDisplays.end(),
                                               [&group](const std::string& f) { return deskGroupOf(f) == group; }),
                                after.voltageDisplays.end());
    return push(QCoreApplication::translate("Befehl", "Pultteil herausnehmen"), std::move(after));
}

bool LabSession::adoptSetup(const Board& add) {
    Board after = m_board;
    if (mergeSetup(after, add, m_catalog) == 0) return false;
    return push(QCoreApplication::translate("Befehl", "Aufbau übernehmen"), std::move(after));
}

bool LabSession::push(const QString& text, Board after, int mergeId, const QString& key,
                      std::vector<sim::StateOverride> statesAfter) {
    m_error = {};
    m_undo.push(new BoardCommand(*this, text, m_board, std::move(after), captureStates(), std::move(statesAfter),
                                 mergeId, key));
    return true;
}

// ------------------------------------------------------------------ Aufbauaenderungen

bool LabSession::addPart(const std::string& type, const std::string& startField, Orientation o, Span span,
                         std::string* newId, double value) {
    const CatalogEntry* e = m_catalog.part(type);
    if (!e) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Unbekanntes Bauteil „%1“."), {type}));
    PartPlacement p;
    p.id = nextPartId(m_board, type);
    p.type = type;
    p.value = value > 0.0 ? value : (type == "widerstand" ? 1000.0 : (type == "elko" ? 100e-6 : 0.0));
    if (!(value > 0.0))
        if (const CatalogEntry* ce = m_catalog.part(type); ce && !ce->variants.empty())
            p.value = ce->param("u_nenn_V", 0.0); // Gluehlampe: Nennspannung der Vorgabe
    Message err;
    if (!placePins(*e, startField, o, span, p.pins, err) || !canPlace(m_board, m_catalog, p, err)) return fail(err);
    Board after = m_board;
    after.parts.push_back(p);
    if (newId) *newId = p.id;
    return push(QCoreApplication::translate("Befehl", "%1 setzen").arg(q(p.id)), std::move(after));
}

bool LabSession::previewPart(const std::string& type, const std::string& startField, Orientation o, Span span,
                             std::vector<std::pair<std::string, std::string>>& pins) const {
    pins.clear();
    const CatalogEntry* e = m_catalog.part(type);
    if (!e) return false;
    PartPlacement p;
    p.id = nextPartId(m_board, type);
    p.type = type;
    Message err;
    if (!placePins(*e, startField, o, span, p.pins, err)) return false;
    pins = p.pins;
    return canPlace(m_board, m_catalog, p, err);
}

bool LabSession::movePart(const std::string& id, const std::string& startField, Orientation o, Span span) {
    const PartPlacement* old = m_board.findPart(id);
    if (!old) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Bauteil %1 nicht gefunden."), {id}));
    const CatalogEntry* e = m_catalog.part(old->type);
    if (!e) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Unbekanntes Bauteil „%1“."), {id}));
    PartPlacement moved = *old;
    Message err;
    if (isSquarePlate(*e)) {
        // 2-x-2-Platine: als Ganzes verschieben, Drehung bleibt erhalten
        if (!translatePlate(moved, startField)) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Das Bauteil passt dort nicht auf die Grundplatte.")));
    } else {
        const bool flipped = isFlipped(*old, *e);
        if (!placePins(*e, startField, o, span, moved.pins, err)) return fail(err);
        if (flipped) flipPins(moved, *e); // Polung bleibt beim Verschieben und Drehen erhalten
    }
    if (!canPlace(m_board, m_catalog, moved, err, id)) return fail(err);
    if (moved.pins == old->pins) return true; // nichts zu tun
    Board after = m_board;
    *findPart(after, id) = moved;
    return push(QCoreApplication::translate("Befehl", "%1 verschieben").arg(q(id)), std::move(after), BoardCommand::MergeMovePart, q(id));
}

bool LabSession::flipPart(const std::string& id) {
    Board after = m_board;
    PartPlacement* p = findPart(after, id);
    if (!p) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Bauteil %1 nicht gefunden."), {id}));
    const CatalogEntry* e = m_catalog.part(p->type);
    if (!e) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Unbekanntes Bauteil „%1“."), {id}));
    flipPins(*p, *e);
    return push(QCoreApplication::translate("Befehl", "%1 umdrehen").arg(q(id)), std::move(after));
}

bool LabSession::rotatePart(const std::string& id) {
    Board after = m_board;
    PartPlacement* p = findPart(after, id);
    if (!p) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Bauteil %1 nicht gefunden."), {id}));
    const CatalogEntry* e = m_catalog.part(p->type);
    if (!e || !isSquarePlate(*e)) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Nur Platinen lassen sich so drehen.")));
    rotatePlate(*p);
    return push(QCoreApplication::translate("Befehl", "%1 drehen").arg(q(id)), std::move(after));
}

bool LabSession::removePart(const std::string& id) {
    Board after = m_board;
    const auto it = std::find_if(after.parts.begin(), after.parts.end(), [&id](const auto& p) { return p.id == id; });
    if (it == after.parts.end()) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Bauteil %1 nicht gefunden."), {id}));
    after.parts.erase(it);
    return push(QCoreApplication::translate("Befehl", "%1 entfernen").arg(q(id)), std::move(after));
}

bool LabSession::setPartValue(const std::string& id, double value) {
    if (!(value > 0.0) || !std::isfinite(value)) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Dieser Wert geht nicht.")));
    Board after = m_board;
    PartPlacement* p = findPart(after, id);
    if (!p) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Bauteil %1 nicht gefunden."), {id}));
    if (const CatalogEntry* ce = m_catalog.part(p->type); ce && !ce->variants.empty()) {
        bool offered = false;
        for (const auto& v : ce->variants)
            if (const auto it = v.find("u_nenn_V"); it != v.end() && std::abs(it->second - value) < 1e-9) offered = true;
        if (!offered) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Diese Nennspannung gibt es nicht.")));
    }
    if (p->value == value) return true;
    p->value = value;
    return push(QCoreApplication::translate("Befehl", "Wert von %1 ändern").arg(q(id)), std::move(after), BoardCommand::MergePartValue,
                q(id));
}

bool LabSession::repairPart(const std::string& id) {
    if (!m_board.findPart(id)) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Bauteil %1 nicht gefunden."), {id}));
    // Reparieren = neues, kaltes, ungeladenes Bauteil an derselben Stelle (Befehl im Verlauf)
    return push(QCoreApplication::translate("Befehl", "%1 reparieren").arg(q(id)), m_board, BoardCommand::NoMerge, {},
                {{id, core::ComponentState{}}});
}

// ------------------------------------------------------------------ Module

bool LabSession::addModule(const std::string& type, int slot, std::string* newId) {
    ModulePlacement m{nextModuleId(m_board, type), type, slot};
    Message err;
    if (!canPlaceModule(m_board, m_catalog, m, err)) return fail(err);
    Board after = m_board;
    after.modules.push_back(m);
    if (newId) *newId = m.id;
    return push(QCoreApplication::translate("Befehl", "%1 einsetzen").arg(q(m.id)), std::move(after));
}

bool LabSession::moveModule(const std::string& id, int slot) {
    Board after = m_board;
    ModulePlacement* m = findModule(after, id);
    if (!m) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Modul %1 nicht gefunden."), {id}));
    if (m->slot == slot) return true;
    ModulePlacement moved = *m;
    moved.slot = slot;
    Message err;
    if (!canPlaceModule(m_board, m_catalog, moved, err, id)) return fail(err);
    // Leitungen auf den Feldern unter dem Modul wandern mit (Leitungen an "<ID>.<Anschluss>" ohnehin)
    const ModuleEntry* e = m_catalog.module(m->type);
    const int width = e ? static_cast<int>(e->pins.size()) : 1;
    const int delta = slot - m->slot;
    for (auto& w : after.wires) {
        for (std::string* end : {&w.from, &w.to}) {
            if (classifyAnchor(*end) != AnchorKind::ModuleSlot) continue;
            const int n = std::stoi(end->substr(2));
            if (n >= m->slot && n < m->slot + width) *end = moduleSlot(n + delta);
        }
    }
    m->slot = slot;
    return push(QCoreApplication::translate("Befehl", "%1 verschieben").arg(q(id)), std::move(after));
}

bool LabSession::removeModule(const std::string& id) {
    Board after = m_board;
    const auto it =
        std::find_if(after.modules.begin(), after.modules.end(), [&id](const auto& m) { return m.id == id; });
    if (it == after.modules.end()) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Modul %1 nicht gefunden."), {id}));
    // Leitungen an Modulanschluessen bleiben auf dem Feld der Leiste liegen
    for (auto& w : after.wires)
        for (std::string* end : {&w.from, &w.to})
            if (end->compare(0, id.size() + 1, id + ".") == 0) {
                const std::string slot = moduleSlotOf(m_board, m_catalog, *end);
                if (!slot.empty()) *end = slot;
            }
    after.modules.erase(it);
    return push(QCoreApplication::translate("Befehl", "%1 herausnehmen").arg(q(id)), std::move(after));
}

bool LabSession::addWire(const std::string& from, const std::string& to, const std::string& color,
                         std::string* newId) {
    Message err;
    if (!canWire(m_board, from, to, err)) return fail(err);
    Board after = m_board;
    WirePlacement w{nextWireId(m_board), from, to, color.empty() ? "rot" : color};
    if (newId) *newId = w.id;
    after.wires.push_back(w);
    return push(QCoreApplication::translate("Befehl", "Leitung %1 ziehen").arg(q(w.id)), std::move(after));
}

bool LabSession::moveWireEnd(const std::string& id, bool fromEnd, const std::string& anchor, bool autoColor) {
    Board after = m_board;
    WirePlacement* w = findWire(after, id);
    if (!w) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Leitung %1 nicht gefunden."), {id}));
    std::string& end = fromEnd ? w->from : w->to;
    if (end == anchor) return true;
    end = anchor;
    Message err;
    if (!canWire(m_board, w->from, w->to, err)) return fail(err);
    if (autoColor && !w->colorFixed) {
        const std::string c = poleColor(m_board, w->from, w->to, id);
        if (!c.empty()) w->color = c;
    }
    return push(QCoreApplication::translate("Befehl", "Leitung %1 umsetzen").arg(q(id)), std::move(after), BoardCommand::MergeMoveWireEnd,
                q(id + (fromEnd ? ":von" : ":nach")));
}

bool LabSession::removeWire(const std::string& id) {
    Board after = m_board;
    const auto it = std::find_if(after.wires.begin(), after.wires.end(), [&id](const auto& w) { return w.id == id; });
    if (it == after.wires.end()) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Leitung %1 nicht gefunden."), {id}));
    after.wires.erase(it);
    return push(QCoreApplication::translate("Befehl", "Leitung %1 löschen").arg(q(id)), std::move(after));
}

bool LabSession::setWireColor(const std::string& id, const std::string& color) {
    Board after = m_board;
    WirePlacement* w = findWire(after, id);
    if (!w) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Leitung %1 nicht gefunden."), {id}));
    if (w->color == color && w->colorFixed) return true;
    w->color = color;
    w->colorFixed = true; // von Hand gewaehlt
    return push(QCoreApplication::translate("Befehl", "Leitung %1 umfärben").arg(q(id)), std::move(after));
}

bool LabSession::toggleVoltageDisplay(const std::string& field) {
    const AnchorKind k = classifyAnchor(field);
    if (k != AnchorKind::Field && k != AnchorKind::Desk) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Die Spannungsanzeige gibt es nur an Feldern.")));
    if (!withinBoard(m_board, field)) return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Dieses Feld liegt außerhalb der Grundplatte.")));
    Board after = m_board;
    auto& v = after.voltageDisplays;
    const auto it = std::find(v.begin(), v.end(), field);
    if (it == v.end()) v.push_back(field);
    else v.erase(it);
    return push(QCoreApplication::translate("Befehl", "Spannungsanzeige %1").arg(q(field)), std::move(after));
}

// ------------------------------------------------------------------ Bedienhandlungen

void LabSession::setMainSwitch(bool on) {
    m_board.desk.mainSwitch = on;
    m_bridge.post({desk_id::kMainSwitch, sim::Action::Type::SetSwitch, on ? 1.0 : 0.0});
    if (hasDeskPart(m_board, "batterie2"))
        for (const char* id : {desk_id::kMainSwitch2, desk_id::kMainSwitch2Minus})
            m_bridge.post({id, sim::Action::Type::SetSwitch, on ? 1.0 : 0.0});
    if (hasDeskPart(m_board, "wechselquelle"))
        m_bridge.post({desk_id::kAcSource, sim::Action::Type::SetSwitch, on ? 1.0 : 0.0});
    if (hasDeskPart(m_board, "generator"))
        m_bridge.post({desk_id::kGenerator, sim::Action::Type::SetSwitch, on ? 1.0 : 0.0});
}

void LabSession::updateStations() {
    // ab Volta 3: Sender der Senderanlage an die Bruecke (ohne Senderanlage nur einmal die leere Liste)
    auto stations = deskStations(m_board);
    if (stations.empty() && !m_hadStations) return;
    m_hadStations = !stations.empty();
    m_bridge.setStations(std::move(stations));
}

void LabSession::setStationOn(int index, bool on) {
    if (index < 0 || index >= static_cast<int>(m_board.desk.stationsOn.size())) return;
    m_board.desk.stationsOn[static_cast<std::size_t>(index)] = on;
    updateStations();
}

void LabSession::setAcRms(int volt) {
    m_board.desk.acRmsV = volt == 6 ? 6 : 9;
    m_bars.clear();
    m_bridge.post({desk_id::kAcSource, sim::Action::Type::SetAcRms, static_cast<double>(m_board.desk.acRmsV)});
}

void LabSession::setGenWaveform(int waveform) {
    m_board.desk.genWaveform = std::clamp(waveform, 0, 2);
    m_bridge.post({desk_id::kGenerator, sim::Action::Type::SetGenWaveform, double(m_board.desk.genWaveform)});
}

void LabSession::setGenFrequency(double hz) {
    m_board.desk.genFrequencyHz = std::isfinite(hz) ? std::clamp(hz, 1.0, 10000.0) : 100.0;
    m_bridge.post({desk_id::kGenerator, sim::Action::Type::SetGenFrequency, m_board.desk.genFrequencyHz});
    updateScope(); // Schrittweite folgt der Frequenz (requiredStepS)
}

void LabSession::setGenAmplitude(double peakV) {
    m_board.desk.genAmplitudeV = std::isfinite(peakV) ? std::clamp(peakV, 0.0, 4.0) : 0.0;
    m_bars.clear();
    m_bridge.post({desk_id::kGenerator, sim::Action::Type::SetGenAmplitude, m_board.desk.genAmplitudeV});
}

void LabSession::setGenOffset(double volt) {
    m_board.desk.genOffsetV = std::isfinite(volt) ? std::clamp(volt, -4.0, 4.0) : 0.0;
    m_bars.clear();
    m_bridge.post({desk_id::kGenerator, sim::Action::Type::SetGenOffset, m_board.desk.genOffsetV});
}

void LabSession::setAudioStream(std::shared_ptr<AudioStream> stream) {
    m_audio = std::move(stream);
    updateScope();
}

void LabSession::setAudible(bool audible) {
    if (audible == m_audible) return;
    m_audible = audible;
    updateScope();
}

void LabSession::setTimeScale(double scale) {
    m_timeScale = std::isfinite(scale) && scale > 0.0 ? scale : 1.0;
    m_bridge.setTimeScale(m_timeScale);
    updateScope();
}

void LabSession::setTracerSettings(const TracerSettings& s) {
    TracerSettings t = s;
    t.voltsIndex = std::clamp(t.voltsIndex, 0, static_cast<int>(tracerVolts().size()) - 1);
    t.limitIndex = std::clamp(t.limitIndex, 0, static_cast<int>(tracerLimitsA().size()) - 1);
    m_board.desk.tracer = t;
    m_bars.clear();
    // erst die Obergrenze, dann die Untergrenze: min <= max bleibt in jedem Zwischenschritt erhalten
    m_bridge.post({desk_id::kTracer, sim::Action::Type::SetTracerMax, t.maxV()});
    m_bridge.post({desk_id::kTracer, sim::Action::Type::SetTracerMin, t.minV()});
    m_bridge.post({desk_id::kTracer, sim::Action::Type::SetTracerOhm, t.seriesOhm()});
    ++m_tracerEpoch;
    updateScope();
}

void LabSession::setCells2(int cells) {
    m_board.desk.cells2 = cells == 3 ? 3 : 6;
    m_bars.clear();
    if (hasDeskPart(m_board, "batterie2"))
        m_bridge.post({desk_id::kBattery2, sim::Action::Type::SetCells, static_cast<double>(m_board.desk.cells2)});
}

void LabSession::setCells(int cells) {
    m_board.desk.cells = std::clamp(cells, 1, 6);
    m_bars.clear(); // Bezugsspannung der Balken aendert sich
    m_bridge.post({desk_id::kBattery, sim::Action::Type::SetCells, static_cast<double>(m_board.desk.cells)});
}

void LabSession::setPartClosed(const std::string& id, bool closed) {
    for (auto& p : m_board.parts)
        if (p.id == id) p.closed = closed; // gehoert zum Aufbau-Zustand, aber nicht zum Verlauf
    m_bridge.post({id, sim::Action::Type::SetSwitch, closed ? 1.0 : 0.0});
}

void LabSession::setDeskSwitch(bool closed) {
    m_board.desk.switchClosed = closed;
    m_bridge.post({desk_id::kSwitch, sim::Action::Type::SetSwitch, closed ? 1.0 : 0.0});
}

void LabSession::setPot(int which, double position) {
    const double pos = std::clamp(position, 0.0, 1.0);
    (which == 2 ? m_board.desk.p2 : m_board.desk.p1) = pos;
    m_bridge.post({which == 2 ? desk_id::kPot2 : desk_id::kPot1, sim::Action::Type::SetPotPosition, pos});
}

void LabSession::setPartPosition(const std::string& id, double position) {
    const double pos = std::clamp(position, 0.0, 1.0);
    for (auto& p : m_board.parts)
        if (p.id == id) {
            p.position = pos; // gehoert zum Aufbau-Zustand, aber nicht zum Verlauf
            // Drehkondensator (ab Volta 3): die Stellung ergibt die Kapazitaet
            const CatalogEntry* e = m_catalog.part(p.type);
            if (e && e->model == "capacitor_variable") {
                m_bridge.post({id, sim::Action::Type::SetCapacitance, variableCapacitanceF(*e, pos)});
                return;
            }
        }
    m_bridge.post({id, sim::Action::Type::SetPotPosition, pos});
}

void LabSession::postEnvironment() {
    for (const auto& p : m_board.parts) {
        const CatalogEntry* e = m_catalog.part(p.type);
        if (!e) continue;
        if (e->model == "ldr") m_bridge.post({p.id, sim::Action::Type::SetIlluminance, illuminanceAt(m_board, p)});
        if (e->model == "phototransistor") {
            m_bridge.post({p.id, sim::Action::Type::SetIlluminance, illuminanceAt(m_board, p)});
            m_bridge.post({p.id, sim::Action::Type::SetShade, shadeAt(m_board, p)});
        }
        if (e->model == "ntc") m_bridge.post({p.id, sim::Action::Type::SetAmbient, airTemperatureAt(m_board, p)});
        if (e->model == "moisture") m_bridge.post({p.id, sim::Action::Type::SetWetness, wetnessAt(m_board, p)});
    }
}

void LabSession::setRoomLight(int level) {
    m_board.desk.roomLight = std::clamp(level, 0, 2);
    postEnvironment();
}

void LabSession::placeTool(const std::string& type, double col, double row) {
    if (type != kToolFlashlight && type != kToolDryer && type != kToolWater && type != kToolCard) return;
    col = std::clamp(col, 0.0, 13.0);
    row = std::clamp(row, -1.5, 5.5);
    auto& tools = m_board.desk.tools;
    auto it = std::find_if(tools.begin(), tools.end(), [&type](const ToolPlacement& t) { return t.type == type; });
    if (it == tools.end()) tools.push_back({type, col, row, true});
    else {
        it->col = col;
        it->row = row;
    }
    postEnvironment();
}

void LabSession::setToolOn(const std::string& type, bool on) {
    for (auto& t : m_board.desk.tools)
        if (t.type == type) t.on = on;
    postEnvironment();
}

void LabSession::removeTool(const std::string& type) {
    auto& tools = m_board.desk.tools;
    tools.erase(std::remove_if(tools.begin(), tools.end(), [&type](const ToolPlacement& t) { return t.type == type; }),
                tools.end());
    postEnvironment();
}

void LabSession::setMotorBlocked(const std::string& id, bool blocked) {
    m_bridge.post({id, sim::Action::Type::SetMotorBlocked, blocked ? 1.0 : 0.0});
}

void LabSession::setMeterMode(core::MeterMode mode) {
    m_board.desk.meterMode = mode;
    m_bridge.post({desk_id::kMeter, sim::Action::Type::SetMeterMode, static_cast<double>(mode)});
}

void LabSession::setMeterAnalog(bool analog) {
    m_board.desk.meterAnalog = analog;
    m_bridge.post({desk_id::kMeter, sim::Action::Type::SetMeterAnalog, analog ? 1.0 : 0.0});
}

bool LabSession::setMeterVisible(bool visible) {
    if (!visible) {
        for (const auto& w : m_board.wires)
            if (classifyAnchor(w.from) == AnchorKind::MeterJack || classifyAnchor(w.to) == AnchorKind::MeterJack)
                return fail(msg(QT_TRANSLATE_NOOP("Meldung", "Zieh zuerst die Leitungen vom Messgerät ab.")));
    }
    m_board.desk.meterVisible = visible;
    return true;
}

bool LabSession::replaceMeterFuse() {
    // neue Sicherung = Messgeraet im Grundzustand (Befehl im Verlauf, wie Reparieren)
    return push(QCoreApplication::translate("Befehl", "Sicherung tauschen"), m_board, BoardCommand::NoMerge, {},
                {{desk_id::kMeter, core::ComponentState{}}});
}

bool LabSession::repairPot(int which) {
    return push(QCoreApplication::translate("Befehl", "Drehregler P%1 reparieren").arg(which == 2 ? 2 : 1), m_board,
                BoardCommand::NoMerge, {}, {{which == 2 ? desk_id::kPot2 : desk_id::kPot1, core::ComponentState{}}});
}

bool LabSession::repairSpeaker() {
    return push(QCoreApplication::translate("Befehl", "Lautsprecher reparieren"), m_board, BoardCommand::NoMerge, {},
                {{desk_id::kSpeaker, core::ComponentState{}}});
}

const core::ComponentReading* LabSession::moduleReading(const std::string& moduleId) const {
    const auto it = m_net.moduleMain.find(moduleId);
    if (!snapshotCurrent() || it == m_net.moduleMain.end() || it->second >= m_snap.readings.size()) return nullptr;
    return &m_snap.readings[it->second];
}

bool LabSession::speakerReading(core::ComponentReading& out) const {
    const core::ComponentReading* r = deskReading(desk_id::kSpeaker);
    if (!r) return false;
    out = *r;
    constexpr double kMinA = 2e-3;         // kleinster Strom, der als Ton zaehlt
    constexpr double kFullScaleA = 0.1;    // wie core::SpeakerParams::fullScaleA
    constexpr double kSameCurrent = 0.2;   // Stroeme gelten als derselbe Strom (Reihenschaltung)
    const double iLs = std::fabs(r->currentA);
    if (r->destroyed || iLs < kMinA) return true;
    for (const auto& m : m_board.modules) {
        if (m.type != "tonmodul") continue;
        const core::ComponentReading* t = moduleReading(m.id);
        if (!t || t->frequencyHz <= 0.0) continue;
        const double iOut = std::fabs(t->currentA);
        if (iOut < kMinA || std::fabs(iOut - iLs) > kSameCurrent * iOut) continue;
        out.frequencyHz = t->frequencyHz;
        out.level = std::clamp(iLs / kFullScaleA, 0.0, 1.0);
        break;
    }
    return true;
}

std::vector<CircuitFinding> LabSession::checkCircuit() const {
    return model::checkCircuit({m_board, m_catalog, [this](const std::string& id) -> const core::ComponentReading* {
                                    return id.rfind("PULT.", 0) == 0 ? deskReading(id) : reading(id);
                                }});
}

const core::ComponentReading* LabSession::deskReading(const std::string& deskId) const {
    const auto it = m_net.desk.find(deskId);
    if (!snapshotCurrent() || it == m_net.desk.end() || it->second >= m_snap.readings.size()) return nullptr;
    return &m_snap.readings[it->second];
}

} // namespace volta::model
