# Architektur

## Ueberblick

Volta trennt drei Schichten mit klaren Zustaendigkeiten. Die Oberflaeche kennt keine Physik, der
Simulationskern keine Darstellung. Inhalte liegen als versionierte Datenpakete vor. Aus einer Codebasis
entstehen alle Baende (Volta 1–3, Zuse 1) als eigene Apps und alle Plattformen (Android, Windows, Linux, iPadOS).

```
+---------------------------------------------------------------+
|  Darstellung und Bedienung   (Qt Quick / QML)        app/qml   |
|  Grundplatte, Bedienpult, Lernpfad, Forscherheft,              |
|  Werkzeugkasten, Readme, Info, Elternbereich                   |
+---------------------------------------------------------------+
|  Schaltungsmodell            (C++17, QObject)   app/src/model  |
|  Feldbelegung, Leitungen, Module, Befehle (Undo/Redo),         |
|  Netzlistenbildung, Diagnose, Erfolgspruefung                  |
+---------------------------------------------------------------+
|  Dienste                     (C++17, Qt)     app/src/services  |
|  Inhaltspakete, Profile, Sicherung, Sprache, Audio,            |
|  Sensoren, verwaltete Konfiguration, Supportangaben            |
+---------------------------------------------------------------+
|  Simulationskern             (C++17, Qt-frei)          core/   |
|  MNA-Loeser, Bauteilmodelle, Zeitintegration, Thermik,         |
|  Ereignisse, Messschnittstelle                                 |
+---------------------------------------------------------------+
   Qt RHI: Vulkan / OpenGL ES (Android), Metal (iPadOS),
           Direct3D (Windows), Vulkan / OpenGL (Linux)
```

## Bausteine

### core/ – Simulationskern
- Bibliothek `volta_core` (statisch), nur C++17-Standardbibliothek.
- Oeffentliche API in `core/include/volta/core/`:
  - `Circuit` – Netzliste aus Knoten und Bauteilinstanzen (Eingabe aus dem Modell)
  - `Simulator` – Zustand, Zeitschritt, Loesung, Ereignisse
  - `ComponentModel` – Schnittstelle fuer Bauteilmodelle (Stempeln in MNA-Matrix, Newton-Linearisierung,
    Zustandsaktualisierung, Thermik)
  - `Probe` – Messschnittstelle: Knotenpotentiale, Zweigstroeme, Bauteilzustaende
  - `SimEvent` – Ereignisse (Bauteil zerstoert, Sicherung ausgeloest, Konvergenzproblem)
- Wiederverwendung im Elektrik-Modul von VTS Education Light. Deshalb keine Abhaengigkeit von Qt oder `app/`.

### app/src/model – Schaltungsmodell
- `Board` – Grundplatte mit Kontaktfeldern (A1–E8 in Volta 1), Pultleiste, Modulleiste.
- `Placement` – Bauteil auf Feldern (Spannweite kurz = 1 Feldabstand, lang = 2).
- `Wire` – frei gezogene Leitung zwischen zwei Anschlusspunkten mit Farbe und optionalem Zwischenpunkt.
- `ModuleInstance` – Modul in der Modulleiste; Teilschaltung oder Verhaltensmodell.
- `NetlistBuilder` – fasst Felder und Leitungen zu Knoten zusammen (Union-Find) und erzeugt `core::Circuit`.
- `CommandStack` auf Basis von `QUndoStack`; jede Aufbauaenderung ist ein `QUndoCommand` (siehe unten).
- `SuccessEvaluator` – wertet Erfolgsbedingungen eines Experiments ueber den simulierten Zustand aus.
- `Diagnostics` – Fehlersuche (offener Kreis, unbeschaltetes Bein, Verpolung, Kurzschluss, fehlender Vorwiderstand).
- Alle fuer QML sichtbaren Typen als `QObject`/`QAbstractListModel` mit `QML_ELEMENT`.

### app/src/services – Dienste
- `ContentRepository` – laedt Inhaltspakete (JSON) und Kataloge, prueft Schema-Version.
- `ProfileStore` – Forscherprofile (Symbol, Spitzname), Export/Import per Datei oder QR-Code.
- `AutoSave` – sichert eigene Schaltungen und Forscherheft nach jeder Aenderung (atomar, Schreiben in Temp-Datei + Umbenennen).
- `LanguageService` – Laufzeitumschaltung ueber `QTranslator`, Sprachcodes `de`, `fr`, `en`, `lb` (Anzeige »LU«).
- `SpeechService` – Qt TextToSpeech; fuer `lb` Abspielen eigener Tonaufnahmen ueber Qt Multimedia.
- `AudioService` – Tonausgabe aus einem Ringpuffer, den der Simulator fuellt; Klassenmodus (aus/Kopfhoerer/normal).
- `SensorService` – Umgebungslicht ueber Qt Sensors, sonst Schieberegler.
- `ManagedConfig` – verwaltete Konfiguration (Android Managed Configurations, iOS Managed App Configuration,
  Registry bzw. `volta.ini` auf dem Desktop). Schluessel: siehe `docs/platforms.md`.
- `SupportInfo` – stellt Version, Build, Plattform, OS, Geraetemodell, Paketversionen zusammen (ohne Personenbezug).

### app/qml – Oberflaeche
- `Main.qml`, `Dashboard.qml` (Startseite mit Flaggensymbol, Info-Knopf), `Lab.qml` (Experiment),
  `BoardView.qml`, `ControlDesk.qml`, `DeskStrip.qml`, `ModuleBar.qml`, `Toolbox/*.qml`, `Readme/*.qml`,
  `InfoPage.qml`, `ParentGate.qml`.
- Eingabe ausschliesslich ueber Pointer Handlers (Finger, Stift, Maus gleichwertig).

## Threads und Datenfluss

```
 UI-Thread (QML, Modell)                 Simulations-Thread (core)
 ----------------------                  -------------------------
 Aufbauaenderung -> Befehl ------------> neue Netzliste (Kopie)  [Topologie-Version n]
 Bedienhandlung (Schalter, Regler) ----> Parameteraenderung      [lockfreie Queue]
                                         Zeitschritt 1 ms (V1), adaptiv ab V2
 Anzeige 60 Hz  <---------------------- Zustandsschnappschuss   [Doppelpuffer, atomarer Tausch]
 AudioService   <---------------------- Audiopuffer 48 kHz      [Ringpuffer, ab V2 relevant]
 Ereignisse     <---------------------- SimEvent                [Queue]
```

- Der Simulator besitzt seine Daten exklusiv. Kommunikation nur ueber Kopien und lockfreie Queues.
- Spannungsanzeigen werden mit ca. 10 Hz und Hysterese aktualisiert, digitale Messgeraeteanzeige mit ca. 3 Hz,
  Zeiger mit nachgebildeter Traegheit (Darstellung, nicht Simulation).
- Keine Speicheranforderungen im Simulationsschritt; Matrizen werden bei Topologiewechsel neu angelegt.

## Undo/Redo (Befehlsmuster)

- Jede Aufbauaenderung ist ein Befehl: Bauteil setzen/verschieben/drehen/entfernen, Wert aendern, Leitung
  ziehen/umsetzen/umfaerben/loeschen, Zwischenpunkt, Modul setzen/entfernen, Spannungsanzeige setzen, Reparieren,
  Uebernahme eines Werts aus dem Werkzeugkasten.
- Nicht erfasst: Schalter, Taster, Drehregler, Hauptschalter, Eintraege im Forscherheft.
- Jeder Befehl speichert vor der Ausfuehrung einen **Zustandsschnappschuss** der betroffenen Bauteile
  (intakt/zerstoert, Temperatur, Kondensatorladung). Undo stellt Aufbau **und** Zustand her; danach wird das
  Gleichgewicht neu berechnet (DC-Arbeitspunkt) und die Simulation fortgesetzt.
- Verlauf: alle Schritte der Sitzung; mit eigenen Schaltungen gespeichert werden die letzten 200.
- Zusammenfassen: aufeinanderfolgende Verschiebungen desselben Objekts werden ueber `mergeWith` vereint.

## Inhaltspakete

- `content/<band>/` enthaelt Experimente, Texte (i18n-Schluessel) und Tonaufnahmen.
- Katalog `content/catalog/bauteile.json` beschreibt Bauteiltypen, Anschluesse, Parameter, Modellbezug.
- Formate: `docs/content-format.md`, Schemata: `content/schema/`.
- Pakete werden zur Laufzeit geladen; neue Experimente erfordern keine neue Programmversion,
  solange keine neuen Bauteilmodelle noetig sind.

## Baende als eigene Apps

- Ein Quellbaum, je Band ein CMake-Ziel bzw. eine Option `VOLTA_EDITION=volta1|volta2|volta3|zuse1`.
- Die Edition bestimmt App-ID, Name, Symbol, enthaltene Inhaltspakete und freigeschaltete Bauteilmodelle.

## Abhaengigkeiten

| Bereich | Bibliothek | Lizenz | Hinweis |
|---|---|---|---|
| Oberflaeche, Dienste | Qt 6.8 LTS+ (Quick, QuickControls2, Multimedia, TextToSpeech, Sensors, Svg) | LGPLv3 / kommerziell | iOS: Lizenzweg vor Phase 5b klaeren (ADR-0001) |
| Kern | C++17-Standardbibliothek | – | keine weiteren Abhaengigkeiten |
| Tests | Catch2 v3 | BSL-1.0 | nur Testziel |

Neue Abhaengigkeiten nur nach Pruefung von Lizenz, Groesse und Datenschutz; Eintrag in diese Tabelle
und in die Lizenzliste des Info-Bereichs.
