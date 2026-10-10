# Roadmap Volta 1

Aufwaende in Personentagen (PT) eines erfahrenen Einzelentwicklers. Jede Phase endet mit einem nutzbaren Ergebnis.

| Phase | Inhalt | PT | Ergebnis | Status |
|---|---|---|---|---|
| 0 Grundlagen | Marktsichtung, Markenrecherche, Kalibrierung Bauteilmodelle, Simulationskern (MNA, Newton, Thermik, Teilschaltungen, Messschnittstelle), Testsuite, Qt-Projektgeruest Android + Desktop, Uebernahme Grafikschicht aus VTS | 25 | getesteter Kern | **aktiv** |
| 1 Prototyp | Grundplatte, Bedienpult I, gezogene Leitungen, Undo/Redo, Spannungsanzeige, 6 Bauteile Stufe 1, Stromfluss, 3 Experimente | 25 | spielbarer Android-Prototyp, Erprobung mit Kindern | offen |
| 2 Inhalt Stufen 1–2 | Lernpfad-Engine, Forschungskreislauf, Hilfen, Vorlesen, Blink-/Tonmodul mit »Blick hinein«, Schaltplan, Farbcode/E-Reihen, Lexikon, ~30 Experimente | 40 | inhaltlich vollstaendige Beta | offen |
| 3 Android 1.0 | Forscherheft, Elternbereich, Readme, Info, Flaggensymbol, Autosave, Profile, Klassenmodus, verwaltete Konfiguration, Barrierefreiheit, Store, Pilot | 25 | Veroeffentlichung Android | offen |
| 4 Stufen 3–4 | Dreifach-Messgeraet, Lichtring, Klanganzeige, Formelrechner, Zeitschreiber, Fehlersuche, Teileliste, Transistor, Zeit-/Verstaerkermodul, Lichtsensor, ~30 Experimente | 45 | Version 1.5 | offen |
| 5a Desktop x86-64 | Maus/Tastatur, Fenster, MSIX/MSI, AppImage, Konfiguration, Test auf Schulrechner und Tafel | 15 | Windows + Linux | offen |
| 5b iPad | Anpassung, Pencil, Qt-Lizenz iOS, Kinderkategorie | 20 | iPadOS | offen |
| 6 Ausbau | Lehrkraftmodus, FR, EN, LU mit Aufnahmen | 50 | Version 2.0 | offen |
| 7 Schulpaket | Handreichung, Arbeitsblaetter, Lehrplanbezug DE/LU/Ostbelgien, Datenschutz-Merkblatt, Praesentationsmodus, Fortschrittscode | 30 | Schulfreigabe | offen |

Summe bis Android 1.0: 115 PT. Vollausbau Volta 1: 275 PT.

## Offene Entscheidungen
Entschieden 10/2026: Volta 1 wird bis zur Veroeffentlichung fertiggestellt, danach folgt Volta 2
(`docs/volta2-planung.md`).

| Entscheidung | Spaetestens |
|---|---|
| Referenztablet fuer Leistungstests | Beginn Phase 0 |
| Ergebnis Markenrecherche, ggf. Anmeldung | Ende Phase 0 |
| Geraetelage Zielschulen, Reihenfolge Android/Desktop/iPad | Ende Phase 0 |
| Gestalt und Name der Begleitfigur | Ende Phase 1 |
| Verwertungsweg (frei, Einmalkauf, Schullizenz) | Ende Phase 2 |
| Mitwirkung der Lehrkraft am Pilot | Beginn Phase 3 |
| Qt-Lizenzweg fuer iPadOS | vor Phase 5b |
| Vollstaendige Anschrift fuer Anbieterkennzeichnung im Info-Bereich | vor Phase 3 |

## Phase 1 – Stand
- 1.1–1.4 Grundplatte, Bauteilfach, gezogene Leitungen, Undo/Redo, Spannungsanzeige, Stromfluss: erledigt.
- 1.5 Bedienpult I (Batterie mit Zellenwahl und Hauptschalter, Drehregler P1/P2 mit Lichtring, Pultschalter S,
  Gruppenlinien, Skalierung ins Fenster): erledigt.
- 1.6 Drei gefuehrte Experimente der Stufe 1 (1.1 Stromkreis mit Gluehlaempchen, 1.2 Klingel mit Taster und Summer,
  1.3 Motor am Pultschalter) mit einfachem Ablauf Frage -> Aufbau mit gestuften Hilfen und Musteraufbau -> Erfolg
  mit Erklaerung; Erfolgspruefung im Simulations-Thread (`StepObserver`). Lokales Release: `tools/deploy_release.py`.
- Nach Phase 1 vorgezogen: Sprachumschaltung DE/FR/EN (LU folgt) und Info oben rechts, Startbildschirm (Desktop),
  Dreifach-Messgeraet bedienbar (Phase 4), Breite der Grundplatte 8/10/12, Transistor auf 2-x-2-Platine.
- Offen: Android-Build (Kit fehlt auf dem Entwicklungsrechner), Erprobung mit Kindern, Begleitfigur.

## Phase 2 – Stand (Inhalt Stufen 1–2)
- D1 Startseite mit Lernpfad, Freischaltung und gespeichertem Fortschritt: erledigt.
- D2 Forschungskreislauf mit Beobachtung, Vergleich mit der Vermutung und Forscherheft: erledigt.
- D3 Werkzeugkasten: Farbcode, E-Reihen, Bauteillexikon: erledigt (Formelrechner, Zeitschreiber, Fehlersuche folgen).
- D4 Schaltplanansicht mit wechselseitiger Hervorhebung: erledigt.
- D5 Experimente: 31 (Auftakt 0.1; Stufe 1: 1.1–1.10; Stufe 2: 2.1–2.20), jede Vorlage im ContentSmokeTest
  gegen ihre Erfolgsbedingung simuliert; Texte DE, FR und EN (FR/EN Entwurf).
- Module: Modulleiste mit dargestellten Modulen (Anschlussnamen auf den Feldern), Modulfach zum Einsetzen,
  Verschieben und Herausnehmen (Befehle); Tonmodul als Verhaltensmodell (`core/modules/tone_module`), das den
  Pult-Lautsprecher mit seinem Ton anzeigt; »Blick hinein«: Blinkmodul als Schaltplan mit laufender Simulation,
  Tonmodul als Blockbild mit Erklaerung des Verhaltensmodells. Erfolgsbedingungen koennen Module ansprechen.
- Schritt-fuer-Schritt-Aufbauhilfe mit Geisterbild und hervorgehobenen Zielfeldern: erledigt.
- Tonausgabe ueber die Soundkarte (Pult-Lautsprecher, Summer, Knacken je Impuls) und Vorlesen ueber die
  Systemstimme (DE/FR/EN): erledigt (ADR-0009).
- Meldungen des Modells und Befehlsnamen (Rueckgaengig) uebersetzbar: erledigt.
- Offen in Phase 2: LU-Texte, muttersprachliche Pruefung FR/EN.

## Phase 4 – Stand (Stufen 3–4)
- Bauteile der Stufe 3 im Kern: Fotowiderstand, Heissleiter, Elektromagnet, Relais (Referenztests); Licht und
  Waerme als Bedienhandlungen (Raumlicht, Taschenlampe, Foehn; ausklappende Werkzeugleiste).
- Zeitschreiber mit zwei Kanaelen (Feldspannung oder Messgeraet).
- Stufe 3: 15 Experimente (3.1–3.15: Spannung, Strom, Widerstand messen, Ohmsches Gesetz, Spannungsteiler,
  Fotowiderstand, Heissleiter, Kondensator laden/entladen, Elektromagnet, Relais); insgesamt 46.
  Erfolgsbedingungen koennen die Anzeige des Messgeraets pruefen (`MESS` / `anzeige`).
- Formelrechner mit Rechenblaettern der Stufen 3 und 4 und Rechenmodus: erledigt.
- Stufe 4: Piezo-Schallgeber, Feuchtesonde (Wassertropfen als Werkzeug), Zeitmodul (monostabile Kippstufe,
  ca. 3,2 s) und Verstaerkermodul (Darlington) in Kern und App; 15 Experimente (4.1–4.15: Transistor als Schalter
  und Verstaerker, Daemmerungsschalter, Lichtschalter, Waerme- und Wassermelder, Treppenhauslicht, Zeitschalter
  und Wechselblinker aus Einzelteilen, Tueralarm, Selbsthaltung, Darlington, Nachlauf, Module im Verbund);
  insgesamt 61. ContentSmokeTest mit Gegenprobe fuer Sensor-Experimente.
- Fehlersuche im Werkzeugkasten: erledigt (alle Vorlagen ohne Befund; dabei Vorlage 4.4 um einen Basiswiderstand
  ergaenzt, der Fotowiderstand war mit Taschenlampe ueberlastet).
- Offen: Teileliste.

## Naechste konkrete Schritte (Phase 0)
1. ~~`core/`: `Circuit`, `Simulator`, dichte LU, Widerstand, Quelle, Diode/LED mit Newton, Referenztests 1–4.~~ erledigt
2. ~~Thermikmodell und Ereignisse; Robustheitstests.~~ erledigt, dazu: alle Bauteile der Stufen 1–4 inkl.
   Transistor, Blinkmodul als Teilschaltung, Dreifach-Messgeraet, Referenztests 1–7, Zufallstest,
   Leistungsprogramm (siehe `docs/simulation-core.md`, `docs/testing.md`)
3. ~~`app/`: Qt-Projektgeruest, `Board`-Modell, `NetlistBuilder`, `Lab.qml` mit Grundplatte.~~ erledigt:
   Vorlage von Experiment 2.4 laeuft Ende-zu-Ende mit Spannungsanzeigen (Desktop, Qt 6.11). VTS enthaelt keine
   uebertragbare Qt-Quick-Grafikschicht (ADR-0008); Leitungen sind Bauteile (ADR-0007).
4. ~~Inhaltsvalidator in CI; `ContentSmokeTest` simuliert jede Vorlage gegen ihre Erfolgsbedingung.~~ erledigt:
   lokales Pruefskript `tools/check_all.py` statt CI (noch kein entferntes Repository); Bewerter fuer
   Erfolgsbedingungen (`model/success.*`) steht auch dem Lernpfad zur Verfuegung.

Offen aus Phase 0 (Kern): Kalibrierung der Bauteilmodelle an realem Material; Leistungsmessung auf dem
Referenztablet (Desktop: schlechtester Einzelschritt ca. 8 ms beim Umkippen einer Kippstufe); ~~Tonmodul als
Verhaltensmodell~~ (erledigt in Phase 2); Nennspannung der Katalog-Gluehlampe (6 V brennt an 9 V nach ca. 5 s durch).
