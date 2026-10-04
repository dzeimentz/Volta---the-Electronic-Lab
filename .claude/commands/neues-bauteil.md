---
description: Fuegt ein neues Bauteilmodell in Kern, Katalog, Tests und Doku ein
argument-hint: <typ> <Kurzbeschreibung>
---

Fuege das Bauteil hinzu: $ARGUMENTS

1. Lies `core/CLAUDE.md` und `docs/simulation-core.md`.
2. Lege das Modell in `core/include/volta/core/components/` und `core/src/components/` an
   (`stamp`, `linearize` falls nichtlinear, `updateState`, `thermal`, `checkLimits`). Kennwerte mit Quelle kommentieren.
3. Schreibe Catch2-Tests in `core/tests/components/`: Arbeitspunkt gegen Handrechnung (≤ 2 %), Grenzwert,
   Zerstoerung, Verpolung, Kurzschluss, keine NaN.
4. Ergaenze `content/catalog/bauteile.json` (Anschluesse, Spannweiten, Parameter, `ab_band`, `ab_stufe`, `lexikon_key`)
   und den Lexikontext in `content/<band>/i18n/de.json`.
5. Ergaenze die Tabelle in `docs/simulation-core.md`.
6. `cmake --preset core-only && cmake --build --preset core-only && ctest --preset core-only` und
   `python3 tools/validate_content.py` ausfuehren.
7. Uebergib an den Agenten `simulations-pruefer`.
