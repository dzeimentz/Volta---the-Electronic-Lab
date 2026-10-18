# Volta 1 auf dem MacBook bauen – iPad zuerst

_Schritt-für-Schritt-Anleitung für die iPad- und die Mac-Version_

Markdown-Quelle der Anleitung; PDF mit `python tools/konzept_pdf.py docs/anleitung-mac-ipad.md`.

| | |
|---|---|
| Dokument | Anleitung |
| Version | 1.0 |
| Status | ZUM AUSPROBIEREN |
| Autor | Daniel Zeimentz · DC Software Engineering |
| Datum | 18.10.2026 |
| Gilt für | Volta 1 Version 0.9.0, Qt 6.11, aktuelles macOS und Xcode |
| Priorität | 1. iPad-Version (Simulator, dann echtes iPad) · 2. Mac-Version |
| Zeitbedarf | Einrichtung ca. 2–3 Stunden (große Downloads), danach je Build wenige Minuten |

## 1. Überblick

Die iPad- und Mac-Version entstehen aus demselben Quellcode wie Windows und Android. Was Apple-spezifisch ist, liegt bereits im Projekt: Info.plist (alle Ausrichtungen, Split View, keine Berechtigungen), Datenschutz-Manifest, Startbildschirm, App-Symbole und die CMake-Einstellungen. Auf dem MacBook wird nur noch eingerichtet, gebaut, signiert und getestet. Apple erlaubt das Bauen für iPad und Mac ausschließlich auf einem Mac mit Xcode.

| Teil | Inhalt | Ergebnis |
|---|---|---|
| A | Mac einrichten: Xcode, Werkzeuge, Qt | Bauumgebung steht |
| B | Projekt auf den Mac bringen | Quellcode im Ordner `~/volta` |
| C | iPad-Version bauen: Simulator, dann echtes iPad | Volta läuft auf dem iPad |
| D | iPad-Version prüfen | Prüfliste abgehakt |
| E | Mac-Version bauen | `Volta 1.app` |
| F | Änderungen zurück nach Windows | gemeinsamer Stand |
| G | Später: Veröffentlichung | Entscheidungen vor dem App Store |

Befehle stehen in grauen Kästen und werden im Programm **Terminal** eingegeben (Programme › Dienstprogramme › Terminal). Zeilen, die mit `#` beginnen, sind Erklärungen und werden nicht eingegeben.

## 2. Was du brauchst

- MacBook mit Apple-Chip (M1 oder neuer) und aktuellem macOS; mindestens **60 GB freier Speicher**.
- Eine **Apple-ID** (deine eigene). Für Tests auf dem eigenen iPad reicht sie kostenlos; für TestFlight und App Store braucht es später das kostenpflichtige Apple Developer Program (Teil G).
- Ein **Qt-Konto** (kostenlos) für den Qt-Installer.
- Das **iPad** mit passendem USB-C-Kabel (bzw. Lightning bei älteren Geräten).
- Einen **USB-Stick** mit den Dateien aus dem Ordner `J:\VOLTA Elektronik-Labor\Mac-Paket\`: die Bündeldatei `volta.bundle` (der komplette Quellcode) und diese Anleitung.
- Wenn das MacBook geliehen ist: ein **eigenes Benutzerkonto** darauf anlegen und nur darin arbeiten (Teil F, Aufräumen).

## 3. Teil A – Mac einrichten

### A1 Xcode installieren

1. **App Store** öffnen, nach »Xcode« suchen, installieren (ca. 15 GB, dauert).
2. Xcode einmal starten, Lizenz bestätigen. Wenn Xcode nach Plattformen fragt, **iOS** auswählen und laden (enthält den iPad-Simulator).
3. Im Terminal die Kommandozeilenwerkzeuge bestätigen:

```bash
xcode-select --install
sudo xcodebuild -license accept
```

4. Xcode › Einstellungen (Settings) › **Accounts** › »+« › Apple ID: mit deiner Apple-ID anmelden. Danach erscheint dort ein »Personal Team«.

### A2 Homebrew und Werkzeuge

Homebrew ist der Paketmanager für Entwicklerwerkzeuge auf dem Mac. Installation nach der Anleitung auf brew.sh (ein Befehl im Terminal), danach:

```bash
brew install cmake ninja git python
pip3 install --user pillow reportlab pdfplumber
```

### A3 Qt 6.11 installieren

1. Den **Qt Online Installer** für macOS von qt.io laden und starten, mit dem Qt-Konto anmelden.
2. Installationsordner: `~/Qt` (Vorgabe).
3. Unter **Qt 6.11.x** (dieselbe Version wie unter Windows) auswählen:
   - **macOS** (für den Mac und als Werkzeug für den iPad-Bau),
   - **iOS**,
   - unter »Additional Libraries«: **Qt Multimedia** und **Qt Speech** (TextToSpeech).
4. Installieren (ca. 5–10 GB).

### A4 Umgebung festlegen

Die Version im Pfad an die installierte Qt-Version anpassen (Ordnernamen unter `~/Qt` ansehen), dann:

```bash
cat >> ~/.zshrc <<'EOF'
export QT_ROOT_DESKTOP="$HOME/Qt/6.11.0/macos"
export QT_ROOT_IOS="$HOME/Qt/6.11.0/ios"
export QT_HOST_PATH="$HOME/Qt/6.11.0/macos"
EOF
source ~/.zshrc
```

### A5 Claude (empfohlen)

Die Claude-Desktop-App für macOS installieren und im Code-Bereich den Ordner `~/volta` öffnen. Wenn beim Bauen Fehler auftreten, kann Claude sie direkt auf dem Mac beheben – die Projektregeln (CLAUDE.md) liegen im Repository.

## 4. Teil B – Projekt auf den Mac bringen

1. USB-Stick anstecken; im Finder erscheint er unter »Orte«.
2. Im Terminal (Namen des Sticks anpassen, hier `STICK`):

```bash
cd ~
git clone /Volumes/STICK/volta.bundle volta
cd volta
git log --oneline -3
```

Die letzte Zeile zeigt die neuesten Änderungen – sie sollten dieselben sein wie unter Windows.

## 5. Teil C – iPad-Version bauen (Priorität)

### C1 Xcode-Projekt erzeugen

```bash
cd ~/volta
cmake --preset ios
open build/ios/Volta.xcodeproj
```

Das Erzeugen dauert beim ersten Mal einige Minuten. Falls `cmake --preset ios` meldet, dass das Preset unbekannt ist oder Qt nicht gefunden wird, die Umgebung aus A4 prüfen (`echo $QT_ROOT_IOS`). Ersatzweise direkt mit dem Qt-Werkzeug:

```bash
~/Qt/6.11.0/ios/bin/qt-cmake -S . -B build/ios -G Xcode -DVOLTA_BUILD_TESTS=OFF
```

### C2 Im iPad-Simulator starten (ohne Signierung)

1. In Xcode oben links das Schema **volta_app** wählen.
2. Daneben als Ziel einen **iPad-Simulator** wählen (z. B. »iPad Air« oder »iPad Pro 13-inch«).
3. **Product › Run** (Cmd+R). Der erste Bau dauert 5–15 Minuten.
4. Der Simulator startet Volta. Erste Prüfung: Startbildschirm, Start-Seite, ein Experiment öffnen, Bauteil setzen, Hauptschalter einschalten.

### C3 Signierung einrichten

1. In Xcode links das Projekt anklicken, unter TARGETS **volta_app** wählen, Reiter **Signing & Capabilities**.
2. **Automatically manage signing** einschalten, unter **Team** dein »Personal Team« wählen.
3. Erscheint »Failed to register bundle identifier«, ist die App-ID `com.dcsoftwareengineering.volta1` beim kostenlosen Team nicht verfügbar. Dann für den Test eine eigene Kennung eintragen, z. B. `com.dcsoftwareengineering.volta1.test`. Für die spätere Veröffentlichung wird die richtige ID im bezahlten Developer-Konto reserviert (Teil G).

### C4 Auf dem echten iPad starten

1. iPad per Kabel anschließen und entsperren; auf dem iPad **»Diesem Computer vertrauen«** bestätigen.
2. Auf dem iPad den **Entwicklermodus** einschalten: Einstellungen › Datenschutz & Sicherheit › Entwicklermodus › ein; das iPad startet neu, danach »Einschalten« bestätigen. (Der Menüpunkt erscheint erst, nachdem das iPad einmal mit Xcode verbunden war.)
3. In Xcode oben als Ziel **dein iPad** wählen. Beim ersten Mal lädt Xcode Gerätedaten (»Preparing device«) – abwarten.
4. **Product › Run** (Cmd+R).
5. Beim ersten Start meldet das iPad »Nicht vertrauenswürdiger Entwickler«: Einstellungen › Allgemein › VPN & Geräteverwaltung › deine Apple-ID › **Vertrauen**. Dann Volta erneut starten.

Mit dem kostenlosen Personal Team läuft die App **7 Tage**; danach in Xcode einfach erneut »Run« ausführen. Bis zu drei eigene Apps gleichzeitig sind möglich.

## 6. Teil D – iPad-Version prüfen

| Prüfpunkt | Erwartung |
|---|---|
| Symbol und Name | Volta-Symbol, Name »Volta 1« |
| Startbildschirm | dunkelblau mit Volta-Logo, dann die Startseite |
| Ausrichtung | Quer- und Hochformat, Drehen während der Benutzung |
| Split View / Slide Over | Volta neben einer zweiten App, Breite ändern |
| Bedienung | Bauteile ziehen mit Finger, Zoom mit zwei Fingern, Apple Pencil, Trackpad/Maus |
| Simulation | LED leuchtet, Messgerät zeigt Werte, Zerstörung bei falscher Polung |
| Ton | Summer und Lautsprecher hörbar, Vorlesen (Sprachausgabe) funktioniert |
| Sprachen | Umschalten DE/FR/EN/LU über die Flagge |
| Speichern | Fortschritt bleibt nach Beenden und Neustart erhalten |
| Elternbereich | PIN-Abfrage, externe Links nur dahinter |
| Sonderzeichen | keine leeren Kästchen statt Symbolen |
| Netz | Flugmodus an: alles funktioniert weiter |
| Datenschutz | Xcode › Product › Archive › »Generate Privacy Report« ohne Auffälligkeiten |

Bildschirmfotos vom iPad: Seitentaste und Lauter-Taste gleichzeitig drücken. Auffälligkeiten mit Foto notieren und an Claude geben (Teil A5) oder mit nach Windows bringen.

## 7. Teil E – Mac-Version bauen

### E1 Prüfen und bauen

```bash
cd ~/volta
python3 tools/check_all.py
```

Am Ende muss »alles bestanden« stehen (Inhalte, Kern, App-Tests).

### E2 Release-Programm erzeugen

```bash
python3 tools/deploy_release.py
open Release/volta1.app
```

Das Skript baut die Release-Fassung und legt die Qt-Laufzeit mit `macdeployqt` in das Programmpaket `Release/volta1.app`. Die Inhalte sind eingebettet.

### E3 Erster Start

Das Programm ist nicht signiert. Beim ersten Öffnen: im Finder Rechtsklick auf `volta1.app` › **Öffnen** › »Öffnen« bestätigen. Auf anderen Macs startet es erst nach Signierung und Notarisierung (Teil G).

Prüfen wie in Teil D, zusätzlich: Fenstergröße ändern, Vollbild, Trackpad-Zoom, Tastenkürzel (Cmd+Z / Cmd+Umschalt+Z für Rückgängig/Wiederholen).

## 8. Teil F – Änderungen zurück nach Windows

Alle Änderungen auf dem Mac (z. B. Korrekturen mit Claude) werden wie gewohnt committet. Zurück nach Windows ohne Internet:

```bash
cd ~/volta
git bundle create /Volumes/STICK/volta-mac.bundle main
```

Unter Windows im Repository:

```bash
git pull J:/Pfad/zum/Stick/volta-mac.bundle main
```

**Geliehenes MacBook zurückgeben:** alles committen und als Bündel sichern, in Xcode › Accounts die Apple-ID abmelden, im Qt-Installer abmelden, dann das eigene Benutzerkonto löschen.

## 9. Teil G – Später: Veröffentlichung (nicht jetzt)

Diese Punkte sind Entscheidungen des Inhabers und werden erst nach der Rückmeldung der Verlage angegangen:

| Thema | Was zu tun ist |
|---|---|
| Apple Developer Program | Mitgliedschaft (ca. 99 € im Jahr); als Firma mit D-U-N-S-Nummer. Nötig für TestFlight, App Store, Signierung für andere Macs |
| App-ID reservieren | `com.dcsoftwareengineering.volta1` im Developer-Konto anlegen; danach unveränderlich |
| Qt-Lizenz für iOS | Qt wird auf iOS statisch eingebunden. Unter LGPLv3 muss das Neu-Linken ermöglicht werden (Objektdateien bereitstellen) – oder eine kommerzielle Qt-Lizenz. Vor der ersten Veröffentlichung entscheiden |
| TestFlight | Testversion für Verlage und Schulen über das Developer-Konto verteilen |
| App Store | Kategorie Bildung, Altersfreigabe, Datenschutzangaben (keine erhobenen Daten), Bildschirmfotos iPad |
| Mac außerhalb des App Store | Signierung mit »Developer ID«, Notarisierung, Weitergabe als DMG |
| Schulen | Apple School Manager, verwaltete Konfiguration (wie Android) |

Signieren, Hochladen und Veröffentlichen geschieht nur auf ausdrückliche Anweisung.

## 10. Hilfe bei Problemen

| Meldung oder Beobachtung | Ursache und Abhilfe |
|---|---|
| `No such preset ios` | CMake zu alt: `brew upgrade cmake`; oder Ersatzbefehl aus C1 |
| `Could not find Qt6` / `QT_HOST_PATH` | Umgebung aus A4 fehlt: `source ~/.zshrc`, Pfade unter `~/Qt` prüfen |
| Xcode: »iOS … is not installed« | Xcode › Settings › Components: iOS-Plattform laden |
| Qt meldet nicht unterstützte Xcode-Version | Qt-Dokumentation »Supported Platforms« für 6.11 prüfen; ggf. passende Xcode-Version (developer.apple.com › Downloads) |
| »Failed to register bundle identifier« | eigene Test-Kennung eintragen (C3) |
| »Developer Mode disabled« | Entwicklermodus auf dem iPad einschalten (C4) |
| »Untrusted Developer« auf dem iPad | Profil vertrauen (C4, Schritt 5) |
| App startet, Seite bleibt leer | Inhalte nicht eingebettet: `build/ios` löschen und C1 wiederholen |
| Leere Kästchen statt Symbolen | Schrift fehlt auf iPadOS: Foto machen, Zeichen werden wie unter Android selbst gezeichnet |
| `macdeployqt` nicht gefunden | `QT_ROOT_DESKTOP` setzen (A4) |
| Mac: »kann nicht geöffnet werden« | Rechtsklick › Öffnen (E3) |

Bei allen anderen Fehlern: die letzten 30 Zeilen der Ausgabe kopieren und Claude geben oder mitbringen.
