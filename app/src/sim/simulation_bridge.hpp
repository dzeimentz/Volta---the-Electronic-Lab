#pragma once

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
#include "volta/core/simulator.hpp"

namespace volta::sim {

// Bedienhandlung, die zwischen zwei Simulationsschritten auf ein Bauteil wirkt (kein Topologiewechsel, kein Undo).
struct Action {
    enum class Type {
        SetSwitch,       // Schalter/Taster/Reedkontakt: value != 0 geschlossen; Wechselschalter: value != 0 Stellung B
        SetPotPosition,  // Potentiometer: value 0..1
        SetCells,        // Batterie: value = Zellenzahl
        SetMotorBlocked, // Motor: value != 0 blockiert
        SetMeterMode,    // Messgeraet: value = 0 V, 1 A, 2 Ohm
        SetMeterAnalog   // Messgeraet: value != 0 analog
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

// Schnappschuss fuer die Oberflaeche. Indizes wie in der Netzliste der Topologie-Version `topology`.
struct Snapshot {
    std::uint64_t topology = 0;
    double timeS = 0.0;
    bool valid = false;
    std::vector<double> nodeVolt;
    std::vector<core::ComponentReading> readings;
    std::vector<core::ComponentState> states; // fuer Befehle (Zustandsschnappschuss vor der Ausfuehrung)
    std::vector<core::SimEvent> events;       // seit dem letzten Abholen
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
    // danach gelten die Vorgaben in overrides. Liefert die Topologie-Version.
    std::uint64_t setCircuit(core::Circuit circuit, std::vector<StateOverride> overrides = {});
    void post(Action action);
    void setTimeScale(double scale); // Zeitraffer 1, 10, 100
    void setPaused(bool paused);

    // Neuesten Schnappschuss holen (Kapazitaet von out wird wiederverwendet). false, wenn nichts Neues vorliegt.
    bool latest(Snapshot& out);

    // Synchron (nur ohne Thread): ausstehende Topologie und Aktionen anwenden, seconds simulieren, veroeffentlichen.
    void runFor(double seconds);

private:
    void loop();
    void applyPending();        // im Simulations-Thread
    void apply(const Action& a);
    void publish(double realtimeRatio);

    core::Simulator m_sim; // nur im Simulations-Thread (bzw. synchron im Aufrufer)
    std::unordered_map<std::string, std::size_t> m_index;
    std::uint64_t m_activeTopology = 0;

    std::mutex m_inMutex; // Eingaenge
    std::unique_ptr<core::Circuit> m_pendingCircuit;
    std::vector<StateOverride> m_pendingOverrides;
    std::uint64_t m_pendingTopology = 0;
    std::uint64_t m_nextTopology = 0;
    std::vector<Action> m_actions;
    std::vector<Action> m_actionsWork;

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
