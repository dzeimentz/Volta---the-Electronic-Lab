#pragma once

#include <QObject>
#include <QTimer>

class QMediaDevices;

namespace volta::services {

// Klassenmodus »nur Kopfhoerer« (docs/platforms.md, klassenmodus_ton): erkennt, ob Kopfhoerer angeschlossen sind.
//  - Android: AudioManager.getDevices(Ausgaenge) mit Kabel-, USB- oder Bluetooth-Kopfhoerer (A2DP, BLE)
//  - Desktop: Name des Standard-Ausgabegeraets (Kopfhoerer/Headphone/Headset/Casque/Ecouteurs) - eine Naeherung
// Ueberprueft bei Geraetewechsel und alle 2 s (Android meldet das Einstecken nicht ueber Qt).
class AudioRoute : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool headphones READ headphones NOTIFY changed)

public:
    explicit AudioRoute(QObject* parent = nullptr);
    bool headphones() const { return m_headphones; }
    // Name des Ausgabegeraets als Kopfhoerer erkennen (Desktop; auch fuer Tests)
    static bool looksLikeHeadphones(const QString& deviceName);

signals:
    void changed();

private:
    void refresh();
    bool detect() const;

    QMediaDevices* m_devices = nullptr;
    QTimer m_poll;
    bool m_headphones = false;
};

} // namespace volta::services
