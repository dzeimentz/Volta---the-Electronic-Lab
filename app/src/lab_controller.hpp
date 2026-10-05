#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <memory>

#include "model/experiment_flow.hpp"
#include "model/lab_session.hpp"
#include "services/content_repository.hpp"

namespace volta {

// QML-Anbindung einer Experimentsitzung (model::LabSession): Aufbau, Anzeigen, Befehle mit Rueckgaengig/Wiederholen
// und Bedienhandlungen. Keine Fachlogik: alles Weitere liegt in LabSession (ohne Oberflaeche testbar).
class LabController : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(LabController)
    QML_UNCREATABLE("Wird von main.cpp bereitgestellt")
    Q_PROPERTY(QString title READ title NOTIFY experimentChanged)
    Q_PROPERTY(QVariantList parts READ parts NOTIFY boardChanged)
    Q_PROPERTY(QVariantList wires READ wires NOTIFY boardChanged)
    Q_PROPERTY(QVariantList modules READ modules NOTIFY boardChanged)
    Q_PROPERTY(QStringList voltageFields READ voltageFields NOTIFY boardChanged)
    Q_PROPERTY(int boardColumns READ boardColumns NOTIFY boardChanged) // 8, 10 oder 12 Felder je Reihe
    Q_PROPERTY(QVariantList catalogParts READ catalogParts CONSTANT)
    Q_PROPERTY(QStringList errors READ errors NOTIFY boardChanged)
    Q_PROPERTY(QString lastEditError READ lastEditError NOTIFY editFailed)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(QString undoText READ undoText NOTIFY historyChanged)
    Q_PROPERTY(QString redoText READ redoText NOTIFY historyChanged)
    Q_PROPERTY(bool mainSwitch READ mainSwitch WRITE setMainSwitch NOTIFY deskChanged)
    Q_PROPERTY(int cells READ cells WRITE setCells NOTIFY deskChanged)
    Q_PROPERTY(double pot1 READ pot1 WRITE setPot1 NOTIFY deskChanged)
    Q_PROPERTY(double pot2 READ pot2 WRITE setPot2 NOTIFY deskChanged)
    Q_PROPERTY(bool deskSwitch READ deskSwitch WRITE setDeskSwitch NOTIFY deskChanged)
    Q_PROPERTY(double simTime READ simTime NOTIFY stateChanged)
    Q_PROPERTY(double timeScale READ timeScale WRITE setTimeScale NOTIFY deskChanged)
    Q_PROPERTY(double realtimeRatio READ realtimeRatio NOTIFY stateChanged)
    Q_PROPERTY(double batteryVolt READ batteryVolt NOTIFY deskChanged)
    // Gefuehrtes Experiment (Phase 1: Frage -> Aufbau -> Erfolg)
    Q_PROPERTY(QVariantList experiments READ experiments CONSTANT)
    Q_PROPERTY(QString experimentId READ experimentId NOTIFY experimentChanged)
    Q_PROPERTY(int phase READ phase NOTIFY flowChanged) // 0 freies Labor, 1 Frage, 2 Aufbau, 3 Erfolg
    Q_PROPERTY(bool destructive READ destructive NOTIFY experimentChanged)
    Q_PROPERTY(QString question READ question NOTIFY experimentChanged)
    Q_PROPERTY(QStringList guesses READ guesses NOTIFY experimentChanged)
    Q_PROPERTY(int guess READ guess NOTIFY flowChanged)
    Q_PROPERTY(QStringList steps READ steps NOTIFY experimentChanged)
    Q_PROPERTY(QStringList hints READ hints NOTIFY flowChanged) // bisher aufgedeckte Hilfen
    Q_PROPERTY(bool moreHints READ moreHints NOTIFY flowChanged)
    Q_PROPERTY(bool solutionShown READ solutionShown NOTIFY flowChanged)
    Q_PROPERTY(QString explanation READ explanation NOTIFY experimentChanged)
    Q_PROPERTY(QString moreKnowledge READ moreKnowledge NOTIFY experimentChanged)
    Q_PROPERTY(QStringList further READ further NOTIFY experimentChanged)
    Q_PROPERTY(QString realLife READ realLife NOTIFY experimentChanged)
    Q_PROPERTY(QString nextExperimentId READ nextExperimentId NOTIFY experimentChanged)

public:
    explicit LabController(QString contentRoot, QObject* parent = nullptr);
    ~LabController() override;

    QString title() const { return m_title; }
    QVariantList parts() const;
    QVariantList wires() const;
    QVariantList modules() const;
    QStringList voltageFields() const;
    QVariantList catalogParts() const;
    QStringList errors() const { return m_errors; }
    QString lastEditError() const { return m_lastEditError; }
    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;
    bool mainSwitch() const;
    void setMainSwitch(bool on);
    int cells() const;
    void setCells(int cells);
    double pot1() const { return m_session->board().desk.p1; }
    void setPot1(double v) { setPot(1, v); }
    double pot2() const { return m_session->board().desk.p2; }
    void setPot2(double v) { setPot(2, v); }
    bool deskSwitch() const { return m_session->board().desk.switchClosed; }
    double simTime() const;
    double timeScale() const { return m_timeScale; }
    void setTimeScale(double scale);
    double realtimeRatio() const;
    double batteryVolt() const { return 1.5 * cells(); }

    // Vorlage eines Experiments als Aufbau laden (ohne Ablauf; Entwicklerhilfen, Tests)
    Q_INVOKABLE bool loadExperiment(const QString& id);
    // Gefuehrtes Experiment: leere Grundplatte, Pultstellung der Vorlage, Ablauf beginnt mit der Frage
    Q_INVOKABLE bool startExperiment(const QString& id);
    Q_INVOKABLE void startFreeLab();
    Q_INVOKABLE void chooseGuess(int index);
    Q_INVOKABLE void nextHint();
    Q_INVOKABLE bool showTemplate(); // Musteraufbau (Befehl, rueckgaengig machbar) - erst nach der Loesungshilfe

    QVariantList experiments() const { return m_experimentList; }
    QString experimentId() const { return m_experiment.id; }
    int phase() const { return static_cast<int>(m_flow.phase()); }
    bool destructive() const { return m_experiment.destructive; }
    QString question() const { return m_repo.text(m_experiment.questionKey); }
    QStringList guesses() const { return texts(m_experiment.guessKeys); }
    int guess() const { return m_flow.guess(); }
    QStringList steps() const { return texts(m_experiment.stepKeys); }
    QStringList hints() const { return texts(m_experiment.hintKeys.mid(0, m_flow.hintsShown())); }
    bool moreHints() const { return m_flow.hintsShown() < m_flow.hintCount(); }
    bool solutionShown() const { return m_flow.solutionShown(); }
    QString explanation() const { return m_repo.text(m_experiment.explanationKey); }
    QString moreKnowledge() const { return m_repo.text(m_experiment.moreKey); }
    QStringList further() const { return texts(m_experiment.furtherKeys); }
    QString realLife() const { return m_repo.text(m_experiment.realKey); }
    QString nextExperimentId() const;
    Q_INVOKABLE double voltage(const QString& anchor) const;
    Q_INVOKABLE int voltageBars(const QString& anchor);
    // Zustand eines Bauteils: currentmA, level, overloaded, destroyed, tempC
    Q_INVOKABLE QVariantMap partState(const QString& id) const;
    // Strom einer Leitung in mA (Stromflussdarstellung)
    Q_INVOKABLE double wireCurrent(const QString& id) const;

    // Befehle (Verlauf). vertical/longSpan: Lage des Bauteils ab dem Startfeld.
    Q_INVOKABLE bool addPart(const QString& type, const QString& startField, bool vertical, bool longSpan);
    Q_INVOKABLE bool movePart(const QString& id, const QString& startField, bool vertical, bool longSpan);
    Q_INVOKABLE bool removePart(const QString& id);
    Q_INVOKABLE bool flipPart(const QString& id);
    Q_INVOKABLE bool setPartValue(const QString& id, double value);
    Q_INVOKABLE bool repairPart(const QString& id);
    Q_INVOKABLE bool addWire(const QString& from, const QString& to, const QString& color);
    Q_INVOKABLE bool moveWireEnd(const QString& id, bool fromEnd, const QString& anchor);
    Q_INVOKABLE bool removeWire(const QString& id);
    Q_INVOKABLE bool setWireColor(const QString& id, const QString& color);
    Q_INVOKABLE bool toggleVoltageDisplay(const QString& field);
    Q_INVOKABLE bool setBoardColumns(int columns); // Befehl; +/- in der Werkzeugleiste
    int boardColumns() const { return m_session->board().columns; }
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // Bedienhandlungen
    Q_INVOKABLE void setPartClosed(const QString& id, bool closed);
    Q_INVOKABLE void setDeskSwitch(bool closed);
    Q_INVOKABLE void setPot(int which, double position);
    Q_INVOKABLE void setMotorBlocked(const QString& id, bool blocked);

    model::LabSession& session() { return *m_session; }

signals:
    void experimentChanged();
    void boardChanged();
    void deskChanged();
    void stateChanged();
    void historyChanged();
    void editFailed();
    void flowChanged();

private:
    void onFrame();
    bool result(bool ok);
    QStringList texts(const QStringList& keys) const;

    services::ContentRepository m_repo;
    std::unique_ptr<model::LabSession> m_session;
    QString m_title;
    services::Experiment m_experiment;
    model::ExperimentFlow m_flow;
    QStringList m_experimentIds;
    QVariantList m_experimentList;
    QStringList m_errors;
    QString m_lastEditError;
    double m_timeScale = 1.0;
    QTimer m_timer;
    QElapsedTimer m_clock;
    double m_sinceNotifyS = 0.0;
};

} // namespace volta
