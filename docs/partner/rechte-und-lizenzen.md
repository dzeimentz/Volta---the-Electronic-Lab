# Volta – Rechte und Lizenzen (Übersicht für Partner)

Stand: 14.10.2026, Version 0.9.0. Diese Übersicht beschreibt, was an Volta von DC Software Engineering stammt,
welche Fremdbestandteile unter welchen Lizenzen enthalten sind und welche Punkte vor einer Lizenzvereinbarung
geklärt werden. Sie ist keine Rechtsberatung; der Vertrag wird anwaltlich geprüft.

## 1. Rechteinhaber

**DC Software Engineering, Daniel Zeimentz**, Aufm Rothenhügel 26, 54666 Irrel, Deutschland.

| Bestandteil | Umfang | Entstehung |
|---|---|---|
| Simulationskern `core/` | MNA-Löser, Newton-Raphson, Zeitintegration, Wärmemodell, alle Bauteil- und Modulmodelle (C++17, ohne Fremdbibliotheken) | eigene Entwicklung |
| Anwendung `app/` | Oberfläche (Qt Quick), Modell, Befehle, Lernpfad, Werkzeuge, Dienste | eigene Entwicklung |
| Inhalte `content/` | 61 Experimente, Stufen, Lexikon, Hilfen, Texte DE/FR/EN/LU | eigene Entwicklung; LU-Texte von Muttersprachlern geprüft bzw. geliefert |
| Gestaltung | Programmsymbol, Kontaktfelder, Bedienpult, Fensterleiste, gezeichnete Symbole, Flaggen | eigene Entwicklung |
| Maskottchen »Tick« | zehn Stimmungsbilder | im Auftrag des Inhabers mit einem KI-Bildwerkzeug (ChatGPT) erzeugt |
| Werkzeuge `tools/` | Prüfung, Paketierung, Anleitungen, Flyer, Store-Grafiken | eigene Entwicklung |

**Nachweis der Urheberschaft:** vollständige Versionsgeschichte (Git) seit dem 04.10.2026 mit Datum und Autor
jeder Änderung (derzeit 137 Änderungen, ca. 400 Dateien, ca. 33 000 Zeilen Programmcode in Kern und Anwendung).

**Hinweis zur Entstehung:** Programmcode, Inhalte und Unterlagen sind unter Leitung und nach Vorgaben des
Inhabers mit KI-Unterstützung (Claude Code) entstanden; das ist in der Versionsgeschichte vermerkt. Die
Nutzungsbedingungen der Werkzeuge räumen dem Nutzer die Rechte an den Ergebnissen ein. Wie weit rein
KI-erzeugte Anteile in der EU urheberrechtlich geschützt sind, ist rechtlich nicht abschließend geklärt; das
betrifft besonders die Figur Tick. Unabhängig davon liegen Konzept, Auswahl, Anordnung, Gestaltungsentscheidungen
und die Gesamtheit des Werks beim Inhaber.

## 2. Fremdbestandteile in der ausgelieferten App

| Bestandteil | Lizenz | Verwendung | Pflichten |
|---|---|---|---|
| Qt 6.11 (Core, Gui, Qml, Quick, Quick Controls, Quick Dialogs, Quick Shapes, Quick Effects, Svg, Multimedia, Text to Speech, Network*) | LGPL v3 | Oberfläche, Grafik, Ton, Vorlesen | dynamisch gebunden (DLL/.so); Lizenztext beilegen; Hinweis auf die Qt-Quelltexte; Austausch der Qt-Bibliotheken durch den Nutzer nicht verhindern |
| qrcodegen (Project Nayuki) | MIT | QR-Codes (Aufgaben, Fortschritt) | Copyright-Hinweis (steht im Info-Fenster) |
| MinGW-Laufzeit (libstdc++, libgcc, winpthreads) – nur Windows | GPL mit GCC-Laufzeitausnahme bzw. MIT/BSD-artig | C++-Laufzeit | Lizenzhinweis |
| libc++ (LLVM), AndroidX, Qt-Java-Teile – nur Android | Apache 2.0 mit LLVM-Ausnahme bzw. Apache 2.0 / LGPL v3 | Laufzeit unter Android | Lizenzhinweis |
| NSIS – nur Windows-Installer | zlib/libpng | Installationsprogramm | keine besonderen |

\* Qt Network wird mitgeliefert, weil andere Qt-Teile davon abhängen; Volta selbst nutzt kein Netz (keine
Berechtigung `INTERNET`, kein Netzwerkcode).

**Nicht ausgeliefert** (nur Entwicklung): Catch2 (BSL-1.0, Tests), Python-Werkzeuge (reportlab, Pillow, PyMuPDF
nur für Vorschauen), Android-SDK/NDK, Compiler.

**Schriften:** Die App nutzt die Schriften des Systems. Die PDF-Unterlagen (Anleitung, Flyer, Aufbaukarte)
betten Teilmengen von Segoe UI ein; für eine Auslieferung durch einen Partner wird das gegen dessen Hausschrift
oder eine freie Schrift (z. B. Noto Sans) getauscht.

**Ausblick iPad/Mac:** Qt für iOS wird statisch gebunden; dafür ist die LGPL ungeeignet, voraussichtlich ist eine
kommerzielle Qt-Lizenz nötig (Kosten je Entwickler und Jahr, vor der iPad-Ausgabe zu klären).

## 3. Marken und Namen

| Name | Stand | Folge für Partner |
|---|---|---|
| »Volta« | mehrere ältere Marken in Klassen 9/41, u. a. mit Bildungs- und Elektrotechnikbezug (Vorrecherche `docs/store/markenpruefung.md`) | eine Partnerausgabe kann unter eigenem Namen erscheinen; »Volta« als Technik-/Reihenname nach anwaltlicher Prüfung |
| »Tick« | Wortmarke »TICK« für Software belegt (Werkzeughersteller), Figurenname unkritischer | Figur austauschbar (siehe Branding-Konzept) |
| »Zuse« | Namensrecht der Familie bzw. Gesellschaften zu klären | Ersatzname vorhalten |

## 4. Abgrenzung zum gestalterischen Vorbild

Volta übernimmt nur das allgemeine Prinzip klassischer Elektronik-Experimentierkästen (Kontaktfelder als Knoten,
Bedienpult, steckbare Module). Texte, Abbildungen, Versuchsaufbauten, Kapitelnamen, Feldnummerierung, Figuren
und Namen eines Vorbilds werden nicht übernommen; alle Experimente sind Eigenentwicklungen. Diese Regel ist
verbindlich festgeschrieben (Projektleitfaden, Regel 2) und wird bei jeder Inhaltsänderung geprüft.
Quellen aus Fachzeitschriften (Schaltungssammlungen) dienen nur als Prinzipquelle.

## 5. Datenschutz und Sicherheit (vereinfacht die Partnerschaft)

Keine Datenerhebung, kein Konto, keine Werbung, keine Käufe, keine Netzwerkverbindung, keine Berechtigungen.
Alle Daten bleiben auf dem Gerät. Damit entfallen Auftragsverarbeitung, Server und Einwilligungen; für Schulen
genügt die Datenschutzerklärung. Kinderschutz: nur Batterien bis 9 V, zerstörende Versuche nur in der App,
externe Links hinter der Elternschranke.

## 6. Offene Punkte vor einer Vereinbarung

1. Anwaltliche Bewertung der Marke »Volta« und des Namens »Zuse«.
2. Schutzfähigkeit der Figur Tick; ggf. eigene Figur des Partners.
3. Rechte an den LU-Texten: schriftliche Rechteeinräumung der Übersetzer einholen.
4. Lizenzhinweise gesammelt in der App (eigene Seite »Lizenzen« mit allen Texten) – technisch vorbereitet, Inhalt
   wird ergänzt.
5. Qt-Lizenzweg für iPad/Mac.
6. Vertraulichkeitsvereinbarung vor Weitergabe von mehr als dem Testpaket; Quellcode wird nicht weitergegeben.
