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

// Anzeigegeraet ab Volta 2 (Konzept 5.2): Handbereichswahl wie ein 3½-stelliges Vielfachmessgeraet, hoechstens
// 1999 Digits; darueber Ueberlauf »1«. Bereich -1 = selbsttaetig (formatMeter).
struct DisplayRange {
    double full;      // Bereichsendwert in SI, z. B. 0.2 fuer 200 mV
    double factor;    // Einheit der Anzeige in SI
    int decimals;     // Nachkommastellen
    const char* unit; // UTF-8
    const char* name; // Beschriftung des Bereichs
};

inline const DisplayRange* displayRanges(core::MeterMode mode, int& count) {
    static const DisplayRange volt[] = {{0.2, 1e-3, 1, "mV", "200 mV"}, {2.0, 1.0, 3, "V", "2 V"},
                                        {20.0, 1.0, 2, "V", "20 V"}};
    static const DisplayRange amp[] = {{2e-3, 1e-3, 3, "mA", "2 mA"}, {20e-3, 1e-3, 2, "mA", "20 mA"},
                                       {0.2, 1e-3, 1, "mA", "200 mA"}, {2.0, 1.0, 3, "A", "2 A"}};
    static const DisplayRange ohm[] = {{200.0, 1.0, 1, "Ω", "200 Ω"},    {2e3, 1e3, 3, "kΩ", "2 kΩ"},
                                       {20e3, 1e3, 2, "kΩ", "20 kΩ"},   {200e3, 1e3, 1, "kΩ", "200 kΩ"},
                                       {2e6, 1e6, 3, "MΩ", "2 MΩ"}};
    if (mode == core::MeterMode::Ampere) { count = 4; return amp; }
    if (mode == core::MeterMode::Ohm) { count = 5; return ohm; }
    count = 3;
    return volt;
}

struct DisplayText {
    std::string value;
    std::string unit;
    bool overrange = false;
};

// Anzeige im gewaehlten Bereich; open = Messkreis offen bzw. ueber dem groessten Bereich (Ohm)
inline DisplayText formatDisplay(core::MeterMode mode, int range, double value, bool open = false) {
    int count = 0;
    const DisplayRange* r = displayRanges(mode, count);
    if (range < 0 || range >= count) {
        if (open) return {"OL", "", true};
        const MeterText t = formatMeter(mode, value);
        return {t.value, t.unit, false};
    }
    const DisplayRange& d = r[range];
    const double scaled = value / d.factor;
    const double digits = std::round(std::fabs(scaled) * std::pow(10.0, d.decimals));
    if (open || digits > 1999.0) return {value < 0.0 && !open ? "-1" : "1", d.unit, true}; // Ueberlauf: nur »1«
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.*f", d.decimals, scaled);
    std::string text = buf;
    if (!text.empty() && text[0] == '-' && digits == 0.0) text.erase(0, 1);
    return {text, d.unit, false};
}

// Leistung aus Spannung und Strom: drei gueltige Stellen in µW, mW oder W
inline MeterText formatPower(double watts) {
    const double a = std::fabs(watts);
    const char* unit = a >= 0.9995 ? "W" : a >= 0.9995e-3 ? "mW" : "µW";
    const double scaled = watts / (a >= 0.9995 ? 1.0 : a >= 0.9995e-3 ? 1e-3 : 1e-6);
    const double s = std::fabs(scaled);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.*f", s < 9.995 ? 2 : (s < 99.95 ? 1 : 0), scaled);
    std::string text = buf;
    if (text == "-0.00") text.erase(0, 1);
    return {text, unit};
}

} // namespace volta::model
