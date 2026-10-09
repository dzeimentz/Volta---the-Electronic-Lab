#pragma once

#include <array>
#include <string>

#include "volta/core/circuit.hpp"
#include "volta/core/component_model.hpp"
#include "volta/core/modules/blink_module.hpp"

namespace volta::core {

// Anschluesse des Tonmoduls in der Netzliste (Katalog: plus, minus, ein, aus).
struct ToneModulePins {
    NodeId plus = kGround;
    NodeId minus = kGround;
    NodeId in = kGround;  // ein: Spannung gegen minus ueber der Schaltschwelle startet die Melodie
    NodeId out = kGround; // aus: Endstufe mit offenem Kollektor nach minus (Lautsprecher von plus nach aus)
};

struct ToneModuleParams {
    double supplyOhm = 4700.0;  // Ruhestrom des Tonbausteins (ca. 2 mA bei 9 V)
    double inputOhm = 100000.0; // Eingangswiderstand ein -> minus
    double outputOhm = 100.0;   // Endstufe eingeschaltet, einschliesslich Schutzwiderstand
    double duty = 0.5;          // Tastverhaeltnis der Endstufe im Tontakt
    double vOnV = 1.5;          // Eingang aktiv ab (gegen minus)
    double vOffV = 1.0;         // Eingang inaktiv unter (Hysterese)
    double vSupplyMinV = 3.0;   // darunter schweigt der Baustein
    double noteS = 0.2;         // Dauer eines Tons der Melodie
};

// Tonmodul als Verhaltensmodell (docs/simulation-core.md, Module): Ein Ton im Hoerbereich ist mit der festen
// Schrittweite von 1 ms nicht als Schwingung simulierbar. Elektrisch ist das Modul deshalb ein Ersatzschaltbild aus
// Ruhestrom (plus -> minus), Eingangswiderstand (ein -> minus) und Endstufe (aus -> minus). Die Endstufe schaltet in
// Wirklichkeit im Tontakt; gerechnet wird mit dem zeitlichen Mittelwert (Leitwert * Tastverhaeltnis), in den Pausen
// der Melodie ist sie gesperrt. Den Ton selbst liefert das Modell als Frequenz: Solange der Eingang aktiv ist, spielt
// es eine kurze Melodie in Schleife.
// Diese Vereinfachung wird im Elternbereich und im »Blick hinein« offengelegt.
//
// Messwerte (ComponentReading): voltV = Versorgung (plus - minus), currentA = Strom in den Ausgang aus (Last),
// level = Lautstaerke 0..1 (aus der Versorgungsspannung), frequencyHz = aktueller Ton (0 in Pausen und still),
// displayValue = Nummer des Tons in der Melodie (0-basiert, -1 still).
// Anschluesse: 0 plus, 1 minus, 2 ein, 3 aus.
class ToneModule final : public ComponentModel {
public:
    static constexpr std::size_t kNotes = 8;
    // Eigene Tonfolge (aufsteigender und absteigender Dreiklang, dann eine Pause), Frequenzen in Hz
    static constexpr std::array<double, kNotes> kMelodyHz{523.25, 659.25, 783.99, 1046.5, 783.99, 659.25, 523.25, 0.0};

    ToneModule(std::string id, const ToneModulePins& pins, ToneModuleParams params = {});

    const ToneModuleParams& params() const noexcept { return m_params; }
    bool playing() const noexcept { return m_active; }
    int noteIndex() const noexcept { return m_active ? m_note : -1; }
    double frequencyHz() const noexcept { return m_active ? kMelodyHz[static_cast<std::size_t>(m_note)] : 0.0; }

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    double outputConductance() const noexcept;

    ToneModuleParams m_params;
    bool m_active = false;     // Kennwerte des laufenden Loesungsvorgangs (in beginSolve festgeschrieben)
    bool m_conducting = false; // Endstufe schaltet (spielt und keine Pause)
    bool m_pending = false;    // aus dem letzten angenommenen Zustand ermittelt
    double m_playS = 0.0;      // Spielzeit seit dem Start der Melodie
    int m_note = 0;
};

// Legt das Tonmodul mit der Kennung "<prefix>" an (ein Verhaltensmodell, ein Bauteil). Der Bereich dient dem
// »Blick hinein« wie bei den Teilschaltungen.
SubcircuitRange addToneModule(Circuit& circuit, const std::string& prefix, const ToneModulePins& pins);

} // namespace volta::core
