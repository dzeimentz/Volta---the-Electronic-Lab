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
- Stand 10/2026: Qt 6.11.2, NDK r27c (27.2.12479018), Plattform android-36, Build-Tools 36.0.0, JDK 21
  (C:/Program Files/Android/openjdk/jdk-21.0.8). Bauen mit `python tools/build_android.py [--install]`: Preset
  android-arm64, androiddeployqt, Ausduennen, Gradle (Ausgabe in Datei, sonst haengt der Aufruf), Debug-APK nach
  Release/android/, Pruefung auf Berechtigungen, mit --install auf alle per adb angeschlossenen Geraete.
  `--armv7`: Test-APK fuer 32-Bit-Tablets (Qt android_armv7, Preset android-armv7). `--aab`: Paket fuer Google
  Play, Release-Bau fuer arm64-v8a und armeabi-v7a zusammen (build/android-release, QT_ANDROID_ABIS), Gradle
  bundleRelease, unsigniert nach Release/android/Volta1-<version>-release-unsigned.aab; signieren mit dem
  Upload-Schluessel ist Sache des Inhabers. Gradle packt jedes Mal frisch (inkrementell blieben Luecken in der Datei).
- Paketgroesse (Ziel < 80 MB): der Stil Basic ist fest (`QQuickStyle` in main.cpp); nicht ausgeliefert werden
  FFmpeg (Ton ueber das Android- bzw. Windows-Medien-Backend), die Controls-Stile Fusion/Imagine/Material/Universal/
  FluentWinUI3/Windows, NativeStyle, virtuelle Tastatur, TLS, Netzinformation, seltene Bildformate, Qt Sensors.
  Auf Android muessen entfernte Bibliotheken auch aus res/values/libs.xml (sonst Absturz beim Start).
  Stand 0.9.0: Test-APK arm64 57,5 MB, armv7 40,8 MB, AAB (beide ABIs) 44,4 MB; Windows-Ordner ca. 118 MB.
- Eigenes Manifest `packaging/android/AndroidManifest.xml`: Paket `com.dcsoftwareengineering.volta1`, Querformat,
  keine Cloud-Sicherung, keine Berechtigungen (Qt traegt ueber Multimedia/TextToSpeech sonst INTERNET, CAMERA,
  RECORD_AUDIO, BLUETOOTH u. a. ein; diese werden mit tools:node="remove" entfernt). Verbleibend nur die interne
  AndroidX-Signaturberechtigung DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION. Pruefung: `aapt dump badging`.
  Intent-Filter fuer `volta1://fortschritt` (Fortschrittscode aus der Kamera-App, `QDesktopServices::setUrlHandler`).
- Entwicklertablet: Meswoa MES-B3, 15,6", 1920 x 1080, 240 dpi (logisch 1280 x 720), arm64.

## Desktop Windows und Linux x86-64 (Phase 5a, Entwicklungsziel ab Phase 0)
- Windows 10/11 x86-64: MSIX und MSI (WiX), signiert; optional Microsoft Store.
- Testversion (10/2026): NSIS-Installer `packaging/windows/volta1.nsi` ueber `tools/build_testpaket.py`; Installation je
  Benutzer ohne Administratorrechte nach %LOCALAPPDATA%\Programs\Volta 1, Startmenue/Desktop, Deinstallation ueber
  Apps; nicht signiert (SmartScreen). Geprueft: stille Installation, Start ohne Qt im Suchpfad, Deinstallation.
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

Umgesetzt (`services/managed_config`, Stand 10/2026): Android ueber `packaging/android/res/xml/app_restrictions.xml`
(Beschriftungen fuer die Verwaltungskonsole in `res/values/strings.xml`) und RestrictionsManager per JNI; Windows
Registry, sonst `volta.ini` neben der EXE (Abschnitt `[General]`); Linux `/etc/xdg/...`; zum Testen
`VOLTA_MANAGED_INI=<Datei>`. Wirkung: `sprache` setzt die Anfangssprache (gesperrt: immer, Flaggensymbol weg),
`stufen_frei` oeffnet Stufen bis zur Nummer, `elternbereich=false` blendet Menue- und Profil-Eintrag aus,
`klassenmodus_ton` legt den Ton fest (im Elternbereich dann nicht aenderbar), `profile` schaltet Profile ein (Zahl =
Hoechstzahl) oder aus. `fotoimport=false` blendet »Foto hinzufuegen« im Forscherheft aus; `praesentation`: auf
verwalteten Geraeten ist der Praesentationsmodus nur mit `praesentation=true` im Elternbereich zu sehen (ohne
Verwaltung immer). Im Elternbereich
weist ein Hinweis auf verwaltete Einstellungen hin.

## Groessen- und Leistungsziele
- Installationsgroesse < 80 MB je Plattform (nur benoetigte Qt-Module).
- 60 fps bei 40 Bauteilen inkl. Modulinhalten auf dem Referenztablet.
- Kaltstart < 3 s auf dem Referenztablet.
