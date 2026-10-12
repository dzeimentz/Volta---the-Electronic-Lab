# Store-Vorbereitung Volta 1 (Google Play)

Stand: Oktober 2026. Alles hier ist vorbereitet; Signieren, Hochladen und Veröffentlichen erledigt der Inhaber selbst.

## Dateien

| Datei | Zweck |
|---|---|
| `listing-de.md`, `-en`, `-fr`, `-lb` | Name (≤ 30), Kurzbeschreibung (≤ 80), Beschreibung (≤ 4000) |
| `datenschutzerklaerung.html` | Datenschutzerklärung DE/EN zum Hosten |
| `play-console-angaben.md` | Antworten für »App-Inhalte« (Werbung, Zugriff, IARC, Zielgruppe, Datensicherheit) |
| `tools/store_assets.py` | erzeugt `Release/Store/`: `icon-512.png`, `feature-<sprache>.png` (1024 × 500), Screenshots 1920 × 1200 je Sprache |

## Checkliste

- [ ] Anschrift in `datenschutzerklaerung.html` eintragen (Markierung »todo«) und unter
      https://dc-software-engineering.com/volta/datenschutz hosten; URL in der Play Console angeben
- [ ] Markenrecherche »Volta« und »Tick« (DPMA/EUIPO, Klassen 9 und 41)
- [ ] FR- und LB-Eintrag von Muttersprachlern gegenlesen lassen
- [ ] Entwicklerkonto (Organisation, D-U-N-S bei Firmenkonto) und Identitätsprüfung
- [ ] Release-Build als **AAB**, signiert mit eigenem Upload-Schlüssel (Play App Signing) – Schritt des Inhabers
- [ ] versionCode/versionName erhöhen (aktuell 0.1.0), targetSdk 36
- [ ] Architekturen: derzeit nur arm64-v8a; armv7 ist geplant (ältere Schul-Tablets)
- [ ] Interner Test → geschlossener Test (bei neuen Privatkonten: 12 Tester über 14 Tage) → Produktion
- [ ] Hinweis: Luxemburgisch ist bei Google Play keine Eintragssprache; die LB-Texte dienen für die eigene
      Webseite bzw. als Ergänzung im deutschen/französischen Eintrag (»Auch auf Luxemburgisch«)
