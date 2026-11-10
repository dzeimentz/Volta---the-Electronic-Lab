#pragma once

#include <string>
#include <vector>

namespace volta::model::formula {

// Formelrechner (Konzept 5.7, Werkzeug ab Stufe 3): rechnet mit Einheiten und Vorsaetzen. Ohne Qt, getestet.
// Die Oberflaeche zeigt zu jedem Ergebnis den Rechenweg (aus den hier gelieferten Zahlen).

// Zahl mit Vorsatz: »4,7k«, »470m«, »100µ« (auch »100u«), »2M2«, »4k7«, »0,5«, »12 V«, »10 kΩ«. Einheitenzeichen am Ende
// (V, A, Ω, Ohm, F, s, W, Hz) werden ignoriert; Komma gilt wie Punkt. ok = false bei ungueltiger Eingabe.
double parse(const std::string& text, bool& ok);

// Wert mit passendem Vorsatz und drei geltenden Stellen, Dezimalpunkt '.': 4700, "Ω" -> "4.7 kΩ"; 0.015, "A" ->
// "15 mA"; 1e-4, "F" -> "100 µF".
std::string format(double value, const std::string& unit);

// Gewoehnlicher Rechenmodus: + - * / (auch × ÷), Klammern, Vorzeichen, Zahlen mit Vorsatz. ok = false bei Fehlern
// (auch Division durch 0).
double evaluate(const std::string& expression, bool& ok);

// ---- Rechenblaetter
// Ohmsches Gesetz: genau eine der drei Groessen ist unbekannt (<= 0); sie wird berechnet. false, wenn nicht
// genau eine fehlt oder R/I unsinnig sind.
bool ohm(double& voltV, double& ohm, double& ampere);

struct LedResistor {
    double ohm = 0.0;     // berechneter Vorwiderstand (Ub - Uf) / I
    double normOhm = 0.0; // naechstgroesserer Normwert E12 (Strom bleibt darunter)
    double normA = 0.0;   // Strom mit dem Normwert
    double powerW = 0.0;  // Leistung im Normwiderstand
    bool tooHot = false;  // ueber 0,25 W (Belastbarkeit der Widerstaende im Kasten)
    bool ok = false;
};
LedResistor ledResistor(double supplyV, double forwardV, double ampere);

double series(const std::vector<double>& ohms);   // Summe; 0 bei leerer Liste
double parallel(const std::vector<double>& ohms); // 1 / Summe(1/R); 0 bei leerer Liste oder R <= 0

// Leistung aus zwei der drei Groessen U, I, R (die dritte <= 0)
bool power(double voltV, double ampere, double ohm, double& watt);

double divider(double inV, double r1Ohm, double r2Ohm); // Ua = Ue * R2 / (R1 + R2) (unbelastet)
double timeConstant(double ohm, double farad);          // tau = R * C
double chargeAt(double tauS, double t);                 // Anteil 0..1: 1 - e^(-t/tau)
// Blinkfrequenz einer symmetrischen astabilen Kippstufe: je Halbperiode ln(2) * R * C -> f = 1 / (2 ln2 R C)
double astableHz(double ohm, double farad);

// ---- Rechenblaetter ab Volta 2 (Konzept Volta 2, Kap. 7)
double cutoffHz(double ohm, double farad); // Grenzfrequenz eines RC-Glieds: f = 1 / (2 pi R C)

// Timer 555 astabil: Laden ueber R1 + R2, Entladen ueber R2 zwischen 1/3 und 2/3 der Versorgung
// tH = ln2 (R1 + R2) C, tL = ln2 R2 C, f = 1 / (tH + tL) ~ 1,44 / ((R1 + 2 R2) C)
struct Timer555 {
    double hz = 0.0;
    double highS = 0.0;
    double lowS = 0.0;
    double duty = 0.0; // Anteil, in dem der Ausgang oben ist (0..1)
    bool ok = false;
};
Timer555 timer555Astable(double r1Ohm, double r2Ohm, double farad);
double timer555MonoS(double ohm, double farad); // monostabil: t = ln3 R C ~ 1,1 R C

// Verstaerkung (Spannungsverhaeltnis Ua / Ue); 0 bei unsinnigen Werten
double gainTransistor(double rcOhm, double reOhm);  // Emitterschaltung mit RE: v ~ -RC / RE (Naeherung)
double gainInverting(double rfOhm, double rinOhm);  // OP invertierend: v = -Rf / Rin
double gainNonInverting(double rfOhm, double rgOhm); // OP nichtinvertierend: v = 1 + Rf / Rg

double rmsFromPeak(double peakV); // Sinus: Ueff = Us / Wurzel 2
// Brummspannung am Ladeelko (Naeherung): dU = I / (n f C), n = 1 Einweg-, 2 Brueckengleichrichter
double rippleV(double ampere, double farad, double hz, bool bridge);

// ab Volta 3
double thomsonHz(double henry, double farad); // Schwingkreis: f0 = 1 / (2 pi Wurzel(L C)); 0 bei L, C <= 0
struct Band {
    double bandwidthHz = 0.0; // B = fo - fu
    double centerHz = 0.0;    // f0 = Wurzel(fu fo) (geometrische Mitte)
    double quality = 0.0;     // Q = f0 / B
    bool ok = false;          // 0 < fu < fo
};
Band band(double lowHz, double highHz);
struct Efficiency {
    double inW = 0.0, outW = 0.0, lossW = 0.0;
    double eta = 0.0;          // Pab / Pzu
    bool ok = false;           // Pzu > 0, Pab >= 0
    bool impossible = false;   // Pab > Pzu: Messung pruefen
};
Efficiency efficiency(double inV, double inA, double outV, double outA);
bool decibel(double inV, double outV, double& db); // Pegel 20 lg(Ua / Ue); false bei Ue, Ua <= 0
double ratioFromDb(double db);                     // Ua / Ue = 10^(dB / 20)
struct Duty {
    double duty = 0.0; // Anteil 0..1 der Zeit oben
    double hz = 0.0, periodS = 0.0;
    bool ok = false;
};
Duty dutyCycle(double highS, double lowS);
double divideHz(double hz, double n); // Frequenzteiler f / n; 0 bei f <= 0 oder n < 1

} // namespace volta::model::formula
