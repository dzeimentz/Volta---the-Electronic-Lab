// Einstiegspunkt der Qt-Anwendung. Modell, Dienste und Lernpfad folgen gemaess docs/architecture.md.
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include "services/audio_route.hpp"
#include "services/audio_service.hpp"
#include "services/managed_config.hpp"
#include "services/speech_service.hpp"
#include "lab_controller.hpp"
#include "app_settings.hpp"
#include "language_service.hpp"
#include "volta/core/version.hpp"

Q_IMPORT_QML_PLUGIN(VoltaPlugin)

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    // Tonausgabe ueber das Windows-eigene Backend (ohne FFmpeg, docs/adr/0009-tonausgabe-und-vorlesen.md)
    if (qEnvironmentVariableIsEmpty("QT_MEDIA_BACKEND")) qputenv("QT_MEDIA_BACKEND", "windows");
#endif
    QGuiApplication app(argc, argv);
#ifdef Q_OS_ANDROID
    // Android: Symbolzeichen (Menue U+2630, Dreiecke U+25BE/U+25B4, Kreuz U+2715, Pfeile U+21B6/7) liegen nur in den
    // Symbolschriften des Systems, die Qt nicht als Ersatzschrift nutzt (sonst leere Kaestchen). Die Dateien direkt
    // laden und ihre Familien als Ersatz fuer Symbole eintragen.
    for (const char* file : {"/system/fonts/NotoSansSymbols-Regular-Subsetted.ttf",
                             "/system/fonts/NotoSansSymbols-Regular-Subsetted2.ttf"}) {
        const int id = QFontDatabase::addApplicationFont(QString::fromLatin1(file));
        for (const QString& family : QFontDatabase::applicationFontFamilies(id)) {
            QFontDatabase::addApplicationFallbackFontFamily(QChar::Script_Common, family);
            qInfo("Volta: Ersatzschrift %s (%s)", qPrintable(family), file);
        }
    }
#endif
    QGuiApplication::setOrganizationName(QStringLiteral("DC Software Engineering"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("dc-software-engineering.com"));
    QGuiApplication::setApplicationName(QStringLiteral(VOLTA_APP_NAME));
    QGuiApplication::setApplicationVersion(QStringLiteral(VOLTA_VERSION));
    QIcon icon; // Fenster- und Taskleistensymbol (Rasterbilder, kein SVG-Plugin noetig)
    for (int size : {32, 64, 256}) icon.addFile(QStringLiteral(":/icons/volta_%1.png").arg(size), QSize(size, size));
    QGuiApplication::setWindowIcon(icon);

    // Inhaltspakete: eingebettet (:/content, Android; VOLTA_EMBED_CONTENT), sonst neben der ausfuehrbaren Datei
    // (Release-Ordner, tools/deploy_release.py), sonst aus dem Quellbaum
#ifdef VOLTA_EMBEDDED_CONTENT
    const QString contentDir = QStringLiteral(":/content");
#else
    const QString besideExe = QCoreApplication::applicationDirPath() + QStringLiteral("/content");
    const bool deployed = QFileInfo::exists(besideExe + QStringLiteral("/catalog/bauteile.json"));
    const QString contentDir = deployed ? besideExe : QStringLiteral(VOLTA_CONTENT_DIR);
#endif
    volta::LabController lab(contentDir);
    // Fortschritt im Lernpfad lokal speichern; Entwicklerhilfen (VOLTA_SCREENSHOT ...) lassen ihn unberuehrt
    if (qEnvironmentVariable("VOLTA_SCREENSHOT").isEmpty()) {
        // Fortschritt, Forscherheft, Laborstand und Forscherprofile im privaten App-Speicher
        const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dataDir);
        lab.useDataRoot(dataDir);
    }

    // Stil Basic fest (auch fuer den Dateidialog): andere Controls-Stile werden nicht ausgeliefert
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("labController"), &lab);
    // Anzeigesprache: Oberflaeche (Uebersetzung, retranslate) und Inhalte (Experimenttexte) gemeinsam umschalten
    // Entwicklerhilfe: VOLTA_DEMO_LANG=fr|en startet in dieser Sprache, ohne die gespeicherte Wahl zu aendern
    const QString demoLang = qEnvironmentVariable("VOLTA_DEMO_LANG");
    volta::LanguageService language(&engine, demoLang.isEmpty());
    if (!demoLang.isEmpty()) language.setLanguage(demoLang);
    lab.setContentLanguage(language.language());
    QObject::connect(&language, &volta::LanguageService::languageChanged, &lab,
                     &volta::LabController::setContentLanguage);
    engine.rootContext()->setContextProperty(QStringLiteral("volta_languageService"), &language);
    // Einstellungen (Menue oben rechts), lokal gespeichert; Entwicklerhilfen aendern sie nicht
    // Entwicklerhilfe: VOLTA_DEMO_FONT=0|1|2 Schriftstufe (ohne Speichern)
    const QString demoFont = qEnvironmentVariable("VOLTA_DEMO_FONT");
    // VOLTA_DEMO_METER_DOCKED=0|1: Messgeraet neben der Grundplatte bzw. oben im Pult (ohne Speichern)
    const QString demoDocked = qEnvironmentVariable("VOLTA_DEMO_METER_DOCKED");
    // VOLTA_DEMO_PROFILES=1: Forscherprofile mit drei Beispielprofilen (Profilauswahl, ohne Speichern)
    const bool demoProfiles = qEnvironmentVariableIntValue("VOLTA_DEMO_PROFILES") == 1;
    volta::AppSettings settings(demoLang.isEmpty() && demoFont.isEmpty() && demoDocked.isEmpty() && !demoProfiles);
    if (demoProfiles) {
        settings.setProfilesEnabled(true);
        lab.addProfile(QStringLiteral("🦊"), QStringLiteral("Funkenfuchs"));
        lab.addProfile(QStringLiteral("🚀"), QStringLiteral("Raketa"));
        lab.addProfile(QStringLiteral("🐢"), QStringLiteral("Turbo"));
    }
    if (!demoFont.isEmpty()) settings.setFontLevel(demoFont.toInt());
    if (!demoDocked.isEmpty()) settings.setMeterDocked(demoDocked.toInt() == 1);
    engine.rootContext()->setContextProperty(QStringLiteral("volta_settings"), &settings);
    // Tonausgabe und Vorlesen (Bildschirmfoto-Entwicklerhilfe: still)
    volta::services::AudioService audio;
    if (!qEnvironmentVariableIsEmpty("VOLTA_SCREENSHOT")) audio.setSuppressed(true);
    lab.setAudio(&audio);
    volta::services::SpeechService speech;
    // Verwaltete Konfiguration der Schule und Kopfhoerer-Erkennung (Klassenmodus Ton)
    volta::services::ManagedConfig managed;
    volta::services::AudioRoute audioRoute;
    if (managed.unlockStage() >= 0) lab.setManagedUnlockStage(managed.unlockStage());
    if (managed.profiles() > 0) lab.setProfileLimit(managed.profiles());
    if (!managed.language().isEmpty() && (managed.languageLocked() || !language.hasSavedChoice()))
        language.setLanguage(managed.language());
    engine.rootContext()->setContextProperty(QStringLiteral("volta_managed"), &managed);
    engine.rootContext()->setContextProperty(QStringLiteral("volta_audioRoute"), &audioRoute);
    engine.rootContext()->setContextProperty(QStringLiteral("volta_audio"), &audio);
    engine.rootContext()->setContextProperty(QStringLiteral("volta_speech"), &speech);
    // Startbildschirm mit Herstellerlogo in der Desktop-Ausgabe (Windows/Linux); abschaltbar in den Einstellungen
    // oder mit VOLTA_NO_SPLASH=1
    const bool splash = volta::AppSettings::splashAvailable() && settings.showSplash() &&
                        qEnvironmentVariableIntValue("VOLTA_NO_SPLASH") != 1;
    engine.rootContext()->setContextProperty(QStringLiteral("volta_showSplash"), splash);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
    engine.loadFromModule("Volta", "Main");
    // Entwicklerhilfen, die einen Aufbau zeigen, oeffnen direkt das Labor statt der Startseite
    // VOLTA_DEMO_FREELAB=1: freies Labor (Sichtpruefung des Bauteilfachs)
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_FREELAB") == 1) lab.startFreeLab();
    for (const char* demo : {"VOLTA_DEMO_GALLERY", "VOLTA_DEMO_SOLVE", "VOLTA_DEMO_METER", "VOLTA_DEMO_COLUMNS",
                             "VOLTA_DEMO_FREELAB", "VOLTA_DEMO_GUIDE", "VOLTA_DEMO_SENSORS", "VOLTA_DEMO_RECORDER"})
        if (!qEnvironmentVariable(demo).isEmpty() && !engine.rootObjects().isEmpty())
            engine.rootObjects().first()->setProperty("inLab", true);

    // Entwicklerhilfe: VOLTA_SCREENSHOT=<datei.png> speichert nach 3 s ein Bild des Fensters und beendet die App
    // (Sichtpruefung ohne Bedienung). Optional VOLTA_SCREENSHOT_MAINSWITCH=1 schaltet vorher den Hauptschalter ein.
    // Entwicklerhilfe: VOLTA_DEMO_GALLERY=1 legt eine Mustertafel mit allen Bauteiltypen im Betrieb an (Sichtpruefung
    // der Gestaltung: Leuchten, Drehen, Toenen, Ueberlast, Zerstoerung).
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_GALLERY") == 1) {
        QTimer::singleShot(200, &lab, [&lab] {
            lab.loadExperiment(QStringLiteral("volta1.2.4"));
            lab.removePart(QStringLiteral("D1"));
            lab.addPart(QStringLiteral("led_gruen"), QStringLiteral("B4"), true, false); // 470 Ohm -> leuchtet
            lab.addWire(QStringLiteral("C4"), QStringLiteral("P:0V"), QStringLiteral("schwarz"));
            struct P { const char* type; const char* field; bool vertical; bool longSpan; };
            const P parts[] = {{"gluehlampe", "A1", false, false}, {"motor", "A4", false, true},
                               {"summer", "C1", false, false},     {"taster", "C6", false, false},
                               {"reedkontakt", "D1", false, false}, {"wechselschalter", "D4", false, false},
                               {"kondensator", "E1", false, false}, {"elko", "E3", false, false},
                               {"diode_si", "E6", false, false},     {"transistor_npn", "D9", false, false},
                               {"led_blau", "D7", true, false},      {"led_rot", "A7", true, false}};
            for (const auto& p : parts)
                lab.addPart(QString::fromLatin1(p.type), QString::fromLatin1(p.field), p.vertical, p.longSpan);
            const char* wires[][2] = {{"P:+9V", "A1"}, {"A2", "P:0V"}, {"P:+9V", "A4"}, {"A6", "P:0V"},
                                      {"P:+9V", "C1"}, {"C2", "P:0V"}, {"P:+9V", "A7"}, {"B7", "P:0V"}};
            for (const auto& w : wires)
                lab.addWire(QString::fromLatin1(w[0]), QString::fromLatin1(w[1]), QStringLiteral("blau"));
            lab.rotatePart(QStringLiteral("T1")); // Platine gedreht (B oben rechts, E oben links)
            lab.setPartClosed(QStringLiteral("RK1"), true);
            lab.setMainSwitch(true);
        });
    }

    // Entwicklerhilfe: VOLTA_DEMO_SOLVE=1 loest das gefuehrte Experiment mit Vermutung, allen Hilfen und Musteraufbau
    // (Sichtpruefung der Phasen Aufbau und Erfolg). VOLTA_SCREENSHOT_S=<s> verschiebt das Bildschirmfoto (Standard 3 s).
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_SOLVE") == 1) {
        // VOLTA_DEMO_EXP=<id> waehlt das Experiment (Standard volta1.1.1)
        const QString exp = qEnvironmentVariable("VOLTA_DEMO_EXP", QStringLiteral("volta1.1.1"));
        QTimer::singleShot(300, &lab, [&lab, exp] {
            lab.setUnlockAll(true);
            lab.startExperiment(exp);
            lab.chooseGuess(1);
            while (lab.moreHints()) lab.nextHint();
            lab.showTemplate();
            lab.setMainSwitch(true);
        });
    }

    // Entwicklerhilfe: VOLTA_DEMO_GUIDE=1 startet das Experiment VOLTA_DEMO_EXP mit eingeschalteter Aufbauhilfe
    // (Sichtpruefung von Zielfeldern und Geisterbild); VOLTA_DEMO_GUIDE_ADD=<Typ>@<Feld> steckt vorher ein Bauteil
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_GUIDE") == 1) {
        const QString exp = qEnvironmentVariable("VOLTA_DEMO_EXP", QStringLiteral("volta1.1.1"));
        const QString add = qEnvironmentVariable("VOLTA_DEMO_GUIDE_ADD");
        QTimer::singleShot(300, &lab, [&lab, exp, add] {
            lab.setUnlockAll(true);
            lab.startExperiment(exp);
            lab.chooseGuess(0);
            lab.setGuideActive(true);
            if (add.contains(QLatin1Char('@')))
                lab.addPart(add.section(QLatin1Char('@'), 0, 0), add.section(QLatin1Char('@'), 1, 1), false, false);
        });
    }

    // Entwicklerhilfe: VOLTA_DEMO_SENSORS=1 zeigt die Bauteile der Stufe 3 im Betrieb: Fotowiderstand mit LED unter
    // der Taschenlampe, Heissleiter vor dem Foehn, Elektromagnet, Relais mit Lampe; Raumlicht Daemmerung
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_SENSORS") == 1)
        QTimer::singleShot(300, &lab, [&lab] {
            lab.setUnlockAll(true);
            lab.startFreeLab();
            lab.setBoardColumns(12);
            lab.addPart(QStringLiteral("fotowiderstand"), QStringLiteral("A2"), false, false);
            lab.addPart(QStringLiteral("widerstand"), QStringLiteral("A3"), false, false);
            lab.setPartValue(QStringLiteral("R1"), 470.0);
            lab.addPart(QStringLiteral("led_rot"), QStringLiteral("A4"), true, false);
            lab.addWire(QStringLiteral("P:+9V"), QStringLiteral("A2"), QStringLiteral("rot"));
            lab.addWire(QStringLiteral("B4"), QStringLiteral("P:0V"), QStringLiteral("schwarz"));
            lab.addPart(QStringLiteral("heissleiter"), QStringLiteral("C2"), false, false);
            lab.addPart(QStringLiteral("elektromagnet"), QStringLiteral("E2"), false, true);
            lab.addWire(QStringLiteral("P:+9V"), QStringLiteral("E2"), QStringLiteral("rot"));
            lab.addWire(QStringLiteral("E4"), QStringLiteral("P:0V"), QStringLiteral("schwarz"));
            lab.addPart(QStringLiteral("relais"), QStringLiteral("C8"), false, false);
            lab.addPart(QStringLiteral("gluehlampe"), QStringLiteral("A10"), false, false);
            lab.addWire(QStringLiteral("P:+9V"), QStringLiteral("C8"), QStringLiteral("rot"));
            lab.addWire(QStringLiteral("C9"), QStringLiteral("P:0V"), QStringLiteral("schwarz"));
            lab.addWire(QStringLiteral("P:+9V"), QStringLiteral("D8"), QStringLiteral("rot"));
            lab.addWire(QStringLiteral("D9"), QStringLiteral("A10"), QStringLiteral("gelb"));
            lab.addWire(QStringLiteral("A11"), QStringLiteral("P:0V"), QStringLiteral("schwarz"));
            lab.setRoomLight(1);
            lab.placeTool(QStringLiteral("taschenlampe"), 2.5, 0.6);
            lab.placeTool(QStringLiteral("foehn"), 2.5, 2.6);
            lab.setMainSwitch(true);
        });

    // Entwicklerhilfe: VOLTA_DEMO_RECORDER=1 laedt einen Elko (100 uF) ueber 10 kOhm und zeichnet die Spannung am
    // Elko (Kanal 1) und den Ladestrom ueber das Messgeraet (Kanal 2) mit dem Zeitschreiber auf
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_RECORDER") == 1)
        QTimer::singleShot(300, &lab, [&lab, &engine] {
            lab.setUnlockAll(true);
            lab.startFreeLab();
            lab.setBoardColumns(12);
            lab.addPart(QStringLiteral("widerstand"), QStringLiteral("B3"), false, false);
            lab.setPartValue(QStringLiteral("R1"), 10000.0);
            lab.addPart(QStringLiteral("elko"), QStringLiteral("B4"), true, false);
            lab.addWire(QStringLiteral("P:+9V"), QStringLiteral("B3"), QStringLiteral("rot"));
            lab.setMeterVisible(true);
            lab.setMeterMode(1);
            lab.addWire(QStringLiteral("C4"), QStringLiteral("MESS:A"), QStringLiteral("blau"));
            lab.addWire(QStringLiteral("MESS:COM"), QStringLiteral("P:0V"), QStringLiteral("schwarz"));
            lab.setRecorderChannel(0, QStringLiteral("B4"));
            lab.setRecorderChannel(1, QStringLiteral("MESS"));
            lab.setRecorderWindow(10);
            lab.setMainSwitch(true);
            const auto roots = engine.rootObjects();
            if (QObject* l = roots.isEmpty() ? nullptr : roots.first()->findChild<QObject*>(QStringLiteral("lab")))
                l->setProperty("recorderVisible", true);
        });

    // Entwicklerhilfe: VOLTA_DEMO_COLUMNS=8|10|12 stellt die Breite der Grundplatte ein (wie +/- in der Werkzeugleiste)
    if (const int columns = qEnvironmentVariableIntValue("VOLTA_DEMO_COLUMNS"); columns > 0)
        QTimer::singleShot(400, &lab, [&lab, columns] { lab.setBoardColumns(columns); });

    // Entwicklerhilfe: VOLTA_DEMO_METER=v|analog|a|ohm baut eine Messung auf (Sichtpruefung des Messgeraets):
    // Spannung der Batterie, Strom durch 470 Ohm, Widerstand 470 Ohm
    const QString demoMeter = qEnvironmentVariable("VOLTA_DEMO_METER");
    if (!demoMeter.isEmpty())
        QTimer::singleShot(300, &lab, [&lab, demoMeter] {
            lab.startFreeLab();
            lab.setMeterVisible(true);
            lab.setMainSwitch(true);
            if (demoMeter == QLatin1String("v") || demoMeter == QLatin1String("analog")) {
                lab.addWire(QStringLiteral("P:+9V"), QStringLiteral("MESS:VOHM"), QStringLiteral("rot"));
                lab.addWire(QStringLiteral("P:0V"), QStringLiteral("MESS:COM"), QStringLiteral("schwarz"));
                lab.setMeterAnalog(demoMeter == QLatin1String("analog"));
            } else {
                lab.addPart(QStringLiteral("widerstand"), QStringLiteral("B6"), false, false); // B6-B7, 470 Ohm
                lab.setPartValue(QStringLiteral("R1"), 470.0);
                if (demoMeter == QLatin1String("a")) {
                    lab.setMeterMode(1);
                    lab.addWire(QStringLiteral("P:+9V"), QStringLiteral("B6"), QStringLiteral("rot"));
                    lab.addWire(QStringLiteral("B7"), QStringLiteral("MESS:A"), QStringLiteral("rot"));
                    lab.addWire(QStringLiteral("MESS:COM"), QStringLiteral("P:0V"), QStringLiteral("schwarz"));
                } else {
                    lab.setMeterMode(2);
                    lab.addWire(QStringLiteral("B6"), QStringLiteral("MESS:VOHM"), QStringLiteral("rot"));
                    lab.addWire(QStringLiteral("B7"), QStringLiteral("MESS:COM"), QStringLiteral("schwarz"));
                }
            }
            if (qEnvironmentVariableIntValue("VOLTA_DEMO_ANALOG") == 1) lab.setMeterAnalog(true); // Zeiger
        });

    // Entwicklerhilfe: VOLTA_DEMO_JOURNAL=1 legt einen Eintrag an (Vermutung, Notiz) und zeigt das Forscherheft
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_JOURNAL") == 1)
        QTimer::singleShot(300, &lab, [&lab, &engine] {
            lab.startExperiment(QStringLiteral("volta1.1.1"));
            lab.chooseGuess(1);
            lab.addNote(QStringLiteral("Bei 3 V leuchtet das Lämpchen schwächer."));
            if (!engine.rootObjects().isEmpty()) engine.rootObjects().first()->setProperty("inJournal", true);
        });

    // Entwicklerhilfe: VOLTA_DEMO_PARENT=gate|area|qr zeigt Elternschranke, Elternbereich bzw. Fortschritts-QR-Code
    if (const QString demo = qEnvironmentVariable("VOLTA_DEMO_PARENT"); !demo.isEmpty())
        QTimer::singleShot(600, &engine, [&engine, demo] {
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            if (QObject* gate = roots.first()->findChild<QObject*>(QStringLiteral("parentGate")); gate && demo == u"gate")
                QMetaObject::invokeMethod(gate, "ask", Q_ARG(QVariant, QVariant()));
            if (QObject* area = roots.first()->findChild<QObject*>(QStringLiteral("parentArea"));
                area && (demo == u"area" || demo == u"qr"))
                QMetaObject::invokeMethod(area, "open");
            if (QObject* qr = roots.first()->findChild<QObject*>(QStringLiteral("qrPopup")); qr && demo == u"qr")
                QMetaObject::invokeMethod(qr, "open");
        });

    // Entwicklerhilfe: VOLTA_DEMO_TOOLBOX=0|1|2|3|4 oeffnet den Werkzeugkasten (Farbcode, E-Reihen, Lexikon, Rechner,
    // Fehlersuche); bei 4 mit einem fehlerhaften Aufbau (LED ohne Vorwiderstand, loser Widerstand), schon geprueft
    // VOLTA_DEMO_TOOLBOX=5: Teileliste zum Experiment VOLTA_DEMO_EXP (Standard volta1.4.4); mit VOLTA_DEMO_CARD=<datei.pdf>
    // wird die Aufbaukarte ohne Dateiauswahl geschrieben (Sichtpruefung des PDF)
    if (qEnvironmentVariable("VOLTA_DEMO_TOOLBOX") == QStringLiteral("5"))
        QTimer::singleShot(300, &lab, [&lab, &engine] {
            lab.setUnlockAll(true);
            lab.startExperiment(qEnvironmentVariable("VOLTA_DEMO_EXP", QStringLiteral("volta1.4.4")));
            const QString card = qEnvironmentVariable("VOLTA_DEMO_CARD");
            if (card.isEmpty()) return;
            QTimer::singleShot(900, &lab, [&lab, &engine, card] {
                const auto roots = engine.rootObjects();
                QObject* panel = roots.isEmpty() ? nullptr : roots.first()->findChild<QObject*>(QStringLiteral("partsList"));
                if (!panel) return;
                QMetaObject::invokeMethod(panel, "refresh");
                QVariant texts;
                QMetaObject::invokeMethod(panel, "cardTexts", Q_RETURN_ARG(QVariant, texts));
                qInfo() << "Aufbaukarte" << card << lab.exportBuildCard(QUrl::fromLocalFile(card), texts.toMap());
            });
        });
    if (qEnvironmentVariable("VOLTA_DEMO_TOOLBOX") == QStringLiteral("4"))
        QTimer::singleShot(300, &lab, [&lab] {
            lab.setUnlockAll(true);
            lab.startFreeLab();
            lab.addPart(QStringLiteral("led_rot"), QStringLiteral("B3"), false, false);
            lab.addPart(QStringLiteral("widerstand"), QStringLiteral("D6"), false, false);
            lab.addWire(QStringLiteral("P:+9V"), QStringLiteral("B3"), QStringLiteral("rot"));
            lab.addWire(QStringLiteral("B4"), QStringLiteral("P:0V"), QStringLiteral("schwarz"));
            lab.addWire(QStringLiteral("B4"), QStringLiteral("D6"), QStringLiteral("blau"));
            lab.setMainSwitch(true);
        });
    if (const QString tb = qEnvironmentVariable("VOLTA_DEMO_TOOLBOX"); !tb.isEmpty())
        QTimer::singleShot(600, &engine, [&engine, tb] {
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            roots.first()->setProperty("inLab", true);
            if (QObject* box = roots.first()->findChild<QObject*>(QStringLiteral("toolbox"))) {
                QMetaObject::invokeMethod(box, "open");
                if (QObject* tabs = box->findChild<QObject*>(QStringLiteral("toolboxTabs")))
                    tabs->setProperty("currentIndex", tb.toInt());
                if (QObject* finder = box->findChild<QObject*>(QStringLiteral("faultFinder")); finder && tb == u"4")
                    QTimer::singleShot(600, finder, [finder] { QMetaObject::invokeMethod(finder, "check"); });
            }
        });

    // Entwicklerhilfe: VOLTA_DEMO_SCHEMATIC=1 baut LED mit Vorwiderstand (plus Parallelwiderstand) und zeigt den Schaltplan
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_SCHEMATIC") == 1)
        QTimer::singleShot(400, &lab, [&lab, &engine] {
            lab.setUnlockAll(true);
            lab.startFreeLab();
            lab.addPart(QStringLiteral("widerstand"), QStringLiteral("B3"), false, false);
            lab.setPartValue(QStringLiteral("R1"), 470.0);
            lab.addPart(QStringLiteral("widerstand"), QStringLiteral("A3"), false, false);
            lab.addPart(QStringLiteral("led_rot"), QStringLiteral("B4"), true, false);
            lab.addWire(QStringLiteral("P:+9V"), QStringLiteral("B3"), QStringLiteral("rot"));
            lab.addWire(QStringLiteral("A3"), QStringLiteral("B3"), QStringLiteral("rot"));
            lab.addWire(QStringLiteral("A4"), QStringLiteral("B4"), QStringLiteral("rot"));
            lab.addWire(QStringLiteral("C4"), QStringLiteral("P:0V"), QStringLiteral("schwarz"));
            lab.setMainSwitch(true);
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            roots.first()->setProperty("inLab", true);
            if (QObject* l = roots.first()->findChild<QObject*>(QStringLiteral("lab"))) {
                l->setProperty("schematicVisible", true);
                l->setProperty("selectedPart", QStringLiteral("R1"));
            }
        });

    // Entwicklerhilfe: VOLTA_DEMO_INSIDE=<Modul-ID> oeffnet »Blick hinein« (mit VOLTA_DEMO_SOLVE und VOLTA_DEMO_EXP)
    if (const QString inside = qEnvironmentVariable("VOLTA_DEMO_INSIDE"); !inside.isEmpty())
        QTimer::singleShot(1500, &engine, [&engine, &lab, inside] {
            const auto roots = engine.rootObjects();
            QObject* dlg = roots.isEmpty() ? nullptr : roots.first()->findChild<QObject*>(QStringLiteral("moduleInside"));
            QString type;
            for (const QVariant& v : lab.modules())
                if (v.toMap().value(QStringLiteral("id")).toString() == inside)
                    type = v.toMap().value(QStringLiteral("type")).toString();
            if (dlg) QMetaObject::invokeMethod(dlg, "show", Q_ARG(QVariant, inside), Q_ARG(QVariant, type));
        });

    // Entwicklerhilfe: VOLTA_DEMO_PLAN=1 blendet den Schaltplan ein (z. B. mit VOLTA_DEMO_SOLVE und VOLTA_DEMO_EXP)
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_PLAN") == 1)
        QTimer::singleShot(800, &engine, [&engine] {
            const auto roots = engine.rootObjects();
            if (QObject* l = roots.isEmpty() ? nullptr : roots.first()->findChild<QObject*>(QStringLiteral("lab")))
                l->setProperty("schematicVisible", true);
        });

    // Entwicklerhilfe: VOLTA_DEMO_SELECT=<Bauteil-ID> bzw. VOLTA_DEMO_SELECT_WIRE=<Leitungs-ID> waehlt aus
    // (Sichtpruefung der Loeschen-Knoepfe)
    if (!qEnvironmentVariableIsEmpty("VOLTA_DEMO_SELECT") || !qEnvironmentVariableIsEmpty("VOLTA_DEMO_SELECT_WIRE"))
        QTimer::singleShot(1200, &engine, [&engine] {
            const auto roots = engine.rootObjects();
            QObject* l = roots.isEmpty() ? nullptr : roots.first()->findChild<QObject*>(QStringLiteral("lab"));
            if (!l) return;
            if (!qEnvironmentVariableIsEmpty("VOLTA_DEMO_SELECT"))
                l->setProperty("selectedPart", qEnvironmentVariable("VOLTA_DEMO_SELECT"));
            else
                l->setProperty("selectedWire", qEnvironmentVariable("VOLTA_DEMO_SELECT_WIRE"));
        });

    // Entwicklerhilfe: VOLTA_DEMO_FLYOUT=1 klappt die Werkzeugleiste »Licht und Waerme« aus (Sichtpruefung)
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_FLYOUT") == 1)
        QTimer::singleShot(1500, &engine, [&engine] {
            const auto roots = engine.rootObjects();
            if (QObject* b = roots.isEmpty() ? nullptr : roots.first()->findChild<QObject*>(QStringLiteral("environmentBar"))) {
                b->setProperty("collapseDelay", 60000);
                QMetaObject::invokeMethod(b, "poke");
            }
        });

    // Entwicklerhilfe: VOLTA_DEMO_INFO=1 oeffnet das Info-Fenster (Sichtpruefung)
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_INFO") == 1)
        QTimer::singleShot(600, &engine, [&engine] {
            const auto roots = engine.rootObjects();
            if (QObject* dlg = roots.isEmpty() ? nullptr : roots.first()->findChild<QObject*>(QStringLiteral("infoDialog")))
                QMetaObject::invokeMethod(dlg, "open");
        });

    // Entwicklerhilfe: VOLTA_DEMO_HELP=<kapitel> oeffnet die Hilfe bei einem Kapitel (z. B. werkzeug, eltern; 1: Anfang)
    if (const QString helpKey = qEnvironmentVariable("VOLTA_DEMO_HELP"); !helpKey.isEmpty())
        QTimer::singleShot(600, &engine, [&engine, helpKey] {
            const auto roots = engine.rootObjects();
            if (QObject* dlg = roots.isEmpty() ? nullptr : roots.first()->findChild<QObject*>(QStringLiteral("helpDialog")))
                QMetaObject::invokeMethod(dlg, "show", Q_ARG(QVariant, helpKey == u"1" ? QString() : helpKey));
        });

    // Entwicklerhilfe: VOLTA_DEMO_TASK=qr zeigt den QR-Code einer Beispielaufgabe (Lehrkraft); jeder andere Wert gilt als
    // Link oder Code und oeffnet die Aufgabe wie nach dem Scannen (Kind)
    if (const QString demoTask = qEnvironmentVariable("VOLTA_DEMO_TASK"); !demoTask.isEmpty())
        QTimer::singleShot(600, &engine, [&engine, &lab, demoTask] {
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            if (demoTask == u"qr") {
                const QStringList ids{QStringLiteral("volta1.2.4"), QStringLiteral("volta1.3.1")};
                if (QObject* dlg = roots.first()->findChild<QObject*>(QStringLiteral("taskQrDialog")))
                    QMetaObject::invokeMethod(dlg, "showTask", Q_ARG(QVariant, lab.makeTaskLink(ids, {}, false, {})),
                                              Q_ARG(QVariant, lab.makeTaskCode(ids)));
            } else if (QObject* dlg = roots.first()->findChild<QObject*>(QStringLiteral("taskDialog"))) {
                QMetaObject::invokeMethod(dlg, "showLink", Q_ARG(QVariant, demoTask));
            }
        });

    const QString shot = qEnvironmentVariable("VOLTA_SCREENSHOT");
    if (!shot.isEmpty()) {
        if (qEnvironmentVariableIntValue("VOLTA_SCREENSHOT_MAINSWITCH") == 1)
            QTimer::singleShot(500, &lab, [&lab] { lab.setMainSwitch(true); });
        bool okDelay = false;
        const double delayS = qEnvironmentVariable("VOLTA_SCREENSHOT_S").toDouble(&okDelay);
        QTimer::singleShot(okDelay ? static_cast<int>(delayS * 1000) : 3000, &app, [&engine, shot] {
            const auto roots = engine.rootObjects();
            if (auto* window = roots.isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(roots.first()))
                window->grabWindow().save(shot);
            QCoreApplication::quit();
        });
    }
    // Fortschritts-Links (QR-Code ueber die Kamera-App, Schema volta1://) an das Labor
    QDesktopServices::setUrlHandler(QStringLiteral("volta1"), &lab, "handleUrl");
    const int rc = QGuiApplication::exec();
    QDesktopServices::unsetUrlHandler(QStringLiteral("volta1"));
    return rc;
}
