#pragma once

#include <cmath>
#include <cstdio>
#include <string>

#include "volta/core/components/meter.hpp"

namespace volta::model {

// Anzeige des Dreifach-Messgeraets (docs/ui-spec.md): drei gueltige Stellen mit passendem Vorsatz
// (mV/V, uA/mA/A, Ohm/kOhm/MOhm). Dezimaltrennzeichen ist der Punkt; die Oberflaeche setzt das der Sprache ein.
struct MeterText {
    std::string value; // z. B. "4.50", "-12.3", "470"
    std::string unit;  // UTF-8, z. B. "V", "mA", "kOhm" als Zeichen
};

inline MeterText formatMeter(core::MeterMode mode, double value) {
    struct Prefix {
        double factor;
        const char* unit;
    };
    // UTF-8: µ = Mikro, Ω = Ohm
    static const Prefix volt[] = {{1e-3, "mV"}, {1.0, "V"}};
    static const Prefix amp[] = {{1e-6, "µA"}, {1e-3, "mA"}, {1.0, "A"}};
    static const Prefix ohm[] = {{1.0, "Ω"}, {1e3, "kΩ"}, {1e6, "MΩ"}};
    const Prefix* p = volt;
    int n = 2;
    int zeroIndex = 1; // Null: V bzw. mA bzw. Ohm
    if (mode == core::MeterMode::Ampere) {
        p = amp;
        n = 3;
        zeroIndex = 1;
    } else if (mode == core::MeterMode::Ohm) {
        p = ohm;
        n = 3;
        zeroIndex = 0;
    }
    const double a = std::fabs(value);
    int i = zeroIndex;
    if (a > 0.0) {
        i = 0;
        while (i + 1 < n && a >= p[i + 1].factor * 0.9995) ++i; // 999,7 mV -> 1,00 V
    }
    const double scaled = value / p[i].factor;
    const double s = std::fabs(scaled);
    const int decimals = s < 9.995 ? 2 : (s < 99.95 ? 1 : 0);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals, scaled);
    std::string text = buf;
    if (text == "-0.00" || text == "-0.0" || text == "-0") text.erase(0, 1);
    return {text, p[i].unit};
}

} // namespace volta::model
