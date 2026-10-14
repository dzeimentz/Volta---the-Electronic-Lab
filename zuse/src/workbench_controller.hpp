#pragma once

// Werkbank des Zuse-Prototyps (Phase Z0). Jede Stufe beginnt mit einer Geschichte und ist entweder
// - »entdecken«: Raetselkisten zwischen Schaltern und LEDs ausprobieren und die passende Regel finden; die Regel
//   (NICHT, UND, ODER) kommt als Werkzeug in die Werkzeugkiste, oder
// - »bauen«: aus den Werkzeugen eine Schaltung bauen (jede Aenderung ein Befehl mit Rueckgaengig), mit der
//   Wahrheitstabelle pruefen und als Erfindung einpacken (wird selbst zum Werkzeug); Blick hinein.
// Eingaenge umschalten ist eine Bedienhandlung. Geometrie (dp) wird hier berechnet, die Oberflaeche zeichnet nur.
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QUndoStack>
#include <QVariantList>
#include <QVariantMap>
#include <map>
#include <string>
#include <vector>

#include "volta/logic/chip.hpp"
#include "volta/logic/logic_sim.hpp"
#include "volta/logic/netlist.hpp"

namespace zuse {

struct Stage {
    QString kuerzel;
    QString kind = QStringLiteral("bauen"); // "entdecken" oder "bauen"
    QString story;     // Geschichte zum Einstieg
    QString merksatz;  // was das Kind am Ende mitnimmt
    QString name;      // Name der Erfindung (bauen), z. B. »Alarmanlage«
    QString tool;      // Grundgatter, das die Entdeckung freischaltet (entdecken), z. B. "AND"
    QStringList inputLabels;  // Bedeutung je Eingang, z. B. »Haustuer«
    QStringList outputLabels; // Bedeutung je LED, z. B. »Klingel«
    std::vector<std::vector<std::vector<volta::logic::Level>>> boxes; // Raetselkisten: Wahrheitstabelle je Kiste
    int rightBox = -1;
    QString symbol;
    bool inverted = false;
    QString title;
    QString question;
    QString hint;
    QStringList inputs;
    QStringList inputKinds; // je Eingang "kipp" (Kippschalter, rastet) oder "taster" (nur solange gedrueckt)
    QStringList outputs;    // Ausgaenge sind LEDs
    bool makesChip = true;  // Stufe 0 (Schalter direkt an LEDs) ergibt keinen Chip
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
    Q_PROPERTY(bool stageMakesChip READ stageMakesChip NOTIFY stageChanged)
    Q_PROPERTY(QString stageKind READ stageKind NOTIFY stageChanged)
    Q_PROPERTY(QString stageStory READ stageStory NOTIFY stageChanged)
    Q_PROPERTY(QString stageMerksatz READ stageMerksatz NOTIFY stageChanged)
    Q_PROPERTY(QString stageName READ stageName NOTIFY stageChanged)
    Q_PROPERTY(bool stageDone READ stageDone NOTIFY stagesChanged)
    Q_PROPERTY(QVariantList tools READ tools NOTIFY stagesChanged)        // Werkzeugkiste
    Q_PROPERTY(QVariantList panelInputs READ panelInputs NOTIFY viewChanged)   // Schalter mit Bedeutung und Pegel
    Q_PROPERTY(QVariantList panelOutputs READ panelOutputs NOTIFY viewChanged) // LEDs mit Bedeutung und Pegel
    Q_PROPERTY(QVariantList boxes READ boxes NOTIFY viewChanged)
    Q_PROPERTY(int boxIndex READ boxIndex NOTIFY viewChanged)
    Q_PROPERTY(QString zettiPose READ zettiPose NOTIFY messageChanged)
    // Zahlensysteme: Schalter bzw. LEDs als Zahl (erster Anschluss = hoechstes Bit) binaer, dezimal, hexadezimal;
    // Anzeige zuschaltbar (Einstellung, Vorgabe an)
    Q_PROPERTY(QVariantMap inputNumber READ inputNumber NOTIFY viewChanged)
    Q_PROPERTY(QVariantMap outputNumber READ outputNumber NOTIFY viewChanged)
    Q_PROPERTY(bool numbersVisible READ numbersVisible WRITE setNumbersVisible NOTIFY numbersVisibleChanged)
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
    // Schaltzeichen: "din" (DIN EN 60617-12, Vorgabe) oder "us" (IEEE Std 91); Einstellung, kein Befehl
    Q_PROPERTY(QString symbolStyle READ symbolStyle WRITE setSymbolStyle NOTIFY symbolStyleChanged)

public:
    // stagesFile: Inhaltsdatei (Vorgabe Ressource), dataDir: Ablage der eigenen Chips und Entwuerfe
    WorkbenchController(const QString& stagesFile, const QString& dataDir, const QString& language,
                        QObject* parent = nullptr);

    static constexpr qreal kFrameWidth = 900.0;
    static constexpr qreal kFrameHeight = 600.0;
    static constexpr qreal kPinPitch = 56.0; // Anschluesse >= 56 dp
    static constexpr qreal kPartWidth = 120.0;
    static constexpr qreal kPortInset = 144.0;  // Schalter und LEDs sitzen im Rand des Rahmens
    static constexpr qreal kPartMargin = 190.0; // Bausteine bleiben zwischen Schaltern und LEDs

    QVariantList stages() const;
    int stageIndex() const { return m_stage; }
    QString stageTitle() const;
    QString stageQuestion() const;
    QString stageHint() const;
    QString stageKuerzel() const;
    QVariantList palette() const;
    QStringList stageInputs() const { return m_stages.empty() ? QStringList() : m_stages[size_t(m_stage)].inputs; }
    QStringList stageOutputs() const { return m_stages.empty() ? QStringList() : m_stages[size_t(m_stage)].outputs; }
    bool stageMakesChip() const { return m_stages.empty() || m_stages[size_t(m_stage)].makesChip; }
    QString stageKind() const { return m_stages.empty() ? QString() : m_stages[size_t(m_stage)].kind; }
    QString stageStory() const { return m_stages.empty() ? QString() : m_stages[size_t(m_stage)].story; }
    QString stageMerksatz() const { return m_stages.empty() ? QString() : m_stages[size_t(m_stage)].merksatz; }
    QString stageName() const { return m_stages.empty() ? QString() : m_stages[size_t(m_stage)].name; }
    bool stageDone() const { return !m_stages.empty() && m_done.contains(m_stages[size_t(m_stage)].kuerzel); }
    QVariantList tools() const;
    QVariantList panelInputs() const;
    QVariantList panelOutputs() const;
    QVariantList boxes() const;
    int boxIndex() const { return m_box; }
    QString zettiPose() const { return m_pose; }
    QVariantMap inputNumber() const;
    QVariantMap outputNumber() const;
    bool numbersVisible() const { return m_numbersVisible; }
    void setNumbersVisible(bool on);
    // Bits (hoechstes zuerst) als Zahl: {"bin", "dec", "hex", "value", "bits"}; mit unbestimmtem Bit "?"
    static QVariantMap numberOf(const std::vector<volta::logic::Level>& bits);
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
    QString symbolStyle() const { return m_symbolStyle; }
    void setSymbolStyle(const QString& style);

    Q_INVOKABLE bool selectStage(int index);
    Q_INVOKABLE QString addPart(const QString& type, qreal x, qreal y);
    Q_INVOKABLE void movePart(const QString& id, qreal x, qreal y);
    Q_INVOKABLE void removePart(const QString& id);
    Q_INVOKABLE bool connectPins(const QString& a, const QString& b);
    Q_INVOKABLE void removeWire(int index);
    Q_INVOKABLE void clearBench();
    Q_INVOKABLE void toggleInput(const QString& name);           // Kippschalter
    Q_INVOKABLE void setInput(const QString& name, bool on);     // Taster: druecken und loslassen
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void check();
    Q_INVOKABLE bool createChip();
    Q_INVOKABLE bool lookInside(const QString& partId);
    Q_INVOKABLE void leaveTo(int depth); // 0 = zurueck auf die eigene Werkbank
    Q_INVOKABLE void showSolution();     // Musterloesung (gestufte Hilfe, letzte Stufe)
    Q_INVOKABLE void selectBox(int index); // Raetselkiste einsetzen (entdecken)
    Q_INVOKABLE bool confirmBox();         // »Diese Kiste passt«

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
    void symbolStyleChanged();
    void numbersVisibleChanged();

private:
    bool loadStages(const QString& file, const QString& language);
    void load();
    void save() const;
    void startStage(int index);
    void rebuild();
    void push(const volta::logic::ChipDef& after, const QString& text);
    void setMessage(const QString& text, const QString& pose = QString());
    bool isDiscover() const { return !m_stages.empty() && m_stages[size_t(m_stage)].kind == "entdecken"; }
    size_t currentRow() const; // Zeile der Wahrheitstabelle zur aktuellen Schalterstellung
    volta::logic::Level boxOutput(size_t output) const;
    QString toolName(const QString& type) const;
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
    QString m_pose = QStringLiteral("zeigend");
    int m_box = -1;             // eingesetzte Raetselkiste
    std::vector<char> m_seen;   // Zeilen, die mit dieser Kiste schon ausprobiert wurden
    QString m_symbolStyle = QStringLiteral("din");
    bool m_numbersVisible = true;
};

} // namespace zuse
