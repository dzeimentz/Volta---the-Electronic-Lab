#pragma once

#include <array>
#include <cstddef>

#include "volta/core/component_model.hpp"
#include "volta/core/components/diode.hpp"

namespace volta::core {

// Kennwerte eines Fototransistors (ab Volta 2), angelehnt an einen 5-mm-NPN-Fototransistor ohne Tageslichtfilter
// (z. B. Datenblattwerte der Bauart SFH 309: Dunkelstrom unter 100 nA, Ic bis 15 mA, Ptot 165 mW). Die
// Lichtempfindlichkeit fuer sichtbares Licht ist ein Ausgangswert fuer die Kalibrierung (Phase 0).
struct PhototransistorParams {
    double luxSensitivityA = 0.5e-6; // Lichtstrom je lx (Zimmer 300 lx: 0,15 mA; Taschenlampe 5000 lx: 2,5 mA)
    double darkA = 10e-9;            // Dunkelstrom
    double kneeV = 0.15;             // Saettigung: Ic = IL * tanh(Uce / kneeV) * (1 + Uce / earlyV)
    double earlyV = 50.0;
    double inverseGain = 0.02;       // verkehrt herum (C und E vertauscht): 2 % des Stroms
    double imaxA = 0.02;             // Dauergrenzstrom
    ThermalParams thermal{450.0, 5.0 / 450.0, 398.0};
};

// Fototransistor (Katalogtyp `fototransistor`, im Optokoppler der Empfaenger): Anschluss 0 = Kollektor, 1 = Emitter,
// ohne Basisanschluss. Verhaltensmodell: Der Lichtstrom IL (Beleuchtung, Strahlung der zugeordneten IR-LEDs,
// Dunkelstrom) fliesst als Kollektorstrom, begrenzt durch die Saettigung bei kleiner Uce.
// Lichtquellen sind Bedienhandlungen bzw. Kopplungen: setIlluminance (lx, schon abgeschattet), setShade (Anteil der
// IR-Strahlung, die ankommt, 0..1), addEmitter (IR-LED mit Kopplungsfaktor: IL += k * I_LED). Der Strom der LED
// stammt aus dem letzten angenommenen Schritt und wird in beginSolve() festgeschrieben (Vertrag ComponentModel).
class Phototransistor : public ComponentModel {
public:
    static constexpr std::size_t kMaxEmitters = 8;

    Phototransistor(std::string id, NodeId collector, NodeId emitter, PhototransistorParams params = {},
                    double lux = 300.0);

    void setIlluminance(double lux) noexcept;
    void setShade(double share) noexcept;
    // Kopplung an eine IR-LED (Zeiger bleibt gueltig, solange die Schaltung besteht); false, wenn voll
    bool addEmitter(const Diode* emitter, double coupling) noexcept;
    double lightCurrentA() const noexcept { return m_lightA; }

    bool isNonlinear() const noexcept override { return true; }
    void beginSolve(const StampContext& ctx) noexcept override;
    bool linearize(const SolutionView& iterate) noexcept override;
    void resetLinearization(const SolutionView& accepted) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    // level: Kollektorstrom / 1 mA (Darstellung), displayValue: Lichtstrom in A
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    void evaluate(double vceV) noexcept;

    PhototransistorParams m_params;
    double m_lux = 300.0;
    double m_shade = 1.0;
    std::array<const Diode*, kMaxEmitters> m_emitters{};
    std::array<double, kMaxEmitters> m_coupling{};
    std::size_t m_emitterCount = 0;
    double m_lightA = 0.0; // Kennwert des laufenden Loesungsvorgangs
    double m_vLin = 0.0; // Linearisierungspunkt Uce
    double m_g = kJunctionGmin;
    double m_ieq = 0.0;
    double m_lastCurrentA = 0.0;
};

// Kennwerte eines Optokopplers (ab Volta 2), angelehnt an PC817: IR-LED 1,2 V bei 20 mA, Uebertragungsfaktor
// (CTR) 100 %, Ausgangstransistor bis 50 mA.
struct OptocouplerParams {
    DiodeParams led = DiodeParams::irLed(1.2);
    double ctr = 1.0;
    PhototransistorParams output{0.0, 10e-9, 0.15, 50.0, 0.02, 0.05, {450.0, 5.0 / 450.0, 398.0}};
};

// Optokoppler (Katalogtyp `optokoppler`, 2-x-2-Platine): Anschluesse 0 = Anode, 1 = Kathode (IR-LED),
// 2 = Kollektor, 3 = Emitter (Fototransistor). Beide Kreise sind elektrisch getrennt; der Ausgang folgt dem
// LED-Strom des letzten Schritts mit dem Uebertragungsfaktor. Zerstoert, sobald LED oder Transistor zerstoert ist.
class Optocoupler final : public ComponentModel {
public:
    Optocoupler(std::string id, NodeId anode, NodeId cathode, NodeId collector, NodeId emitter,
                OptocouplerParams params = {});

    const Diode& led() const noexcept { return m_led; }
    const Phototransistor& output() const noexcept { return m_out; }

    std::size_t internalNodeCount() const noexcept override { return m_led.internalNodeCount(); }
    bool isNonlinear() const noexcept override { return true; }
    void beginSolve(const StampContext& ctx) noexcept override;
    bool linearize(const SolutionView& iterate) noexcept override;
    void resetLinearization(const SolutionView& accepted) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void updateState(const SolutionView& x, double dtS) noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    // level: LED-Strom / 20 mA; terminalCurrentA je Anschluss
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    Diode m_led;
    Phototransistor m_out;
};

} // namespace volta::core
