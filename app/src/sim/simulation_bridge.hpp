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
#include "volta/core/frequency_response.hpp"
#include "volta/core/signal_stats.hpp"
#include "volta/core/simulator.hpp"
#include "volta/core/components/radio.hpp"
#include "model/audio_stream.hpp"
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
        SetAmbient,      // Heissleiter, Diode (ab Volta 3): value = Lufttemperatur am Bauteil in K
        SetWetness,      // Feuchtesonde: value = Naesse 0..1
        SetLoad,         // Kraftsensor (ab Volta 3): value = Last in kg
        SetField,        // Hall-Sensor (ab Volta 3): value = Flussdichte in mT
        SetAcRms,        // Wechselquelle: value = Effektivwert in V
        SetGenWaveform,  // Funktionsgenerator: value = 0 Sinus, 1 Dreieck, 2 Rechteck
        SetGenFrequency, // Funktionsgenerator: value = Frequenz in Hz
        SetGenAmplitude, // Funktionsgenerator: value = Spitze in V
        SetGenOffset,    // Funktionsgenerator: value = Gleichanteil in V
        SetGenAmDepth,   // Funktionsgenerator (ab Volta 3): value = Tiefe der Amplitudenmodulation 0..1
        SetGenAmTone,    // Funktionsgenerator (ab Volta 3): value = Ton der Amplitudenmodulation in Hz
        SetGenSweepFactor, // Funktionsgenerator (ab Volta 3): value = Wobbelfaktor (1 = aus)
        SetGenSweepTime,   // Funktionsgenerator (ab Volta 3): value = Dauer eines Wobbeldurchlaufs in s
        SetTracerMin,    // Kennlinienschreiber: value = kleinste Pruefspannung in V
        SetTracerMax,    // Kennlinienschreiber: value = hoechste Pruefspannung in V
        SetTracerOhm,    // Kennlinienschreiber: value = Vorwiderstand in Ohm
        SetCapacitance,  // Drehkondensator (ab Volta 3): value = Kapazitaet in F; stimmt das Radio neu ab
        SetFmStations    // UKW-Modul (ab Volta 3): value = Bitmaske der UKW-Sender, die senden
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
    // Hoerbare Simulation: Strom des Bauteils audioId (Pult-Lautsprecher) in der Abtastrate des Tonstroms
    std::shared_ptr<model::AudioStream> audio;
    std::string audioId;
    // > 0: statt des Stroms klingt die Spannung des Bauteils, umgerechnet in A je V (Kristallhoerer, ab Volta 3)
    double audioVoltScale = 0.0;
    // Logikanalysator (ab Volta 3): Logik-Tastkoepfe je Kanal (leer = keiner), Abtastabstand und Puffergroesse
    std::array<std::string, model::kLogicChannels> logicIds;
    double logicIntervalS = 0.0;
    std::size_t logicCapacity = 0;
};

// Frequenzgang-Schreiber (ab Volta 3): Ergebnis eines Durchlaufs 1 Hz ... 10 MHz vom Generator zum Tastkopf
struct SweepData {
    std::uint64_t request = 0; // Nummer der Anforderung (requestSweep)
    std::uint64_t topology = 0;
    core::Status status = core::Status::NoCircuit;
    std::vector<double> freqsHz;
    core::FrequencyResponse response;
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
    // Logikanalysator (ab Volta 3): Spannungen der Logik-Tastkoepfe (aelteste zuerst), je Kanal angeschlossen
    std::vector<model::LogicSample> logic;
    std::array<bool, model::kLogicChannels> logicHas{};
    std::shared_ptr<const SweepData> sweep; // Frequenzgang-Schreiber: letzter Durchlauf (leer = keiner)
    core::Status lastStatus = core::Status::Ok;
    double realtimeRatio = 1.0;               // erreichte Simulationszeit je Wanduhrzeit (1 = Echtzeit)
    core::DetectorDiode::Drive radio;         // staerkster empfangener Sender (e0V = 0: keiner), fuer den Ersatzton
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
    // Sender der Senderanlage (ab Volta 3): nach jedem Topologiewechsel und jeder Aenderung wird neu abgestimmt
    // (core::tuneRadio, zwischen zwei Schritten)
    void setStations(std::vector<core::RadioStation> stations);
    // Frequenzgang-Schreiber (ab Volta 3): zwischen zwei Schritten einen Durchlauf von der Anregung excitationId
    // (Funktionsgenerator) zur Spannung am Tastkopf probeId gegen 0V rechnen; Ergebnis in Snapshot::sweep.
    // Liefert die Nummer der Anforderung.
    std::uint64_t requestSweep(std::string excitationId, std::string probeId);
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
    std::vector<model::LogicSample> m_logicRing; // Logikanalysator
    std::size_t m_logicHead = 0, m_logicCount = 0;
    std::array<long, model::kLogicChannels> m_logicIndex{-1, -1, -1, -1, -1, -1, -1, -1};
    double m_logicLastT = -1.0;
    std::vector<model::ScopeSample> m_tracerRing; // Kennlinienschreiber
    std::size_t m_tracerHead = 0, m_tracerCount = 0;
    long m_tracerIndex = -1;
    double m_tracerLastT = -1.0;
    long m_audioIndex = -1; // hoerbare Simulation: Lautsprecher, naechster Abtastzeitpunkt, letzter Schritt
    double m_audioNextT = -1.0, m_audioPrevT = 0.0, m_audioPrevA = 0.0;
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
    bool m_stationsPending = false;
    bool m_sweepPending = false;
    std::string m_sweepExcitation, m_sweepProbe;
    std::uint64_t m_sweepRequest = 0;
    std::shared_ptr<const SweepData> m_sweep; // im Simulations-Thread erzeugt, unveraendert weitergereicht
    std::vector<core::RadioStation> m_pendingStations;
    std::vector<core::RadioStation> m_stations; // im Simulations-Thread
    core::DetectorDiode::Drive m_radio;         // staerkster Sender nach dem letzten Abstimmen
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
