# ADR-0005: Undo/Redo ueber Befehlsmuster mit Zustandswiederherstellung

- Status: angenommen
- Datum: 2026-10-04

## Entscheidung
Jede Aufbauaenderung ist ein `QUndoCommand`. Vor der Ausfuehrung wird der Zustand der betroffenen Bauteile
gesichert (intakt/zerstoert, Temperatur, Ladung). Undo stellt Aufbau und Zustand her; danach DC-Arbeitspunkt.
Bedienhandlungen (Schalter, Regler) und Forscherheft sind ausgenommen.

## Begruendung
- Kinder erleben Rueckgaengig als verstaendliche Zeitreise (zerstoerte LED ist wieder heil).
- Von Anfang an in der Architektur, nicht nachtraeglich (Risiko R8).

## Folgen
- Jeder neue Befehl braucht Tests fuer `redo`, `undo`, `mergeWith`.
- Verlauf: Sitzung vollstaendig, gespeichert die letzten 200 Schritte.
