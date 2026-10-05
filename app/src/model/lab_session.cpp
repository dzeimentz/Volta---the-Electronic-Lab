#include "lab_session.hpp"

#include <algorithm>
#include <cmath>

#include "anchor.hpp"

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
Netlist successIndex(const Netlist& net, const Board& board, const Board& tmpl) {
    Netlist idx;
    idx.nodes = net.nodes;
    idx.parts = net.parts;
    idx.desk = net.desk;
    idx.modules = net.modules;
    std::vector<std::string> taken;
    auto isTaken = [&](const std::string& id) { return std::find(taken.begin(), taken.end(), id) != taken.end(); };
    // 1. gleiche Kennung und gleicher Typ
    std::vector<const PartPlacement*> open;
    for (const auto& t : tmpl.parts) {
        const PartPlacement* p = board.findPart(t.id);
        if (p && p->type == t.type)
            taken.push_back(p->id);
        else
            open.push_back(&t);
    }
    // 2. uebrige Vorlagenbauteile: naechstes freies Bauteil gleichen Typs
    for (const PartPlacement* t : open) {
        idx.parts.erase(t->id);
        for (const auto& p : board.parts) {
            if (p.type != t->type || isTaken(p.id)) continue;
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

void LabSession::setSuccess(const SuccessCondition& condition, const Board& templateBoard) {
    m_success = std::make_shared<SuccessTracker>(condition);
    m_successTemplate = templateBoard;
    rebuild({});
}

void LabSession::clearSuccess() {
    m_success.reset();
    m_successTemplate = {};
    rebuild({});
}

bool LabSession::successReached() const { return m_success && m_success->reached.load(); }

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
    if (m_success)
        observer = std::make_shared<SuccessObserver>(m_success, &m_success->eval, &m_success->reached,
                                                     successIndex(m_net, m_board, m_successTemplate));
    m_topology = m_bridge.setCircuit(std::move(m_net.circuit), states, std::move(observer));
    m_idHistory.emplace_back(m_topology, m_net.componentIds);
    while (m_idHistory.size() > 32) m_idHistory.pop_front();
    if (!states.empty()) m_pendingStates.emplace_back(m_topology, states);
    if (onBoardChanged) onBoardChanged();
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

int LabSession::voltageBars(const std::string& anchor) {
    const double ref = 1.5 * m_board.desk.cells;
    const double ratio = ref > 0.0 ? voltage(anchor) / ref : 0.0;
    auto barsFor = [](double r) {
        if (r < 1.0 / 8) return 0;
        if (r < 3.0 / 8) return 1;
        if (r < 5.0 / 8) return 2;
        if (r < 7.0 / 8) return 3;
        return 4;
    };
    const int candidate = barsFor(ratio);
    const auto it = m_bars.find(anchor);
    if (it == m_bars.end()) {
        m_bars[anchor] = candidate;
        return candidate;
    }
    if (candidate != it->second) {
        const int lo = std::min(candidate, it->second);
        const double threshold = (2.0 * lo + 1.0) / 8.0; // Schwelle zwischen lo und lo + 1
        if (std::fabs(ratio - threshold) > kBarHysteresis) it->second = candidate;
    }
    return it->second;
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

bool LabSession::fail(std::string message) {
    m_error = std::move(message);
    return false;
}

bool LabSession::setColumns(int columns) {
    if (columns < kMinColumns || columns > kMaxColumns || (columns - kMinColumns) % kColumnStep != 0)
        return fail("Grundplatte: 8, 10 oder 12 Felder je Reihe");
    if (columns == m_board.columns) return true;
    if (highestUsedColumn(m_board) > columns)
        return fail("Felder " + std::to_string(columns + 1) + "-" + std::to_string(m_board.columns) +
                    " sind noch belegt - erst Bauteile und Leitungen dort entfernen");
    Board after = m_board;
    after.columns = columns;
    // Spannungsanzeigen auf wegfallenden Feldern entfallen (kommen mit Rueckgaengig zurueck)
    after.voltageDisplays.erase(std::remove_if(after.voltageDisplays.begin(), after.voltageDisplays.end(),
                                               [&after](const std::string& f) { return !withinBoard(after, f); }),
                                after.voltageDisplays.end());
    return push(QStringLiteral("Grundplatte 1-%1").arg(columns), std::move(after));
}

bool LabSession::replaceBoard(const Board& after, const QString& text) {
    Board b = after;
    b.desk = m_board.desk;
    return push(text, std::move(b));
}

bool LabSession::push(const QString& text, Board after, int mergeId, const QString& key,
                      std::vector<sim::StateOverride> statesAfter) {
    m_error.clear();
    m_undo.push(new BoardCommand(*this, text, m_board, std::move(after), captureStates(), std::move(statesAfter),
                                 mergeId, key));
    return true;
}

// ------------------------------------------------------------------ Aufbauaenderungen

bool LabSession::addPart(const std::string& type, const std::string& startField, Orientation o, Span span,
                         std::string* newId, double value) {
    const CatalogEntry* e = m_catalog.part(type);
    if (!e) return fail("unbekannter Bauteiltyp '" + type + "'");
    PartPlacement p;
    p.id = nextPartId(m_board, type);
    p.type = type;
    p.value = value > 0.0 ? value : (type == "widerstand" ? 1000.0 : (type == "elko" ? 100e-6 : 0.0));
    std::string err;
    if (!placePins(*e, startField, o, span, p.pins, err) || !canPlace(m_board, m_catalog, p, err)) return fail(err);
    Board after = m_board;
    after.parts.push_back(p);
    if (newId) *newId = p.id;
    return push(QStringLiteral("%1 setzen").arg(q(p.id)), std::move(after));
}

bool LabSession::movePart(const std::string& id, const std::string& startField, Orientation o, Span span) {
    const PartPlacement* old = m_board.findPart(id);
    if (!old) return fail("Bauteil " + id + " nicht gefunden");
    const CatalogEntry* e = m_catalog.part(old->type);
    if (!e) return fail("unbekannter Bauteiltyp");
    PartPlacement moved = *old;
    std::string err;
    if (isSquarePlate(*e)) {
        // 2-x-2-Platine: als Ganzes verschieben, Drehung bleibt erhalten
        if (!translatePlate(moved, startField)) return fail(old->type + ": passt nicht auf die Grundplatte");
    } else {
        const bool flipped = isFlipped(*old, *e);
        if (!placePins(*e, startField, o, span, moved.pins, err)) return fail(err);
        if (flipped) flipPins(moved, *e); // Polung bleibt beim Verschieben und Drehen erhalten
    }
    if (!canPlace(m_board, m_catalog, moved, err, id)) return fail(err);
    if (moved.pins == old->pins) return true; // nichts zu tun
    Board after = m_board;
    *findPart(after, id) = moved;
    return push(QStringLiteral("%1 verschieben").arg(q(id)), std::move(after), BoardCommand::MergeMovePart, q(id));
}

bool LabSession::flipPart(const std::string& id) {
    Board after = m_board;
    PartPlacement* p = findPart(after, id);
    if (!p) return fail("Bauteil " + id + " nicht gefunden");
    const CatalogEntry* e = m_catalog.part(p->type);
    if (!e) return fail("unbekannter Bauteiltyp");
    flipPins(*p, *e);
    return push(QStringLiteral("%1 umdrehen").arg(q(id)), std::move(after));
}

bool LabSession::rotatePart(const std::string& id) {
    Board after = m_board;
    PartPlacement* p = findPart(after, id);
    if (!p) return fail("Bauteil " + id + " nicht gefunden");
    const CatalogEntry* e = m_catalog.part(p->type);
    if (!e || !isSquarePlate(*e)) return fail(id + ": nur Platinen lassen sich so drehen");
    rotatePlate(*p);
    return push(QStringLiteral("%1 drehen").arg(q(id)), std::move(after));
}

bool LabSession::removePart(const std::string& id) {
    Board after = m_board;
    const auto it = std::find_if(after.parts.begin(), after.parts.end(), [&id](const auto& p) { return p.id == id; });
    if (it == after.parts.end()) return fail("Bauteil " + id + " nicht gefunden");
    after.parts.erase(it);
    return push(QStringLiteral("%1 entfernen").arg(q(id)), std::move(after));
}

bool LabSession::setPartValue(const std::string& id, double value) {
    if (!(value > 0.0) || !std::isfinite(value)) return fail("ungueltiger Wert");
    Board after = m_board;
    PartPlacement* p = findPart(after, id);
    if (!p) return fail("Bauteil " + id + " nicht gefunden");
    if (p->value == value) return true;
    p->value = value;
    return push(QStringLiteral("Wert von %1 aendern").arg(q(id)), std::move(after), BoardCommand::MergePartValue,
                q(id));
}

bool LabSession::repairPart(const std::string& id) {
    if (!m_board.findPart(id)) return fail("Bauteil " + id + " nicht gefunden");
    // Reparieren = neues, kaltes, ungeladenes Bauteil an derselben Stelle (Befehl im Verlauf)
    return push(QStringLiteral("%1 reparieren").arg(q(id)), m_board, BoardCommand::NoMerge, {},
                {{id, core::ComponentState{}}});
}

bool LabSession::addWire(const std::string& from, const std::string& to, const std::string& color,
                         std::string* newId) {
    std::string err;
    if (!canWire(m_board, from, to, err)) return fail(err);
    Board after = m_board;
    WirePlacement w{nextWireId(m_board), from, to, color.empty() ? "rot" : color};
    if (newId) *newId = w.id;
    after.wires.push_back(w);
    return push(QStringLiteral("Leitung %1 ziehen").arg(q(w.id)), std::move(after));
}

bool LabSession::moveWireEnd(const std::string& id, bool fromEnd, const std::string& anchor) {
    Board after = m_board;
    WirePlacement* w = findWire(after, id);
    if (!w) return fail("Leitung " + id + " nicht gefunden");
    std::string& end = fromEnd ? w->from : w->to;
    if (end == anchor) return true;
    end = anchor;
    std::string err;
    if (!canWire(m_board, w->from, w->to, err)) return fail(err);
    return push(QStringLiteral("Leitung %1 umsetzen").arg(q(id)), std::move(after), BoardCommand::MergeMoveWireEnd,
                q(id + (fromEnd ? ":von" : ":nach")));
}

bool LabSession::removeWire(const std::string& id) {
    Board after = m_board;
    const auto it = std::find_if(after.wires.begin(), after.wires.end(), [&id](const auto& w) { return w.id == id; });
    if (it == after.wires.end()) return fail("Leitung " + id + " nicht gefunden");
    after.wires.erase(it);
    return push(QStringLiteral("Leitung %1 loeschen").arg(q(id)), std::move(after));
}

bool LabSession::setWireColor(const std::string& id, const std::string& color) {
    Board after = m_board;
    WirePlacement* w = findWire(after, id);
    if (!w) return fail("Leitung " + id + " nicht gefunden");
    if (w->color == color) return true;
    w->color = color;
    return push(QStringLiteral("Leitung %1 umfaerben").arg(q(id)), std::move(after));
}

bool LabSession::toggleVoltageDisplay(const std::string& field) {
    const AnchorKind k = classifyAnchor(field);
    if (k != AnchorKind::Field && k != AnchorKind::Desk) return fail("Spannungsanzeige nur an Feldern");
    if (!withinBoard(m_board, field)) return fail("Spannungsanzeige: Feld liegt ausserhalb der Grundplatte");
    Board after = m_board;
    auto& v = after.voltageDisplays;
    const auto it = std::find(v.begin(), v.end(), field);
    if (it == v.end()) v.push_back(field);
    else v.erase(it);
    return push(QStringLiteral("Spannungsanzeige %1").arg(q(field)), std::move(after));
}

// ------------------------------------------------------------------ Bedienhandlungen

void LabSession::setMainSwitch(bool on) {
    m_board.desk.mainSwitch = on;
    m_bridge.post({desk_id::kMainSwitch, sim::Action::Type::SetSwitch, on ? 1.0 : 0.0});
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
                return fail("Erst die Leitungen vom Messgeraet abziehen");
    }
    m_board.desk.meterVisible = visible;
    return true;
}

bool LabSession::replaceMeterFuse() {
    // neue Sicherung = Messgeraet im Grundzustand (Befehl im Verlauf, wie Reparieren)
    return push(QStringLiteral("Sicherung tauschen"), m_board, BoardCommand::NoMerge, {},
                {{desk_id::kMeter, core::ComponentState{}}});
}

bool LabSession::repairSpeaker() {
    return push(QStringLiteral("Lautsprecher reparieren"), m_board, BoardCommand::NoMerge, {},
                {{desk_id::kSpeaker, core::ComponentState{}}});
}

const core::ComponentReading* LabSession::deskReading(const std::string& deskId) const {
    const auto it = m_net.desk.find(deskId);
    if (!snapshotCurrent() || it == m_net.desk.end() || it->second >= m_snap.readings.size()) return nullptr;
    return &m_snap.readings[it->second];
}

} // namespace volta::model
