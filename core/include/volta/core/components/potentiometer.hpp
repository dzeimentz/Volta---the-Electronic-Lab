#pragma once

#include "volta/core/component_model.hpp"

namespace volta::core {

struct PotentiometerParams {
    double ohm = 10000.0; // Gesamtwiderstand (Pult-Drehregler: 10 kOhm linear, Ausgangswert)
    double pmaxW = 0.25;  // Belastbarkeit der ganzen Widerstandsbahn (Kohleschicht-Poti 1/4 W, Datenblattwert)
    double minOhm = 1.0;  // Restwiderstand je Teilbahn an den Anschlaegen (Schleiferkontakt)
    // Ein kurzes Bahnstueck gibt seine Waerme seitlich ueber Traeger und Gehaeuse ab und verkraftet daher mindestens
    // diesen Anteil von pmaxW (62,5 mW). Kalibrierung, keine Datenblattangabe: Datenblaetter begrenzen den Bahnstrom
    // streng auf sqrt(pmaxW / ohm) = 5 mA, praktisch uebersteht ein kurzes Bahnstueck einige 10 mW; so bleibt der
    // Dimmer mit 470 Ohm (Experiment 2.18, hoechstens ca. 27 mW) in jeder Stellung heil.
    double minShare = 0.25;
    // Thermik je Teilbahn wie beim Kohleschichtwiderstand 1/4 W (resistor.hpp): die ganze Bahn bei pmaxW ca. 85 K
    // ueber Umgebung (340 K/W), Zeitkonstante didaktisch 10 s, Bahn brennt bei ca. 230 Grad C durch, d. h. dauerhaft
    // ab ca. 2,5-facher Belastbarkeit. Ein Bahnstueck mit Anteil f hat Rth / f und Cth * f (gleiche Zeitkonstante).
    ThermalParams thermal{340.0, 10.0 / 340.0, 505.0};
};

// Potentiometer (Pult-Drehregler P1/P2, Katalogtyp `potentiometer`): Anschluesse 0 = a, 1 = Schleifer s, 2 = b.
// Zwei Teilwiderstaende a-s = position * R und s-b = (1 - position) * R, jeweils mindestens minOhm.
// Die Stellung (0..1, stufenlos) ist eine Bedienhandlung und wird in beginSolve() fuer den Loesungsvorgang
// festgeschrieben. Ueberlastet, wenn eine Teilbahn mehr als ihren Anteil an pmaxW umsetzt (mindestens minShare).
// Jede Teilbahn erwaermt sich und kuehlt ab (Thermik erster Ordnung); ueber der Grenztemperatur brennt die Bahn durch
// (danach offen). Die Temperaturen gehoeren zur Teilbahn a-s bzw. s-b, auch wenn sich die Stellung aendert
// (Vereinfachung). Zustand fuer Undo: beide Temperaturen; tempK ist die heissere.
class Potentiometer final : public ComponentModel {
public:
    Potentiometer(std::string id, NodeId a, NodeId wiper, NodeId b, PotentiometerParams params = {},
                  double position = 0.5);

    const PotentiometerParams& params() const noexcept { return m_params; }
    double position() const noexcept { return m_position; }
    void setPosition(double position) noexcept;
    // Teilwiderstaende fuer den laufenden Loesungsvorgang
    double ohmAS() const noexcept { return 1.0 / m_gAS; }
    double ohmSB() const noexcept { return 1.0 / m_gSB; }
    // Temperatur der Teilbahnen a-s und s-b
    double tempASK() const noexcept { return m_tempASK; }
    double tempSBK() const noexcept { return m_tempSBK; }

    void beginSolve(const StampContext& ctx) noexcept override;
    void stamp(MnaSystem& sys, const StampContext& ctx) const noexcept override;
    double terminalCurrent(const SolutionView& x, std::size_t t) const noexcept override;
    void thermal(const SolutionView& x, double dtS) noexcept override;
    void checkLimits(EventQueue& events, std::size_t index, double timeS) noexcept override;
    ComponentReading read(const SolutionView& x) const noexcept override;
    ComponentState saveState() const noexcept override;
    void restoreState(const ComponentState& state) noexcept override;

private:
    void applyPosition() noexcept;
    double share(double fraction) const noexcept;

    PotentiometerParams m_params;
    double m_position = 0.5;
    double m_gAS = 1.0 / 5000.0;
    double m_gSB = 1.0 / 5000.0;
    double m_lastPowerAS = 0.0;
    double m_lastPowerSB = 0.0;
    double m_tempASK = kAmbientK;
    double m_tempSBK = kAmbientK;
};

} // namespace volta::core
