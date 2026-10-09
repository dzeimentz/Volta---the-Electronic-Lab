// Einstiegspunkt der Qt-Anwendung. Modell, Dienste und Lernpfad folgen gemaess docs/architecture.md.
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml/qqmlextensionplugin.h>

#include "services/audio_service.hpp"
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
    QGuiApplication::setOrganizationName(QStringLiteral("DC Software Engineering"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("dc-software-engineering.com"));
    QGuiApplication::setApplicationName(QStringLiteral(VOLTA_APP_NAME));
    QGuiApplication::setApplicationVersion(QStringLiteral(VOLTA_VERSION));
    QIcon icon; // Fenster- und Taskleistensymbol (Rasterbilder, kein SVG-Plugin noetig)
    for (int size : {32, 64, 256}) icon.addFile(QStringLiteral(":/icons/volta_%1.png").arg(size), QSize(size, size));
    QGuiApplication::setWindowIcon(icon);

    // Inhaltspakete: zuerst neben der ausfuehrbaren Datei (Release-Ordner, tools/deploy_release.py), sonst aus dem
    // Quellbaum. Die Einbettung als Ressource folgt mit der Paketierung (Phase 5).
    const QString besideExe = QCoreApplication::applicationDirPath() + QStringLiteral("/content");
    const bool deployed = QFileInfo::exists(besideExe + QStringLiteral("/catalog/bauteile.json"));
    volta::LabController lab(deployed ? besideExe : QStringLiteral(VOLTA_CONTENT_DIR));
    // Fortschritt im Lernpfad lokal speichern; Entwicklerhilfen (VOLTA_SCREENSHOT ...) lassen ihn unberuehrt
    if (qEnvironmentVariable("VOLTA_SCREENSHOT").isEmpty()) {
        lab.useProgressFile(volta::services::ProgressStore::defaultFile());
        lab.useJournalFile(volta::services::JournalStore::defaultFile());
    }

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
    volta::AppSettings settings(demoLang.isEmpty() && demoFont.isEmpty());
    if (!demoFont.isEmpty()) settings.setFontLevel(demoFont.toInt());
    engine.rootContext()->setContextProperty(QStringLiteral("volta_settings"), &settings);
    // Tonausgabe und Vorlesen (Bildschirmfoto-Entwicklerhilfe: still)
    volta::services::AudioService audio;
    if (!qEnvironmentVariableIsEmpty("VOLTA_SCREENSHOT")) audio.setSuppressed(true);
    lab.setAudio(&audio);
    volta::services::SpeechService speech;
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
                             "VOLTA_DEMO_FREELAB", "VOLTA_DEMO_GUIDE"})
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

    // Entwicklerhilfe: VOLTA_DEMO_TOOLBOX=0|1|2 oeffnet den Werkzeugkasten (Farbcode, E-Reihen, Lexikon)
    if (const QString tb = qEnvironmentVariable("VOLTA_DEMO_TOOLBOX"); !tb.isEmpty())
        QTimer::singleShot(600, &engine, [&engine, tb] {
            const auto roots = engine.rootObjects();
            if (roots.isEmpty()) return;
            roots.first()->setProperty("inLab", true);
            if (QObject* box = roots.first()->findChild<QObject*>(QStringLiteral("toolbox"))) {
                QMetaObject::invokeMethod(box, "open");
                if (QObject* tabs = box->findChild<QObject*>(QStringLiteral("toolboxTabs")))
                    tabs->setProperty("currentIndex", tb.toInt());
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

    // Entwicklerhilfe: VOLTA_DEMO_INFO=1 oeffnet das Info-Fenster (Sichtpruefung)
    if (qEnvironmentVariableIntValue("VOLTA_DEMO_INFO") == 1)
        QTimer::singleShot(600, &engine, [&engine] {
            const auto roots = engine.rootObjects();
            if (QObject* dlg = roots.isEmpty() ? nullptr : roots.first()->findChild<QObject*>(QStringLiteral("infoDialog")))
                QMetaObject::invokeMethod(dlg, "open");
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
    return QGuiApplication::exec();
}
