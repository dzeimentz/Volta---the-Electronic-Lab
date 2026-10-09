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

} // namespace volta::model::formula
