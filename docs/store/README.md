# Store-Vorbereitung Volta 1 (Google Play)

Stand: Oktober 2026. Alles hier ist vorbereitet; Signieren, Hochladen und Veröffentlichen erledigt der Inhaber selbst.

## Dateien

| Datei | Zweck |
|---|---|
| `listing-de.md`, `-en`, `-fr`, `-lb` | Name (≤ 30), Kurzbeschreibung (≤ 80), Beschreibung (≤ 4000) |
| `datenschutzerklaerung.html` | Datenschutzerklärung DE/EN zum Hosten |
| `markenpruefung.md` | Vorrecherche »Volta« und »Tick« im Markenregister (TMview) |
| `play-console-angaben.md` | Antworten für »App-Inhalte« (Werbung, Zugriff, IARC, Zielgruppe, Datensicherheit) |
| `tools/store_assets.py` | erzeugt `Release/Store/`: `icon-512.png`, `feature-<sprache>.png` (1024 × 500), Screenshots 1920 × 1200 je Sprache |

## Checkliste

- [ ] Anschrift in `datenschutzerklaerung.html` eintragen (Markierung »todo«) und unter
      https://dc-software-engineering.com/volta/datenschutz hosten; URL in der Play Console angeben
- [ ] Markenlage »Volta«/»Tick« anwaltlich bewerten lassen – Vorrecherche in `markenpruefung.md`
- [ ] FR- und LB-Eintrag von Muttersprachlern gegenlesen lassen
- [ ] Entwicklerkonto (Organisation, D-U-N-S bei Firmenkonto) und Identitätsprüfung
- [x] Release-Build als **AAB** (arm64 + armv7): `python tools/build_android.py --aab` →
      `Release/android/Volta1-<version>-release-unsigned.aab`
- [ ] AAB mit eigenem Upload-Schlüssel signieren (Play App Signing) – Schritt des Inhabers
- [ ] Version: Testphase 0.9.x (versionCode 900 ff., aus CMakeLists.txt berechnet); vor dem Produktions-Release 1.0.0 (versionCode 10000), targetSdk 36
- [x] Architekturen arm64-v8a und armeabi-v7a (ältere Schul-Tablets); armv7-Test-APK mit `--armv7`
- [ ] Interner Test → geschlossener Test (bei neuen Privatkonten: 12 Tester über 14 Tage) → Produktion
- [ ] Hinweis: Luxemburgisch ist bei Google Play keine Eintragssprache; die LB-Texte dienen für die eigene
      Webseite bzw. als Ergänzung im deutschen/französischen Eintrag (»Auch auf Luxemburgisch«)
