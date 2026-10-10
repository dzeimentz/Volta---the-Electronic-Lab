# Plattformen, Paketierung, Konfiguration

## Android (Phase 1)
- Qt for Android, ABIs `arm64-v8a` und `armeabi-v7a`, Auslieferung als App Bundle (AAB), zusaetzlich signierte APK.
- minSdk 26 (Android 8.0); targetSdk nach aktuellen Google-Play-Vorgaben.
- Keine Berechtigungen; kein `INTERNET` im Manifest.
- Familienrichtlinien von Google Play; Managed Google Play fuer Schulen.
- Referenztablet: Einstiegsgeraet mit 3 GB RAM (Festlegung offen, siehe Roadmap).
- Inhaltspakete sind eingebettet (`:/content`, CMake-Option `VOLTA_EMBED_CONTENT`, auf Android immer an); die App
  braucht kein Dateisystem neben sich. Fortschritt und Forscherheft im privaten App-Speicher (`AppDataLocation`).
- Entwicklungsrechner: Qt 6.11 for Android (arm64-v8a, armeabi-v7a; fuer den Emulator x86_64) ueber das Qt
  Maintenance Tool, Android SDK (Plattform 35, Build-Tools 35), das von Qt 6.11 verlangte NDK, JDK 17.
  Testgeraete: Tablet mit USB-Debugging oder Emulator (AVD). Erste Test-APKs nur mit Debug-Signatur.

## Desktop Windows und Linux x86-64 (Phase 5a, Entwicklungsziel ab Phase 0)
- Windows 10/11 x86-64: MSIX und MSI (WiX), signiert; optional Microsoft Store.
- Linux x86-64: AppImage, optional Flatpak.
- Fenster frei skalierbar, Mindestgroesse 1280 × 720, hohe DPI, Vollbild fuer Praesentationsmodus.
- Vollstaendige Maus- und Tastaturbedienung (siehe `docs/ui-spec.md`).
- Daten im Benutzerverzeichnis (`QStandardPaths::AppDataLocation`).

## iPadOS (Phase 5b)
- Qt for iOS, arm64; Apple Pencil; Kinderkategorie des App Store; Apple School Manager.
- Lizenz: statisches Linken -> LGPL-Neulink-Moeglichkeit oder kommerzielle Qt-Lizenz (ADR-0001, Risiko R17).
- Benoetigt aktuellen Mac (gemeinsam mit VTS-Portierung).

## macOS (optional)
- Gleicher Code, Apple Silicon, sobald der Mac vorhanden ist.

## Verwaltete Konfiguration
| Schluessel | Typ | Standard | Wirkung |
|---|---|---|---|
| `sprache` | String (`de`,`fr`,`en`,`lb`) | Systemsprache | Anfangssprache |
| `sprache_gesperrt` | Bool | false | Flaggensymbol ausblenden |
| `stufen_frei` | Int | 1 | hoechste freigegebene Stufe |
| `elternbereich` | Bool | true | Elternbereich anzeigen |
| `fotoimport` | Bool | true | Fotos ins Forscherheft |
| `klassenmodus_ton` | String (`aus`,`kopfhoerer`,`normal`) | normal | Tonausgabe |
| `profile` | Int (0 = aus) | 0 | Anzahl Forscherprofile |
| `praesentation` | Bool | false | Praesentationsmodus erlauben |

Quellen: Android `RestrictionsManager` (app_restrictions.xml), iOS `com.apple.configuration.managed`,
Windows Registry `HKLM\Software\DC Software Engineering\Volta1` oder `volta.ini` neben der EXE,
Linux `/etc/xdg/dc-software-engineering/volta1.ini`.

## Groessen- und Leistungsziele
- Installationsgroesse < 80 MB je Plattform (nur benoetigte Qt-Module).
- 60 fps bei 40 Bauteilen inkl. Modulinhalten auf dem Referenztablet.
- Kaltstart < 3 s auf dem Referenztablet.
