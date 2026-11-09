#pragma once

#include <array>
#include <string>

#include "volta/core/circuit.hpp"
#include "volta/core/component_model.hpp"
#include "volta/core/modules/blink_module.hpp"

namespace volta::core {

// Digitale Module der Stufe 8 (ab Volta 3), ereignisgesteuert wie das Gatter-Modul (gate_module.hpp): Eingaenge
// als Schmitt-Trigger (H ueber 0,6 U, L unter 0,4 U, gegen minus), Zustand je Loesungsvorgang fest, Ereignisortung
// an den Schwellen; Ausgaenge als Leitwert nach +U bzw. -U. Kennwerte angelehnt an CMOS-Zaehler (4520, 4017) und
// den Decoder 4511 bei 9 V.
struct LogicModuleParams {
    double upperFraction = 0.6;
    double lowerFraction = 0.4;
    double outOhm = 200.0;
    double minSupplyV = 3.0;
};

// Gemeinsamer Teil: Versorgung (Anschluss 0 plus, 1 minus) und bis zu vier Schmitt-Eingaenge ab Anschluss
// firstInput. Abgeleitete Modelle werten die Eingaenge in onInputs() aus.
class LogicModelBase : public ComponentModel {
public:
    LogicModelBase(std::string id, std::initializer_list<NodeId> terminals, std::size_t firstInput,
                   std::size_t inputCount, LogicModuleParams params);
    bool powered() const noexcept { return m_powered; }
    bool inputHigh(std::size_t k) const noexcept { return k < 4 && m_in[k]; }

    void beginSolve(const StampContext& ctx) noexcept override;
    double eventFraction(const SolutionView& accepted, const SolutionView& next, double dtS) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;

protected:
    double rel(const SolutionView& x, std::size_t t) const noexcept;
    // Ausgang an Anschluss t: H = Leitwert nach plus, L = nach minus (nur versorgt)
    void stampOutput(MnaSystem& sys, std::size_t t, bool high) const noexcept;
    double outputCurrent(const SolutionView& x, std::size_t t, bool high) const noexcept; // aus dem Ausgang heraus
    // Nach dem Fortschreiben der Eingaenge (prev = Zustand davor); unversorgt: onPowerLost()
    virtual void onInputs(const std::array<bool, 4>& prev) noexcept = 0;
    virtual void onPowerLost() noexcept {}
    LogicModuleParams m_params;
    std::array<bool, 4> m_in{false, false, false, false};
    std::size_t m_firstInput = 2;
    std::size_t m_inputCount = 0;
    bool m_powered = false;
};

// 4-Bit-Binaerzaehler (Katalogtyp `zaehlermodul`): Anschluesse 0 plus, 1 minus, 2 takt, 3 reset, 4-7 Q0-Q3.
// Zaehlt bei jeder steigenden Flanke an takt (0 ... 15, dann wieder 0); reset H setzt auf 0 und haelt dort.
// Messwerte: voltV = Versorgung, displayValue = Zaehlerstand, level = Q0.
class BinaryCounter final : public LogicModelBase {
public:
    BinaryCounter(std::string id, NodeId plus, NodeId minus, NodeId clock, NodeId reset, std::array<NodeId, 4> q,
                  LogicModuleParams params = {});
    int count() const noexcept { return m_count; }

    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    void onInputs(const std::array<bool, 4>& prev) noexcept override;
    void onPowerLost() noexcept override { m_count = 0; }
    bool bit(std::size_t k) const noexcept { return ((m_count >> k) & 1) != 0; }
    int m_count = 0;
};

// Dekadenzaehler (Katalogtyp `dekadenzaehler`, wie 4017): genau ein Ausgang Q0 ... Q9 ist H; jede steigende Flanke
// an takt schiebt ihn weiter, reset H setzt auf Q0. Das Hauptbauteil hat Anschluesse 0 plus, 1 minus, 2 takt,
// 3 reset, 4-7 Q0-Q3; Q4-Q9 sind eigene Ausgangsbauteile (DecadeOutput), die den Zaehlerstand lesen.
class DecadeCounter final : public LogicModelBase {
public:
    DecadeCounter(std::string id, NodeId plus, NodeId minus, NodeId clock, NodeId reset, std::array<NodeId, 4> q,
                  LogicModuleParams params = {});
    int count() const noexcept { return m_count; }
    const LogicModuleParams& params() const noexcept { return m_params; }

    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    void onInputs(const std::array<bool, 4>& prev) noexcept override;
    void onPowerLost() noexcept override { m_count = 0; }
    int m_count = 0;
};

// Ausgang Qk (k = 4 ... 9) des Dekadenzaehlers: Anschluesse 0 plus, 1 minus, 2 aus; H, solange der Zaehler bei k
// steht (der Zaehler liegt im Circuit vor diesem Bauteil und lebt so lange wie es).
class DecadeOutput final : public ComponentModel {
public:
    DecadeOutput(std::string id, NodeId plus, NodeId minus, NodeId out, const DecadeCounter& counter, int index);

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    const DecadeCounter* m_counter;
    int m_index;
    bool m_high = false;
    bool m_powered = false;
};

// Anzeige-Modul (Katalogtyp `anzeigemodul`): Decoder (BCD -> Siebensegment, wie 4511: ueber 9 dunkel) mit
// Siebensegmentanzeige (gemeinsame Kathode) und eingebauten Vorwiderstaenden auf einer Traegerplatte.
// Anschluesse 0 plus, 1 minus, 2-5 A, B, C, D (Wertigkeit 1, 2, 4, 8). Jedes leuchtende Segment zieht
// (U - segmentV) / segmentOhm aus plus. Messwerte: displayValue = angezeigte Ziffer (-1 dunkel),
// displayFlags = Segmente a ... g als Bits 0 ... 6, level = Anteil leuchtender Segmente, currentA = Strom.
struct DisplayModuleParams {
    double segmentV = 1.9;     // Flussspannung einer roten Segment-LED
    double segmentOhm = 680.0; // eingebauter Vorwiderstand je Segment
};

class SevenSegmentModule final : public LogicModelBase {
public:
    SevenSegmentModule(std::string id, NodeId plus, NodeId minus, std::array<NodeId, 4> bcd,
                       DisplayModuleParams display = {}, LogicModuleParams params = {});
    int digit() const noexcept { return m_digit; }
    int segments() const noexcept { return m_segments; }

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    void onInputs(const std::array<bool, 4>& prev) noexcept override;
    void onPowerLost() noexcept override { decode(); }
    void decode() noexcept;
    DisplayModuleParams m_display;
    int m_digit = 0;
    int m_segments = 0;
    double m_segmentA = 0.0; // Strom je Segment, je Loesungsvorgang fest
};

// Bitmuster der Segmente a ... g (Bit 0 ... 6) fuer die Ziffern 0 ... 9
int sevenSegmentPattern(int digit) noexcept;

struct CounterModulePins {
    NodeId plus = kGround, minus = kGround, clock = kGround, reset = kGround;
    std::array<NodeId, 10> q{}; // Binaerzaehler: nur Q0-Q3
};

SubcircuitRange addBinaryCounterModule(Circuit& circuit, const std::string& prefix, const CounterModulePins& pins);
SubcircuitRange addDecadeCounterModule(Circuit& circuit, const std::string& prefix, const CounterModulePins& pins);
SubcircuitRange addDisplayModule(Circuit& circuit, const std::string& prefix, NodeId plus, NodeId minus,
                                 const std::array<NodeId, 4>& bcd);

} // namespace volta::core
