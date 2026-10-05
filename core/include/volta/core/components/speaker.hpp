#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

struct SpeakerParams {
    double ohm = 8.0;        // Schwingspule
    double pmaxW = 0.5;      // Belastbarkeit
    // Schwingspule: Rth 120 K/W, Zeitkonstante didaktisch 5 s, zerstoert oberhalb ca. 180 Grad C (dauerhaft ab
    // ca. 3 * pmaxW)
    ThermalParams thermal{120.0, 5.0 / 120.0, 453.0};
    double fullScaleA = 0.1; // Wechselstrom (Effektivwert) fuer volle Lautstaerke der Klanganzeige
};

// Lautsprecher des Bedienpults (Pultfelder LS1, LS2): elektrisch ein ohmscher Widerstand (Schwingspule), dazu eine
// Auswertung des Stromverlaufs fuer die Klanganzeige (docs/ui-spec.md, Lautsprecher mit Leuchtring):
//  - Wechselanteil: Strom minus gleitender Mittelwert (tau 0,3 s). Reiner Gleichstrom bleibt stumm; Ein- und
//    Ausschalten erzeugen ein Knacken (ein Impuls), wie beim echten Lautsprecher.
//  - Tonhoehe aus den Abstaenden steigender Nulldurchgaenge (mit Hysterese) -> ComponentReading::frequencyHz;
//    nach drei ausbleibenden Perioden (mindestens 1,5 s) Stille (0).
//  - Signalstaerke: Effektivwert des Wechselanteils / fullScaleA -> level 0..1.
//  - Jeder steigende Nulldurchgang zaehlt als Impuls -> displayValue (Zaehler; die Anzeige blinkt unter 20 Hz je
//    Impuls kurz auf).
// Grenze: Mit der festen Schrittweite (1 ms) sind Tonhoehen nur bis etwa 250 Hz aufloesbar. Hoehere Toene liefern
// Verhaltensmodelle (Summer, Tonmodul) direkt als Frequenz.
class Speaker final : public ComponentModel {
public:
    Speaker(std::string id, NodeId a, NodeId b, SpeakerParams params = {});

    const SpeakerParams& params() const noexcept { return m_params; }
    double frequencyHz() const noexcept { return m_freqHz; }
    double pulses() const noexcept { return m_pulses; }

    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    double conductance() const noexcept { return m_destroyed ? kGmin : 1.0 / m_params.ohm; }
    double current(const SolutionView& x) const noexcept {
        return (x.voltage(terminal(0)) - x.voltage(terminal(1))) * conductance();
    }
    void resetTracking() noexcept;

    SpeakerParams m_params;
    double m_lastPowerW = 0.0;
    // Auswertung (fluechtig, nicht Teil des Undo-Zustands)
    double m_timeS = 0.0;
    double m_meanA = 0.0;   // gleitender Mittelwert (Gleichanteil)
    double m_peakA = 0.0;   // Huellkurve des Wechselanteils (fuer die Hysterese)
    double m_msA2 = 0.0;    // gleitendes Quadratmittel des Wechselanteils
    bool m_high = false;    // Wechselanteil zuletzt ueber der oberen Schwelle
    double m_lastEdgeS = -1.0;
    double m_periodS = 0.0; // geglaettete Periodendauer
    double m_freqHz = 0.0;
    double m_pulses = 0.0;
    bool m_started = false;
};

} // namespace volta::core
