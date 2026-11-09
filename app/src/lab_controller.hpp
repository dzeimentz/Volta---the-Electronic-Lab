#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QRectF>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include <array>
#include <memory>

#include "model/build_guide.hpp"
#include "model/experiment_flow.hpp"
#include "model/lab_session.hpp"
#include "model/oscilloscope.hpp"
#include "model/recorder.hpp"
#include "model/tone_synth.hpp"
#include "services/content_repository.hpp"
#include "services/journal_store.hpp"
#include "services/profile_store.hpp"
#include "services/progress_store.hpp"
#include "services/session_store.hpp"
#include "services/task_code.hpp"

namespace volta {
namespace services {
class AudioService;
}

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
    // Felder der Grundplatte (ohne Pult), an denen ein Bauteil oder eine Leitung steckt (Spannungsanzeige automatisch)
    Q_PROPERTY(QStringList occupiedFields READ occupiedFields NOTIFY boardChanged)
    Q_PROPERTY(int boardColumns READ boardColumns NOTIFY boardChanged) // 5, 6, 8, 10 oder 12 Felder je Reihe
    // kleinste waehlbare Breite (8, 10 oder 12): Aufbau und Musteraufbau muessen hineinpassen
    Q_PROPERTY(int minBoardColumns READ minBoardColumns NOTIFY boardChanged)
    Q_PROPERTY(int boardRows READ boardRows NOTIFY boardChanged)       // 2 ... 5 Reihen ab A
    // Stufenanpassung (Elternbereich, Vorgabe an): im gefuehrten Experiment Steckbrett und Pult nach der Stufe -
    // Stufe 0-2: 2 x 5 Felder, Stufe 3: 3 x 6, ab Stufe 4: 5 x 10; das Pult zeigt nur die Bauteile der Stufe (und alles,
    // was die Vorlage oder der Aufbau nutzt). Aus: ganze Grundplatte und ganzes Pult wie bisher.
    Q_PROPERTY(bool stageLayout READ stageLayout WRITE setStageLayout NOTIFY columnsModeChanged)
    Q_PROPERTY(QStringList deskGroups READ deskGroups NOTIFY boardChanged) // batterie, p1, p2, schalter, lautsprecher
    // Modulleiste: mit Stufenanpassung nur, wenn das Experiment Module nutzt (sonst immer)
    Q_PROPERTY(bool moduleBarVisible READ moduleBarVisible NOTIFY boardChanged)
    Q_PROPERTY(QVariantList catalogParts READ catalogParts CONSTANT)
    // Bauteilfach: im gefuehrten Experiment die freigegebenen Bauteile, im freien Labor alle (locked: noch gesperrt)
    Q_PROPERTY(QVariantList trayParts READ trayParts NOTIFY progressChanged)
    // Modulfach: [{type, kind, stage, pins[], locked}]; im gefuehrten Experiment die Module seiner Vorlage
    Q_PROPERTY(QVariantList trayModules READ trayModules NOTIFY progressChanged)
    // Lernpfad: [{number, title, question, unlocked, done, total, experiments: [{id, title, done, destructive}]}]
    Q_PROPERTY(QVariantList stages READ stages NOTIFY progressChanged)
    // Forscherprofile (Konzept 8.2): eingeschaltet im Elternbereich; [{id, symbol, name}] und das gewaehlte Profil
    // ({} ohne Auswahl). Je Profil eigene Dateien fuer Fortschritt, Forscherheft und Laborstand.
    Q_PROPERTY(bool profilesEnabled READ profilesEnabled WRITE setProfilesEnabled NOTIFY profilesChanged)
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY profilesChanged)
    Q_PROPERTY(QVariantMap currentProfile READ currentProfile NOTIFY profilesChanged)
    Q_PROPERTY(int profileLimit READ profileLimit NOTIFY profilesChanged) // Hoechstzahl (verwaltete Konfiguration)
    // Alle Bauteile und Stufen freigeben (Einstellung; spaeter hinter der Elternschranke)
    Q_PROPERTY(bool unlockAll READ unlockAll WRITE setUnlockAll NOTIFY progressChanged)
    // Gespeicherte Arbeitsumgebung (AppSettings): Felder je Reihe fuer neue Aufbauten (sofern die Vorlage passt) und
    // Messgeraet im freien Labor
    Q_PROPERTY(int preferredColumns MEMBER m_preferredColumns)
    Q_PROPERTY(bool preferMeter MEMBER m_preferMeter)
    // Farbautomatik fuer neue Leitungen: am Pluspol rot, am Minuspol schwarz (Einstellung)
    Q_PROPERTY(bool autoWireColor MEMBER m_autoWireColor)
    // Breite der Grundplatte automatisch (Einstellung im Menue): Lernpfad 8, freies Labor 12 Felder je Reihe;
    // Messgeraet neben der Grundplatte 10, im Pult 12 - die Grundplatte springt so nicht bei jedem Einblenden
    Q_PROPERTY(bool autoColumns READ autoColumns WRITE setAutoColumns NOTIFY columnsModeChanged)
    Q_PROPERTY(bool meterDocked READ meterDocked WRITE setMeterDocked NOTIFY columnsModeChanged)
    Q_PROPERTY(QVariantMap task READ task NOTIFY taskChanged) // aktuelle Aufgabe (leer: keine)
    // Praesentationsmodus (Lehrkraft am Beamer, aus dem Elternbereich): alle Stufen offen, nichts wird gespeichert -
    // Fortschritt, Forscherheft und Laborstand des Profils bleiben unberuehrt; beim Beenden ist alles wie vorher
    Q_PROPERTY(bool presentation READ presentation WRITE setPresentation NOTIFY presentationChanged)
    // Vorfuehrung: fertiger Musteraufbau im Betrieb, ohne Fortschritt, Forscherheft und Laborstand zu beruehren
    Q_PROPERTY(bool demo READ demo NOTIFY demoChanged)
    Q_PROPERTY(QStringList errors READ errors NOTIFY boardChanged)
    Q_PROPERTY(QString lastEditError READ lastEditError NOTIFY editFailed)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(QString undoText READ undoText NOTIFY historyChanged)
    Q_PROPERTY(QString redoText READ redoText NOTIFY historyChanged)
    Q_PROPERTY(bool mainSwitch READ mainSwitch WRITE setMainSwitch NOTIFY deskChanged)
    Q_PROPERTY(int cells READ cells WRITE setCells NOTIFY deskChanged)
    Q_PROPERTY(int cells2 READ cells2 WRITE setCells2 NOTIFY deskChanged) // Batterie 2: 3 oder 6 Zellen
    Q_PROPERTY(int acRms READ acRms WRITE setAcRms NOTIFY deskChanged)    // Wechselquelle: 6 oder 9 V~
    // Funktionsgenerator: Signalform 0 Sinus, 1 Dreieck, 2 Rechteck; Frequenz in Hz, Spitze und Gleichanteil in V
    Q_PROPERTY(int genWaveform READ genWaveform WRITE setGenWaveform NOTIFY deskChanged)
    Q_PROPERTY(double genFrequency READ genFrequency WRITE setGenFrequency NOTIFY deskChanged)
    Q_PROPERTY(double genAmplitude READ genAmplitude WRITE setGenAmplitude NOTIFY deskChanged)
    Q_PROPERTY(double genOffset READ genOffset WRITE setGenOffset NOTIFY deskChanged)
    Q_PROPERTY(double genAmDepth READ genAmDepth WRITE setGenAmDepth NOTIFY deskChanged) // ab Volta 3
    Q_PROPERTY(double genAmTone READ genAmTone WRITE setGenAmTone NOTIFY deskChanged)    // ab Volta 3
    // Pult nach Bedarf (ab Volta 2): Geraetefach statt Stufenanpassung; deskTray: [{part, present, locked, stage}]
    //   locked: noch nicht frei; unlock: "stufe" (stage), "experiment" (experiment: Nummer, z. B. "0.3") oder "nutzung"
    Q_PROPERTY(bool deskOnDemand READ deskOnDemand CONSTANT)
    // Startbreite der Grundplatte des Bandes (0: wie Volta 1); die App setzt damit die Vorgabe der Einstellung
    Q_PROPERTY(int startColumns READ startColumns CONSTANT)
    Q_PROPERTY(QString band READ band CONSTANT) // Band der Edition (z. B. Themennamen des Bauteilfachs)
    Q_PROPERTY(QVariantList deskTray READ deskTray NOTIFY boardChanged)
    Q_PROPERTY(double pot1 READ pot1 WRITE setPot1 NOTIFY deskChanged)
    Q_PROPERTY(double pot2 READ pot2 WRITE setPot2 NOTIFY deskChanged)
    Q_PROPERTY(bool deskSwitch READ deskSwitch WRITE setDeskSwitch NOTIFY deskChanged)
    // Dreifach-Messgeraet: 0 V, 1 A, 2 Ohm; analog (Zeiger) in allen drei Messarten
    Q_PROPERTY(bool meterVisible READ meterVisible WRITE setMeterVisible NOTIFY deskChanged)
    Q_PROPERTY(int meterMode READ meterMode WRITE setMeterMode NOTIFY deskChanged)
    Q_PROPERTY(bool meterAnalog READ meterAnalog WRITE setMeterAnalog NOTIFY deskChanged)
    // Anzeigegeraet ab Volta 2 (Konzept 5.2): doppelt (zwei Anzeigen), Leistung aus V und A
    Q_PROPERTY(int displayCount READ displayCount WRITE setDisplayCount NOTIFY deskChanged) // Anzeigen 1 ... 3
    Q_PROPERTY(bool displayPower READ displayPower WRITE setDisplayPower NOTIFY deskChanged)
    Q_PROPERTY(double simTime READ simTime NOTIFY stateChanged)
    Q_PROPERTY(bool showSimTime READ showSimTime CONSTANT) // Entwicklerhilfe VOLTA_SHOW_TIME=1
    Q_PROPERTY(double timeScale READ timeScale WRITE setTimeScale NOTIFY deskChanged)
    // Tonanzeige (ab Volta 2, Konzept 5.3): "" kein Ton am Pult-Lautsprecher, "echt" hoerbar gerechnet,
    // "ersatz" Ton aus der simulierten Frequenz (Simulation langsamer als Echtzeit), "zeitlupe" stumm
    Q_PROPERTY(QString toneMode READ toneMode NOTIFY toneModeChanged)
    Q_PROPERTY(double realtimeRatio READ realtimeRatio NOTIFY stateChanged)
    Q_PROPERTY(double batteryVolt READ batteryVolt NOTIFY deskChanged)
    // Umgebung: Raumlicht 0 dunkel / 1 Daemmerung / 2 hell; Werkzeuge [{type, col, row, on}] auf der Grundplatte
    Q_PROPERTY(int roomLight READ roomLight WRITE setRoomLight NOTIFY deskChanged)
    Q_PROPERTY(QVariantList tools READ tools NOTIFY deskChanged)
    // Gefuehrtes Experiment (Phase 1: Frage -> Aufbau -> Erfolg)
    Q_PROPERTY(QVariantList experiments READ experiments NOTIFY experimentChanged)
    Q_PROPERTY(QString experimentId READ experimentId NOTIFY experimentChanged)
    Q_PROPERTY(bool freeLab READ freeLab NOTIFY experimentChanged)
    Q_PROPERTY(int experimentStage READ experimentStage NOTIFY experimentChanged) // -1: freies Labor
    // Bauteile des Musteraufbaus [{type, fields}] (Aufbautext: Steckfelder eines Bauteils nebeneinander)
    Q_PROPERTY(QVariantList templateParts READ templateParts NOTIFY experimentChanged)
    // 0 freies Labor, 1 Frage, 2 Aufbau, 3 Beobachtung, 4 Erklaerung (Forschungskreislauf, Konzept 5.6)
    Q_PROPERTY(int phase READ phase NOTIFY flowChanged)
    Q_PROPERTY(QStringList observations READ observations NOTIFY experimentChanged)
    Q_PROPERTY(int observation READ observation NOTIFY flowChanged)
    // Vergleich der Beobachtung mit der Vermutung: 0 noch keiner, 1 passt, 2 anders, 3 offen
    Q_PROPERTY(int comparison READ comparison NOTIFY flowChanged)
    // Forscherheft: [{id, title, date, guess, observation, comparison, notes[]}], neueste zuerst
    Q_PROPERTY(QVariantList journal READ journal NOTIFY journalChanged)
    Q_PROPERTY(bool destructive READ destructive NOTIFY experimentChanged)
    Q_PROPERTY(bool experimentScope READ experimentScope NOTIFY experimentChanged) // Oszilloskop beim Start zeigen
    Q_PROPERTY(QString question READ question NOTIFY experimentChanged)
    Q_PROPERTY(QStringList guesses READ guesses NOTIFY experimentChanged)
    Q_PROPERTY(int guess READ guess NOTIFY flowChanged)
    Q_PROPERTY(QStringList steps READ steps NOTIFY experimentChanged)
    // Aufbau uebernehmen (»aufbau_von«, ab Volta 2): nur wenn das fruehere Experiment geschafft ist;
    // {step (ab 0), title, done (alles steckt schon)}, sonst leer
    Q_PROPERTY(QVariantMap adoptOffer READ adoptOffer NOTIFY adoptChanged)
    Q_PROPERTY(QStringList hints READ hints NOTIFY flowChanged) // bisher aufgedeckte Hilfen
    Q_PROPERTY(bool moreHints READ moreHints NOTIFY flowChanged)
    Q_PROPERTY(bool solutionShown READ solutionShown NOTIFY flowChanged)
    // »Fertig« im Aufbau: erst klickbar, wenn alle Bauteile der Vorlage stecken und die Erfolgsbedingung erfuellt ist
    Q_PROPERTY(bool buildComplete READ buildComplete NOTIFY finishChanged)
    Q_PROPERTY(bool canFinish READ canFinish NOTIFY finishChanged)
    Q_PROPERTY(QString explanation READ explanation NOTIFY experimentChanged)
    Q_PROPERTY(QString moreKnowledge READ moreKnowledge NOTIFY experimentChanged)
    Q_PROPERTY(QStringList further READ further NOTIFY experimentChanged)
    Q_PROPERTY(QString realLife READ realLife NOTIFY experimentChanged)
    Q_PROPERTY(QString nextExperimentId READ nextExperimentId NOTIFY experimentChanged)
    // Schritt-fuer-Schritt-Aufbauhilfe (model::GuideStep): eingeschaltet vom Kind; guideStep beschreibt den ersten
    // offenen Schritt: {index, count, kind ("modul", "bauteil", "leitung", "hauptschalter", "fertig"), id, type,
    // value, pins: [{name, field}], from, to, color, targets[], labels[] (Feldnamen fuer den Text)}
    Q_PROPERTY(bool guideActive READ guideActive WRITE setGuideActive NOTIFY guideChanged)
    Q_PROPERTY(QVariantMap guideStep READ guideStep NOTIFY guideChanged)
    // Zahlenwerte (z. B. Hz am Lautsprecher) ab Stufe 3 und im freien Labor (docs/ui-spec.md)
    Q_PROPERTY(bool showNumbers READ showNumbers NOTIFY experimentChanged)

public:
    // band: Band der Edition (z. B. "volta2"), dessen Inhaltspaket die App zeigt
    explicit LabController(QString contentRoot, QString band = QStringLiteral("volta1"), QObject* parent = nullptr);
    ~LabController() override;

    QString title() const { return m_title; }
    QVariantList parts() const;
    QVariantList wires() const;
    QVariantList modules() const;
    QStringList voltageFields() const;
    QStringList occupiedFields() const;
    QVariantList catalogParts() const;
    QVariantList trayParts() const;
    QVariantList trayModules() const;
    QVariantList stages() const;
    bool unlockAll() const { return m_unlockAll; }
    void setUnlockAll(bool on);
    // Fortschritt dauerhaft speichern (main.cpp; ohne Aufruf nur im Speicher, z. B. in Tests)
    void useProgressFile(const QString& path);
    // Ablageort aller Daten (Fortschritt, Forscherheft, Laborstand, Profile); ohne Profile direkt dort
    void useDataRoot(const QString& dir);
    bool profilesEnabled() const { return m_profilesEnabled; }
    int profileLimit() const { return m_profileLimit; }
    // Verwaltete Konfiguration (Schule): Hoechstzahl der Profile, mindestens freigegebene Stufe (-1: keine Vorgabe)
    void setProfileLimit(int limit);
    void setManagedUnlockStage(int stage);
    void setProfilesEnabled(bool on);
    QVariantList profiles() const;
    QVariantMap currentProfile() const;
    // neues Profil (Symbol, Spitzname); leere ID, wenn es nicht angelegt werden konnte
    Q_INVOKABLE QString addProfile(const QString& symbol, const QString& name);
    // Profil waehlen: seine Daten werden geladen (der Laborstand mit resumeSession)
    Q_INVOKABLE bool selectProfile(const QString& id);
    // zurueck zur Profilauswahl (aktuellen Stand vorher sichern)
    Q_INVOKABLE void leaveProfile();
    // Elternbereich: ein Profil bzw. alle Profile samt Daten loeschen
    Q_INVOKABLE bool removeProfile(const QString& id);
    Q_INVOKABLE void removeAllProfiles();
    // ---- Mitnahme auf ein anderes Geraet (services/profile_transfer)
    // Fortschrittscode als Link (fuer den QR-Code) und QR-Raster {size, cells: "0101..."} eines Textes
    Q_INVOKABLE QString progressLink() const;
    Q_INVOKABLE QVariantMap qrCode(const QString& text) const;
    // Aufgaben der Lehrkraft (services/task_code): nur Hinweg, per QR-Code (Link) oder Code zum Eintippen.
    // ids: volle Kennungen ("volta1.2.4"); withBoard: der aktuelle Aufbau geht als eigener Aufbau mit.
    Q_INVOKABLE QString makeTaskLink(const QStringList& ids, const QString& text, bool withBoard, const QString& title) const;
    Q_INVOKABLE QString makeTaskCode(const QStringList& ids) const; // leer, wenn es nicht in einen Code passt
    // Aufgabe lesen (Link oder Code): ok, experiments [{id, title, stage, done}], title, text, hasBoard
    Q_INVOKABLE QVariantMap readTask(const QString& linkOrCode) const;
    Q_INVOKABLE bool acceptTask(const QString& linkOrCode); // wird zur aktuellen Aufgabe (je Profil gespeichert)
    Q_INVOKABLE bool startTaskBoard();                       // eigener Aufbau der Aufgabe im freien Labor
    Q_INVOKABLE void clearTask();
    // erledigte Experimente aus einem Link (zur Rueckfrage) bzw. uebernehmen (Anzahl neu erledigter)
    Q_INVOKABLE QStringList progressFromLink(const QString& link) const;
    Q_INVOKABLE int importProgressLink(const QString& link);
    // Profildatei: speichern; ansehen {ok, symbol, name, done}; laden (mit Profilen als neues Profil, sonst ersetzt
    // sie den Stand dieses Geraets)
    Q_INVOKABLE bool exportProfile(const QUrl& target);
    Q_INVOKABLE QVariantMap inspectProfileFile(const QUrl& source) const;
    Q_INVOKABLE bool importProfileFile(const QUrl& source);
    // Automatisches Speichern des Laborstands (Aufbau, Experiment mit Ablauf, Labor offen); ohne Aufruf nur im Speicher
    void useSessionFile(const QString& path);
    // gespeicherten Stand laden (Start der App); true, wenn ein Stand wiederhergestellt wurde
    Q_INVOKABLE bool resumeSession();
    // war das Labor beim letzten Speichern offen? (Start direkt im Labor)
    Q_INVOKABLE bool savedLabOpen() const;
    // die Oberflaeche meldet, ob das Labor offen ist (wird mitgespeichert)
    Q_INVOKABLE void setLabOpen(bool open);
    // sofort speichern (App geht in den Hintergrund, Beenden)
    Q_INVOKABLE void saveSession();
    Q_INVOKABLE bool isDone(const QString& experimentId) const { return m_progress.isDone(experimentId); }
    Q_INVOKABLE bool isUnlocked(const QString& experimentId) const;
    Q_INVOKABLE void resetProgress();
    // Forscherheft leeren (Elternbereich)
    Q_INVOKABLE void resetJournal();
    // Foto ins Forscherheft des Experiments (Systemauswahl liefert die Adresse); false, wenn es nicht lesbar ist
    Q_INVOKABLE bool addJournalPhoto(const QString& experimentId, const QUrl& source);
    Q_INVOKABLE bool removeJournalPhoto(const QString& experimentId, const QString& fileName);

    // Werkzeugkasten (Konzept 5.7): Farbcode, E-Reihen, Bauteillexikon
    Q_INVOKABLE QVariantMap colorCode(double ohm, int rings) const; // {valid, rings[], ohm, tolerance}
    Q_INVOKABLE double decodeColors(const QVariantList& rings) const;
    Q_INVOKABLE double parseOhm(const QString& text) const;
    Q_INVOKABLE QVariantList eSeries(int n) const;
    Q_INVOKABLE QVariantMap nearestE(double ohm, int n) const; // {value, deviation, isMember}
    // Formelrechner (model/formula.hpp): Eingabe mit Vorsatz -> {ok, value}; Wert -> Text mit Vorsatz und Einheit
    // (Dezimalzeichen der Sprache); Rechenmodus -> {ok, value}; Rechenblatt -> Ergebniswerte (siehe .cpp)
    Q_INVOKABLE QVariantMap formulaParse(const QString& text) const;
    Q_INVOKABLE QString formulaFormat(double value, const QString& unit) const;
    Q_INVOKABLE QVariantMap formulaEval(const QString& expression) const;
    Q_INVOKABLE QVariantMap formulaSheet(const QString& sheet, const QVariantMap& in) const;
    // hoechste freigeschaltete Stufe (alles freigegeben: 99) - z. B. fuer die Rechenblaetter ab Stufe 3/4
    Q_PROPERTY(int unlockedStage READ unlockedStage NOTIFY progressChanged)
    int unlockedStage() const { return highestUnlockedStage(); }
    // Lexikon: [{type, text}] aller Bauteile mit Lexikontext, in der Reihenfolge des Bauteilfachs; ab Volta 2 dahinter
    // die Module
    Q_PROPERTY(QVariantList lexicon READ lexicon NOTIFY experimentChanged)
    // Schaltplan des Aufbaus nach DIN EN 61082-1 (model/schematic_layout), in Bildpunkten der Zeichnung:
    // {width, height, nodes: [{label, anchors, drawn, x, x2, y, path}], dots: [{x, y, node}],
    //  elements: [{id, type, nodes, desk, module, value, x, y, rotation, block, w, h, labelX, labelY, pinLabels}]}
    Q_PROPERTY(QVariantMap schematic READ schematic NOTIFY boardChanged)
    // Schaltplanansicht verfuegbar: freies Labor, ab Stufe 2 oder alles freigegeben (Konzept 5.7), ab Volta 2 immer
    Q_PROPERTY(bool schematicAvailable READ schematicAvailable NOTIFY progressChanged)
    // Zeitschreiber (model::Recorder): ab Stufe 3, im freien Labor oder alles freigegeben; Kanaele ["B4", "MESS", ""]
    Q_PROPERTY(bool recorderAvailable READ recorderAvailable NOTIFY progressChanged)
    Q_PROPERTY(QStringList recorderChannels READ recorderChannels NOTIFY recorderChanged)
    Q_PROPERTY(double recorderWindow READ recorderWindow WRITE setRecorderWindow NOTIFY recorderChanged)
    Q_PROPERTY(bool recorderPaused READ recorderPaused WRITE setRecorderPaused NOTIFY recorderChanged)
    QVariantList lexicon() const;
    QVariantMap schematic() const;
    // Volta 1 ab Stufe 2; ab Volta 2 von Anfang an (die Kinder kennen ihn aus Volta 1)
    bool schematicAvailable() const {
        return m_unlockAll || m_freeLab || m_experiment.stage >= 2 || (!m_band.isEmpty() && m_band != QLatin1String("volta1"));
    }
    // Volta 1: Zeitschreiber ab Stufe 3; ab Volta 2 das Oszilloskop von Anfang an (Tastkoepfe ab Stufe 0)
    bool recorderAvailable() const {
        return m_unlockAll || m_freeLab || m_experiment.stage >= 3 || (!m_band.isEmpty() && m_band != QLatin1String("volta1"));
    }
    QStringList recorderChannels() const;
    double recorderWindow() const { return m_recorder.window(); }
    void setRecorderWindow(double seconds);
    bool recorderPaused() const { return m_recorder.paused(); }
    void setRecorderPaused(bool paused);
    // Kanal 0/1 an einen Anschlusspunkt ("B4") bzw. das Messgeraet ("MESS") anschliessen; "" trennt
    Q_INVOKABLE void setRecorderChannel(int channel, const QString& source);
    Q_INVOKABLE void clearRecorder();
    // Darstellung: {window, channels: [{source, unit, points: [t0, v0, t1, v1, ...], min, max, any, last}]}
    Q_INVOKABLE QVariantMap recorderData(int maxPoints) const;
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
    int cells2() const;
    void setCells2(int cells);
    int acRms() const { return m_session->board().desk.acRmsV; }
    void setAcRms(int volt);
    int genWaveform() const { return m_session->board().desk.genWaveform; }
    void setGenWaveform(int waveform);
    double genFrequency() const { return m_session->board().desk.genFrequencyHz; }
    void setGenFrequency(double hz);
    double genAmplitude() const { return m_session->board().desk.genAmplitudeV; }
    void setGenAmplitude(double peakV);
    double genOffset() const { return m_session->board().desk.genOffsetV; }
    void setGenOffset(double volt);
    double genAmDepth() const { return m_session->board().desk.genAmDepth; }
    void setGenAmDepth(double depth);
    double genAmTone() const { return m_session->board().desk.genAmToneHz; }
    void setGenAmTone(double hz);
    bool deskOnDemand() const { return !m_deskOffers.isEmpty(); }
    int startColumns() const { return m_startColumns; }
    QVariantList deskTray() const;
    Q_INVOKABLE bool addDeskPart(const QString& part);    // Befehl
    Q_INVOKABLE bool showWholeDesk(); // »Pult zeigen«: alles Freigeschaltete in einem Befehl ins Pult
    Q_INVOKABLE bool removeDeskPart(const QString& part); // Befehl; Leitungen an seinen Feldern gehen mit
    // Leitungen an den Feldern eines Pultteils (Rueckfrage vor dem Herausnehmen)
    Q_INVOKABLE int deskPartWires(const QString& part) const;
    double pot1() const { return m_session->board().desk.p1; }
    void setPot1(double v) { setPot(1, v); }
    double pot2() const { return m_session->board().desk.p2; }
    void setPot2(double v) { setPot(2, v); }
    bool deskSwitch() const { return m_session->board().desk.switchClosed; }
    bool meterVisible() const { return m_session->board().desk.meterVisible; }
    QVariantMap task() const; // aktuelle Aufgabe wie readTask (leer: keine)
    bool presentation() const { return m_presentation; }
    void setPresentation(bool on);
    bool demo() const { return m_demo; }
    // Experiment mit Musteraufbau starten und einschalten (beim ersten Aufruf auf leeren Datenordner umschalten)
    Q_INVOKABLE bool startDemo(const QString& experimentId);
    Q_INVOKABLE void endDemo(); // Vorfuehrung verwerfen, Profil wieder laden
    bool autoColumns() const { return m_autoColumns; }
    void setAutoColumns(bool on);
    bool stageLayout() const { return m_stageLayout; }
    void setStageLayout(bool on);
    QStringList deskGroups() const;
    bool moduleBarVisible() const;
    bool stageLayoutActive() const; // Stufenanpassung gilt fuer den aktuellen Aufbau (gefuehrtes Experiment)
    // Groesse des Steckbretts fuer eine Stufe, vergroessert, bis der Aufbau hineinpasst
    std::pair<int, int> stageBoardSize(int stage, const model::Board& mustFit) const;
    bool meterDocked() const { return m_meterDocked; }
    void setMeterDocked(bool docked);
    // Breite nach der Automatik fuer einen Aufbau (Vorlage bzw. eigener Aufbau muss hineinpassen)
    int autoColumnsFor(const model::Board& board, bool freeLab, bool meterVisible) const;
    void applyAutoColumns(); // aktuellen Aufbau anpassen (Befehl wie +/-), nur wenn die Automatik an ist
    void setMeterVisible(bool visible);
    int meterMode() const { return static_cast<int>(m_session->board().desk.meterMode); }
    void setMeterMode(int mode);
    bool meterAnalog() const { return m_session->board().desk.meterAnalog; }
    void setMeterAnalog(bool analog);
    double simTime() const;
    bool showSimTime() const { return qEnvironmentVariableIntValue("VOLTA_SHOW_TIME") == 1; }
    double timeScale() const { return m_timeScale; }
    void setTimeScale(double scale);
    double realtimeRatio() const;
    double batteryVolt() const { return 1.5 * cells(); }

    // Vorlage eines Experiments als Aufbau laden (ohne Ablauf; Entwicklerhilfen, Tests)
    Q_INVOKABLE bool loadExperiment(const QString& id);
    // Gefuehrtes Experiment: leere Grundplatte, Pultstellung der Vorlage, Ablauf beginnt mit der Frage
    Q_INVOKABLE bool startExperiment(const QString& id);
    Q_INVOKABLE void startFreeLab();
    // Sprache der Inhalte (Experimenttexte, Titel); Oberflaeche: LanguageService
    Q_INVOKABLE void setContentLanguage(const QString& code);
    // Programminfo fuer das Info-Fenster: name, version, qtVersion, compiler, platform, hasLogo
    Q_INVOKABLE QVariantMap appInfo() const;
    // Seite »Lizenzen«: Fremdbestandteile dieser Plattform (name, version, license, copyright, texts, modules) und
    // die Lizenztexte im Original (tools/make_licenses.py)
    Q_INVOKABLE QVariantList licenses() const;
    Q_INVOKABLE QString licenseText(const QString& id) const;
    Q_INVOKABLE void chooseGuess(int index);
    Q_INVOKABLE void nextHint();
    Q_INVOKABLE void chooseObservation(int index);
    Q_INVOKABLE bool addNote(const QString& text); // Notiz zum laufenden Experiment ins Forscherheft
    QStringList observations() const { return texts(m_experiment.observationKeys); }
    int observation() const { return m_flow.observation(); }
    int comparison() const { return static_cast<int>(m_flow.comparison()); }
    QVariantList journal() const;
    void useJournalFile(const QString& path);
    Q_INVOKABLE bool showTemplate(); // Musteraufbau (Befehl, rueckgaengig machbar) - erst nach der Loesungshilfe
    // Experiment abschliessen (»Fertig«): weiter zur Beobachtung; Lernpfad: erledigt
    Q_INVOKABLE bool finishExperiment();
    bool buildComplete() const { return m_buildComplete; }
    bool canFinish() const { return m_canFinish; }

    QVariantList experiments() const { return m_experimentList; }
    QString experimentId() const { return m_experiment.id; }
    bool freeLab() const { return m_freeLab; }
    QVariantList templateParts() const;
    int experimentStage() const { return m_freeLab || m_experiment.id.isEmpty() ? -1 : m_experiment.stage; }
    int phase() const { return static_cast<int>(m_flow.phase()); }
    bool destructive() const { return m_experiment.destructive; }
    bool experimentScope() const { return !m_freeLab && m_experiment.scopeShown; }
    QString question() const { return m_repo.text(m_experiment.questionKey); }
    QStringList guesses() const { return texts(m_experiment.guessKeys); }
    int guess() const { return m_flow.guess(); }
    QStringList steps() const { return texts(m_experiment.stepKeys); }
    QVariantMap adoptOffer() const;
    Q_INVOKABLE bool adoptSetup(); // Befehl
    QStringList hints() const { return texts(m_experiment.hintKeys.mid(0, m_flow.hintsShown())); }
    bool moreHints() const { return m_flow.hintsShown() < m_flow.hintCount(); }
    bool solutionShown() const { return m_flow.solutionShown(); }
    QString explanation() const { return m_repo.text(m_experiment.explanationKey); }
    QString moreKnowledge() const { return m_repo.text(m_experiment.moreKey); }
    QStringList further() const { return texts(m_experiment.furtherKeys); }
    QString realLife() const { return m_repo.text(m_experiment.realKey); }
    QString nextExperimentId() const;
    bool showNumbers() const { return m_experiment.id.isEmpty() || m_experiment.stage >= 3; }
    Q_INVOKABLE double voltage(const QString& anchor) const;
    Q_INVOKABLE int voltageBars(const QString& anchor);
    // {bars, negative, ac}: Spannungsanzeige mit Vorzeichen und Wechselspannung (ab Volta 2)
    Q_INVOKABLE QVariantMap voltageDisplay(const QString& anchor);
    // Zustand eines Bauteils: currentmA, level, overloaded, destroyed, tempC
    Q_INVOKABLE QVariantMap partState(const QString& id) const;
    // Zustand eines Moduls fuer die Darstellung: active (Ausgang ein bzw. spielt), frequencyHz, level, note
    Q_INVOKABLE QVariantMap moduleState(const QString& id) const;
    // »Blick hinein«: {type, behavioral, items: [{name, kind, currentmA, voltV, on}], supplyV, inV, outmA, note,
    // frequencyHz, playing}. Teilschaltung: Messwerte der inneren Bauteile; Verhaltensmodell: Ein- und Ausgaenge.
    Q_INVOKABLE QVariantMap moduleInside(const QString& id) const;
    // Strom einer Leitung in mA (Stromflussdarstellung)
    Q_INVOKABLE double wireCurrent(const QString& id) const;

    // Befehle (Verlauf). vertical/longSpan: Lage des Bauteils ab dem Startfeld.
    Q_INVOKABLE bool addPart(const QString& type, const QString& startField, bool vertical, bool longSpan);
    // Steckvorschau beim Ziehen aus dem Fach: {pins: [{name, field}], fields: [...], ok}
    Q_INVOKABLE QVariantMap partPreview(const QString& type, const QString& startField, bool vertical, bool longSpan) const;
    // Leitungsfuehrung um die Bauteile herum (model/wire_router): bounds Arbeitsflaeche, obstacles [rect] der
    // Bauteilkoerper, wires [{from, to}]; Ergebnis je Leitung {points: [point], arc: bool}
    Q_INVOKABLE QVariantList routeWires(const QRectF& bounds, const QVariantList& obstacles, const QVariantList& wires) const;
    Q_INVOKABLE bool movePart(const QString& id, const QString& startField, bool vertical, bool longSpan);
    Q_INVOKABLE bool removePart(const QString& id);
    Q_INVOKABLE bool flipPart(const QString& id);
    Q_INVOKABLE bool rotatePart(const QString& id); // 2-x-2-Platine: 90 Grad im Uhrzeigersinn
    Q_INVOKABLE bool setPartValue(const QString& id, double value);
    Q_INVOKABLE bool repairPart(const QString& id);
    Q_INVOKABLE bool addWire(const QString& from, const QString& to, const QString& color);
    // Farbe einer neuen Leitung von from (nach to, darf leer sein): mit Farbautomatik rot am Plus-, schwarz am
    // Minuspol (auch ueber Leitungen verbunden), sonst fallback (Lieblingsfarbe)
    Q_INVOKABLE QString wireColorFor(const QString& from, const QString& to, const QString& fallback) const;
    Q_INVOKABLE bool moveWireEnd(const QString& id, bool fromEnd, const QString& anchor);
    Q_INVOKABLE bool removeWire(const QString& id);
    Q_INVOKABLE bool setWireColor(const QString& id, const QString& color);
    Q_INVOKABLE bool toggleVoltageDisplay(const QString& field);
    Q_INVOKABLE bool setBoardColumns(int columns); // Befehl; +/- in der Werkzeugleiste
    // Module (Befehle). slotAnchor: Feld der Modulleiste, auf dem der erste Anschluss liegt ("M:3")
    Q_INVOKABLE bool addModule(const QString& type, const QString& slotAnchor);
    Q_INVOKABLE bool moveModule(const QString& id, const QString& slotAnchor);
    Q_INVOKABLE bool removeModule(const QString& id);
    // Fehlersuche (Werkzeugkasten): [{kind, part, anchors[], text}] - Fragen, wichtigste zuerst
    Q_INVOKABLE QVariantList checkCircuit() const;
    // Anzahl der gerade klingenden Summer/Piezo-Schallgeber (Tonausgabe, Tests)
    Q_INVOKABLE int soundingParts() const;
    // Teileliste und Aufbaukarte (Werkzeugkasten): im Experiment die Vorlage, im freien Labor der eigene Aufbau.
    // partsList: kind ("modul", "bauteil", "leitung", "werkzeug", "batterie"), type, value, count, ids.
    Q_INVOKABLE QVariantList partsList() const;
    // alle Aufbauschritte wie guideStep (ohne Hauptschalter), fuer die Karte
    Q_INVOKABLE QVariantList buildSteps() const;
    // Vorschlag fuer den Dateinamen, z. B. "Volta1-Aufbaukarte-1.4.pdf"
    Q_INVOKABLE QString buildCardFileName() const;
    // PDF schreiben; texts: Felder von services::BuildCardTexts (fertig uebersetzt), labels als Map
    Q_INVOKABLE bool exportBuildCard(const QUrl& target, const QVariantMap& texts);
    // Muelleimer ohne Auswahl: alle Bauteile, Leitungen und Module abbauen (ein Befehl, rueckgaengig machbar)
    Q_INVOKABLE bool clearBoard();
    int boardColumns() const { return m_session->board().columns; }
    int minBoardColumns() const;
    int boardRows() const { return m_session->board().rows; }
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

    // Bedienhandlungen
    Q_INVOKABLE void setPartClosed(const QString& id, bool closed);
    Q_INVOKABLE void setPartPosition(const QString& id, double position); // Trimmer (ab Volta 2)
    Q_INVOKABLE void setDeskSwitch(bool closed);
    Q_INVOKABLE void setPot(int which, double position);
    Q_INVOKABLE void setMotorBlocked(const QString& id, bool blocked);
    int roomLight() const { return m_session->board().desk.roomLight; }
    void setRoomLight(int level);
    QVariantList tools() const;
    Q_INVOKABLE void placeTool(const QString& type, double col, double row);
    Q_INVOKABLE void setToolOn(const QString& type, bool on);
    Q_INVOKABLE void removeTool(const QString& type);
    // Anzeige des Messgeraets: text, unit (Dezimalzeichen der Sprache), level (Zeiger), range, mode,
    // overrange, foreignVoltage, fuseBlown, valid
    Q_INVOKABLE QVariantMap meterState();
    // Anzeigegeraet: gesteckte Messstellen [{id, label, kind, tag}], Zustand einer Anzeige, Leistung
    Q_INVOKABLE QVariantList measurePoints() const;
    Q_INVOKABLE QVariantMap displayState(int slot) const;
    Q_INVOKABLE QVariantMap displayPowerState() const;
    Q_INVOKABLE void setDisplaySource(int slot, const QString& id);
    Q_INVOKABLE void setDisplayRange(int slot, int range);
    Q_INVOKABLE void setDisplayAc(int slot, bool ac);
    // Frequenzzaehler (ab Volta 3, nur Spannung): 0 aus, 1 Torzeit 0,1 s, 2 Torzeit 1 s
    Q_INVOKABLE void setDisplayGate(int slot, int gate);
    // Oszilloskop ab Volta 2: Bild samt Einstellungen und Kennwerten; Einstellung je Schluessel (timebase, roll, xy,
    // triggerChannel, triggerRising, triggerLevel, volts0/1, position0/1); scopeActive = eingeblendet
    Q_INVOKABLE QVariantMap scopeState() const;
    Q_INVOKABLE void setScope(const QString& key, const QVariant& value);
    // Kennlinienschreiber (ab Volta 2): Einstellungen (voltsIndex, bipolar, limitIndex, Texte), Achsen in V und A,
    // Kurve des letzten Durchlaufs als flache Liste (x, y je 0 ... 1 im Schirm) und der aktuelle Punkt.
    // setTracer: Schluessel volts, bipolar, limit (Bedienhandlung)
    Q_INVOKABLE QVariantMap tracerState() const;
    // Logikanalysator (ab Volta 3): {fine, windowText, highText, lowText, channels: [{label, has, level (0 L,
    // 1 ?, 2 H), segs (flach: x0, x1, Pegel je Abschnitt, x 0 ... 1 im Fenster)}]}; setLogicFine: Zeitfenster
    Q_INVOKABLE QVariantMap logicState() const;
    Q_INVOKABLE void setLogicFine(bool fine);
    // Frequenzgang-Schreiber (ab Volta 3): Generator -> Tastkopf K1. bodeAvailable: Volta 3 mit Generator im Pult.
    // bodeState: {status ("ok", "nogen", "noprobe", "off", "switching", "wait", "error"), switching: [Typen, die
    // schalten], curve/phase (flach: x 0 ... 1 ueber 1 Hz ... 10 MHz logarithmisch, y 0 ... 1 von unten), topDb,
    // bottomDb, kind, markers [{x, kind ("res", "cut"), text}], resonanceText, cutLowText, cutHighText,
    // bandwidthText, qualityText, maxDbText}; requestBode: neuen Durchlauf anfordern (zwischen zwei Schritten)
    Q_PROPERTY(bool bodeAvailable READ bodeAvailable NOTIFY deskChanged)
    bool bodeAvailable() const;
    Q_INVOKABLE QVariantMap bodeState() const;
    Q_INVOKABLE void requestBode();
    double logicSupplyV() const; // Bezug der Logikschwellen: Batterie des Pults
    // Senderanlage (ab Volta 3): Sender 0..2 an/aus (Bedienhandlung)
    Q_INVOKABLE bool stationOn(int index) const;
    Q_INVOKABLE void setStationOn(int index, bool on);
    Q_INVOKABLE void setTracer(const QString& key, const QVariant& value);
    Q_PROPERTY(bool scopeActive READ scopeActive WRITE setScopeActive NOTIFY deskChanged)
    bool scopeActive() const;
    void setScopeActive(bool active);
    int displayCount() const;
    void setDisplayCount(int count);
    bool displayPower() const;
    void setDisplayPower(bool on);
    Q_INVOKABLE bool replaceMeterFuse(); // Befehl
    // Klanganzeige: frequencyHz, level 0..1, pulses (Zaehler), overloaded, destroyed
    Q_INVOKABLE QVariantMap speakerState() const;
    Q_INVOKABLE bool repairSpeaker(); // Befehl
    // Drehregler P1/P2 des Pults: {destroyed, overloaded} (leer ohne gueltige Messung); Reparieren ist ein Befehl
    Q_INVOKABLE QVariantMap potState(int which) const;
    Q_INVOKABLE bool repairPot(int which);

    bool guideActive() const { return m_guideActive; }
    void setGuideActive(bool on);
    QVariantMap guideStep() const;
    QVariantMap stepMap(const model::GuideStep& s, const model::Board& board) const;
    const model::Board& cardBoard() const; // Aufbau fuer Teileliste und Aufbaukarte

    model::LabSession& session() { return *m_session; }
    // Tonausgabe (main.cpp); ohne Dienst (Tests) bleibt es still
    void setAudio(services::AudioService* audio);
    QString toneMode() const { return m_toneMode; }

signals:
    void toneModeChanged();
    void experimentChanged();
    void adoptChanged();
    void boardChanged();
    void deskChanged();
    void stateChanged();
    void partDestroyed(const QString& id, const QString& type); // ein Bauteil ist gerade kaputt gegangen
    void experimentSolved(); // »Fertig« mit funktionierendem Aufbau (Glueckwunsch)
    void historyChanged();
    void editFailed();
    void flowChanged();
    void progressChanged();
    void journalChanged();
    void columnsModeChanged();
    void columnsChosen(int columns); // vom Kind gewaehlte Breite (zum Merken in den Einstellungen)
    void guideChanged();
    void recorderChanged();
    void finishChanged();
    void profilesChanged();
    // Volta wurde mit einem Fortschritts-Link geoeffnet (QR-Code ueber die Kamera-App): Rueckfrage in der Oberflaeche
    void progressLinkReceived(const QString& link);
    void taskLinkReceived(const QString& link);
    void taskChanged();
    void presentationChanged();
    void demoChanged();

public slots:
    // Link-Handler fuer das Schema volta1 (QDesktopServices::setUrlHandler in main.cpp)
    void handleUrl(const QUrl& url);

private:
    void updateFinish();
    void onFrame();
    bool result(bool ok);
    QStringList texts(const QStringList& keys) const;
    void rebuildExperimentList();

    services::ContentRepository m_repo;
    QString m_band; // Band der Edition, z. B. "volta1"
    QList<services::DeskOffer> m_deskOffers; // Geraetefach; leer: festes Pult (Volta 1)
    int m_startColumns = 0;                  // ab Volta 2: 5 x 10 von Anfang an (Konzept Volta 2, 9.4)
    // Aufbau laden; im Pult nach Bedarf ohne gespeicherte Pultbelegung das, was der Aufbau benutzt
    void loadSessionBoard(model::Board board);
    bool deskOfferOpen(const services::DeskOffer& offer) const;
    void adaptDesk(model::Board& board) const;
    std::unique_ptr<model::LabSession> m_session;
    QString m_title;
    services::Experiment m_experiment;
    model::ExperimentFlow m_flow;
    QStringList m_experimentIds;
    QVariantList m_experimentList;
    QList<services::Stage> m_stages;
    services::ProgressStore m_progress;
    services::SessionStore m_sessionStore;
    QTimer m_saveTimer;      // speichert kurz nach einer Aenderung
    bool m_saveEnabled = false;
    bool m_labOpen = false;
    bool m_restoring = false;
    QString m_dataRoot;
    services::ProfileStore m_profiles;
    bool m_profilesEnabled = false;
    int m_profileLimit = services::ProfileStore::kMaxProfiles;
    int m_managedStage = -1;
    QString m_currentProfile;
    QString m_dataDir; // aktiver Datenordner (Profil oder ohne Profil); leer: nur im Speicher
    QString band() const;
    // Fortschritt, Forscherheft und Laborstand aus diesem Ordner (leer: nur im Speicher); Labor leer
    void activateDataDir(const QString& dir, bool saveFirst = true);
    services::JournalStore m_journal;
    bool m_unlockAll = false;
    bool m_presentation = false;
    QString m_presentationDir;      // Datenordner vor der Praesentation
    bool m_presentationUnlock = false;
    bool m_demo = false;
    QString m_demoDir;              // Datenordner vor der Vorfuehrung
    bool m_demoUnlock = false;
    bool m_buildComplete = false;
    bool m_canFinish = false;
    bool m_guideActive = false;
    model::Recorder m_recorder;
    void sampleRecorder();
    bool m_autoWireColor = true;
    bool m_autoColumns = false; // die App bindet die Einstellung (Vorgabe dort: an)
    bool m_stageLayout = false; // die App bindet die Einstellung (Vorgabe dort: an)
    bool m_meterDocked = false;
    services::AudioService* m_audio = nullptr;
    double m_lastPulses = -1.0; // Impulszaehler des Pult-Lautsprechers (Knacken bei neuem Impuls)
    void updateAudio(double elapsedS);
    bool speakerWired() const;
    QString m_toneMode;
    bool m_toneFallback = false; // Ersatzton statt hoerbarer Rechnung
    double m_slowS = 0.0;        // so lange schon langsamer als Echtzeit
    double m_fastS = 0.0;        // so lange schon wieder in Echtzeit
    int m_preferredColumns = 12;
    bool m_preferMeter = false;
    int highestUnlockedStage() const;
    QString m_contentLanguage = QStringLiteral("de");
    bool m_freeLab = false;
    QStringList m_errors;
    QString m_lastEditError;
    double m_timeScale = 1.0;
    QTimer m_timer;
    QElapsedTimer m_clock;
    double m_sinceNotifyS = 0.0;
    QSet<QString> m_destroyedSeen; // bereits gemeldete kaputte Bauteile (geheilt: wieder meldbar)
    void checkDestroyed();
    // Oszilloskop »Auto« fuer die Zeit: sucht die Periode; ohne erkennbare Periode laengere Zeitbasis und neu messen,
    // sobald der Puffer gefuellt ist (hoechstens kScopeAutoTries Durchgaenge)
    QTimer m_scopeAutoTimer;
    int m_scopeAutoTries = 0;
    void scopeAutoTimeStep();
    // Aufbau des frueheren Experiments (nur der Teil, den die Vorlage auch hat), beim Wechsel des Experiments geladen
    QString m_adoptFor;
    QString m_adoptTitleKey;
    model::Board m_adoptBoard;
    void loadAdoptSource();
    bool scopeBufferMeasure(std::array<model::ScopeMeasure, model::kScopeChannels>& m) const;
    QString m_taskLink; // aktuelle Aufgabe als Link (aufgabe.txt im Profil)
    QVariantMap taskMap(const services::task::Task& t) const;
    void loadTask();
    std::vector<model::SynthVoice> buzzerVoices() const;
    double m_meterRangeV = 30.0; // zuletzt gelieferter Analogbereich (fuer die Skala)
};

} // namespace volta
