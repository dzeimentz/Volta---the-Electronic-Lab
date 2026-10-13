# ADR-0010: Zuse als eigene Produktlinie mit Werkbank und Systemplatine

- Status: vorgeschlagen
- Datum: 2026-10-12
- Konzept: `docs/Konzept_Zuse_v0.2.pdf` bzw. `docs/Konzept_Zuse.md`; Entwuerfe in `docs/zuse/`

## Kontext
Das Konzept Volta v3.0 sieht Zuse 1 als Erweiterung der Volta-Reihe auf derselben Grundplatte mit Kontaktfeldern
vor (Abschnitte 4.3, 6.3, 11.2). Erste Entwuerfe zeigten: Ein 4-Bit-Rechner besteht aus mehreren Hundert Gattern.
Auf Kontaktfeldern mit Einzelleitungen entsteht ein Leitungsknaeuel, das auf dem Tablet nicht bedienbar ist und die
Mindestgroessen (48 dp / 56 dp) nicht einhaelt. Ein eng gerastertes 74xx-Steckbrett ist fuer Volta ausdruecklich
ein Nicht-Ziel und ebenso zu kleinteilig. Das Lernziel der Computertechnik ist Abstraktion: Ein gepruefter Baustein
wird zur Black Box und traegt die naechste Ebene.

## Entscheidung
Zuse wird eine eigene Produktlinie neben Volta mit drei Baenden (Zuse 1: Gatter bis 4-Bit-Rechner, Zuse 2: 8-Bit-CPU
und Assembler, Zuse 3: Mikrocontroller mit Sensoren und Aktoren).

- Arbeitsflaeche: **Werkbank** (aus vorhandenen Bausteinen einen neuen bauen, mit Wahrheitstabelle oder Testfolge
  pruefen, als **Chip** speichern; »Blick hinein« ueber alle Ebenen) und **Systemplatine** (geprueften eigenen Chips
  an Bus und Steckplaetzen, Takt, Einzelschritt, Programm). Busse mit 4/8 Bit statt Einzelleitungen.
- Simulation: neue Qt-freie Bibliothek `logic/` (C++17, nur Standardbibliothek), ereignisgesteuert mit Werten
  0, 1, X und Z. Ein Verhaltensmodell eines Chips entsteht nur aus einer bestandenen Pruefung desselben Chips.
  Kopplung an den Volta-Kern erst in Zuse 3 an den Anschluessen nach aussen.
- Gestaltung: eigene Linie (dunkle Platine, goldene Loetpads, Monospace fuer Werte, Akzent je Band; Pegel 1 =
  Akzent und durchgezogen, 0 = grau und gestrichelt).
- Gemeinsam mit Volta bleiben Qt-Stapel und Plattformen (ADR-0001), Befehlsmuster (ADR-0005), keine
  Datenerhebung (ADR-0006), Inhaltspakete und Einbettung (ADR-0009), Lernpfad, Mehrsprachigkeit und
  Barrierefreiheit.
- Vorschlag Code: im selben Repository als Editionen `zuse1|zuse2|zuse3` mit eigenem QML-Modul »Zuse«
  (Entscheidung vor Phase Z1, siehe Konzept Abschnitt 11).
- Reihenfolge: Volta 1 hat Vorrang; vor Volta 1.0 nur Markenrecherche und Prototyp der Werkbank (Phase Z0).

## Folgen
- Didaktisch: Jede Stufe endet mit einem selbst gebauten, geprueften Chip; am Ende laeuft ein Rechner aus eigenen
  Chips. Busse und grosse Bausteine halten die Oberflaeche tablettauglich.
- Technisch: neue Logikbibliothek, neue Oberflaeche (Werkbank, Systemplatine, Logikanalysator, Programmeditor)
  und ein Bausteinformat (JSON, Schema in `content/schema/`). Die schwierigste Huerde des Volta-Konzepts, die
  Kopplung zweier Simulationsarten ohne Zeitversatz, entfaellt fuer Zuse 1 und 2.
- Grobschaetzung Zuse 1: ca. 145-185 PT (vor dem Prototyp unsicher).
- Das Konzept Volta v3.0 verweist beim naechsten Stand fuer Zuse auf das eigene Konzept.

## Verworfen
- Volta-Grundplatte mit Gatter-Bausteinen: ab dem Addierer unuebersichtlich, kein Rechner baubar.
- Steckbrett mit 74xx-Chips: kleinteilig, sehr viele Leitungen, Nicht-Ziel der Reihe.
- Reines Programmierspiel ohne Hardwareebene: verfehlt das Lernziel »wie ein Rechner funktioniert«.
