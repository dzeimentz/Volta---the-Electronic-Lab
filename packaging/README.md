# Paketierung

| Plattform | Verzeichnis | Format | Hinweise |
|---|---|---|---|
| Android | `android/` | AAB + signierte APK | `AndroidManifest.xml` (aus der Qt-Vorlage abgeleitet, **keine** Berechtigungen, kein `INTERNET`), `res/` mit Symbolen (`tools/make_icons.py`) und Startthema; offen: `app_restrictions.xml` fuer verwaltete Konfiguration (mit `ManagedConfig`, Schluessel in `docs/platforms.md`) |
| Windows | `windows/` | MSIX, MSI (WiX) | signiert; Registry-Schluessel `HKLM\Software\DC Software Engineering\Volta1` fuer Voreinstellungen |
| Linux | `linux/` | AppImage | `.desktop`-Datei und Symbol; optional Flatpak |
| macOS | `macos/` | App-Bundle (optional) | `Info.plist.in`, `volta.icns` (`tools/make_icons.py`); `tools/deploy_release.py` mit `macdeployqt`; Weitergabe nur mit Developer ID und Notarisierung |
| iPadOS | `ios/` | App Store / Apple School Manager | Phase 5b; Qt-Lizenzweg vorher klaeren (ADR-0001). Vorbereitet: `Info.plist.in`, `Assets.xcassets` (AppIcon, LaunchLogo), `LaunchScreen.storyboard`, `PrivacyInfo.xcprivacy` (Entwurf) |

Signierschluessel liegen **nicht** im Repository (siehe `.gitignore`, `.claude/settings.json`).
Vor jeder Veroeffentlichung `/release-check` ausfuehren.
