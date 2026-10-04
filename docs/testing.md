# Teststrategie

## Simulationskern (Catch2, `core/tests/`)
- **Referenzschaltungen** mit analytischer Loesung; Toleranz 2 % (relativ) bzw. 1 mV / 1 µA (absolut):
  1. Spannungsteiler 9 V, 1 kΩ / 2 kΩ -> 6,000 V
  2. LED rot an 9 V ueber 470 Ω -> ca. 15 mA (Uf aus Modell)
  3. RC-Ladung 10 kΩ / 100 µF: U(τ) = 63,2 % (τ = 1 s)
  4. Reihe/parallel gemischt, Kirchhoff-Pruefung (Summe der Stroeme je Knoten < 1e-9 A)
  5. Transistorschalter: Kollektorstrom bei Saettigung
  6. Gluehlampe: Einschaltstromstoss > 5 × Betriebsstrom
  7. Messgeraet V-Modus 10 MΩ belastet 1-MΩ-Teiler messbar
- **Robustheit**: absichtlich unsinnige Schaltungen (Kurzschluss, Schleife nur aus Leitungen, freie Knoten,
  Elko verpolt, LED ohne Vorwiderstand an 9 V, Messgeraet im A-Modus parallel zur Batterie). Erwartung:
  keine Ausnahme, kein NaN, passende `SimEvent`s.
- **Thermik**: LED kurzer Stromstoss uebersteht, Dauerueberlast zerstoert innerhalb 0,3–1 s.
- **Leistung**: Benchmark 40 Bauteile, 1000 Schritte < 50 ms auf Desktop-Referenz:
  `cmake --preset core-release && cmake --build --preset core-release && build/core-release/core/bench/volta_core_bench`
  (Rueckgabe 1 bei Verfehlen; misst auch die Schrittdauern des Blinkmoduls).
- **Zufallsschaltungen** (`test_fuzz.cpp`, Risiko R5): 400 (Debug 150) zufaellig gesteckte Aufbauten aus allen
  Katalogbauteilen mit Bedienhandlungen; kein NaN, Kirchhoff, Nichtkonvergenz hoechstens 0,5 % (Stand: 0).
- **Echtzeit** (`test_realtime.cpp`): ersetzter `operator new` zaehlt Speicheranforderungen in `step()` (muss 0 sein).

## Modell (Qt Test, `app/tests/`)
- Netzlistenbildung aus Feldern und Leitungen (Union-Find).
- Undo/Redo: jede Befehlsart, inkl. Wiederherstellung zerstoerter Bauteile und Kondensatorladung.
- Erfolgspruefung: Experiment 2.4 mit Vorlage, mit alternativem Aufbau (2 × 220 Ω), ohne Widerstand (scheitert).
- Inhaltspakete laden, fehlende Schluessel, Schema-Version.

## Inhalte
- `python3 tools/validate_content.py` in CI und vor jedem Commit mit Aenderungen in `content/`.
- Jedes Experiment wird automatisch mit seiner Vorlage simuliert; die Erfolgsbedingung muss erfuellt werden
  (Test `ContentSmokeTest`).

## Manuell / mit Kindern
- Ab Phase 1 Erprobung mit Kindern; Beobachtungsbogen in `docs/erprobung/`.
- Pilot in der Arbeitsgemeinschaft ab Phase 3.

## CI
- Desktop-Build Linux und Windows, `ctest`, Inhaltspruefung; Android-Build als Artefakt.
