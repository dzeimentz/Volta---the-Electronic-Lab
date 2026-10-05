# ADR-0008: Eigene 2D-Grafikschicht in Qt Quick statt Uebernahme aus VTS

- Status: angenommen
- Datum: 2026-10-05

## Kontext
Das Konzept (v3.0, Abschnitt 7.2 und Phase 0) sieht vor, die Grafikschicht aus Visual Technic Studio zu
uebernehmen. Die Sichtung von `J:\VISUAL Technic STUDIO` in Phase 0 ergab: VTS ist eine 3D-Anwendung auf Basis von
Qt Widgets, OpenGL (QOpenGLWidget) und Python (pybind11). Es gibt keine eigene Qt-Quick/QML-Grafikschicht; die
einzigen QML-Dateien gehoeren zum mitgelieferten PyQt6. Eine 2D-Darstellung von Kontaktfeldern, gezogenen Leitungen
und Spannungsanzeigen in Qt Quick existiert dort nicht.

## Entscheidung
Volta baut seine 2D-Grafik eigenstaendig in Qt Quick auf: QML-Komponenten je Element (`ContactField`,
`VoltageBars`, `PartItem`, `WireItem`), Leitungen als `QtQuick.Shapes` (Kurvenrenderer), Farben und Masse in
`Theme.qml`. Darstellung ueber Qt RHI wie vorgesehen.

## Folgen
- Die im Konzept angenommene Ersparnis fuer die Grafikschicht in Phase 0 entfaellt; der Aufwand verschiebt sich in
  Phase 1 (Gestaltung von Bauteilen, Ziehen, Einrasten).
- Weiterhin aus VTS uebernehmbar: Lizenztoken (`.vtslic`), Windows-Paketierung (MSIX/MSI), Erfahrungen mit
  Inhaltspaketen. Zu pruefen in Phase 3 bzw. 5a.
- Der Simulationskern bleibt wie geplant Qt-frei und fuer das Elektrik-Modul von VTS wiederverwendbar.
