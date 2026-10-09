#pragma once

#include <QIODevice>
#include <QObject>

#include <memory>
#include <mutex>
#include <vector>

#include "model/tone_synth.hpp"

class QAudioSink;

namespace volta::services {

// Tonausgabe ueber die Soundkarte (Klanganzeige, docs/ui-spec.md): Der LabController meldet je Bild die klingenden
// Quellen der Simulation (Pult-Lautsprecher, Summer) und Impulse; der Dienst erzeugt daraus mit model::ToneSynth
// einen Tonstrom (Mono, 16 Bit) fuer QAudioSink im Ziehbetrieb. Kein Netz, keine Dateien: nur selbst erzeugte
// Abtastwerte (Qt Multimedia, Windows-Backend; docs/adr/0009-tonausgabe-und-vorlesen.md).
// enabled: Einstellung »Ton« (AppSettings); active: Labor sichtbar (sonst schweigt die Ausgabe).
class AudioService : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY changed)
    Q_PROPERTY(bool available READ available CONSTANT) // Ausgabegeraet vorhanden

public:
    explicit AudioService(QObject* parent = nullptr);
    ~AudioService() override;

    bool enabled() const { return m_enabled; }
    void setEnabled(bool on);
    bool active() const { return m_active; }
    void setActive(bool on);
    bool available() const { return m_available; }
    // Entwicklerhilfen (Bildschirmfoto): ohne Ton, unabhaengig von den Einstellungen
    void setSuppressed(bool on) {
        m_suppressed = on;
        update();
    }

    // Aus dem LabController (GUI-Thread), etwa 60-mal je Sekunde
    void setVoices(const std::vector<model::SynthVoice>& voices);
    void click(double strength);

signals:
    void changed();

private:
    class Source;
    void update();

    bool m_enabled = true;
    bool m_active = true;
    bool m_available = false;
    bool m_suppressed = false;
    std::mutex m_mutex;
    std::unique_ptr<model::ToneSynth> m_synth;
    std::unique_ptr<Source> m_source;
    QAudioSink* m_sink = nullptr;
};

} // namespace volta::services
