#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

// Kennwerte der Wechselquelle im Pult (ab Volta 2, Konzept 5.1): Kleinspannungs-Transformator, Sekundaerwicklung
// potentialfrei. Fest 50 Hz (entschieden 01.11.2026). Innenwiderstand wie die Sekundaerwicklung eines kleinen
// Printtransformators (3 VA: typisch 1,5-3 Ohm); Nennstrom 0,33 A eff. (3 VA / 9 V).
struct AcSourceParams {
    double rmsV = 9.0;        // Effektivwert; das Pult bietet 6 und 9 V~
    double frequencyHz = 50.0;
    double innerOhm = 2.0;
    double ratedRmsA = 0.33;  // darueber ueberlastet (gleitender Effektivwert)
    double rmsWindowS = 0.2;  // Zeitkonstante des gleitenden Effektivwerts
};

// Wechselquelle (Pultfelder W~a, W~b): u(t) = sqrt(2) * U_eff * sin(2 pi f t) zwischen Anschluss 0 (a) und 1 (b),
// mit Innenwiderstand. Die Phase laeuft als eigener Zustand mit (in updateState fortgeschrieben, ueber
// Topologiewechsel hinweg stetig). Potentialfrei: den Ableitwiderstand nach 0V legt die App an (Netzliste).
// Ueberlastet, solange der gleitende Effektivwert des Stroms ueber dem Nennstrom liegt; keine Zerstoerung
// (Transformator mit Uebertemperatursicherung, die Fehlersuche erklaert den Kurzschluss).
class AcSource final : public ComponentModel {
public:
    AcSource(std::string id, NodeId a, NodeId b, AcSourceParams params = {});

    const AcSourceParams& params() const noexcept { return m_params; }
    void setRms(double rmsV) noexcept; // Bedienhandlung (6 / 9 V~)
    // Hauptschalter (Bedienhandlung): trennt die Primaerseite; die Sekundaerwicklung (Innenwiderstand) bleibt
    // zwischen den Anschluessen, die Quellenspannung ist dann null
    void setEnabled(bool on) noexcept { m_enabled = on; }
    bool isEnabled() const noexcept { return m_enabled; }
    double peakV() const noexcept;
    double phase() const noexcept { return m_phase; } // 0 .. 1
    // Quellenspannung bei der Phase p (ohne Innenwiderstand)
    double emfAt(double phase) const noexcept;
    double rmsCurrentA() const noexcept;

    std::size_t branchCount() const noexcept override { return 1; }
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    AcSourceParams m_params;
    bool m_enabled = true;
    double m_phase = 0.0;   // Phase am Ende des letzten angenommenen Schritts
    double m_meanSqA2 = 0.0; // gleitendes Mittel von i^2
};

enum class Waveform { Sine, Triangle, Square };

// Kennwerte des Funktionsgenerators (ab Volta 2, Konzept 5.2): 1 Hz ... 10 kHz, Amplitude 0 ... 4 V Spitze,
// Gleichanteil -4 ... +4 V; Ausgangswiderstand 50 Ohm wie bei Laborgeraeten.
struct GeneratorParams {
    Waveform waveform = Waveform::Sine;
    double frequencyHz = 100.0;
    double amplitudeV = 1.0;
    double offsetV = 0.0;
    double outputOhm = 50.0;

    static constexpr double kMinFrequencyHz = 1.0;
    static constexpr double kMaxFrequencyHz = 10000.0;
    static constexpr double kMaxAmplitudeV = 4.0;
    static constexpr double kMaxOffsetV = 4.0;
};

// Funktionsgenerator (Pultfelder G, G0; G0 legt die App an 0V): Anschluss 0 = Ausgang, Anschluss 1 = Masse.
// Sinus, Dreieck (beginnt bei 0 steigend wie der Sinus) und Rechteck (erste Halbperiode positiv). Die Phase laeuft
// als eigener Zustand, damit ein Frequenzwechsel keinen Sprung macht. Der Rechteckwechsel faellt auf das Ende des
// Zeitschritts, in dem er liegt (bei 20 us Schrittweite bis etwa 5 kHz sauber, Konzept 5.2).
class FunctionGenerator final : public ComponentModel {
public:
    FunctionGenerator(std::string id, NodeId out, NodeId ground, GeneratorParams params = {});

    const GeneratorParams& params() const noexcept { return m_params; }
    // Bedienhandlungen; Werte werden auf die Grenzen des Geraets begrenzt
    void setWaveform(Waveform w) noexcept { m_params.waveform = w; }
    void setFrequency(double hz) noexcept;
    void setAmplitude(double peakV) noexcept;
    void setOffset(double volt) noexcept;
    double phase() const noexcept { return m_phase; }
    // Leerlaufspannung bei der Phase p
    double emfAt(double phase) const noexcept;

    std::size_t branchCount() const noexcept override { return 1; }
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    GeneratorParams m_params;
    double m_phase = 0.0;
};

// Kennwerte des Kennlinienschreibers (ab Volta 2, Konzept 5.2): Pruefspannung -20 ... +20 V, Vorwiderstand zur
// Strombegrenzung 10 Ohm ... 100 kOhm, ein Durchlauf (hin und zurueck) in periodS.
struct TracerParams {
    double minV = -10.0;
    double maxV = 10.0;
    double seriesOhm = 500.0;
    double periodS = 2.0;

    static constexpr double kMaxV = 20.0;
    static constexpr double kMinSeriesOhm = 10.0;
    static constexpr double kMaxSeriesOhm = 100e3;
};

// Kennlinienschreiber (Pultfelder KS+, KS-): Anschluss 0 = KS+, Anschluss 1 = KS-. Die Leerlaufspannung faehrt als
// Dreieck von minV nach maxV und zurueck; der Vorwiderstand liegt im Zweig (Quelle mit Innenwiderstand) und begrenzt
// den Strom wie bei einem echten Kennlinienschreiber. Was der Schreiber zeichnet, ist die Messung an den Klemmen:
// read().voltV = Spannung zwischen KS+ und KS-, -read().currentA = Strom aus KS+ durch das Bauteil.
class CurveTracer final : public ComponentModel {
public:
    CurveTracer(std::string id, NodeId plus, NodeId minus, TracerParams params = {});

    const TracerParams& params() const noexcept { return m_params; }
    // Bedienhandlungen; Werte werden auf die Grenzen des Geraets begrenzt (min <= max)
    void setRange(double minV, double maxV) noexcept;
    void setSeriesOhm(double ohm) noexcept;
    double phase() const noexcept { return m_phase; }
    double emfAt(double phase) const noexcept; // Leerlaufspannung; Phase 0 = minV, 0,5 = maxV

    std::size_t branchCount() const noexcept override { return 1; }
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;

private:
    TracerParams m_params;
    double m_phase = 0.0;
};

} // namespace volta::core
