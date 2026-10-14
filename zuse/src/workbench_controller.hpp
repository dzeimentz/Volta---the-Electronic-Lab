#pragma once

// Werkbank des Zuse-Prototyps (Phase Z0): Stufe waehlen, Bausteine setzen und verbinden (jede Aenderung ein Befehl
// mit Rueckgaengig), Eingaenge umschalten (Bedienhandlung), mit der Wahrheitstabelle pruefen, Chip erstellen,
// Blick hinein. Geometrie (dp) wird hier berechnet, die Oberflaeche zeichnet nur.
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QUndoStack>
#include <QVariantList>
#include <map>
#include <string>
#include <vector>

#include "volta/logic/chip.hpp"
#include "volta/logic/logic_sim.hpp"
#include "volta/logic/netlist.hpp"

namespace zuse {

struct Stage {
    QString kuerzel;
    QString symbol;
    bool inverted = false;
    QString title;
    QString question;
    QString hint;
    QStringList inputs;
    QStringList outputs;
    QStringList palette;
    std::vector<std::vector<volta::logic::Level>> table;
    volta::logic::ChipDef solution; // Musterloesung (Teile und Verbindungen)
};

class WorkbenchController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList stages READ stages NOTIFY stagesChanged)
    Q_PROPERTY(int stageIndex READ stageIndex NOTIFY stageChanged)
    Q_PROPERTY(QString stageTitle READ stageTitle NOTIFY stageChanged)
    Q_PROPERTY(QString stageQuestion READ stageQuestion NOTIFY stageChanged)
    Q_PROPERTY(QString stageHint READ stageHint NOTIFY stageChanged)
    Q_PROPERTY(QString stageKuerzel READ stageKuerzel NOTIFY stageChanged)
    Q_PROPERTY(QVariantList palette READ palette NOTIFY stageChanged)
    Q_PROPERTY(QStringList stageInputs READ stageInputs NOTIFY stageChanged)
    Q_PROPERTY(QStringList stageOutputs READ stageOutputs NOTIFY stageChanged)
    Q_PROPERTY(QVariantList parts READ parts NOTIFY viewChanged)
    Q_PROPERTY(QVariantList ports READ ports NOTIFY viewChanged)
    Q_PROPERTY(QVariantList wires READ wires NOTIFY viewChanged)
    Q_PROPERTY(bool inside READ inside NOTIFY viewChanged)
    Q_PROPERTY(QStringList path READ path NOTIFY viewChanged)
    Q_PROPERTY(qreal frameWidth READ frameWidth CONSTANT)
    Q_PROPERTY(qreal frameHeight READ frameHeight CONSTANT)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(QVariantList checkRows READ checkRows NOTIFY checkChanged)
    Q_PROPERTY(QString checkState READ checkState NOTIFY checkChanged)
    Q_PROPERTY(bool chipReady READ chipReady NOTIFY checkChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)

public:
    // stagesFile: Inhaltsdatei (Vorgabe Ressource), dataDir: Ablage der eigenen Chips und Entwuerfe
    WorkbenchController(const QString& stagesFile, const QString& dataDir, const QString& language,
                        QObject* parent = nullptr);

    static constexpr qreal kFrameWidth = 900.0;
    static constexpr qreal kFrameHeight = 600.0;
    static constexpr qreal kPinPitch = 56.0; // Anschluesse >= 56 dp
    static constexpr qreal kPartWidth = 120.0;

    QVariantList stages() const;
    int stageIndex() const { return m_stage; }
    QString stageTitle() const;
    QString stageQuestion() const;
    QString stageHint() const;
    QString stageKuerzel() const;
    QVariantList palette() const;
    QStringList stageInputs() const { return m_stages.empty() ? QStringList() : m_stages[size_t(m_stage)].inputs; }
    QStringList stageOutputs() const { return m_stages.empty() ? QStringList() : m_stages[size_t(m_stage)].outputs; }
    QVariantList parts() const;
    QVariantList ports() const;
    QVariantList wires() const;
    bool inside() const { return !m_path.empty(); }
    QStringList path() const;
    qreal frameWidth() const { return kFrameWidth; }
    qreal frameHeight() const { return kFrameHeight; }
    bool canUndo() const { return m_undo.canUndo(); }
    bool canRedo() const { return m_undo.canRedo(); }
    QVariantList checkRows() const;
    QString checkState() const;
    bool chipReady() const { return m_checkDone && m_check.passed; }
    QString message() const { return m_message; }

    Q_INVOKABLE bool selectStage(int index);
    Q_INVOKABLE QString addPart(const QString& type, qreal x, qreal y);
    Q_INVOKABLE void movePart(const QString& id, qreal x, qreal y);
    Q_INVOKABLE void removePart(const QString& id);
    Q_INVOKABLE bool connectPins(const QString& a, const QString& b);
    Q_INVOKABLE void removeWire(int index);
    Q_INVOKABLE void clearBench();
    Q_INVOKABLE void toggleInput(const QString& name);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void check();
    Q_INVOKABLE bool createChip();
    Q_INVOKABLE bool lookInside(const QString& partId);
    Q_INVOKABLE void leaveTo(int depth); // 0 = zurueck auf die eigene Werkbank
    Q_INVOKABLE void showSolution();     // Musterloesung (gestufte Hilfe, letzte Stufe)

    // fuer Befehle und Tests
    const volta::logic::ChipDef& def() const { return m_def; }
    const volta::logic::Library& library() const { return m_library; }
    void applyDef(const volta::logic::ChipDef& def);
    bool isDone(const QString& kuerzel) const { return m_done.contains(kuerzel); }
    volta::logic::Level levelAt(const QString& endpoint) const;

signals:
    void stagesChanged();
    void stageChanged();
    void viewChanged();
    void historyChanged();
    void checkChanged();
    void messageChanged();

private:
    bool loadStages(const QString& file, const QString& language);
    void load();
    void save() const;
    void startStage(int index);
    void rebuild();
    void push(const volta::logic::ChipDef& after, const QString& text);
    void setMessage(const QString& text);
    bool unlocked(int index) const;
    const volta::logic::ChipDef* shownDef() const; // eigene Werkbank oder geoeffneter Chip
    const volta::logic::Scope* shownScope() const;
    QString symbolOf(const QString& type, bool* inverted) const;
    void partSize(const std::string& type, qreal& w, qreal& h) const;
    bool pinPos(const volta::logic::ChipDef& def, const std::string& endpoint, qreal& x, qreal& y) const;
    QString levelString(const std::string& endpoint) const;

    std::vector<Stage> m_stages;
    int m_stage = 0;
    QString m_dataDir;
    volta::logic::Library m_library; // erstellte Chips
    QSet<QString> m_done;
    std::map<QString, volta::logic::ChipDef> m_drafts;
    volta::logic::ChipDef m_def;
    QUndoStack m_undo;
    volta::logic::Netlist m_netlist;
    volta::logic::LogicSim m_sim;
    bool m_netOk = false;
    std::vector<volta::logic::Level> m_inputs;
    volta::logic::CheckResult m_check;
    bool m_checkDone = false;
    std::vector<std::string> m_path; // Teil-IDs beim Blick hinein
    QString m_message;
};

} // namespace zuse
