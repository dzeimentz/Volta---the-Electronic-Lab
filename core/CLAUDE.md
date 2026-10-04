# core/ – Regeln fuer den Simulationskern

Lies vor Aenderungen `docs/simulation-core.md` und `docs/adr/0002-qt-freier-kern.md`.

## Harte Regeln
- Nur C++17-Standardbibliothek. Kein `#include <Q...>`, keine Abhaengigkeit zu `app/`.
- Keine Ausnahmen und keine Speicheranforderungen im Simulationsschritt (`Simulator::step`).
  Fehler als `Status`/`SimEvent`. Allokation nur bei Topologiewechsel.
- Niemals NaN/Inf nach aussen geben; vor Uebergabe an `Probe` pruefen und ggf. letzten gueltigen Zustand behalten.
- Einheiten im SI-System als `double` (V, A, Ω, F, H, s, K); Namen mit Einheit, wo mehrdeutig (`tempK`, `imaxA`).
- Jedes Bauteilmodell implementiert `ComponentModel` vollstaendig: `stamp`, `linearize` (falls nichtlinear),
  `updateState`, `thermal`, `checkLimits`.

## Neues Bauteilmodell
1. Header in `include/volta/core/components/`, Quelle in `src/components/`.
2. Kennwerte als Parameterstruktur mit Standardwerten; Herkunft der Werte im Kommentar (Datenblatt/Messung).
3. Tests in `tests/components/`: Arbeitspunkt, Grenzwerte, Zerstoerung, Robustheit (Verpolung, Kurzschluss).
4. Eintrag in `content/catalog/bauteile.json` (Feld `modell`) und Tabelle in `docs/simulation-core.md`.

## Tests
```bash
cmake --preset core-only && cmake --build --preset core-only && ctest --preset core-only
```
Toleranz gegen Referenz: 2 % relativ oder 1 mV / 1 µA absolut. Kirchhoff-Pruefung in jedem Referenztest.

## Stil
- Namensraum `volta::core`. Header mit `#pragma once`. Keine globalen Zustaende.
- Numerische Konstanten benannt (`kGmin = 1e-9`, `kWireOhm = 0.01`, `kMaxNewtonIter = 50`).
