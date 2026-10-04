#pragma once

#include "volta/core/components/diode.hpp"

namespace volta::core {

// Aktiver Summer mit eingebautem Tongeber, angelehnt an 9-V-Summer (ca. 25 mA bei 9 V, 2,4 kHz).
struct BuzzerParams {
    double resistanceOhm = 330.0; // Ersatzlast in Durchlassrichtung
    double frequencyHz = 2400.0;  // Katalog `f_Hz`
    double vMinV = 1.5;           // unterhalb schweigt der Summer
    double vFullV = 9.0;          // volle Lautstaerke
};

// Summer (Katalogtyp `summer`): gepolte Last. Elektrisch eine Diode mit grossem Bahnwiderstand (verpolt fliesst
// praktisch kein Strom), der Ton wird nicht im Netzwerk simuliert, sondern aus der anliegenden Spannung erzeugt
// (Verhaltensanteil, im Elternbereich offengelegt): level = Lautstaerke 0..1, frequencyHz = Tonfrequenz.
// Anschluss 0 = Plus, Anschluss 1 = Minus.
class Buzzer final : public Diode {
public:
    Buzzer(std::string id, NodeId plus, NodeId minus, BuzzerParams params = {});

    const BuzzerParams& buzzerParams() const noexcept { return m_buzzer; }
    ComponentReading read(const SolutionView& x) const noexcept override;

private:
    static DiodeParams electricalParams(const BuzzerParams& p) noexcept;
    BuzzerParams m_buzzer;
};

} // namespace volta::core
