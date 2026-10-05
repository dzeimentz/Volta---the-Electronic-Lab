#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <memory>

#include "model/board.hpp"
#include "model/catalog.hpp"
#include "model/netlist_builder.hpp"
#include "services/content_repository.hpp"
#include "volta/core/simulator.hpp"

namespace volta {

// Verbindet Aufbau (Board), Netzliste und Simulator mit der Oberflaeche (Lab.qml).
// Phase-0-Geruest: Die Simulation laeuft im UI-Thread ueber einen Timer in Echtzeit (1-ms-Schritte, aufgeholt je
// Bild). Die Entkopplung ueber SimulationBridge in einem eigenen Thread (docs/architecture.md) folgt in Phase 1.
class LabController : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(LabController)
    QML_UNCREATABLE("Wird von main.cpp bereitgestellt")
    Q_PROPERTY(QString title READ title NOTIFY experimentChanged)
    Q_PROPERTY(QVariantList parts READ parts NOTIFY boardChanged)
    Q_PROPERTY(QVariantList wires READ wires NOTIFY boardChanged)
    Q_PROPERTY(QVariantList modules READ modules NOTIFY boardChanged)
    Q_PROPERTY(QStringList voltageFields READ voltageFields NOTIFY boardChanged)
    Q_PROPERTY(QStringList errors READ errors NOTIFY boardChanged)
    Q_PROPERTY(bool mainSwitch READ mainSwitch WRITE setMainSwitch NOTIFY deskChanged)
    Q_PROPERTY(double simTime READ simTime NOTIFY stateChanged)
    Q_PROPERTY(double batteryVolt READ batteryVolt NOTIFY deskChanged)

public:
    explicit LabController(QString contentRoot, QObject* parent = nullptr);

    QString title() const { return m_title; }
    QVariantList parts() const;
    QVariantList wires() const;
    QVariantList modules() const;
    QStringList voltageFields() const { return m_voltageFields; }
    QStringList errors() const { return m_errors; }
    bool mainSwitch() const { return m_board.desk.mainSwitch; }
    void setMainSwitch(bool on);
    double simTime() const { return m_sim.timeS(); }
    double batteryVolt() const { return 1.5 * m_board.desk.cells; }

    Q_INVOKABLE bool loadExperiment(const QString& id);
    // Potential eines Anschlusspunkts gegen 0V (0, wenn unbenutzt)
    Q_INVOKABLE double voltage(const QString& anchor) const;
    // Spannungsanzeige: 0-4 Balken mit Hysterese (docs/domain-model.md, Spannungsanzeige)
    Q_INVOKABLE int voltageBars(const QString& anchor);
    // Zustand eines Bauteils: currentmA, level, overloaded, destroyed, tempC
    Q_INVOKABLE QVariantMap partState(const QString& id) const;

    // Fuer Tests und Werkzeuge: Simulation um seconds vorruecken (ohne Echtzeitbindung)
    void advance(double seconds);
    const core::Simulator& simulator() const { return m_sim; }
    const model::Netlist& netlist() const { return m_net; }

signals:
    void experimentChanged();
    void boardChanged();
    void deskChanged();
    void stateChanged();

private:
    void rebuild();
    void onFrame();
    void applyDesk();

    services::ContentRepository m_repo;
    model::Catalog m_catalog;
    model::Board m_board;
    model::Netlist m_net;
    core::Simulator m_sim;
    QString m_title;
    QStringList m_voltageFields;
    QStringList m_errors;
    QHash<QString, int> m_bars; // zuletzt angezeigte Balkenzahl je Feld (Hysterese)
    QTimer m_timer;
    QElapsedTimer m_clock;
    double m_lagS = 0.0;
    double m_sinceNotifyS = 0.0;
};

} // namespace volta
