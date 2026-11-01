#pragma once

#include <cstddef>
#include <vector>

namespace volta::core {

// Kennwerte eines Signals ueber ein abgeschlossenes Messfenster (Spannungsanzeige, Messgeraete, Tastkoepfe).
struct SignalSummary {
    bool valid = false;      // mindestens ein Fenster abgeschlossen
    double mean = 0.0;       // Gleichanteil (arithmetischer Mittelwert)
    double rms = 0.0;        // Effektivwert des ganzen Signals
    double acRms = 0.0;      // Effektivwert des Wechselanteils: sqrt(rms^2 - mean^2)
    double min = 0.0;
    double max = 0.0;
    double frequencyHz = 0.0; // aus steigenden Durchgaengen durch den Mittelwert des vorigen Fensters; 0 = keine
    // Wechselgroesse: der Wechselanteil ueberwiegt den Gleichanteil (Konzept Volta 2, 5.4)
    bool isAc() const noexcept { return valid && acRms > 1e-3 && acRms > (mean < 0.0 ? -mean : mean); }
};

// Fensterweise Statistik fuer mehrere Kanaele (z. B. alle Knotenspannungen). Jeder Schritt liefert einen Wert je
// Kanal mit seiner Schrittweite; nach windowS Sekunden wird das Fenster abgeschlossen und als latest() bereitgestellt
// (kein Flackern mit der Signalfrequenz). Die Frequenz zaehlt steigende Durchgaenge durch den Mittelwert des vorigen
// Fensters mit Hysterese (5 % der vorigen Spannweite), Zeitpunkte linear zwischen zwei Schritten interpoliert.
// Allokation nur in reset(); add() ist allokationsfrei (Simulations-Thread).
class SignalWindow {
public:
    void reset(std::size_t channels, double windowS);
    void add(const double* values, std::size_t count, double dtS) noexcept;
    const std::vector<SignalSummary>& latest() const noexcept { return m_latest; }
    std::size_t channels() const noexcept { return m_acc.size(); }

private:
    struct Acc {
        double sum = 0.0, sumSq = 0.0, min = 0.0, max = 0.0;
        double prev = 0.0;          // letzter Wert (Interpolation)
        bool havePrev = false;
        bool armed = false;         // unter der unteren Hysteresegrenze gewesen
        int rises = 0;
        double firstRiseS = 0.0, lastRiseS = 0.0;
    };
    double m_windowS = 0.1;
    double m_elapsedS = 0.0;
    std::vector<Acc> m_acc;
    std::vector<SignalSummary> m_latest;
};

} // namespace volta::core
