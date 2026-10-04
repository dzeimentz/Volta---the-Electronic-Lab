# app/ – Regeln fuer Anwendung, Modell und Oberflaeche

Lies vor Aenderungen `docs/architecture.md`, `docs/domain-model.md` und `docs/ui-spec.md`.

## Modell (`src/model/`)
- Jede Aufbauaenderung ist ein `QUndoCommand` in `src/model/commands/` mit Zustandsschnappschuss (ADR-0005).
  Neue Befehle brauchen Tests fuer `redo`, `undo` und ggf. `mergeWith`.
- Bedienhandlungen (Schalter, Taster, Regler, Hauptschalter) sind **keine** Befehle.
- Kommunikation mit dem Simulator nur ueber `SimulationBridge` (Kopien, lockfreie Queues). Kein direkter Zugriff
  auf Simulatorzustand aus QML.
- Feld-, Pult- und Modulbezeichner exakt wie in `docs/domain-model.md`.

## Dienste (`src/services/`)
- Kein Netzwerkcode. `QNetworkAccessManager` ist verboten.
- Dateizugriffe nur ueber `QStandardPaths`; Sicherung atomar (`QSaveFile`).
- Verwaltete Konfiguration nur ueber `ManagedConfig`; Schluessel siehe `docs/platforms.md`.

## Oberflaeche (`qml/`)
- Eine Komponente je Datei, `PascalCase.qml`. Keine Fachlogik in QML.
- Eingaben nur ueber Pointer Handlers (`DragHandler`, `TapHandler`, `WheelHandler`, `PinchHandler`);
  Maus, Finger und Stift gleichwertig. Tastenkuerzel ueber `Shortcut`.
- Jeder sichtbare Text ueber `qsTr()` oder Inhalts-Schluessel. Keine festen Texte.
- Mindestgroessen: Bedienelemente 48 dp, Kontaktfelder 56 dp. Farbe nie allein.
- Farben und Masse zentral in `qml/Theme.qml` (Singleton).
- Externe Links (`Qt.openUrlExternally`) nur hinter `ParentGate`.

## Bauen
```bash
cmake --preset desktop-debug && cmake --build --preset desktop-debug && ctest --preset desktop-debug
```
