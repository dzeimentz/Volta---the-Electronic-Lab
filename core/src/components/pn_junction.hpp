#pragma once

// Gemeinsame Hilfen fuer pn-Uebergaenge (Diode, Transistor). Nur intern im Kern verwendet.
#include <cmath>

namespace volta::core::pn {

// Argument der Exponentialfunktion, oberhalb dessen die Kennlinie Is*exp(x) linear fortgesetzt wird: dort, wo ihre
// Steigung Is*exp(x)/nVt den Leitwert gMax erreicht. Damit bleibt die Linearisierung bei begrenztem Leitwert
// konsistent (Ableitung = Steigung der Kennlinie), und das Newton-Verfahren konvergiert auch bei einem Uebergang
// direkt an der Batterie quadratisch. Hoechstens 80 (Schutz vor Ueberlauf).
inline double linearFromArg(double isA, double nVt, double gMax) noexcept {
    const double x = std::log(gMax * nVt / isA);
    return std::isfinite(x) && x < 80.0 ? x : 80.0;
}

// exp(x) mit linearer Fortsetzung oberhalb xMax; liefert auch die Ableitung.
inline void safeExp(double x, double xMax, double& value, double& deriv) noexcept {
    if (x > xMax) {
        const double e = std::exp(xMax);
        value = e * (1.0 + x - xMax);
        deriv = e;
    } else {
        value = std::exp(x);
        deriv = value;
    }
}

// Spannungsbegrenzung fuer pn-Uebergaenge nach SPICE3 (DEVpnjlim).
inline double pnjlim(double vnew, double vold, double nVt, double vcrit, bool& limited) noexcept {
    if (vnew > vcrit && std::fabs(vnew - vold) > 2.0 * nVt) {
        limited = true;
        if (vold > 0.0) {
            const double arg = 1.0 + (vnew - vold) / nVt;
            return arg > 0.0 ? vold + nVt * std::log(arg) : vcrit;
        }
        return nVt * std::log(vnew / nVt);
    }
    return vnew;
}

} // namespace volta::core::pn
