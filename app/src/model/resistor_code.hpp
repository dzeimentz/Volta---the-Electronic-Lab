#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

namespace volta::model {

// Farbcode von Widerstaenden nach IEC 60062 und E-Reihen nach IEC 60063 (Werkzeugkasten, Konzept 5.7).
// Farben als Index: 0 schwarz, 1 braun, 2 rot, 3 orange, 4 gelb, 5 gruen, 6 blau, 7 violett, 8 grau, 9 weiss,
// 10 gold, 11 silber. Ohne Qt, ohne Oberflaeche testbar.
namespace resistor_code {

inline constexpr int kGold = 10;
inline constexpr int kSilver = 11;

// Multiplikator-Ring: Zehnerpotenz je Farbe (gold -1, silber -2)
inline int multiplierExponent(int color) {
    if (color == kGold) return -1;
    if (color == kSilver) return -2;
    return color; // 0..9
}
inline int colorForExponent(int exp) {
    if (exp == -1) return kGold;
    if (exp == -2) return kSilver;
    return exp;
}
// Toleranz in Prozent je Farbe; 0 = keine Toleranzfarbe
inline double tolerancePercent(int color) {
    switch (color) {
    case 1: return 1.0;
    case 2: return 2.0;
    case 5: return 0.5;
    case 6: return 0.25;
    case 7: return 0.1;
    case 8: return 0.05;
    case kGold: return 5.0;
    case kSilver: return 10.0;
    default: return 0.0;
    }
}

struct Code {
    std::vector<int> rings; // Ziffern, Multiplikator, Toleranz (4 oder 5 Ringe)
    double ohm = 0.0;       // dargestellter Wert (auf die Ziffern gerundet)
    bool valid = false;
};

// Wert -> Ringe: 4 Ringe = 2 Ziffern (Toleranz gold 5 %), 5 Ringe = 3 Ziffern (Toleranz braun 1 %).
// Darstellbar sind 0,1 Ohm bis 99 GOhm (Multiplikator silber bis weiss).
inline Code encode(double ohm, int ringCount) {
    Code c;
    const int digits = ringCount == 5 ? 3 : 2;
    if (!(ohm > 0.0) || !std::isfinite(ohm)) return c;
    int exp = static_cast<int>(std::floor(std::log10(ohm))) - (digits - 1);
    long long n = std::llround(ohm / std::pow(10.0, exp));
    if (n >= static_cast<long long>(std::pow(10.0, digits))) { // Rundung auf 100 bzw. 1000
        n /= 10;
        ++exp;
    }
    if (exp < -2 || exp > 9) return c;
    std::vector<int> ds;
    for (int i = 0; i < digits; ++i) {
        ds.insert(ds.begin(), static_cast<int>(n % 10));
        n /= 10;
    }
    c.rings = ds;
    c.rings.push_back(colorForExponent(exp));
    c.rings.push_back(digits == 3 ? 1 : kGold);
    long long value = 0;
    for (int d : ds) value = value * 10 + d;
    c.ohm = static_cast<double>(value) * std::pow(10.0, exp);
    c.valid = true;
    return c;
}

// Ringe -> Wert (Ziffern, Multiplikator, Toleranz). 0, wenn ungueltig (z. B. gold als Ziffer).
inline double decode(const std::vector<int>& rings) {
    if (rings.size() != 4 && rings.size() != 5) return 0.0;
    const std::size_t digits = rings.size() - 2;
    long long value = 0;
    for (std::size_t i = 0; i < digits; ++i) {
        if (rings[i] < 0 || rings[i] > 9) return 0.0;
        value = value * 10 + rings[i];
    }
    const int m = rings[digits];
    if (m < 0 || m > kSilver) return 0.0;
    return static_cast<double>(value) * std::pow(10.0, multiplierExponent(m));
}

// E-Reihe (eine Dekade, 1,0 ... < 10): E6, E12, E24
inline std::vector<double> eSeries(int n) {
    static const std::vector<double> e6{1.0, 1.5, 2.2, 3.3, 4.7, 6.8};
    static const std::vector<double> e12{1.0, 1.2, 1.5, 1.8, 2.2, 2.7, 3.3, 3.9, 4.7, 5.6, 6.8, 8.2};
    static const std::vector<double> e24{1.0, 1.1, 1.2, 1.3, 1.5, 1.6, 1.8, 2.0, 2.2, 2.4, 2.7, 3.0,
                                         3.3, 3.6, 3.9, 4.3, 4.7, 5.1, 5.6, 6.2, 6.8, 7.5, 8.2, 9.1};
    return n == 6 ? e6 : (n == 24 ? e24 : e12);
}

struct Nearest {
    double value = 0.0;          // naechster Normwert (in Ohm, gleiche Dekade bzw. Nachbardekade)
    double deviationPercent = 0; // (eingegeben - Normwert) / Normwert * 100
    bool isMember = false;       // eingegebener Wert ist selbst ein Normwert
};

// Naechster Normwert der Reihe zu einem Wert (logarithmischer Abstand, auch ueber die Dekadengrenze)
inline Nearest nearest(double value, int n) {
    Nearest r;
    if (!(value > 0.0) || !std::isfinite(value)) return r;
    const double decade = std::pow(10.0, std::floor(std::log10(value)));
    std::vector<double> cand;
    for (double f : eSeries(n)) cand.push_back(f * decade);
    cand.push_back(10.0 * decade); // 1,0 der naechsten Dekade
    double best = cand.front();
    for (double v : cand)
        if (std::fabs(std::log(value / v)) < std::fabs(std::log(value / best))) best = v;
    r.value = best;
    r.deviationPercent = (value - best) / best * 100.0;
    r.isMember = std::fabs(r.deviationPercent) < 1e-6;
    return r;
}

// Eingabe wie »470«, »4,7k«, »4.7 kOhm«, »4k7«, »470R«, »2M2« -> Ohm; 0 bei ungueltiger Eingabe.
// Ohm-Zeichen und Leerzeichen werden ignoriert (UTF-8-Bytes ausserhalb ASCII), Komma gilt wie Punkt.
inline double parseOhm(const std::string& text) {
    std::string s;
    for (unsigned char ch : text)
        if (ch < 128 && !std::isspace(ch)) s += static_cast<char>(ch);
    if (s.empty()) return 0.0;
    // Ohm-Woerter am Ende entfernen
    for (const char* w : {"Ohm", "ohm", "OHM"})
        if (s.size() > 3 && s.compare(s.size() - 3, 3, w) == 0) s.erase(s.size() - 3);
    double factor = 1.0;
    std::string number;
    bool sawLetter = false;
    for (char ch : s) {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            number += ch;
        } else if (ch == ',' || ch == '.') {
            if (number.find('.') != std::string::npos) return 0.0;
            number += '.';
        } else if (!sawLetter && (ch == 'k' || ch == 'K' || ch == 'M' || ch == 'm' || ch == 'R' || ch == 'r')) {
            factor = (ch == 'k' || ch == 'K') ? 1e3 : (ch == 'M') ? 1e6 : (ch == 'm') ? 1e-3 : 1.0;
            sawLetter = true;
            // Schreibweise 4k7 / 2M2 / 4R7: Buchstabe als Dezimaltrennzeichen
            if (number.find('.') == std::string::npos) number += '.';
        } else {
            return 0.0;
        }
    }
    if (number.empty() || number == ".") return 0.0;
    if (number.back() == '.') number.pop_back();
    try {
        const double v = std::stod(number) * factor;
        return std::isfinite(v) && v > 0.0 ? v : 0.0;
    } catch (...) {
        return 0.0;
    }
}

} // namespace resistor_code
} // namespace volta::model
