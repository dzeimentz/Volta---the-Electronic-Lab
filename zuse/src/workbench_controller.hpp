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
#include <QTimer>
#include <QUrl>
#include <QUndoStack>
#include <QVariantList>
#include <QVariantMap>
#include <map>
#include <string>
#include <vector>

#include "volta/logic/chip.hpp"
#include "volta/logic/logic_sim.hpp"
#include "volta/logic/netlist.hpp"
#include "services/journal_store.hpp"

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
    // Entdecken: Zustand in Alltagssprache fuer 0 und 1 je Eingang bzw. LED (»Fenster 1 offen« / »Fenster 1 zu«)
    QList<QStringList> inputStates;
    QList<QStringList> outputStates;
    std::vector<std::vector<std::vector<volta::logic::Level>>> boxes; // Raetselkisten: Wahrheitstabelle je Kiste
    int rightBox = -1;
    QString symbol;
    bool inverted = false;
    QString title;
    QString question;
    QString hint;
    QStringList inputs;
    // je Eingang "kipp" (Kippschalter, rastet), "taster" (nur solange gedrueckt) oder "takt" (Taktgeber, schaltet
    // von allein im Takt um)
    QStringList inputKinds;
    QStringList outputs;    // Ausgaenge sind LEDs
    bool makesChip = true;  // Stufe 0 (Schalter direkt an LEDs) ergibt keinen Chip
    bool free = false;      // Freies Labor: keine Aufgabe, alle eigenen Werkzeuge, nicht im Lernpfad
    QStringList palette;
    std::vector<std::vector<volta::logic::Level>> table;
    volta::logic::ChipDef solution; // Musterloesung (Teile und Verbindungen)
    // Art »zahlen«: fest verdrahtetes Bedienfeld, das Kind stellt nacheinander diese Zahlen an den LEDs ein
    std::vector<int> targets;
    QStringList taskTexts; // Text je Aufgabe (sonst »Zeige die n«)
    QString successText;   // Zetti nach der letzten Aufgabe (sonst Text zum Zaehlen bis 15)
    // Schalter als mehrere Zahlen lesen (z. B. A und B je 4 Bit); leer = alle Schalter eine Zahl
    std::vector<std::pair<QString, int>> inputGroups;
    // grosse Tabellen (ab 6 Eingaengen): nur diese Zeilen zeigen, mit Rechnung; geprueft wird trotzdem alles
    std::vector<std::pair<size_t, QString>> samples;
    // Schaltungen mit Speicher: Pruefung als Ablauf Schritt fuer Schritt statt Wahrheitstabelle; Schritte ohne
    // Text werden in der Tabelle nicht gezeigt (z. B. Loslassen beim Zaehlen)
    std::vector<volta::logic::SequenceStep> steps;
    QStringList stepTexts;
};

// Lernstufe wie bei Volta: fasst Experimente zusammen (einfach beginnen, pro Stufe anspruchsvoller)
struct LearningLevel {
    int number = 0;
    QString title;
    QString question;     // Leitfrage der Stufe
    QStringList kuerzel;  // Experimente der Stufe in Reihenfolge; leer = geplant
    QStringList upcoming; // Titel geplanter Experimente (erscheinen gesperrt mit »folgt«)
    QString outlook;      // Ausblick am Ende der Stufe (z. B. auf Zuse 2)
};

class WorkbenchController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList stages READ stages NOTIFY stagesChanged)
    Q_PROPERTY(QVariantList levels READ levels NOTIFY stagesChanged) // Lernstufen mit ihren Experimenten
    Q_PROPERTY(bool stageSequence READ stageSequence NOTIFY stageChanged) // Pruefung als Ablauf (Speicher)
    // wie Volta: »lernpfad« (Experiment), »frei« (Freies Labor) oder »vorfuehrung« (fertiger Aufbau, nichts gespeichert)
    Q_PROPERTY(QString mode READ mode NOTIFY stageChanged)
    Q_PROPERTY(QVariantList demos READ demos NOTIFY stagesChanged)   // Aufbauten fuer die Vorfuehrung
    Q_PROPERTY(QVariantMap resume READ resume NOTIFY stageChanged)   // »Weitermachen«: zuletzt bearbeitetes Experiment
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
    Q_PROPERTY(QStringList doneStages READ doneStages NOTIFY stagesChanged) // geschaffte Experimente (Lexikon)
    Q_PROPERTY(QVariantList panelInputs READ panelInputs NOTIFY viewChanged)   // Schalter mit Bedeutung und Pegel
    Q_PROPERTY(QVariantList panelOutputs READ panelOutputs NOTIFY viewChanged) // LEDs mit Bedeutung und Pegel
    Q_PROPERTY(QVariantList boxes READ boxes NOTIFY viewChanged)
    Q_PROPERTY(int boxIndex READ boxIndex NOTIFY viewChanged)
    Q_PROPERTY(bool guessing READ guessing NOTIFY checkChanged) // Entdecken: erst vermuten, dann ausprobieren
    Q_PROPERTY(QString zettiPose READ zettiPose NOTIFY messageChanged)
    // Zahlensysteme: Schalter bzw. LEDs als Zahl (erster Anschluss = hoechstes Bit) binaer, dezimal, hexadezimal;
    // Anzeige zuschaltbar (Einstellung, Vorgabe an)
    Q_PROPERTY(QVariantMap inputNumber READ inputNumber NOTIFY viewChanged)
    Q_PROPERTY(QVariantMap outputNumber READ outputNumber NOTIFY viewChanged)
    Q_PROPERTY(QVariantList inputNumbers READ inputNumbers NOTIFY viewChanged) // je Zahlengruppe mit "title"
    Q_PROPERTY(QVariantList tasks READ tasks NOTIFY checkChanged)              // Art »zahlen«: Aufgaben
    Q_PROPERTY(bool benchLocked READ benchLocked NOTIFY stageChanged)          // fest verdrahtet (Art »zahlen«)
    Q_PROPERTY(bool hasClock READ hasClock NOTIFY stageChanged)                // Stufe hat einen Taktgeber
    Q_PROPERTY(bool clockRunning READ clockRunning NOTIFY clockChanged)
    Q_PROPERTY(bool numbersVisible READ numbersVisible WRITE setNumbersVisible NOTIFY numbersVisibleChanged)
    Q_PROPERTY(QVariantList parts READ parts NOTIFY viewChanged)
    Q_PROPERTY(QVariantList ports READ ports NOTIFY viewChanged)
    Q_PROPERTY(QVariantList wires READ wires NOTIFY viewChanged)
    Q_PROPERTY(QVariantMap schematic READ schematic NOTIFY viewChanged)        // Schaltplan zum Aufbau
    Q_PROPERTY(QVariantList faultFindings READ faultFindings NOTIFY viewChanged) // Fehlersuche
    Q_PROPERTY(QVariantList partsList READ partsList NOTIFY viewChanged)       // Teileliste
    Q_PROPERTY(bool inside READ inside NOTIFY viewChanged)
    Q_PROPERTY(QStringList path READ path NOTIFY viewChanged)
    // Feldgroesse wie bei Volta (+/-): 0 = kleinste (Vorgabe), 1 und 2 mehr Flaeche; die Ansicht skaliert auf die
    // verfuegbare Flaeche, also wirken Raster, Bausteine und Bedienfeld kleiner. Reicht ein Aufbau weiter, als die
    // gewaehlte Groesse zulaesst, bleibt das Feld so gross wie noetig.
    Q_PROPERTY(int boardSize READ boardSize NOTIFY viewChanged)
    Q_PROPERTY(bool canGrowBoard READ canGrowBoard NOTIFY viewChanged)
    Q_PROPERTY(bool canShrinkBoard READ canShrinkBoard NOTIFY viewChanged)
    Q_PROPERTY(qreal frameWidth READ frameWidth NOTIFY viewChanged)
    Q_PROPERTY(qreal frameHeight READ frameHeight NOTIFY viewChanged)
    Q_PROPERTY(qreal grid READ grid CONSTANT)
    Q_PROPERTY(qreal logicLeft READ logicLeft CONSTANT)
    Q_PROPERTY(qreal logicRight READ logicRight NOTIFY viewChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(QVariantList checkRows READ checkRows NOTIFY checkChanged)
    Q_PROPERTY(bool stageInWords READ stageInWords NOTIFY stageChanged) // Tabelle als Situationskarten
    Q_PROPERTY(QString checkState READ checkState NOTIFY checkChanged)
    Q_PROPERTY(bool chipReady READ chipReady NOTIFY checkChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    // Schaltzeichen: "din" (DIN EN 60617-12, Vorgabe) oder "us" (IEEE Std 91); Einstellung, kein Befehl
    Q_PROPERTY(QVariantList journal READ journal NOTIFY journalChanged) // Forscherheft, neueste Eintraege zuerst
    Q_PROPERTY(QString symbolStyle READ symbolStyle WRITE setSymbolStyle NOTIFY symbolStyleChanged)

public:
    // stagesFile: Inhaltsdatei (Vorgabe Ressource), dataDir: Ablage der eigenen Chips und Entwuerfe
    WorkbenchController(const QString& stagesFile, const QString& dataDir, const QString& language,
                        QObject* parent = nullptr);

    static constexpr qreal kFrameWidth = 896.0; // 32 Rasterschritte
    static constexpr qreal kFrameHeight = 600.0;
    static constexpr qreal kPinPitch = 56.0; // Anschluesse >= 56 dp
    // Drei Schichten (Designsprache der Zuse-Reihe): Eingangsschicht links, Logikschicht mit Steckraster in der
    // Mitte, Ausgangsschicht rechts. Bausteine rasten im Raster ein, ihre Anschluesse liegen auf Rasterpunkten.
    static constexpr qreal kGrid = 28.0;
    static constexpr qreal kPartWidth = 112.0;   // 4 Rasterschritte
    static constexpr qreal kPortInset = 140.0;   // Loetpads der Schalter und LEDs am Rand ihrer Schicht
    static constexpr qreal kLogicLeft = 168.0;   // Grenze Eingangs-/Logikschicht
    static constexpr size_t kCompactPorts = 4; // ab so vielen Schaltern bzw. LEDs je Seite die flache Form (56 dp)

    QVariantList stages() const;
    QVariantList levels() const;
    bool stageSequence() const { return !m_stages.empty() && !m_stages[size_t(m_stage)].steps.empty(); }
    QString mode() const;
    QVariantList demos() const;
    QVariantMap resume() const;
    Q_INVOKABLE void startFreeLab();
    Q_INVOKABLE bool startDemo(int index); // Experiment index mit Musterloesung zeigen, ohne zu speichern
    Q_INVOKABLE void leaveSpecial();       // Freies Labor bzw. Vorfuehrung beenden, zurueck zum Lernpfad
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
    QStringList doneStages() const { return QStringList(m_done.begin(), m_done.end()); }
    QVariantList panelInputs() const;
    QVariantList panelOutputs() const;
    QVariantList boxes() const;
    int boxIndex() const { return m_box; }
    bool guessing() const;
    QString zettiPose() const { return m_pose; }
    QVariantMap inputNumber() const;
    QVariantMap outputNumber() const;
    QVariantList inputNumbers() const;
    QVariantList tasks() const;
    bool benchLocked() const { return editLocked(); }
    bool hasClock() const;
    bool clockRunning() const { return m_clock.isActive(); }
    bool numbersVisible() const { return m_numbersVisible; }
    void setNumbersVisible(bool on);
    // Bits (hoechstes zuerst) als Zahl: {"bin", "dec", "hex", "value", "bits"}; mit unbestimmtem Bit "?"
    static QVariantMap numberOf(const std::vector<volta::logic::Level>& bits);
    QVariantList parts() const;
    QVariantList ports() const;
    QVariantList wires() const;
    QVariantMap schematic() const;      // in schematic.cpp
    QVariantList faultFindings() const;
    QVariantList partsList() const;
    bool inside() const { return !m_path.empty(); }
    QStringList path() const;
    static constexpr int kMaxBoardSize = 2;
    static qreal frameWidthAt(int size) { return kFrameWidth + 8 * kGrid * size; }   // je Stufe 8 Rasterspalten mehr
    static qreal frameHeightAt(int size) { return kFrameHeight + 6 * kGrid * size; } // und 6 Rasterzeilen
    int boardSize() const { return std::max(m_boardSize, requiredBoardSize()); }
    bool canGrowBoard() const { return boardSize() < kMaxBoardSize; }
    bool canShrinkBoard() const { return boardSize() > requiredBoardSize(); }
    Q_INVOKABLE void setBoardSize(int size);
    qreal frameWidth() const { return frameWidthAt(boardSize()); }
    qreal frameHeight() const { return frameHeightAt(boardSize()); }
    qreal grid() const { return kGrid; }
    qreal logicLeft() const { return kLogicLeft; }
    qreal logicRight() const { return frameWidth() - kLogicLeft; }
    bool canUndo() const { return m_undo.canUndo(); }
    bool canRedo() const { return m_undo.canRedo(); }
    QVariantList checkRows() const;
    bool stageInWords() const;
    QString checkState() const;
    bool chipReady() const { return m_checkDone && m_check.passed && !m_demo && !isFree(); }
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
    Q_INVOKABLE void toggleClock();
    // Anzeigesprache der Inhalte (Geschichten, Fragen, Bezeichnungen); Fortschritt und Aufbau bleiben
    void setLanguage(const QString& language);
    QString language() const { return m_language; }                              // Taktgeber anhalten bzw. weiterlaufen lassen
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void check();
    Q_INVOKABLE bool createChip();
    Q_INVOKABLE bool lookInside(const QString& partId);
    Q_INVOKABLE void leaveTo(int depth); // 0 = zurueck auf die eigene Werkbank
    Q_INVOKABLE void showSolution();     // Musterloesung (gestufte Hilfe, letzte Stufe)
    Q_INVOKABLE void selectBox(int index); // Raetselkiste einsetzen (entdecken)
    Q_INVOKABLE bool confirmBox();         // »Diese Kiste passt«
    Q_INVOKABLE void toggleGuess(int row); // Vermutung einer Zeile: ? -> 1 -> 0 -> 1 ...
    Q_INVOKABLE void setGuess(int row, bool on); // Vermutung einer Situation direkt setzen
    Q_INVOKABLE void trySituation(int row);      // Schalter so stellen wie in dieser Zeile (Situationskarte)
    Q_INVOKABLE void skipGuess();          // ohne Vermutung ausprobieren
    // Forscherheft: Zetti schreibt Vermutung und Ergebnis, das Kind ergaenzt Notizen und Fotos
    QVariantList journal() const;
    Q_INVOKABLE bool addJournalNote(const QString& id, const QString& text);
    Q_INVOKABLE bool addJournalPhoto(const QString& id, const QUrl& source);
    Q_INVOKABLE bool removeJournalPhoto(const QString& id, const QString& file);
    Q_INVOKABLE void resetJournal();
    static QString journalFile(const QString& dataDir);

    // fuer Befehle und Tests
    const volta::logic::ChipDef& def() const { return m_def; }
    const volta::logic::Library& library() const { return m_library; }
    void applyDef(const volta::logic::ChipDef& def);
    bool isDone(const QString& kuerzel) const { return m_done.contains(kuerzel); }
    volta::logic::Level levelAt(const QString& endpoint) const;
    void tickClock(); // ein Takt des Taktgebers: alle Eingaenge der Art »takt« schalten um
    void playSteps(); // Ablauf der Stufe an den Eingaengen nachspielen (Entwicklerhilfe, Tests)
    static constexpr int kClockMs = 700; // halbe Periode: langsam genug zum Mitverfolgen

signals:
    void stagesChanged();
    void stageChanged();
    void viewChanged();
    void historyChanged();
    void checkChanged();
    void messageChanged();
    void symbolStyleChanged();
    void numbersVisibleChanged();
    void clockChanged();
    void journalChanged();

private:
    bool loadStages(const QString& file, const QString& language);
    void load();
    void save() const;
    void startStage(int index);
    void rebuild();
    void push(const volta::logic::ChipDef& after, const QString& text);
    void setMessage(const QString& text, const QString& pose = QString());
    bool isDiscover() const { return !m_stages.empty() && m_stages[size_t(m_stage)].kind == "entdecken"; }
    bool isCounting() const { return !m_stages.empty() && m_stages[size_t(m_stage)].kind == "zahlen"; }
    bool isFree() const { return !m_stages.empty() && m_stages[size_t(m_stage)].free; }
    bool editLocked() const { return isCounting() || m_demo; } // fest verdrahtet oder Vorfuehrung
    QString stageLabel(int index) const; // »4.5«: Lernstufe und Platz darin
    void endDemo();                      // eigene Werkzeugkiste zurueck
    void checkTarget(); // Art »zahlen«: aktuelle Aufgabe erfuellt?
    void applyInput(size_t index, volta::logic::Level level); // Eingang setzen (Bedienung oder Taktgeber)
    void dropStaleChips(); // Erfindungen, deren Anschluesse nicht mehr zur Stufe passen (aeltere Inhalte), verwerfen
    size_t currentRow() const; // Zeile der Wahrheitstabelle zur aktuellen Schalterstellung
    volta::logic::Level boxOutput(size_t output) const;
    QString toolName(const QString& type) const;
    bool unlocked(int index) const;
    const volta::logic::ChipDef* shownDef() const; // eigene Werkbank oder geoeffneter Chip
    const volta::logic::Scope* shownScope() const;
    QString symbolOf(const QString& type, bool* inverted) const;
    void partSize(const std::string& type, qreal& w, qreal& h) const;
    void snapPart(volta::logic::Part& part) const; // ins Raster der Logikschicht
    qreal portY(size_t index, size_t count) const;  // Hoehe eines Schalters bzw. einer LED
    int requiredBoardSize() const; // kleinste Feldgroesse, in die der gezeigte Aufbau passt
    bool pinPos(const volta::logic::ChipDef& def, const std::string& endpoint, qreal& x, qreal& y) const;
    QString levelString(const std::string& endpoint) const;
    QString portState(const std::string& name) const; // Zustand eines Schalters bzw. einer LED in Worten
    int levelOf(const QString& kuerzel) const; // Lernstufe eines Experiments, -1 = keine
    QString situationLine(const Stage& s, size_t row, const std::vector<volta::logic::Level>& outs) const;
    void journalGuess();  // Vermutung ins Forscherheft
    void journalResult(); // Ergebnis der Stufe ins Forscherheft

    std::vector<Stage> m_stages;
    std::vector<LearningLevel> m_levels;
    int m_stage = 0;
    QString m_stagesFile;
    QString m_language;
    QString m_dataDir;
    volta::logic::Library m_library; // erstellte Chips
    volta::logic::Library m_demoLibrary;  // alle Musterloesungen (Vorfuehrung)
    volta::logic::Library m_ownLibrary;   // eigene Chips, solange die Vorfuehrung laeuft
    bool m_demo = false;
    int m_lastStage = 0;                  // zuletzt bearbeitetes Experiment im Lernpfad
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
    std::vector<volta::logic::Level> m_guess; // Vermutung je Zeile (X = noch offen)
    bool m_guessSkipped = false;
    bool m_solutionShown = false; // Musterloesung in dieser Stufe angeschaut (Forscherheft)
    volta::services::JournalStore m_journal;
    size_t m_task = 0; // Art »zahlen«: aktuelle Aufgabe
    QTimer m_clock;    // Taktgeber
    QString m_symbolStyle = QStringLiteral("din");
    bool m_numbersVisible = true;
    int m_boardSize = 0; // gewaehlte Feldgroesse (Einstellung, kein Befehl)
};

} // namespace zuse
