# ADR-0002: Qt-freier Simulationskern in C++17

- Status: angenommen
- Datum: 2026-10-04

## Entscheidung
`core/` verwendet ausschliesslich die C++17-Standardbibliothek. Keine Abhaengigkeit zu Qt oder `app/`.

## Begruendung
- Wiederverwendung im Elektrik-Modul von VTS Education Light und in kuenftigen Werkzeugen.
- Schnelle, isolierte Tests (`cmake --preset core-only`).
- Echtzeitfaehigkeit: kontrollierte Speicherverwaltung, keine Ausnahmen im Simulationspfad.

## Folgen
- Uebergabe an das Modell ueber eigene Datentypen (`Circuit`, `Probe`, `SimEvent`).
- Threading-Primitive aus `<thread>`, `<atomic>`; lockfreie Queues selbst implementiert oder header-only ohne Qt.
