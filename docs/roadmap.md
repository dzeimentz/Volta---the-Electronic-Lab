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

## Naechste konkrete Schritte (Phase 0)
1. ~~`core/`: `Circuit`, `Simulator`, dichte LU, Widerstand, Quelle, Diode/LED mit Newton, Referenztests 1–4.~~ erledigt
2. ~~Thermikmodell und Ereignisse; Robustheitstests.~~ erledigt, dazu: alle Bauteile der Stufen 1–4 inkl.
   Transistor, Blinkmodul als Teilschaltung, Dreifach-Messgeraet, Referenztests 1–7, Zufallstest,
   Leistungsprogramm (siehe `docs/simulation-core.md`, `docs/testing.md`)
3. ~~`app/`: Qt-Projektgeruest, `Board`-Modell, `NetlistBuilder`, `Lab.qml` mit Grundplatte.~~ erledigt:
   Vorlage von Experiment 2.4 laeuft Ende-zu-Ende mit Spannungsanzeigen (Desktop, Qt 6.11). VTS enthaelt keine
   uebertragbare Qt-Quick-Grafikschicht (ADR-0008); Leitungen sind Bauteile (ADR-0007).
4. Inhaltsvalidator in CI; `ContentSmokeTest` simuliert jede Vorlage gegen ihre Erfolgsbedingung.

Offen aus Phase 0 (Kern): Kalibrierung der Bauteilmodelle an realem Material; Leistungsmessung auf dem
Referenztablet (Desktop: schlechtester Einzelschritt ca. 8 ms beim Umkippen einer Kippstufe); Tonmodul als
Verhaltensmodell; Nennspannung der Katalog-Gluehlampe (6 V brennt an 9 V nach ca. 5 s durch).
