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
#include "sim/simulation_bridge.hpp"

namespace volta {

// Verbindet Aufbau (Board), Netzliste und Simulation mit der Oberflaeche (Lab.qml). Die Simulation laeuft in der
// SimulationBridge in einem eigenen Thread; der Controller holt je Bild den neuesten Schnappschuss und verwendet ihn
// nur, wenn er zur aktuellen Netzliste (Topologie-Version) passt.
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
    Q_PROPERTY(double timeScale READ timeScale WRITE setTimeScale NOTIFY deskChanged)
    Q_PROPERTY(double realtimeRatio READ realtimeRatio NOTIFY stateChanged)
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
    double simTime() const { return m_snap.timeS; }
    double timeScale() const { return m_timeScale; }
    void setTimeScale(double scale);
    double realtimeRatio() const { return m_snap.realtimeRatio; }
    double batteryVolt() const { return 1.5 * m_board.desk.cells; }

    Q_INVOKABLE bool loadExperiment(const QString& id);
    // Potential eines Anschlusspunkts gegen 0V (0, wenn unbenutzt)
    Q_INVOKABLE double voltage(const QString& anchor) const;
    // Spannungsanzeige: 0-4 Balken mit Hysterese (docs/domain-model.md, Spannungsanzeige)
    Q_INVOKABLE int voltageBars(const QString& anchor);
    // Zustand eines Bauteils: currentmA, level, overloaded, destroyed, tempC
    Q_INVOKABLE QVariantMap partState(const QString& id) const;

    const model::Netlist& netlist() const { return m_net; }

signals:
    void experimentChanged();
    void boardChanged();
    void deskChanged();
    void stateChanged();

private:
    void rebuild();
    void onFrame();
    bool snapshotCurrent() const { return m_snap.valid && m_snap.topology == m_topology; }

    services::ContentRepository m_repo;
    model::Catalog m_catalog;
    model::Board m_board;
    model::Netlist m_net;          // Zuordnungen der aktuellen Topologie (die Schaltung selbst gehoert der Bruecke)
    sim::SimulationBridge m_bridge;
    sim::Snapshot m_snap;
    std::uint64_t m_topology = 0;
    double m_timeScale = 1.0;
    QString m_title;
    QStringList m_voltageFields;
    QStringList m_errors;
    QHash<QString, int> m_bars; // zuletzt angezeigte Balkenzahl je Feld (Hysterese)
    QTimer m_timer;
    QElapsedTimer m_clock;
    double m_sinceNotifyS = 0.0;
};

} // namespace volta
