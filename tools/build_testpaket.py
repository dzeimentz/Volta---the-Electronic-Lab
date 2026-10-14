#!/usr/bin/env python3
"""Testpaket fuer Partner, Schulen und Verlage (nicht fuer den Store):

    python3 tools/build_testpaket.py          # Release/Testpaket/Volta1-<version>-Testpaket(.zip)

Inhalt:
- Volta1-<version>-Windows-Setup.exe  NSIS-Installer (packaging/windows/volta1.nsi), ohne Administratorrechte,
                                       nicht signiert (SmartScreen fragt beim ersten Start)
- Volta1-<version>-Android.apk        Test-APK arm64 (Debug-Signatur, Installation aus unbekannten Quellen)
- Kurzanleitung, Anleitung und Produkt-Flyer (PDF), soweit vorhanden
- LIESMICH.txt / README.txt            Installation, Hinweise zur Testversion, Kontakt

Voraussetzungen: aktuelles lokales Release (tools/deploy_release.py), Test-APK (tools/build_android.py), NSIS 3
(makensis). Nichts wird signiert oder hochgeladen (CLAUDE.md).
"""
from __future__ import annotations

import re
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RELEASE = ROOT / "Release"
OUT = RELEASE / "Testpaket"
NSI = ROOT / "packaging" / "windows" / "volta1.nsi"
ICON = ROOT / "app" / "resources" / "icons" / "volta.ico"
MAKENSIS = [Path("C:/Program Files (x86)/NSIS/makensis.exe"), Path("C:/Program Files/NSIS/makensis.exe")]
# Programmdateien im Release-Ordner (alles andere dort sind Begleitdateien: APK, Store, Anleitungen ...)
PROGRAM_DIRS = ["content", "generic", "iconengines", "imageformats", "multimedia", "platforminputcontexts",
                "platforms", "qml", "texttospeech", "tls", "styles", "networkinformation"]


def version() -> str:
    m = re.search(r"project\(Volta\s+VERSION\s+([0-9.]+)", (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"))
    return m.group(1) if m else "0"


LIESMICH = """Volta 1 – Das Elektronik-Labor · Testversion {ver}
==================================================

Vielen Dank, dass Sie Volta ausprobieren! Dies ist eine Testversion für Partner, Schulen und Verlage. Bitte
geben Sie sie nicht weiter und veröffentlichen Sie keine Bildschirmfotos ohne Rücksprache.

Inhalt
------
  Volta1-{ver}-Windows-Setup.exe   Installation für Windows 10/11 (64 Bit)
  Volta1-{ver}-Android.apk         Testversion für Android-Tablets (ab Android 8, empfohlen ab 10 Zoll)
  Volta1-Kurzanleitung-de.pdf      zwei Seiten zum Einstieg
  Volta1-Anleitung-de.pdf          ausführliche Anleitung
  Volta1-Produktflyer-*.pdf        Produktinformation

Windows
-------
1. Volta1-{ver}-Windows-Setup.exe starten. Es werden keine Administratorrechte gebraucht.
2. Die Testversion ist nicht signiert. Windows zeigt deshalb beim ersten Start »Der Computer wurde durch
   Windows geschützt«: auf »Weitere Informationen« und dann »Trotzdem ausführen« klicken.
3. Volta liegt danach im Startmenü (auf Wunsch auch auf dem Desktop). Entfernen über Einstellungen → Apps.

Android-Tablet
--------------
1. Die APK-Datei auf das Tablet kopieren (USB-Kabel, Speicherkarte oder Cloud-Laufwerk).
2. Datei antippen. Android fragt, ob Apps aus dieser Quelle installiert werden dürfen: für die Dateien-App
   (bzw. den Browser) erlauben und »Installieren« antippen.
3. Die App braucht keine Berechtigungen und keine Internetverbindung.

Zum Ausprobieren
----------------
- Startseite → »Vorführung«: zehn fertige Aufbauten aus allen Stufen, ohne Spuren im Fortschritt.
- Startseite → »Kurzanleitung«: in sechs Schritten zum ersten Experiment.
- Elternbereich: Menü (drei Striche) → »Elternbereich …«, Start-PIN 1234.
- Alle Stufen sofort öffnen: Elternbereich → »Alle Stufen und Bauteile freigeben«.

Datenschutz
-----------
Volta erhebt keine Daten, braucht kein Konto und keine Internetverbindung. Fortschritt, Forscherheft und
Aufbauten bleiben auf dem Gerät.

Kontakt
-------
DC Software Engineering · Daniel Zeimentz · Aufm Rothenhügel 26 · 54666 Irrel · Deutschland
info@dc-software-engineering.com · https://dc-software-engineering.com
"""

README = """Volta 1 – The Electronics Lab · test version {ver}
==================================================

Thank you for trying Volta! This is a test version for partners, schools and publishers. Please do not pass
it on or publish screenshots without asking us first.

Windows 10/11 (64-bit): run Volta1-{ver}-Windows-Setup.exe (no administrator rights needed). The test version
is not signed, so Windows SmartScreen asks on first start: click »More info«, then »Run anyway«.

Android tablet (Android 8 or later, 10 inches or more recommended): copy the APK to the tablet, tap it and allow
installing apps from this source. The app needs no permissions and no internet connection.

To try: home page → »Demo« (ten ready-made builds), »Quick guide«; parents' area via the menu (three lines),
start PIN 1234. Language: flag at the top right (Deutsch, Français, English, Lëtzebuergesch).

Privacy: Volta collects no data, needs no account and no internet connection.

Contact: DC Software Engineering · Daniel Zeimentz · Aufm Rothenhügel 26 · 54666 Irrel · Germany
info@dc-software-engineering.com · https://dc-software-engineering.com
"""


def main() -> int:
    ver = version()
    makensis = next((m for m in MAKENSIS if m.exists()), None)
    if not makensis:
        print("NSIS (makensis) nicht gefunden")
        return 1
    if not (RELEASE / "volta1.exe").exists():
        print("Release fehlt: zuerst python tools/deploy_release.py")
        return 1
    name = f"Volta1-{ver}-Testpaket"
    pkg = OUT / name
    if pkg.exists():
        shutil.rmtree(pkg)
    pkg.mkdir(parents=True)

    # Programmdateien zusammenstellen (ohne Begleitdateien)
    stage = OUT / "_programm"
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir()
    for f in RELEASE.iterdir():
        if f.is_file() and f.suffix.lower() in (".exe", ".dll"):
            shutil.copy2(f, stage / f.name)
        elif f.is_dir() and f.name in PROGRAM_DIRS:
            shutil.copytree(f, stage / f.name)

    # NSIS-Skript als UTF-8 mit BOM uebergeben (sonst liest makensis die ANSI-Codepage)
    nsi = OUT / "_volta1.nsi"
    nsi.write_text(NSI.read_text(encoding="utf-8"), encoding="utf-8-sig")
    setup = pkg / f"Volta1-{ver}-Windows-Setup.exe"
    res = subprocess.run([str(makensis), "/V2", f"/DVERSION={ver}", f"/DSRC={stage}", f"/DOUT={setup}",
                          f"/DICON={ICON}", str(nsi)], capture_output=True, text=True, encoding="utf-8",
                         errors="replace")
    if res.returncode != 0 or not setup.exists():
        print(res.stdout[-3000:], res.stderr[-2000:])
        return 1
    shutil.rmtree(stage)
    nsi.unlink()

    apk = RELEASE / "android" / f"Volta1-{ver}-arm64-debug.apk"
    if apk.exists():
        shutil.copy2(apk, pkg / f"Volta1-{ver}-Android.apk")
    else:
        print("Hinweis: keine Test-APK gefunden (tools/build_android.py)")
    for pdf in [RELEASE / "Anleitung" / "Volta1-Kurzanleitung-de.pdf", RELEASE / "Anleitung" / "Volta1-Anleitung-de.pdf",
                *sorted((RELEASE / "Flyer").glob("Volta1-Produktflyer-*.pdf"))]:
        if pdf.exists():
            shutil.copy2(pdf, pkg / pdf.name)
    (pkg / "LIESMICH.txt").write_text(LIESMICH.format(ver=ver), encoding="utf-8-sig")
    (pkg / "README.txt").write_text(README.format(ver=ver), encoding="utf-8-sig")

    archive = OUT / f"{name}.zip"
    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for f in sorted(pkg.iterdir()):
            z.write(f, f"{name}/{f.name}")
    for f in sorted(pkg.iterdir()):
        print(f"  {f.name:42s} {f.stat().st_size / 2**20:6.1f} MB")
    print(f"Testpaket: {archive} ({archive.stat().st_size / 2**20:.1f} MB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
