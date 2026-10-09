#include "formula.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "resistor_code.hpp"

namespace volta::model::formula {

namespace {

// Vorsatz nach UTF-8-Zeichen bzw. ASCII; 0 = kein Vorsatz
double prefixFactor(const std::string& s, std::size_t& i) {
    if (i >= s.size()) return 0.0;
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (c == 0xC2 && i + 1 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0xB5) { i += 2; return 1e-6; } // µ
    if (c == 0xCE && i + 1 < s.size() && static_cast<unsigned char>(s[i + 1]) == 0xBC) { i += 2; return 1e-6; } // μ
    switch (c) {
    case 'p': ++i; return 1e-12;
    case 'n': ++i; return 1e-9;
    case 'u': ++i; return 1e-6;
    case 'm': ++i; return 1e-3;
    case 'k': case 'K': ++i; return 1e3;
    case 'M': ++i; return 1e6;
    default: return 0.0;
    }
}

bool isDigit(char c) { return c >= '0' && c <= '9'; }

// Einheitenzeichen am Ende ueberspringen
void skipUnit(const std::string& s, std::size_t& i) {
    static const char* units[] = {"Ohm", "ohm", "\xCE\xA9", "\xE2\x84\xA6", "Hz", "V", "A", "F", "W", "s"};
    for (const char* u : units) {
        const std::string w(u);
        if (s.compare(i, w.size(), w) == 0) {
            i += w.size();
            return;
        }
    }
}

void skipSpace(const std::string& s, std::size_t& i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
}

// Zahl mit Vorsatz ab Position i (auch 4k7); false, wenn keine Zahl beginnt
bool number(const std::string& s, std::size_t& i, double& out) {
    std::string digits;
    bool dot = false;
    while (i < s.size() && (isDigit(s[i]) || s[i] == ',' || s[i] == '.')) {
        if (s[i] == ',' || s[i] == '.') {
            if (dot) return false;
            dot = true;
            digits += '.';
        } else {
            digits += s[i];
        }
        ++i;
    }
    if (digits.empty() || digits == ".") return false;
    skipSpace(s, i);
    std::size_t j = i;
    double factor = prefixFactor(s, j);
    if (factor != 0.0) {
        // 4k7: Ziffern direkt nach dem Vorsatz sind Nachkommastellen
        if (!dot && j < s.size() && isDigit(s[j])) {
            digits += '.';
            while (j < s.size() && isDigit(s[j])) digits += s[j++];
        }
        // »m« allein am Ende ohne Ziffern davor waere sinnlos; »mA« usw. sind Vorsatz + Einheit
        i = j;
    } else {
        factor = 1.0;
    }
    skipSpace(s, i);
    skipUnit(s, i);
    out = std::atof(digits.c_str()) * factor;
    return std::isfinite(out);
}

// Rekursiver Abstieg fuer den Rechenmodus
struct Parser {
    const std::string& s;
    std::size_t i = 0;
    bool ok = true;

    void space() { skipSpace(s, i); }
    bool eat(const char* tok) {
        space();
        const std::string t(tok);
        if (s.compare(i, t.size(), t) == 0) {
            i += t.size();
            return true;
        }
        return false;
    }
    double expr() {
        double v = term();
        for (;;) {
            if (eat("+")) v += term();
            else if (eat("-") || eat("\xE2\x88\x92")) v -= term(); // - und −
            else return v;
        }
    }
    double term() {
        double v = factor();
        for (;;) {
            if (eat("*") || eat("\xC3\x97") || eat("\xC2\xB7")) v *= factor(); // * × ·
            else if (eat("/") || eat("\xC3\xB7") || eat(":")) {               // / ÷ :
                const double d = factor();
                if (d == 0.0) ok = false;
                else v /= d;
            } else return v;
        }
    }
    double factor() {
        space();
        if (eat("-") || eat("\xE2\x88\x92")) return -factor();
        if (eat("+")) return factor();
        if (eat("(")) {
            const double v = expr();
            if (!eat(")")) ok = false;
            return v;
        }
        double v = 0.0;
        if (!number(s, i, v)) ok = false;
        return v;
    }
};

} // namespace

double parse(const std::string& text, bool& ok) {
    std::size_t i = 0;
    skipSpace(text, i);
    double v = 0.0;
    ok = number(text, i, v);
    skipSpace(text, i);
    if (i != text.size()) ok = false;
    return ok ? v : 0.0;
}

std::string format(double value, const std::string& unit) {
    if (!std::isfinite(value)) return "–";
    if (value == 0.0) return "0 " + unit;
    static const struct { double f; const char* p; } prefixes[] = {
        {1e6, "M"}, {1e3, "k"}, {1.0, ""}, {1e-3, "m"}, {1e-6, "\xC2\xB5"}, {1e-9, "n"}, {1e-12, "p"}};
    const double a = std::fabs(value);
    double f = 1e-12;
    const char* p = "p";
    for (const auto& x : prefixes)
        if (a >= x.f * 0.9995) { f = x.f; p = x.p; break; }
    const double x = value / f;
    const double ax = std::fabs(x);
    char buf[32];
    std::snprintf(buf, sizeof buf, ax >= 99.95 ? "%.0f" : ax >= 9.995 ? "%.1f" : "%.2f", x);
    std::string s(buf);
    if (s.find('.') != std::string::npos) { // Nullen am Ende weg
        while (s.back() == '0') s.pop_back();
        if (s.back() == '.') s.pop_back();
    }
    return s + " " + p + unit;
}

double evaluate(const std::string& expression, bool& ok) {
    Parser p{expression};
    const double v = p.expr();
    p.space();
    ok = p.ok && p.i == expression.size() && std::isfinite(v);
    return ok ? v : 0.0;
}

bool ohm(double& voltV, double& ohmR, double& ampere) {
    const int missing = (voltV <= 0.0) + (ohmR <= 0.0) + (ampere <= 0.0);
    if (missing != 1) return false;
    if (voltV <= 0.0) voltV = ohmR * ampere;
    else if (ohmR <= 0.0) ohmR = voltV / ampere;
    else ampere = voltV / ohmR;
    return std::isfinite(voltV) && std::isfinite(ohmR) && std::isfinite(ampere);
}

LedResistor ledResistor(double supplyV, double forwardV, double ampere) {
    LedResistor r;
    if (!(supplyV > forwardV) || !(ampere > 0.0)) return r;
    r.ohm = (supplyV - forwardV) / ampere;
    // naechstgroesserer E12-Wert: Strom bleibt hoechstens beim Wunschwert
    const double decade = std::pow(10.0, std::floor(std::log10(r.ohm)));
    double best = 10.0 * decade;
    for (double v : resistor_code::eSeries(12))
        if (v * decade >= r.ohm * (1.0 - 1e-9)) { best = v * decade; break; }
    r.normOhm = best;
    r.normA = (supplyV - forwardV) / r.normOhm;
    r.powerW = r.normA * r.normA * r.normOhm;
    r.tooHot = r.powerW > 0.25;
    r.ok = true;
    return r;
}

double series(const std::vector<double>& ohms) {
    double s = 0.0;
    for (double r : ohms)
        if (r > 0.0) s += r;
    return s;
}

double parallel(const std::vector<double>& ohms) {
    double g = 0.0;
    for (double r : ohms) {
        if (!(r > 0.0)) return 0.0;
        g += 1.0 / r;
    }
    return g > 0.0 ? 1.0 / g : 0.0;
}

bool power(double voltV, double ampere, double ohmR, double& watt) {
    if (voltV > 0.0 && ampere > 0.0) watt = voltV * ampere;
    else if (ohmR > 0.0 && ampere > 0.0) watt = ampere * ampere * ohmR;
    else if (voltV > 0.0 && ohmR > 0.0) watt = voltV * voltV / ohmR;
    else return false;
    return std::isfinite(watt);
}

double divider(double inV, double r1, double r2) {
    if (!(r1 >= 0.0) || !(r2 >= 0.0) || r1 + r2 <= 0.0) return 0.0;
    return inV * r2 / (r1 + r2);
}

double timeConstant(double ohmR, double farad) { return ohmR > 0.0 && farad > 0.0 ? ohmR * farad : 0.0; }

double chargeAt(double tauS, double t) { return tauS > 0.0 ? 1.0 - std::exp(-t / tauS) : 1.0; }

double astableHz(double ohmR, double farad) {
    const double tau = timeConstant(ohmR, farad);
    return tau > 0.0 ? 1.0 / (2.0 * std::log(2.0) * tau) : 0.0;
}

} // namespace volta::model::formula
