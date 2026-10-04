---
description: Baut und testet Kern, App und Inhalte und fasst das Ergebnis zusammen
---

1. `cmake --preset core-only && cmake --build --preset core-only && ctest --preset core-only --output-on-failure`
2. Falls Qt verfuegbar: `cmake --preset desktop-debug && cmake --build --preset desktop-debug && ctest --preset desktop-debug --output-on-failure`
3. `python3 tools/validate_content.py`

Fasse zusammen: Anzahl Tests, Fehlschlaege mit Ursache, Inhaltsfehler und -warnungen. Behebe nichts ungefragt,
sondern schlage die Korrektur vor.
