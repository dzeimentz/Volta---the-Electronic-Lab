# ADR-0012: Baende mit Grundband (Edition volta2 und folgende)

- Status: angenommen
- Datum: 2026-10-31

## Kontext
Volta 2 baut auf Volta 1 auf (Konzept Volta 2, Kapitel 2): Alle Bauteile und Module von Volta 1 stehen von Anfang an
zur Verfuegung, ihre Lexikontexte gelten weiter, und Volta 2 kommt aus derselben Codebasis (`VOLTA_EDITION`). Bisher
war der Band `volta1` in `LabController` fest eingetragen, und jeder Band haette Katalogtexte und Lexikon doppelt
fuehren muessen.

## Entscheidung
- `paket.json` eines Bandes nennt seinen Grundband in `"basis"` (z. B. `volta2` -> `volta1`). Die Kette heisst
  Abstammung (`ContentRepository::lineage`, ebenso in `tools/validate_content.py`).
- `ContentRepository::loadTexts(band, sprache)` laedt die Texte der ganzen Abstammung, vom Grundband an; der Band
  selbst hat Vorrang (z. B. eigene Stufentitel unter denselben Schluesseln).
- `ContentRepository::loadCatalog(katalog, band)` nimmt nur Eintraege der Abstammung auf; Eintraege eines
  Grundbandes sind sofort frei (`fromStage` 0), Eintraege des Bandes selbst folgen `ab_stufe`. Ohne Band (Tests,
  Werkzeuge) bleibt der ganze Katalog wie bisher.
- `LabController` bekommt den Band im Konstruktor; `main.cpp` uebergibt `VOLTA_EDITION`. Symbol und Windows-Ressource
  kommen aus `app/resources/icons/<band>/` bzw. `app/resources/<band>.rc`, sofern vorhanden.
- Entwickelt wird Volta 2 im Worktree `volta-volta2` (Zweig `volta2`); Volta 1 auf `main` bleibt eingefroren.

- Pult nach Bedarf (Konzept Volta 2, 5.5): `paket.json` nennt in `"pult_nach_bedarf"` die Pultteile des
  Geraetefachs mit genau einer Freischaltregel: `ab_stufe`, `ab_experiment` (sobald das Experiment frei ist; fehlt
  es noch im Paket, gilt seine Stufe) oder `bei_nutzung` (mit dem ersten freien Experiment, dessen Vorlage das Teil
  benutzt). »Pult zeigen« setzt alles Freigeschaltete in einem Befehl ein. Ist die Liste da, traegt der Aufbau seine Pultbelegung (`Board::deskParts`, JSON
  `"pultteile"`, Befehl mit Rueckgaengig); sonst gilt das feste Pult von Volta 1. Batterie 2 (`P:+B2`, `P:-B2`) ist
  potentialfrei, haengt zweipolig am Hauptschalter und hat 10 MOhm von `P:-B2` nach `0V` (rechenbar, ausgeschaltet
  unter 0,05 V, als Last 0,45 uA).
- Wechselquelle (`P:W~a`, `P:W~b`, Pultteil `wechselquelle`): `AcSource` direkt zwischen den Feldern, potentialfrei
  mit 100 MOhm von `W~b` nach `0V`. Der Hauptschalter trennt wie beim Transformator die Primaerseite: ausgeschaltet
  ist die Quellenspannung null, die Wicklung (2 Ohm) bleibt liegen. Zwei Schalter in Reihe wie bei Batterie 2 haetten
  ausgeschaltet ueber die Isolationsleitwerte eine Scheinspannung von einigen Volt gezeigt.
- Funktionsgenerator (`P:G`, `P:G0`, Pultteil `generator`): `FunctionGenerator` mit fester Leitung von `G0` nach `0V`,
  unabhaengig vom Hauptschalter (Laborgeraet); Einstellungen im Generatorfenster, gespeichert im Pult des Aufbaus.
  Solange die App mit 1-ms-Schritten rechnet, sind Signale ueber etwa 50 Hz grob abgetastet; die feine Schrittweite
  fuer Ton und Oszilloskop folgt in Schritt 4.
- Leitungsfarbe nach Funktion (Konzept Volta 2, 9.3): `+B2` an `0V` ist Masse (schwarz), `-B2` dann negative
  Versorgung (blau); sonst `+B2` rot und `-B2` schwarz, an `V+` gehaengt (obere Batterie) rot.
- `paket.json "start_spalten"` (8, 10, 12) setzt die Startbreite der Grundplatte; ohne den Wert gelten die kleinen
  Bretter der ersten Stufen von Volta 1. Volta 2: 10, auch als Vorgabe der Einstellung, solange keine gespeichert ist.
- Aufgaben der Lehrkraft mit Aufbau tragen dessen Pultbelegung mit; im Pult nach Bedarf genuegt auch ein Pult ohne
  Bauteile.

- Volta 3 (07.11.2026, Konzept Volta 3, 12: Schritt 2): Edition `volta3` mit `"basis": "volta2"`, Abstammung
  volta1 -> volta2 -> volta3. Alles aus Volta 1 und 2 ist sofort frei (Bauteile, Pultteile mit `ab_stufe` 0,
  Blaetter des Formelrechners, Hilfe mit den Kapitelbildern ab Volta 2); Vorfuehrung vorerst mit den Aufbauten von
  Volta 2. Eigenes Startbild, Symbol (`icons/volta3/`), Android-Paket `com.dcsoftwareengineering.volta3`,
  Version 0.1.0. Abfragen der Oberflaeche lauten `band !== "volta1"` (ab Volta 2) und gelten damit auch fuer Volta 3.

## Folgen
- Volta 1 aendert sich nicht (kein Grundband, ganzer Katalog gehoert ihm).
- Experimente des Grundbandes bleiben ladbar (z. B. Vorfuehrung); eigene Vorfuehrungen fuer Volta 2 folgen mit den
  Inhalten (Umsetzungsplan Schritt 5).
- Ein Bauteil, das erst ab Volta 2 gilt, traegt `"ab_band": "volta2"` und erscheint in Volta 1 nicht.
