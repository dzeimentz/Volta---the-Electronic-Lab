# ADR-0011: Inhaltspakete auf Mobilgeraeten als Qt-Ressource

- Status: angenommen
- Datum: 2026-10-05 (Entwurf im Cloud-Zweig als ADR-0009; lokal als ADR-0011 uebernommen am 2026-10-14, weil
  ADR-0009 hier die Tonausgabe beschreibt)

## Kontext
Die Inhaltspakete (`content/`, JSON) werden auf dem Desktop aus dem Quellbaum (`VOLTA_CONTENT_DIR`) oder aus einem
Ordner neben der ausfuehrbaren Datei (lokales Release) gelesen. Auf Android und iOS gibt es keinen solchen Ordner:
Die App ist ein Paket (APK/AAB bzw. App-Bundle). Moeglich sind Android-Assets (nur Android), Dateien im iOS-Bundle
(nur iOS) oder eine Qt-Ressource (`:/`, alle Plattformen).

## Entscheidung
Die Inhalte werden als Qt-Ressource `:/content` in die App eingebettet: CMake-Option `VOLTA_EMBED_CONTENT`
(auf Android voreingestellt, auf dem Desktop abschaltbar), `qt_add_resources` mit `content/catalog` und dem
Inhaltspaket des Bandes; Schemata und `CLAUDE.md` bleiben draussen. Ist eingebettet (`VOLTA_EMBEDDED_CONTENT`),
liest `main.cpp` aus `:/content`, sonst aus `content/` neben der ausfuehrbaren Datei bzw. aus dem Quellbaum.
`ContentRepository` liest ueber `QDir`/`QFile` und funktioniert mit beiden. Fuer iOS gilt dasselbe, sobald die
iPad-Ausgabe gebaut wird.

## Folgen
- Ein Weg fuer alle Mobilplattformen ohne plattformabhaengigen Code; die Android-Test-APK prueft das im Betrieb.
- Inhalte sind schreibgeschuetzt und mit der App versioniert; neue Experimente brauchen auf Mobil eine neue
  App-Version (Nachladen waere Netzzugriff und ist ausgeschlossen, ADR-0006).
- Groesse: Inhalte sind klein (ca. 100 KB). Tonaufnahmen (Phase 6) koennen die App vergroessern; dann
  `qt_add_big_resources` oder Android-Assets pruefen (Installationsgroesse < 80 MB, `docs/platforms.md`).
- Offen aus dem Cloud-Zweig: eigener Test der eingebetteten Ressource (`volta_embedded_content_tests`).
