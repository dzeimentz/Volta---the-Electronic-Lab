# ADR-0009: Inhaltspakete auf Android und iOS als Qt-Ressource

- Status: angenommen
- Datum: 2026-10-05

## Kontext
Die Inhaltspakete (`content/`, JSON) wurden bisher aus dem Quellbaum (`VOLTA_CONTENT_DIR`) oder aus einem Ordner
neben der ausfuehrbaren Datei (Desktop-Release) gelesen. Auf Android und iOS gibt es keinen solchen Ordner: Die App
ist ein Paket (APK/AAB bzw. App-Bundle). Moeglich sind Android-Assets (`assets:/`, nur Android), Dateien im
iOS-Bundle (nur iOS) oder eine Qt-Ressource (`:/`, alle Plattformen).

## Entscheidung
Die Inhalte werden auf Android und iOS als Qt-Ressource `:/content` in die App eingebettet (CMake-Option
`VOLTA_EMBED_CONTENT`, dort voreingestellt; Funktion `volta_embed_content` in `app/CMakeLists.txt`). Schemata und
`CLAUDE.md` bleiben draussen. `main.cpp` sucht in dieser Reihenfolge: `content/` neben der ausfuehrbaren Datei,
`:/content`, `content/` im Quellbaum. `ContentRepository` liest ueber `QDir`/`QFile` und funktioniert mit beiden.

## Folgen
- Ein Weg fuer beide Mobilplattformen, ohne plattformabhaengigen Code; der Desktop-Test
  `volta_embedded_content_tests` prueft die eingebettete Ressource (Dateiliste, Inhalt, Laden).
- Inhalte sind schreibgeschuetzt und mit der App versioniert; neue Experimente brauchen auf Mobil eine neue
  App-Version (Nachladen von Paketen waere Netzzugriff und ist ohnehin ausgeschlossen, ADR-0006).
- Groesse: Inhalte sind klein (ca. 100 KB). Tonaufnahmen (Phase 6) koennen die App vergroessern; dann
  `qt_add_big_resources` oder Android-Assets pruefen (Installationsgroesse < 80 MB, `docs/platforms.md`).
