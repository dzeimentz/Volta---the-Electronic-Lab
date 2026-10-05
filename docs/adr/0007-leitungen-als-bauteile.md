# ADR-0007: Leitungen als Bauteile in der Netzliste statt Knotenverschmelzung

- Status: angenommen
- Datum: 2026-10-05

## Kontext
Die Architektur sah vor, durch Leitungen verbundene Felder im `NetlistBuilder` per Union-Find zu einem Knoten
zusammenzufassen. Die Oberflaeche braucht aber den Strom **je Leitung**: Stromflussdarstellung (wandernde Punkte
entlang der Leitungen), Gluehen einer Leitung bei Kurzschluss, Fehlersuche (»durch diese Leitung fliesst nichts«).
Bei verschmolzenen Knoten ist der Strom einer einzelnen Leitung nicht mehr bestimmbar.

## Entscheidung
Jede gezogene Leitung wird als eigenes Bauteil `core::Wire` (0,01 Ohm) in die Netzliste aufgenommen. Jeder benutzte
Anschlusspunkt (Kontaktfeld, Pultfeld, Messgeraetebuchse, Feld der Modulleiste) ist ein eigener Knoten. Union-Find
entfaellt.

## Folgen
- Mehr Knoten im Gleichungssystem (hoechstens ca. 75 in Volta 1 plus innere Knoten); die dichte LU bleibt fuer
  Volta 1 ausreichend (Leistungsprogramm: 42 Bauteile, 1000 Schritte ca. 41 ms).
- Kirchhoff und Strom je Leitung sind exakt verfuegbar (`Probe`, `ComponentReading` der Leitung).
- Der Leitungswiderstand von 0,01 Ohm war ohnehin vorgesehen (Robustheit gegen ideale Schleifen).
- Ab Volta 2 profitiert die duenn besetzte LU besonders, da Leitungen nur je zwei Eintraege beitragen.
