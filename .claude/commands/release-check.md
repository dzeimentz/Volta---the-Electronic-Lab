---
description: Prueft vor einer Veroeffentlichung alle Freigabekriterien
argument-hint: <plattform> <version>
---

Freigabepruefung fuer $ARGUMENTS:

1. Alle Tests gruen (`/tests`).
2. Kein Netzwerkcode: suche nach `QNetwork`, `QWebSocket`, `QTcp`, `QUdp`, `http://`, `https://` ausserhalb von Info/Readme-Texten.
3. Android-Manifest ohne `INTERNET` und ohne gefaehrliche Berechtigungen.
4. Externe Links nur hinter `ParentGate` (`Qt.openUrlExternally`-Aufrufe pruefen).
5. Info-Seite: Version, Build, Hersteller, Anschrift, Website, Support-E-Mail, Lizenzliste vollstaendig und aktuell.
6. Datenschutzerklaerung offline enthalten; Store-Datensicherheitsangaben »keine Daten erhoben«.
7. Inhalte: `python3 tools/validate_content.py` ohne Fehler; alle `zerstoerend`-Experimente mit Hinweis.
8. Der Begriff »Kosmos« kommt in `app/`, `content/` und `packaging/` nicht vor.
9. Installationsgroesse < 80 MB.
10. Versionsnummer in `CMakeLists.txt`, Paket-Metadaten und »Was ist neu« konsistent.

Gib eine Liste mit erfuellt / nicht erfuellt und konkreten Fundstellen aus. Nichts signieren oder hochladen.
