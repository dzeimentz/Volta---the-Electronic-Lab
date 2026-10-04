# ADR-0001: Qt 6 mit Qt Quick als Anwendungsrahmen

- Status: angenommen
- Datum: 2026-10-04

## Kontext
Volta soll auf Android, Windows/Linux (x86-64) und iPadOS aus einer Codebasis entstehen und von einem
Einzelentwickler gepflegt werden, der parallel Visual Technic Studio (Qt 6, C++) entwickelt.

## Entscheidung
Qt 6.8 LTS oder neuer mit Qt Quick/QML fuer die Oberflaeche, C++17 fuer Modell und Dienste, Qt RHI fuer die Grafik.

## Begruendung
- Gleicher Stapel wie VTS: gemeinsame Bausteine (Grafikschicht, Lizenztoken, Paketierung, Inhaltspakete).
- Keine Sprachgrenze zum Simulationskern.
- Desktop-Fassung nahezu ohne Mehraufwand; Desktop ist zugleich Entwicklungs- und Testziel.
- TextToSpeech, Multimedia, Sensors und Linguist als fertige Module.

## Verworfen
Flutter (zweiter Stapel neben VTS, Sprachgrenze), Kotlin Multiplatform (Kern nicht fuer VTS nutzbar),
Web-App (Leistung auf Schultablets), zweimal nativ (doppelter Aufwand).

## Folgen
- LGPLv3: Android/Desktop dynamisch linken. iPadOS: Neulink-Moeglichkeit bereitstellen oder kommerzielle Lizenz
  (vor Phase 5b, gemeinsam mit VTS entscheiden).
- Paketgroesse beobachten; nur benoetigte Module einbinden.
