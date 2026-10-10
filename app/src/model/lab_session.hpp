#pragma once

#include <QString>
#include <QUndoStack>

#include <atomic>
#include <cstdint>
#include <memory>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "board.hpp"
#include "board_edit.hpp"
#include "catalog.hpp"
#include "circuit_check.hpp"
#include "commands/board_command.hpp"
#include "netlist_builder.hpp"
#include "success.hpp"
#include "sim/simulation_bridge.hpp"

namespace volta::model {

// Eine Experimentsitzung ohne Oberflaeche: Aufbau, Netzliste, Simulation (SimulationBridge), Verlauf
// (Rueckgaengig/Wiederholen) und Bedienhandlungen. Die QML-Anbindung (LabController) ist eine duenne Schicht darueber;
// Tests treiben die Sitzung synchron (bridge().runFor).
//
// Jede Aufbauaenderung ist ein Befehl (BoardCommand) mit Zustandsschnappschuss aller Bauteile (ADR-0005).
// Bedienhandlungen (Hauptschalter, Taster, Regler, Zellenwahl) sind keine Befehle.
class LabSession : public BoardDocument {
public:
    explicit LabSession(Catalog catalog);
    ~LabSession() override;

    // Neuer Aufbau (z. B. Vorlage eines Experiments); der Verlauf wird geleert.
    void loadBoard(const Board& board);

    const Board& board() const { return m_board; }
    const Catalog& catalog() const { return m_catalog; }
    const Netlist& netlist() const { return m_net; }
    sim::SimulationBridge& bridge() { return m_bridge; }
    QUndoStack& undoStack() { return m_undo; }
    // Letzte Meldung einer abgelehnten Aenderung: deutscher Text bzw. Meldung zum Uebersetzen
    std::string lastError() const { return m_error.text(); }
    const Message& lastMessage() const { return m_error; }

    // ---- Simulation
    bool poll(); // neuesten Schnappschuss holen; true, wenn neu
    const sim::Snapshot& snapshot() const { return m_snap; }
    bool snapshotCurrent() const { return m_snap.valid && m_snap.topology == m_topology; }
    double voltage(const std::string& anchor) const;
    const core::ComponentReading* reading(const std::string& partId) const;
    // Spannungsanzeige 0-4 Balken mit Hysterese (docs/domain-model.md)
    int voltageBars(const std::string& anchor);

    // ---- Aufbauaenderungen (Befehle). false und lastError() bei ungueltiger Aenderung.
    bool addPart(const std::string& type, const std::string& startField, Orientation o, Span span,
                 std::string* newId = nullptr, double value = 0.0);
    bool movePart(const std::string& id, const std::string& startField, Orientation o, Span span);
    // Vorschau beim Ziehen aus dem Fach (kein Befehl): Anschluesse, die das Bauteil belegen wuerde (pins leer, wenn es
    // nicht auf die Grundplatte passt); true, wenn es dort auch gesteckt werden darf
    bool previewPart(const std::string& type, const std::string& startField, Orientation o, Span span,
                     std::vector<std::pair<std::string, std::string>>& pins) const;
    bool removePart(const std::string& id);
    bool flipPart(const std::string& id); // umdrehen (Polung tauschen)
    bool rotatePart(const std::string& id); // 2-x-2-Platine um 90 Grad im Uhrzeigersinn drehen
    bool setPartValue(const std::string& id, double value);
    bool repairPart(const std::string& id);
    bool addWire(const std::string& from, const std::string& to, const std::string& color,
                 std::string* newId = nullptr);
    // autoColor: Farbautomatik (poleColor), sofern die Farbe nicht von Hand gewaehlt wurde
    bool moveWireEnd(const std::string& id, bool fromEnd, const std::string& anchor, bool autoColor = false);
    bool removeWire(const std::string& id);
    bool setWireColor(const std::string& id, const std::string& color);
    bool toggleVoltageDisplay(const std::string& field);
    // Module in der Modulleiste (Modulfach). slot: erstes belegtes Feld M:<slot>. Leitungen an den Feldern unter dem
    // Modul wandern beim Verschieben mit; beim Entfernen bleiben sie auf den Feldern der Leiste liegen.
    bool addModule(const std::string& type, int slot, std::string* newId = nullptr);
    bool moveModule(const std::string& id, int slot);
    bool removeModule(const std::string& id);
    // Breite der Grundplatte (8, 10, 12 Spalten). Verkleinern nur, wenn die wegfallenden Spalten frei sind.
    bool setColumns(int columns);
    // Ganzer Aufbau auf einmal (z. B. Musteraufbau als Loesungshilfe); Pultstellung bleibt
    bool replaceBoard(const Board& after, const QString& text);

    // ---- Bedienhandlungen (kein Verlauf)
    void setMainSwitch(bool on);
    void setCells(int cells);
    void setPartClosed(const std::string& id, bool closed); // Schalter, Taster, Reedkontakt, Wechselschalter
    void setDeskSwitch(bool closed);
    void setPot(int which, double position); // Pult-Drehregler 1/2
    void setMotorBlocked(const std::string& id, bool blocked);
    // Umgebung (model/environment.hpp): Raumlicht 0 dunkel / 1 Daemmerung / 2 hell; Taschenlampe und Foehn legen,
    // verschieben, ein-/ausschalten, wegraeumen (je Werkzeug hoechstens eines). Bedienhandlungen.
    void setRoomLight(int level);
    void placeTool(const std::string& type, double col, double row);
    void setToolOn(const std::string& type, bool on);
    void removeTool(const std::string& type);
    // Dreifach-Messgeraet: Messart und Anzeigeart (Bedienhandlungen); Ein-/Ausblenden (nicht, solange Leitungen an
    // den Buchsen stecken); Sicherung tauschen ist ein Befehl (wie Reparieren).
    void setMeterMode(core::MeterMode mode);
    void setMeterAnalog(bool analog);
    bool setMeterVisible(bool visible);
    bool replaceMeterFuse();
    // Lautsprecher des Pults reparieren (Befehl, wie Reparieren eines Bauteils)
    bool repairSpeaker();
    // Messwert eines Pultbauteils (z. B. desk_id::kMeter) aus dem aktuellen Schnappschuss
    const core::ComponentReading* deskReading(const std::string& deskId) const;
    // Messwerte eines Moduls (Netlist::moduleMain) aus dem aktuellen Schnappschuss
    const core::ComponentReading* moduleReading(const std::string& moduleId) const;
    // Lautsprecher des Pults fuer die Klanganzeige. Treibt ein Tonmodul den Lautsprecher (derselbe Strom fliesst
    // durch Lautsprecher und Ausgang des Moduls, Abweichung hoechstens 20 %), uebernimmt die Anzeige dessen Ton:
    // Die Endstufe schaltet in Wirklichkeit im Tontakt, die Simulation rechnet mit dem Mittelwert (Verhaltensmodell).
    // Signalstaerke: Wechselanteil einer Rechteckschwingung mit 50 % Tastverhaeltnis = Mittelwert des Stroms.
    bool speakerReading(core::ComponentReading& out) const;
    // Fehlersuche: Auffaelligkeiten im Aufbau und in der laufenden Simulation (model/circuit_check)
    std::vector<CircuitFinding> checkCircuit() const;

    // ---- Erfolgspruefung (laeuft im Simulations-Thread nach jedem Schritt)
    // Bauteilbezuege der Bedingung beziehen sich auf die Vorlage; die Zuordnung zum Aufbau des Kindes erfolgt ueber
    // den Typ (docs/content-format.md, Erfolgsbedingungen): gleiche Kennung und gleicher Typ, sonst die Bauteile
    // dieses Typs in Reihenfolge. Jeder funktionierende Aufbau gilt.
    void setSuccess(const SuccessCondition& condition, const Board& templateBoard);
    void clearSuccess();
    bool successActive() const { return m_success != nullptr; }
    // Erfolg im aktuellen Aufbau erreicht (gilt bis zur naechsten Aufbauaenderung; Bedienhandlungen loeschen ihn nicht)
    bool successReached() const;
    // Alle Bauteile und Module der Vorlage stecken (Zuordnung ueber den Typ, Anzahl je Typ); Leitungen zaehlen nicht,
    // denn jeder funktionierende Aufbau gilt
    bool templateComplete() const;

    // Wird nach jeder Aenderung des Aufbaus aufgerufen (Oberflaeche aktualisieren)
    std::function<void()> onBoardChanged;

    // BoardDocument
    void applyBoard(const Board& board, const std::vector<sim::StateOverride>& states) override;

private:
    void rebuild(const std::vector<sim::StateOverride>& states);
    void postEnvironment(); // Licht und Waerme an alle Fotowiderstaende und Heissleiter
    std::vector<sim::StateOverride> captureStates() const;
    bool push(const QString& text, Board after, int mergeId = BoardCommand::NoMerge, const QString& key = {},
              std::vector<sim::StateOverride> statesAfter = {});
    bool fail(Message message);

    Catalog m_catalog;
    Board m_board;
    Netlist m_net;
    sim::SimulationBridge m_bridge;
    sim::Snapshot m_snap;
    std::uint64_t m_topology = 0;
    QUndoStack m_undo;
    Message m_error;
    std::unordered_map<std::string, int> m_bars;
    struct SuccessTracker;
    std::shared_ptr<SuccessTracker> m_success;
    Board m_successTemplate;
    // Bauteilkennungen der letzten Topologie-Versionen: Zustaende aus einem Schnappschuss einer aelteren Version
    // (Befehle ohne Simulationsfortschritt dazwischen) bleiben ueber die Kennung zuordenbar.
    std::deque<std::pair<std::uint64_t, std::vector<std::string>>> m_idHistory;
    // Zustandsvorgaben der juengsten Topologien, die noch in keinem Schnappschuss sichtbar sind (Topologie, Vorgaben)
    std::vector<std::pair<std::uint64_t, std::vector<sim::StateOverride>>> m_pendingStates;
};

} // namespace volta::model
