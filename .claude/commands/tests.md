---
description: Baut und testet Kern, App und Inhalte und fasst das Ergebnis zusammen
---

1. `python3 tools/check_all.py` (Inhalte, Kern, App inkl. ContentSmokeTest; findet Qt und MinGW selbst)
2. Bei Aenderungen am Kern zusaetzlich `python3 tools/check_all.py --bench` (Leistungsvorgabe 1000 Schritte < 50 ms)

Fasse zusammen: Anzahl Tests, Fehlschlaege mit Ursache, Inhaltsfehler und -warnungen. Behebe nichts ungefragt,
sondern schlage die Korrektur vor.
