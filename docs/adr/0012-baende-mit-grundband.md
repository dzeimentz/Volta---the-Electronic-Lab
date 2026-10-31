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
  Geraetefachs mit `ab_stufe`. Ist die Liste da, traegt der Aufbau seine Pultbelegung (`Board::deskParts`, JSON
  `"pultteile"`, Befehl mit Rueckgaengig); sonst gilt das feste Pult von Volta 1. Batterie 2 (`P:+B2`, `P:-B2`) ist
  potentialfrei, haengt zweipolig am Hauptschalter und hat 10 MOhm von `P:-B2` nach `0V` (rechenbar, ausgeschaltet
  unter 0,05 V, als Last 0,45 uA).

## Folgen
- Volta 1 aendert sich nicht (kein Grundband, ganzer Katalog gehoert ihm).
- Experimente des Grundbandes bleiben ladbar (z. B. Vorfuehrung); eigene Vorfuehrungen fuer Volta 2 folgen mit den
  Inhalten (Umsetzungsplan Schritt 5).
- Ein Bauteil, das erst ab Volta 2 gilt, traegt `"ab_band": "volta2"` und erscheint in Volta 1 nicht.
