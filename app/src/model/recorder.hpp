#pragma once

#include <array>
#include <cstddef>
#include <deque>
#include <string>
#include <vector>

namespace volta::model {

// Zeitschreiber (Konzept 5.7, Werkzeug ab Stufe 3): zeichnet bis zu zwei Kanaele ueber die Simulationszeit auf - die
// Spannung an einem Kontaktfeld (gegen 0 V) oder die Anzeige des Messgeraets. Vorstufe zum Oszilloskop in Volta 2.
// Ohne Qt, damit testbar; der LabController fuettert ihn mit jedem neuen Schnappschuss.
// Zeit: Simulationszeit. Springt sie zurueck (neue Netzliste beginnt bei 0), laeuft die Aufzeichnung lueckenlos
// weiter (Versatz). Gespeichert wird nur das Zeitfenster (2 / 10 / 30 s) plus Reserve.
class Recorder {
public:
    static constexpr std::size_t kChannels = 2;
    struct Sample {
        double t = 0.0;
        std::array<double, kChannels> v{};
        std::array<bool, kChannels> has{};
    };
    struct Channel {
        std::string source; // Anschlusspunkt (z. B. "B4") oder "MESS" (Anzeige des Messgeraets); leer = aus
    };

    void setWindow(double seconds);
    double window() const { return m_windowS; }
    void setChannel(std::size_t ch, const std::string& source); // leert die Aufzeichnung dieses Kanals nicht
    const Channel& channel(std::size_t ch) const { return m_channels[ch]; }
    bool active() const;
    void setPaused(bool paused) { m_paused = paused; }
    bool paused() const { return m_paused; }
    void clear();

    // Neuer Messpunkt (Simulationszeit, Werte je Kanal; has = Wert vorhanden)
    void push(double simTimeS, const std::array<double, kChannels>& v, const std::array<bool, kChannels>& has);
    double now() const { return m_lastT; }
    const std::deque<Sample>& samples() const { return m_samples; }

    // Darstellung: hoechstens maxPoints Punkte je Kanal im Fenster [now - window, now], als (t relativ zum
    // Fensteranfang, Wert); Minimum und Maximum der Werte im Fenster (fuer die Skala)
    struct Series {
        std::vector<std::pair<double, double>> points;
        double min = 0.0, max = 0.0;
        bool any = false;
    };
    Series series(std::size_t ch, std::size_t maxPoints) const;

private:
    double m_windowS = 10.0;
    std::array<Channel, kChannels> m_channels{};
    bool m_paused = false;
    std::deque<Sample> m_samples;
    double m_offsetS = 0.0;  // Versatz bei Zeitspruengen der Simulation
    double m_lastRaw = -1.0; // zuletzt gesehene Simulationszeit
    double m_lastT = 0.0;    // fortlaufende Zeit der Aufzeichnung
};

} // namespace volta::model
