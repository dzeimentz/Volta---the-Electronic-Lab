#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "volta/core/circuit.hpp"
#include "volta/core/signal_stats.hpp"
#include "volta/core/simulator.hpp"
#include "model/oscilloscope.hpp"

namespace volta::sim {

// Bedienhandlung, die zwischen zwei Simulationsschritten auf ein Bauteil wirkt (kein Topologiewechsel, kein Undo).
struct Action {
    enum class Type {
        SetSwitch,       // Schalter/Taster/Reedkontakt: value != 0 geschlossen; Wechselschalter: value != 0 Stellung B
        SetPotPosition,  // Potentiometer: value 0..1
        SetCells,        // Batterie: value = Zellenzahl
        SetMotorBlocked, // Motor: value != 0 blockiert
        SetMeterMode,    // Messgeraet: value = 0 V, 1 A, 2 Ohm
        SetMeterAnalog,  // Messgeraet: value != 0 analog
        SetIlluminance,  // Fotowiderstand, Fototransistor: value = Beleuchtungsstaerke in lx
        SetShade,        // Fototransistor: value = Anteil der IR-Strahlung, der ankommt (Abdeckkarte), 0..1
        SetAmbient,      // Heissleiter: value = Lufttemperatur am Bauteil in K
        SetWetness,      // Feuchtesonde: value = Naesse 0..1
        SetAcRms,        // Wechselquelle: value = Effektivwert in V
        SetGenWaveform,  // Funktionsgenerator: value = 0 Sinus, 1 Dreieck, 2 Rechteck
        SetGenFrequency, // Funktionsgenerator: value = Frequenz in Hz
        SetGenAmplitude, // Funktionsgenerator: value = Spitze in V
        SetGenOffset,    // Funktionsgenerator: value = Gleichanteil in V
        SetTracerMin,    // Kennlinienschreiber: value = kleinste Pruefspannung in V
        SetTracerMax,    // Kennlinienschreiber: value = hoechste Pruefspannung in V
        SetTracerOhm     // Kennlinienschreiber: value = Vorwiderstand in Ohm
    };
    std::string componentId;
    Type type = Type::SetSwitch;
    double value = 0.0;
};

// Zustandsvorgabe fuer ein Bauteil beim Topologiewechsel (Undo/Redo: Zustand zum Zeitpunkt des Befehls).
struct StateOverride {
    std::string componentId;
    core::ComponentState state;
};

// Beobachter, der im Simulations-Thread nach jedem Schritt laeuft (z. B. Erfolgspruefung eines Experiments).
// Er gehoert zu einer Topologie und wird mit ihr gewechselt; afterStep darf nicht blockieren.
class StepObserver {
public:
    virtual ~StepObserver() = default;
    virtual void afterStep(const core::Simulator& sim) = 0;
};

// Oszilloskop (ab Volta 2): Tastkoepfe (Bauteilkennungen, leer = Kanal aus), Abtastabstand, Groesse des
// Ringpuffers (0 = aus) und Schrittweite der Simulation (feiner bei kurzer Zeitbasis)
struct ScopeConfig {
    std::array<std::string, model::kScopeChannels> probeIds;
    double intervalS = 0.0;
    std::size_t capacity = 0;
    double stepS = core::kDefaultStepS;
    // Kennlinienschreiber: Bauteilkennung (leer = aus); je Abtastung Spannung und Strom an seinen Klemmen
    std::string tracerId;
    double tracerIntervalS = 0.0;
    std::size_t tracerCapacity = 0;
    int tracerEpoch = 0; // neue Einstellungen: Puffer leeren
};

// Schnappschuss fuer die Oberflaeche. Indizes wie in der Netzliste der Topologie-Version `topology`.
struct Snapshot {
    std::uint64_t topology = 0;
    double timeS = 0.0;
    bool valid = false;
    std::vector<double> nodeVolt;
    std::vector<core::ComponentReading> readings;
    std::vector<core::ComponentState> states; // fuer Befehle (Zustandsschnappschuss vor der Ausfuehrung)
    std::vector<core::SimEvent> events;       // seit dem letzten Abholen
    // Statistik des letzten abgeschlossenen Messfensters (kStatsWindowS): je Knoten die Spannung, je Bauteil
    // Spannung (2 i) und Strom (2 i + 1) wie in readings (Spannungsanzeige, Wechselgroessen der Messgeraete)
    std::vector<core::SignalSummary> nodeStats;
    std::vector<core::SignalSummary> readingStats;
    // Oszilloskop: Abtastwerte zeitlich geordnet (aelteste zuerst), je Kanal angeschlossen
    std::vector<model::ScopeSample> scope;
    std::array<bool, model::kScopeChannels> scopeHas{};
    // Kennlinienschreiber: v[0] = Spannung KS+ gegen KS-, v[1] = Strom aus KS+ (aelteste zuerst)
    std::vector<model::ScopeSample> tracer;
    core::Status lastStatus = core::Status::Ok;
    double realtimeRatio = 1.0;               // erreichte Simulationszeit je Wanduhrzeit (1 = Echtzeit)
};

// Entkoppelt Oberflaeche und Simulator (docs/architecture.md, Threads und Datenfluss).
// - Der Simulator gehoert exklusiv dem Simulations-Thread.
// - UI -> Simulation: neue Netzliste (die neueste gewinnt), Bedienhandlungen (Warteschlange), Zeitraffer, Pause.
// - Simulation -> UI: Schnappschuss im Doppelpuffer, mit ca. 120 Hz veroeffentlicht.
// - Sperren nur beim Austausch zwischen zwei Schritten, nie im Simulationsschritt.
// Ohne start() arbeitet die Bruecke synchron (runFor), etwa in Tests.
class SimulationBridge {
public:
    SimulationBridge();
    ~SimulationBridge();
    SimulationBridge(const SimulationBridge&) = delete;
    SimulationBridge& operator=(const SimulationBridge&) = delete;

    void start(); // Simulations-Thread starten (Echtzeit)
    void stop();
    bool running() const { return m_thread.joinable(); }

    // Neue Topologie; Zustaende werden aus der bisherigen Schaltung uebernommen (gleiche Bauteilkennung),
    // danach gelten die Vorgaben in overrides. Verdraengt sie eine noch nicht uebernommene Topologie, bleiben deren
    // Vorgaben erhalten (die neuen gehen vor). Liefert die Topologie-Version.
    // observer (optional) laeuft ab dieser Topologie nach jedem Schritt.
    std::uint64_t setCircuit(core::Circuit circuit, std::vector<StateOverride> overrides = {},
                             std::shared_ptr<StepObserver> observer = {});
    void post(Action action);
    void setTimeScale(double scale); // Zeitraffer 1, 10, 100
    void setPaused(bool paused);
    void setScope(const ScopeConfig& config); // Oszilloskop ein-/umstellen (auch Schrittweite)
    double stepS() const { return m_stepS; }   // aktuelle Schrittweite (nur im Simulations-Thread bzw. synchron)

    // Neuesten Schnappschuss holen (Kapazitaet von out wird wiederverwendet). false, wenn nichts Neues vorliegt.
    bool latest(Snapshot& out);

    // Synchron (nur ohne Thread): ausstehende Topologie und Aktionen anwenden, seconds simulieren, veroeffentlichen.
    void runFor(double seconds);

private:
    void loop();
    void applyPending();        // im Simulations-Thread
    void apply(const Action& a);
    void publish(double realtimeRatio);
    void step();                // ein Simulationsschritt samt Beobachter
    void resolveScope();        // Tastkoepfe in Bauteilindizes uebersetzen, Puffer leeren

    core::Simulator m_sim; // nur im Simulations-Thread (bzw. synchron im Aufrufer)
    core::SignalWindow m_nodeStats;    // nur im Simulations-Thread
    core::SignalWindow m_readingStats;
    std::vector<double> m_readingValues; // Arbeitsspeicher fuer m_readingStats
    std::unordered_map<std::string, std::size_t> m_index;
    std::uint64_t m_activeTopology = 0;
    std::shared_ptr<StepObserver> m_observer; // nur im Simulations-Thread
    // Oszilloskop (nur im Simulations-Thread): Ringpuffer, Kanalindizes in readings, letzter Abtastzeitpunkt
    ScopeConfig m_scope;
    std::vector<model::ScopeSample> m_scopeRing;
    std::size_t m_scopeHead = 0, m_scopeCount = 0;
    std::array<long, model::kScopeChannels> m_scopeIndex{-1, -1};
    double m_scopeLastT = -1.0;
    std::vector<model::ScopeSample> m_tracerRing; // Kennlinienschreiber
    std::size_t m_tracerHead = 0, m_tracerCount = 0;
    long m_tracerIndex = -1;
    double m_tracerLastT = -1.0;
    double m_stepS = core::kDefaultStepS;

    std::mutex m_inMutex; // Eingaenge
    std::unique_ptr<core::Circuit> m_pendingCircuit;
    std::vector<StateOverride> m_pendingOverrides;
    std::shared_ptr<StepObserver> m_pendingObserver;
    std::uint64_t m_pendingTopology = 0;
    std::uint64_t m_nextTopology = 0;
    std::vector<Action> m_actions;
    std::vector<Action> m_actionsWork;
    bool m_scopePending = false;
    ScopeConfig m_pendingScope;

    std::mutex m_outMutex; // Ausgang (Doppelpuffer)
    Snapshot m_front;
    Snapshot m_back;
    bool m_fresh = false;
    std::vector<core::SimEvent> m_eventBacklog;

    std::atomic<bool> m_run{false};
    std::atomic<double> m_timeScale{1.0};
    std::atomic<bool> m_paused{false};
    std::thread m_thread;
};

} // namespace volta::sim
