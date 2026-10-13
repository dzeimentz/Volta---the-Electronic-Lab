# CLAUDE.md – Volta – Das Elektronik-Labor

Diese Datei ist der Einstieg fuer Claude Code. Sie beschreibt, was Volta ist, wie das Repository aufgebaut ist,
wie gebaut und getestet wird und welche Regeln ohne Ausnahme gelten. Details stehen in `docs/`; die
Unterverzeichnisse `core/`, `app/` und `content/` haben eigene CLAUDE.md-Dateien mit Regeln fuer ihren Bereich.

## Projekt in Kuerze

Volta ist eine Reihe virtueller Elektronik-Experimentierkaesten fuer Kinder und Jugendliche
(Volta 1: 8–12 Jahre, Volta 2: 11–14, Volta 3: 13–16; spaeter Zuse 1 – Computertechnik fuer Einsteiger).
Kinder stecken Bauteile zwischen bezeichnete Kontaktfelder einer Grundplatte, ziehen Leitungen, bedienen ein
Bedienpult und beobachten das Ergebnis einer echten Schaltungssimulation. Gestalterisches Vorbild ist das
Prinzip der Kosmos-X-Reihe (Kontaktfelder als Knoten, Bedienpult, Module) – uebernommen wird nur das Prinzip.

- Hersteller: DC Software Engineering, Daniel Zeimentz, Irrel (DE)
- Web: https://dc-software-engineering.com – Support: info@dc-software-engineering.com
- Massgebliches Konzept: `docs/Konzept_Volta_v3.0.pdf` (falls vorhanden) bzw. die Zusammenfassung in `docs/`
- Aktuelles Ziel: **Volta 1**, Phase 0/1 (Simulationskern, Projektgeruest, Prototyp), siehe `docs/roadmap.md`

## Technikstapel (identisch mit Visual Technic Studio)

- **Qt 6.8 LTS oder neuer**, Qt Quick/QML fuer die Oberflaeche, Qt RHI fuer die Grafik
- **C++17** durchgehend; Simulationskern `core/` ist **frei von Qt** (nur Standardbibliothek)
- CMake ≥ 3.24 mit Presets, Ninja
- Tests: Catch2 v3 (Kern), Qt Test (Modell/App)
- Plattformen: Android arm64 + armv7 (Phase 1), Windows/Linux x86-64 (Phase 5a), iPadOS arm64 (Phase 5b)

## Repository

```
CLAUDE.md                 dieser Leitfaden
CMakeLists.txt            Wurzelprojekt
CMakePresets.json         desktop-debug, desktop-release, android-arm64, android-armv7, ios
core/                     Simulationskern (C++17, Qt-frei)  -> core/CLAUDE.md
  include/volta/core/     oeffentliche Header
  src/                    MNA, Bauteilmodelle, Zeitintegration, Ereignisse
  tests/                  Catch2-Tests, Referenzschaltungen
app/                      Qt-Anwendung                       -> app/CLAUDE.md
  src/model/              Schaltungsmodell, Befehle (Undo/Redo), Feldbelegung
  src/services/           Inhaltspakete, Profile, Sicherung, Konfiguration, Audio, Sprache
  qml/                    Oberflaeche (Grundplatte, Pult, Werkzeugkasten, Readme, Info)
content/                  Inhaltspakete (JSON)               -> content/CLAUDE.md
  schema/                 JSON-Schemata
  catalog/                Bauteil- und Modulkatalog
  volta1/                 Experimente und Texte von Volta 1
i18n/                     Qt-Linguist-Dateien (volta_de.ts, _fr, _en, _lb)
packaging/                android, windows (MSIX/MSI), linux (AppImage), ios
tools/                    Hilfsskripte (validate_content.py)
docs/                     Architektur, Fachmodell, Spezifikationen, ADRs
.claude/                  Einstellungen, Slash-Befehle, Agenten fuer Claude Code
```

## Bauen und Testen

```bash
# Desktop (Entwicklungs- und Testziel)
cmake --preset desktop-debug
cmake --build --preset desktop-debug
ctest --preset desktop-debug

# Nur der Simulationskern (ohne Qt, schnell)
cmake --preset core-only
cmake --build --preset core-only
ctest --preset core-only

# Leistungsmessung des Kerns (Release)
cmake --preset core-release
cmake --build --preset core-release
build/core-release/core/bench/volta_core_bench

# Inhaltspakete pruefen (vor jedem Commit, der content/ beruehrt)
python3 tools/validate_content.py

# Gesamtpruefung: Inhalte, Kern, App inkl. ContentSmokeTest (--quick: ohne App, --bench: mit Leistungsmessung)
python3 tools/check_all.py

# Windows-Entwicklungsrechner (Git Bash): MinGW aus der Qt-Installation, Qt 6.11
#   export PATH="/c/Qt/Tools/mingw1310_64/bin:/c/Qt/6.11.0/mingw_64/bin:$PATH" QT_ROOT_DESKTOP="C:/Qt/6.11.0/mingw_64"
#   beim ersten Konfigurieren zusaetzlich: -DCMAKE_CXX_COMPILER=g++
# App mit Bildschirmfoto statt Bedienung (Sichtpruefung): VOLTA_SCREENSHOT=bild.png build/desktop-debug/app/volta1.exe
#   weitere Entwicklerhilfen: VOLTA_SCREENSHOT_S=<s>, VOLTA_SCREENSHOT_MAINSWITCH=1, VOLTA_DEMO_GALLERY=1,
#   VOLTA_DEMO_SOLVE=1 (loest das gefuehrte Experiment mit dem Musteraufbau; VOLTA_DEMO_EXP=<id> waehlt es,
#   VOLTA_DEMO_INSIDE=<Modul-ID> oeffnet dazu »Blick hinein«), VOLTA_DEMO_GUIDE=1 (Experiment VOLTA_DEMO_EXP mit
#   Aufbauhilfe; VOLTA_DEMO_GUIDE_ADD=<Typ>@<Feld> steckt vorher ein Bauteil), VOLTA_DEMO_PLAN=1 (Schaltplan einblenden), VOLTA_DEMO_SENSORS=1 (Bauteile der Stufe 3 mit Taschenlampe und Foehn), VOLTA_DEMO_RECORDER=1 (Zeitschreiber am ladenden Elko), VOLTA_DEMO_FLYOUT=1 (Werkzeugleiste ausgeklappt), VOLTA_DEMO_TOOLBOX=3 (Formelrechner), VOLTA_DEMO_TOOLBOX=4 (Fehlersuche mit fehlerhaftem Aufbau), VOLTA_DEMO_TOOLBOX=5 (Teileliste zu VOLTA_DEMO_EXP; mit VOLTA_DEMO_CARD=<datei.pdf> wird die Aufbaukarte direkt geschrieben), VOLTA_DEMO_PARENT=gate|area|qr (PIN-Abfrage, Elternbereich, Fortschritts-QR-Code), VOLTA_MANAGED_INI=<Datei> (verwaltete Konfiguration testen), VOLTA_DEMO_PROFILES=1 (Profilauswahl mit Beispielprofilen), VOLTA_DEMO_SELECT=<Bauteil-ID> bzw. VOLTA_DEMO_SELECT_WIRE=<Leitungs-ID>, VOLTA_DEMO_COLUMNS=8|10|12, VOLTA_DEMO_SIZE=<b>x<h> (Fenstergroesse in dp), VOLTA_DEMO_ZOOM=<massstab> (Arbeitsflaeche gezoomt),
#   VOLTA_DEMO_LANG=fr|en (Sprache ohne Speichern), VOLTA_SHOW_TIME=1 (Simulationszeit), VOLTA_DEMO_INFO=1, VOLTA_DEMO_HELP=1|<kapitel> (Hilfe, z. B. werkzeug, eltern), VOLTA_DEMO_GUIDE_SHORT=1 (Kurzanleitung), VOLTA_DEMO_PRESENTATION=1 (Praesentationsmodus), VOLTA_DEMO_TASK=qr|<Code oder Link> (Aufgabe der Lehrkraft zeigen bzw. wie gescannt oeffnen),
#   VOLTA_DEMO_FONT=0|1|2 (Schriftstufe), VOLTA_DEMO_JOURNAL=1, VOLTA_DEMO_TOOLBOX=0|1|2, VOLTA_DEMO_SCHEMATIC=1, VOLTA_DEMO_FREELAB=1, VOLTA_NO_SPLASH=1 (ohne Startbildschirm), VOLTA_DEMO_METER=v|analog|a|ohm (Messaufbau; mit VOLTA_DEMO_ANALOG=1 als Zeiger), VOLTA_DEMO_METER_DOCKED=0|1 (Messgeraet neben der Grundplatte / oben im Pult)

# Lokales Release (lauffaehiges Programm mit Qt-Laufzeit und Inhalten in Release/, nicht im Repository)
python3 tools/deploy_release.py

# Anleitungen als PDF (ausfuehrlich und Kurzanleitung, mit Bildschirmfotos) nach Release/Anleitung/
python3 tools/manual_pdfs.py            # --bilder nimmt die Bildschirmfotos neu auf

# App-Symbol: Quelle app/resources/icons/volta.svg; PNG (16-1024 px) und volta.ico daraus neu erzeugen
python3 tools/make_icons.py

# Android-Test-APK (bauen, ausduennen, pruefen; --install spielt sie auf angeschlossene Tablets)
python3 tools/build_android.py --install

# Android (benoetigt QT_ROOT_ANDROID, QT_HOST_PATH, ANDROID_NDK_ROOT, ANDROID_SDK_ROOT)
cmake --preset android-arm64
cmake --build --preset android-arm64
```

Nach jeder Aenderung am Kern: `ctest --preset core-only` muss gruen sein. Nach Aenderungen an `content/`:
`python3 tools/validate_content.py` muss ohne Fehler durchlaufen.

## Unverhandelbare Regeln

1. **Keine Datenerhebung, kein Netz.** Kein Konto, keine Analyse, keine Werbung, keine Kaeufe in der App,
   kein Netzwerkzugriff im Kernbetrieb. Keine Bibliothek einbinden, die das verletzt. Siehe `docs/privacy-safety.md`.
2. **Keine Nachbildung des Vorbilds.** Keine Texte, Abbildungen, Versuchsaufbauten, Kapitelnamen,
   Feldnummerierung, Figuren oder Namen aus Kosmos-Handbuechern. Der Name »Kosmos« erscheint nirgends
   in App, Store-Texten oder Inhalten. Experimente sind Eigenentwicklungen.
3. **Physik ist ehrlich.** Verhalten entsteht aus der Simulation, nie aus hinterlegten Ergebnissen.
   Vereinfachungen sind erlaubt, Verfaelschungen nicht. Abweichung zu Referenzwerten < 2 %.
4. **Kern bleibt Qt-frei.** `core/` darf nur die C++-Standardbibliothek verwenden (Wiederverwendung in VTS).
5. **Jede Aufbauaenderung ist ein Befehl.** Alles, was den Aufbau aendert, laeuft ueber das Befehlsmuster
   (Undo/Redo) – inklusive Wiederherstellung des Bauteilzustands. Bedienhandlungen (Schalter, Regler) nicht.
6. **Kindersicherheit.** Zerstoerende Experimente tragen »Nur in der App!«; »Jetzt in echt« schlaegt nie
   zerstoerende Versuche vor; keine Netzspannung als Experimentierquelle. Externe Links nur hinter der Elternschranke.
7. **Barrierefreiheit.** Bedienelemente ≥ 48 dp, Kontaktfelder ≥ 56 dp hoch; Farbe nie alleiniger Informationstraeger.
8. **Mehrsprachig von Anfang an.** Kein sichtbarer Text ohne Uebersetzungsschluessel. Sprachen DE, FR, EN, LU
   (Anzeige »LU«, interner Code `lb`).

## Konventionen

- C++: Namen auf Englisch, `CamelCase` fuer Typen, `camelCase` fuer Funktionen, `m_` fuer Member,
  `snake_case` fuer Dateien. Formatierung mit `.clang-format`. Keine Ausnahmen im Simulationspfad
  (Echtzeit-Thread); Fehler als Statuswerte.
- QML: eine Komponente je Datei, `PascalCase.qml`; keine Logik, die ins Modell gehoert.
- Fachbegriffe im Code: `Field` (Kontaktfeld), `Wire` (Leitung), `Component` (Bauteil), `Module` (Modul),
  `ControlDesk` (Bedienpult), `DeskStrip` (Pultleiste), `ModuleBar` (Modulleiste), `Meter` (Dreifach-Messgeraet).
  Siehe `docs/glossary.md`.
- Inhaltspakete: JSON-Schluessel deutsch wie in `docs/content-format.md` festgelegt.
- Kommentare und Commit-Nachrichten auf Deutsch, in Codebloecken ASCII (ae, oe, ue, ss).
- Commits klein und thematisch; Format `bereich: kurze Beschreibung` (z. B. `core: Diodenmodell mit Spannungsbegrenzung`).

## Arbeitsweise fuer Claude Code

- Vor groesseren Aenderungen die passende Datei in `docs/` lesen; bei Architekturfragen `docs/architecture.md`
  und die ADRs in `docs/adr/`.
- Neue Architekturentscheidungen als ADR in `docs/adr/` festhalten (Vorlage: bestehende ADRs).
- Neue Bauteile: Slash-Befehl `/neues-bauteil`; neue Experimente: `/neues-experiment`.
- Vor Abschluss einer Aufgabe: Tests ausfuehren, `tools/validate_content.py` bei Inhaltsaenderungen,
  bei Kernaenderungen Agent `simulations-pruefer`, bei Inhalten Agent `inhalts-pruefer`.
- Nichts pushen, keine Releases signieren, keine Store-Uploads ohne ausdrueckliche Anweisung.

## Weiterfuehrende Dokumente

| Datei | Inhalt |
|---|---|
| `docs/architecture.md` | Schichten, Module, Threads, Datenfluss, Undo/Redo |
| `docs/simulation-core.md` | MNA, Newton-Raphson, Zeitintegration, Bauteilmodelle, Thermik |
| `docs/domain-model.md` | Grundplatte, Kontaktfelder, Pultleiste, Modulleiste, Messgeraet, Bezeichner |
| `docs/content-format.md` | Experimente, Module, Bauteilkatalog, Erfolgsbedingungen |
| `docs/ui-spec.md` | Bedienung, Lichtring, Klanganzeige, Spannungsanzeige, Werkzeugkasten, Readme, Info |
| `docs/i18n.md` | Sprachen, Uebersetzung, Tonaufnahmen |
| `docs/platforms.md` | Android, Desktop x86-64, iPadOS, Paketierung, verwaltete Konfiguration |
| `docs/testing.md` | Teststrategie, Referenzschaltungen, Leistungsmessung |
| `docs/privacy-safety.md` | Datenschutz, Kinderschutz, Store-Auflagen |
| `docs/roadmap.md` | Phasen, Aufwaende, offene Entscheidungen |
| `docs/volta2-planung.md` | Entwurf Volta 2: Schwerpunkte, Bedienpult II, Audio-Risiko, Stufen, Timer 555 |
| `docs/Konzept_Zuse.md` | Konzept der Zuse-Reihe (v0.2): Werkbank, Systemplatine, Logiksimulation, Lernpfade; ADR-0010 |
| `docs/zuse1-planung.md` | Ergaenzungen zu Zuse 1: Entscheidungen, Auswertung ELRAD-Lehrgang Digitaltechnik |
| `docs/glossary.md` | Begriffe deutsch/englisch |
