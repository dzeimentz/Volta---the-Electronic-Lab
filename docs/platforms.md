# Plattformen, Paketierung, Konfiguration

## Android (Phase 1)
- Qt for Android, ABIs `arm64-v8a` und `armeabi-v7a`, Auslieferung als App Bundle (AAB), zusaetzlich signierte APK.
- minSdk 26 (Android 8.0); targetSdk nach aktuellen Google-Play-Vorgaben.
- Keine Berechtigungen; kein `INTERNET` im Manifest.
- Familienrichtlinien von Google Play; Managed Google Play fuer Schulen.
- Referenztablet: Einstiegsgeraet mit 3 GB RAM (Festlegung offen, siehe Roadmap).
- Paketierung in `packaging/android/` (wird ueber `QT_ANDROID_PACKAGE_SOURCE_DIR` mit der Qt-Vorlage gemischt):
  `AndroidManifest.xml` (aus der Qt-6-Vorlage abgeleitet: keine Berechtigungen, Platzhalter fuer
  Modulberechtigungen entfernt und `INTERNET` u. a. per `tools:node="remove"` ausgeschlossen, `allowBackup=false`,
  Hoch- und Querformat), `res/mipmap-*` (App-Symbol, adaptives Symbol), `res/values*/styles.xml` (Startthema).
  Beim Wechsel der Qt-Version das Manifest mit `$QT_ROOT_ANDROID/src/android/templates/AndroidManifest.xml`
  vergleichen.
- App-ID `com.dcsoftwareengineering.volta1` (CMake-Cache `VOLTA_APP_ID`, Android-Paketname und iOS-Bundle-ID) –
  **vor der ersten Veroeffentlichung bestaetigen**, danach unveraenderlich. Versionscode aus der Projektversion
  (`0.1.0` -> 100).
- Inhaltspakete als Qt-Ressource `:/content` eingebettet (`VOLTA_EMBED_CONTENT`, auf Android/iOS voreingestellt;
  Test `volta_embedded_content_tests` vergleicht die Ressource mit `content/`). Ein Ordner `content/` neben der
  ausfuehrbaren Datei (Desktop-Release) hat Vorrang.
- Startbildschirm ueber die Systemmechanik: Startthema `VoltaLaunchTheme` (Fensterhintergrund Navy mit Motiv;
  ab Android 12 SplashScreen des Systems mit `windowSplashScreenAnimatedIcon`). `Splash.qml` nur auf dem Desktop.
- Bedienung: siehe `docs/ui-spec.md` (Touch, Zoom mit zwei Fingern, Hochformat).

### Werkzeuge fuer den Android-Build (Windows-Entwicklungsrechner)
Benoetigt werden (Stand Qt 6.11; genaue Versionen in der Qt-Dokumentation »Getting Started with Qt for Android«,
Abschnitt Requirements, nachsehen – sie aendern sich mit jeder Qt-Version):

| Werkzeug | Bezug | Hinweis |
|---|---|---|
| Qt 6.11 fuer Android `arm64-v8a` und `armeabi-v7a` | Qt Maintenance Tool -> Qt 6.11.0 -> Android | dazu die Desktop-Qt als Host (`mingw_64`, vorhanden) |
| JDK 17 (LTS) | z. B. Eclipse Temurin oder das JDK von Android Studio | `JAVA_HOME` setzen |
| Android SDK (Command-line Tools, Platform-Tools, Build-Tools, Plattform) | Qt Creator: Einstellungen -> Geraete -> Android -> »SDK-Einstellungen einrichten« laedt die zu Qt passenden Pakete | Plattform = targetSdk; die Mindestvorgabe von Google Play (2025: API 35, steigt jaehrlich) in der Play Console pruefen |
| Android NDK in der von Qt 6.11 verlangten Version | ebenfalls ueber Qt Creator bzw. `sdkmanager "ndk;<version>"` | nur die von Qt genannte NDK-Version verwenden |
| Ninja, CMake >= 3.24 | Qt Maintenance Tool (Developer and Designer Tools) | vorhanden |

Empfohlener Weg: Qt 6.11 Android ueber das Maintenance Tool installieren, dann Qt Creator die SDK-/NDK-Pakete
laden lassen (prueft die Versionen selbst). Umgebungsvariablen (Git Bash, Pfade anpassen):

```bash
export QT_ROOT_ANDROID="C:/Qt/6.11.0/android_arm64_v8a"   # fuer android-armv7: C:/Qt/6.11.0/android_armv7
export QT_HOST_PATH="C:/Qt/6.11.0/mingw_64"
export ANDROID_SDK_ROOT="$LOCALAPPDATA/Android/Sdk"
export ANDROID_NDK_ROOT="$ANDROID_SDK_ROOT/ndk/<ndk-version>"
export JAVA_HOME="C:/Program Files/Eclipse Adoptium/jdk-17..."
cmake --preset android-arm64 && cmake --build --preset android-arm64   # APK zum Testen (unsigniert/Debug-signiert)
cmake --preset android-aab && cmake --build --preset android-aab       # App Bundle mit beiden ABIs (Google Play)
```

Signieren (Release-Schluessel, `QT_ANDROID_SIGN_AAB` bzw. `androiddeployqt --sign`) nur auf ausdrueckliche
Anweisung; Schluessel liegen nie im Repository. Pruefpunkte beim ersten Build: Manifest im Build
(`build/android-arm64/app/android-build/AndroidManifest.xml`) enthaelt keine `uses-permission`; Symbol und
Startbildschirm; Hoch-/Querformat; Randlos-Darstellung ab Android 15 (Statusleiste darf die Werkzeugleiste nicht
verdecken, sonst `SafeArea` aus Qt Quick nutzen).

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
- Vorbereitet (ungetestet, braucht macOS): Preset `ios` (Xcode-Generator, Toolchain aus `QT_ROOT_IOS`, nur auf
  macOS sichtbar), `packaging/ios/`: `Info.plist.in` (alle Ausrichtungen, Split View, keine
  Berechtigungsbeschreibungen, `ITSAppUsesNonExemptEncryption=false`), `Assets.xcassets` (AppIcon 1024 px ohne
  Transparenz, LaunchLogo), `LaunchScreen.storyboard` (Startbildschirm des Systems, `QT_IOS_LAUNCH_SCREEN`),
  `PrivacyInfo.xcprivacy` (Entwurf: kein Tracking, keine erhobenen Daten; Gruende fuer API-Zugriffe von Qt pruefen).
  CMake: nur iPad (`TARGETED_DEVICE_FAMILY=2`), App-ID aus `VOLTA_APP_ID`, Inhalte eingebettet.
- Schritte auf dem Mac:
  1. Qt-Lizenzweg entscheiden (offene Entscheidung, `docs/roadmap.md`): Qt for iOS linkt statisch; LGPLv3 verlangt
     dann die Neulink-Moeglichkeit (Objektdateien bereitstellen) oder eine kommerzielle Qt-Lizenz.
  2. Xcode (aktuelle Version aus dem App Store) und Kommandozeilenwerkzeuge installieren; Apple-Developer-Konto
     (Team-ID) fuer Signierung und Geraetetest.
  3. Qt 6.11 fuer macOS (Host) und iOS ueber das Maintenance Tool installieren.
  4. `export QT_ROOT_IOS=~/Qt/6.11.0/ios QT_HOST_PATH=~/Qt/6.11.0/macos`, dann `cmake --preset ios` und das erzeugte
     Xcode-Projekt (`build/ios/Volta.xcodeproj`) oeffnen; Team unter »Signing & Capabilities« waehlen.
  5. Pruefen: Symbol, Startbildschirm, Hoch-/Querformat, Split View, Apple Pencil (als Finger/Stift ueber
     Pointer Handlers), Xcode »Privacy Report«.

## macOS (optional)
- Gleicher Code, Apple Silicon (und Intel), Presets `desktop-debug` / `desktop-release` wie unter Windows/Linux.
- Vorbereitet (ungetestet): App-Bundle mit `packaging/macos/Info.plist.in` und Symbol `packaging/macos/volta.icns`
  (aus `tools/make_icons.py`), Inhalte eingebettet (`VOLTA_EMBED_CONTENT` auf Apple voreingestellt), Startbildschirm
  `Splash.qml` wie auf dem Desktop. `tools/deploy_release.py` legt `Release/volta1.app` mit `macdeployqt` an.
- Weitergabe ausserhalb des eigenen Macs nur signiert (Developer ID, Apple Developer Program) und notarisiert;
  Mac App Store zusaetzlich mit App Sandbox. Beides nur auf ausdrueckliche Anweisung.

## Entwicklung auf dem Mac (iPadOS und macOS)
Gilt auch fuer einen geliehenen Mac (z. B. MacBook Air M1). Reihenfolge:

1. **Eigenes Benutzerkonto** auf dem Mac anlegen und darin arbeiten; eigene Apple-ID verwenden. Freier Speicher
   mindestens ca. 60 GB (Xcode mit iOS-Simulator ca. 30-40 GB, Qt ca. 5-10 GB, Builds).
2. **Xcode** aus dem App Store (Mindestversion von macOS dort beachten), einmal starten, iOS-Plattform mit Simulator
   laden; `xcode-select --install`.
3. **Homebrew**, dann `brew install cmake ninja git python gh`; `pip3 install pillow` (Symbole).
4. **Qt 6.11** ueber den Qt Online Installer: macOS und iOS, Module wie unter Windows (Multimedia, TextToSpeech,
   Sensors, Svg, Qt Quick).
5. **Claude Code** installieren und im Repository starten; `gh auth login`, Repository klonen.
6. Umgebung (`~/.zshrc`):
   ```bash
   export QT_ROOT_DESKTOP="$HOME/Qt/6.11.0/macos"
   export QT_ROOT_IOS="$HOME/Qt/6.11.0/ios"
   export QT_HOST_PATH="$HOME/Qt/6.11.0/macos"
   ```
7. **Pruefen und bauen:**
   ```bash
   python3 tools/check_all.py                       # Inhalte, Kern, App-Tests (macOS)
   VOLTA_SCREENSHOT=bild.png build/desktop-debug/app/volta1.app/Contents/MacOS/volta1
   python3 tools/make_icons.py                      # Symbole mit Qt neu rendern (optional)
   cmake --preset ios && open build/ios/*.xcodeproj  # iPad: Team waehlen, Simulator oder iPad per USB
   python3 tools/deploy_release.py                  # Release/volta1.app (nicht signiert)
   ```
   Simulator ohne Xcode-Oberflaeche: `xcrun simctl list devices`, `xcrun simctl boot <id>`,
   `xcrun simctl install booted <App>`, `xcrun simctl io booted screenshot bild.png`.
8. **Signierung:** Zum Testen reicht das kostenlose »Personal Team« der eigenen Apple-ID (App laeuft 7 Tage auf dem
   eigenen iPad). TestFlight, App Store und Developer-ID brauchen das Apple Developer Program (Firmenkonto mit
   D-U-N-S-Nummer). Vor einer Veroeffentlichung den Qt-Lizenzweg fuer iOS entscheiden (Roadmap).
9. **Beim Zurueckgeben:** alles committen und pushen, `gh auth logout`, Apple-ID in Xcode abmelden, Zertifikate
   nicht auf dem Mac lassen (ggf. vorher geschuetzt exportieren), Benutzerkonto loeschen.

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
