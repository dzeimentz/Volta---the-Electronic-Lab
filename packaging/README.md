# Paketierung

| Plattform | Verzeichnis | Format | Hinweise |
|---|---|---|---|
| Android | `android/` | AAB + signierte APK | `AndroidManifest.xml` aus der Qt-Vorlage ableiten; **keine** `INTERNET`-Berechtigung, keine weiteren Berechtigungen; `app_restrictions.xml` fuer verwaltete Konfiguration (Schluessel in `docs/platforms.md`) |
| Windows | `windows/` | MSIX, MSI (WiX) | signiert; Registry-Schluessel `HKLM\Software\DC Software Engineering\Volta1` fuer Voreinstellungen |
| Linux | `linux/` | AppImage | `.desktop`-Datei und Symbol; optional Flatpak |
| iPadOS | `ios/` | App Store / Apple School Manager | Phase 5b; Qt-Lizenzweg vorher klaeren (ADR-0001) |

Signierschluessel liegen **nicht** im Repository (siehe `.gitignore`, `.claude/settings.json`).
Vor jeder Veroeffentlichung `/release-check` ausfuehren.
