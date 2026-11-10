#!/usr/bin/env python3
"""Produkt-Flyer Volta 3 als PDF (deutsch und englisch), fuer Schulen, Eltern und Gespraeche mit Partnern:

    python3 tools/flyer3_pdf.py            # Release/Flyer/Volta3-Produktflyer-de.pdf und -en.pdf
    python3 tools/flyer3_pdf.py --sprache en
    python3 tools/flyer3_pdf.py --bilder   # vorher die Bildschirmfotos neu aufnehmen

Gleiche Gestaltung und Zeichenhilfen wie die Flyer von Volta 1 und 2 (tools/flyer_pdf.py, tools/flyer2_pdf.py);
Uebersetzungen in tools/flyer3_texte.py (Schluessel = deutscher Text), fehlende Uebersetzungen brechen ab.
Bildschirmfotos ueber die Entwicklerhilfen der Volta-3-Ausgabe (build/volta3-release, sonst volta3-debug), je Sprache (VOLTA_DEMO_LANG).

Fuenf Seiten A4: Titel, Messgeraete, Lernpfad, Werkzeugkasten und Projekte, Schule/Reihe/Kontakt. Inhalte ehrlich
nach dem Stand der App (Experimente und Stufen aus content/volta3).
"""
from __future__ import annotations

import glob
import json
import os
import subprocess
import sys
from collections import Counter
from pathlib import Path

from PIL import Image as PILImage
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfgen import canvas as pdfcanvas

sys.path.insert(0, str(Path(__file__).resolve().parent))
import flyer_pdf as base  # noqa: E402
from flyer2_texte import MUSTER2, TEXTE2  # noqa: E402
from flyer3_texte import MUSTER3, TEXTE3  # noqa: E402

ROOT = base.ROOT
OUT = base.OUT
SHOTS = OUT / "bilder-volta3"
# Release-Build (deploy_release --volta3), sonst Debug: Debug rechnet zu langsam fuer volle Logik- und Lupenbilder
_NAME = "volta3.exe" if os.name == "nt" else "volta3"
EXE = next((p for p in (ROOT / "build" / "volta3-release" / "app" / _NAME, ROOT / "build" / "volta3-debug" / "app" / _NAME)
            if p.exists()), ROOT / "build" / "volta3-debug" / "app" / _NAME)
NAVY, BLUE, YELLOW, GREY, LIGHT, CREAM, GREEN, ORANGE = (base.NAVY, base.BLUE, base.YELLOW, base.GREY, base.LIGHT,
                                                         base.CREAM, base.GREEN, base.ORANGE)
TEAL = colors.HexColor("#0F6E7A")    # Volta 2 in der Reihe
PURPLE = colors.HexColor("#5B2A6E")  # Volta 3 in der Reihe
T = base.T

SCENES = {
    "start": {},
    "frequenzgang": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta3.0.1", "VOLTA_DEMO_BODE": "1",
                     "VOLTA_SCREENSHOT_S": "7"},
    "lupe": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta3.0.3", "VOLTA_DEMO_STRETCH": "1",
             "VOLTA_DEMO_GEN_HZ": "10000", "VOLTA_SCREENSHOT_S": "8"},
    "logik": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta3.0.2", "VOLTA_SCREENSHOT_S": "8"},
    "radio": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta3.2.4", "VOLTA_DEMO_PLAN": "1",
              "VOLTA_SCREENSHOT_S": "6"},
    "zaehler": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta3.8.5", "VOLTA_SCREENSHOT_S": "6"},
}


def take_screenshots(force: bool):
    (SHOTS / base.LANG).mkdir(parents=True, exist_ok=True)
    for name, extra in SCENES.items():
        target = SHOTS / base.LANG / f"{name}.png"
        if target.exists() and not force:
            continue
        env = dict(os.environ)
        env.update({"VOLTA_NO_SPLASH": "1", "VOLTA_DEMO_LANG": base.LANG, "QT_SCALE_FACTOR": "1.5",
                    "VOLTA_SCREENSHOT": str(target), "VOLTA_SCREENSHOT_MAINSWITCH": "1"})
        env.update(extra)
        subprocess.run([str(EXE)], env=env, timeout=120, check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print(f"  Bild {base.LANG}/{name}: {'ok' if target.exists() else 'FEHLT'}")


def img(name):
    return SHOTS / base.LANG / f"{name}.png"


def stages():
    """Stufen (Titel, Frage) und Zahl der Experimente je Stufe aus content/volta3 (deutsche Quelle)."""
    de = json.loads((ROOT / "content" / "volta3" / "i18n" / "de.json").read_text(encoding="utf-8"))
    de = de.get("texte", de)
    count = Counter()
    for f in glob.glob(str(ROOT / "content" / "volta3" / "experiments" / "*.json")):
        count[json.loads(Path(f).read_text(encoding="utf-8"))["id"].split(".")[1]] += 1
    st = json.loads((ROOT / "content" / "volta3" / "stufen.json").read_text(encoding="utf-8"))["stufen"]
    return [(str(s["nr"]), de[s["titel_key"]], de[s["frage_key"]], count[str(s["nr"])]) for s in st], sum(count.values())


class Page(base.Page):
    def footer(self, n):
        self.text(18 * mm, 10 * mm, "Volta 3 – Das Elektronik-Labor · DC Software Engineering · dc-software-engineering.com",
                  8, "Segoe", GREY)
        self.text(self.W - 18 * mm, 10 * mm, f"{n}", 8, "Segoe", GREY, "r")


# ---------------------------------------------------------------- Seiten

def page_cover(p: Page, ver: str, total: int, n_stages: int):
    c, W, H = p.c, p.W, p.H
    steps = 60
    for i in range(steps):  # Verlauf Navy -> Violett (Farbe von Volta 3)
        t = i / (steps - 1)
        c.setFillColor(colors.Color(0.04 + 0.32 * t, 0.15 + 0.02 * t, 0.25 + 0.18 * t))
        c.rect(0, H * (1 - (i + 1) / steps), W, H / steps + 1, fill=1, stroke=0)
    logo = ROOT / "app" / "resources" / "icons" / "volta_256.png"
    c.drawImage(str(logo), 18 * mm, H - 40 * mm, 22 * mm, 22 * mm, mask="auto")
    p.text(45 * mm, H - 27 * mm, "Volta 3", 34, "Segoe-Bold", colors.white)
    p.text(45 * mm, H - 36 * mm, "Das Elektronik-Labor", 18, "Segoe-Bold", YELLOW)
    p.text(18 * mm, H - 52 * mm, "Schwingen, senden, schalten, zählen – ehrlich gerechnet.", 13, "Segoe", colors.white)
    p.text(18 * mm, H - 59 * mm, "Der dritte Band des virtuellen Elektronik-Experimentierkastens, für 13 bis 16 Jahre.",
           11, "Segoe", colors.HexColor("#E3D3EC"))
    framed = SHOTS / base.LANG / "_tablet_frequenzgang.png"
    base.tablet(img("frequenzgang"), framed)
    tw = W - 36 * mm
    iw, ih = PILImage.open(framed).size
    th = tw * ih / iw
    c.drawImage(str(framed), 18 * mm, H - 68 * mm - th, tw, th, mask="auto")
    c.drawImage(str(base.TICK / "tick_freundlich.png"), W - 48 * mm, H - 68 * mm - th - 50 * mm, 36 * mm, 36 * mm,
                mask="auto")
    y = H - 68 * mm - th - 14 * mm
    facts = [(str(total), f"Experimente in {n_stages} Stufen"), ("7", "Projekte zum Nachbauen"),
             ("4", "Sprachen: DE · FR · EN · LU"), ("0", "Daten, Werbung, Käufe – offline")]
    x = 18 * mm
    for big, small in facts:
        c.setFillColor(colors.Color(1, 1, 1, alpha=0.12))
        c.roundRect(x, y - 13 * mm, 33 * mm, 20 * mm, 3 * mm, fill=1, stroke=0)
        p.text(x + 16.5 * mm, y - 1 * mm, big, 18, "Segoe-Bold", YELLOW, "c")
        p.para(x + 2.5 * mm, y - 6.5 * mm, 28 * mm, small, 8, 9.5, "Segoe", colors.white)
        x += 36 * mm
    yb = y - 30 * mm
    p.text(18 * mm, yb, "Neu in Volta 3", 12, "Segoe-Bold", colors.white)
    x, yc = 18 * mm, yb - 9 * mm
    for label in ("Frequenzgang-Schreiber", "Logikanalysator", "Lupe ×10 … ×1000", "Senderanlage und Radio",
                  "Modulation und Wobbeln", "Gatter und Zähler"):
        wch = pdfmetrics.stringWidth(T(label), "Segoe-Bold", 9) + 7 * mm
        if x + wch > W - 52 * mm:
            x, yc = 18 * mm, yc - 9 * mm
        x += p.chip(x, yc, label, colors.Color(1, 1, 1, alpha=0.18), colors.white, 9) + 2.5 * mm
    p.text(18 * mm, yc - 12 * mm, "Zuerst für Windows · Android-Tablet und iPad folgen", 11, "Segoe-Bold", YELLOW)
    p.text(18 * mm, 14 * mm, f"Version {ver} (Entwicklung) · November 2026 · DC Software Engineering, Irrel",
           8.5, "Segoe", colors.HexColor("#E3D3EC"))


def page_instruments(p: Page):
    p.band("Messgeräte für schnelle Signale", "Was im Oszilloskop zu schnell oder zu viel ist, zeigen neue Geräte")
    W, H = p.W, p.H
    x1, x2, cw = 18 * mm, W / 2 + 4 * mm, W / 2 - 22 * mm
    y = H - 38 * mm
    yl = p.feature(x1, y, cw, "Frequenzgang-Schreiber",
                   "Generator an den Eingang, Tastkopf an den Ausgang: Volta zeichnet die Verstärkung in dB von 1 Hz "
                   "bis 10 MHz, auf Wunsch mit Phase. Resonanz, Grenzfrequenzen, Bandbreite und Güte stehen als Zahl "
                   "daneben. Bei Schaltungen, die schalten, erscheint ein Hinweis statt einer falschen Kurve.",
                   img("frequenzgang"))
    yr = p.feature(x2, y, cw, "Lupe im Oszilloskop",
                   "Vorgänge, die schneller sind als die Simulation in Echtzeit rechnet, dehnt die Lupe ×10, ×100 "
                   "oder ×1000. Volta rechnet dann in feineren Schritten – die Zeit auf dem Schirm bleibt die echte. "
                   "Gedehnte Bilder tragen einen gestrichelten Rahmen und den Faktor.",
                   img("lupe"))
    y = min(yl, yr) - 6 * mm
    yl = p.feature(x1, y, cw, "Logikanalysator",
                   "Bis zu acht Logik-Tastköpfe, jeder mit einer Zeile H und L. Trigger auf Flanke oder Muster, zwei "
                   "Zeitmarken mit Abstand in µs und ms, Schwellen zum Einstellen. Im feinen Fenster sieht man, wie "
                   "ein Taster prellt.",
                   img("logik"))
    yr = p.feature(x2, y, cw, "Senderanlage und Radio",
                   "Eine kleine Senderanlage im Pult sendet eigene Aufnahmen auf Mittelwelle und UKW – nur in Volta, "
                   "ohne Netz. Mit Spule, Drehkondensator und Diode entsteht ein Radio ohne Batterie, danach mit "
                   "Verstärker und Modul.",
                   img("radio"))
    p.footer(2)


def page_path(p: Page, st):
    p.band("Der Lernpfad", "Vom Schwingkreis bis zum Zähler")
    c, W, H = p.c, p.W, p.H
    x1 = 18 * mm
    y = H - 38 * mm
    rows = len(st)
    box = 14 * mm + rows * 7.2 * mm
    c.setFillColor(LIGHT)
    c.roundRect(x1, y - box, W - 36 * mm, box + 2 * mm, 3 * mm, fill=1, stroke=0)
    p.text(x1 + 5 * mm, y - 6 * mm, "Die Stufen", 12.5, "Segoe-Bold", BLUE)
    yy = y - 14 * mm
    for nr, title, q, n in st:
        c.setFillColor(PURPLE)
        c.circle(x1 + 9 * mm, yy + 1.3 * mm, 3.2 * mm, fill=1, stroke=0)
        p.text(x1 + 9 * mm, yy, nr, 10, "Segoe-Bold", colors.white, "c")
        size = 10
        while pdfmetrics.stringWidth(T(title), "Segoe-Bold", size) > 64 * mm:
            size -= 0.25
        p.text(x1 + 15 * mm, yy, title, size, "Segoe-Bold")
        size = 9.5  # lange Fragen kleiner, damit die Zahl der Experimente frei bleibt
        while pdfmetrics.stringWidth(T(q), "Segoe-Italic", size) > W - 23 * mm - 16 * mm - (x1 + 82 * mm):
            size -= 0.25
        p.text(x1 + 82 * mm, yy, q, size, "Segoe-Italic", GREY)
        p.text(W - 23 * mm, yy, f"{n} Exp.", 9.5, "Segoe-Bold", NAVY, "r")
        yy -= 7.2 * mm
    y = y - box - 8 * mm
    x2, cw = W / 2 + 4 * mm, W / 2 - 22 * mm
    yl = p.feature(x1, y, cw, "Gatter, Zähler, Anzeige",
                   "Aus Transistoren wird ein NICHT, aus zwei NAND ein UND, aus zwei Gattern ein Gedächtnis. Danach "
                   "zählen Zähler- und Dekadenzähler-Modul, das Anzeige-Modul zeigt Ziffern – bis zum elektronischen "
                   "Würfel und zum Frequenzzähler.",
                   img("zaehler"))
    yr = p.feature(x2, y, cw, "Tick erklärt weiter",
                   "Jedes Experiment beginnt mit einer Forscherfrage und einer Vermutung. Tick zeigt Schritt für "
                   "Schritt, wohin jedes Teil gehört, gibt Tipps und erklärt am Ende, warum es so ist. Wer mehr "
                   "wissen will, liest »Mehr wissen« und »Jetzt in echt«.",
                   img("start"))
    p.footer(3)


def page_tools(p: Page):
    p.band("Werkzeugkasten und Projekte", "Rechnen, nachschlagen, Fehler finden – und am Ende ein Gerät")
    c, W, H = p.c, p.W, p.H
    x1, x2, cw = 18 * mm, W / 2 + 4 * mm, W / 2 - 22 * mm
    y = H - 40 * mm
    groups = [
        ("Formelrechner", ["Schwingkreis (Thomson-Formel)", "Bandbreite und Güte", "Wirkungsgrad und Verlust",
                           "Pegel in dB", "Tastverhältnis und Mittelwert", "Frequenzteiler"]),
        ("Lexikon", ["alle Bauteile und Module", "Modulation, Resonanz, Wirkungsgrad", "Pegel, Flanke, Prellen"]),
        ("Fehlersuche", ["Thyristor ohne Haltestrom", "MOSFET-Gate offen", "Spule ohne Freilaufdiode",
                         "Gatter-Eingang offen", "Taster ohne Entprellung", "Schwingkreis ohne Widerstand"]),
    ]
    yy = y
    for title, items in groups:
        h = 12 * mm + len(items) * 5.6 * mm
        c.setFillColor(LIGHT)
        c.roundRect(x1, yy - h, cw, h + 2 * mm, 3 * mm, fill=1, stroke=0)
        p.text(x1 + 5 * mm, yy - 6 * mm, title, 12, "Segoe-Bold", BLUE)
        y2 = yy - 12.5 * mm
        for it in items:
            c.setFillColor(GREEN)
            c.circle(x1 + 6.5 * mm, y2 + 1.2 * mm, 1.2 * mm, fill=1, stroke=0)
            p.text(x1 + 10 * mm, y2, it, 9.5)
            y2 -= 5.6 * mm
        yy -= h + 6 * mm
    p.para(x1, yy - 1 * mm, cw,
           "Die Fehlersuche stellt Fragen statt Lösungen – und bemängelt nie einen Aufbau, der funktioniert.",
           9, color=GREY)
    projects = ["Der Klangschalen-Oszillator", "Ein UKW-Radio", "Eine Taschenlampe mit einer Zelle",
                "Ein Lampendimmer für Kleinspannung", "Der sparsamste Verstärker", "Eine Waage mit Leuchtbalken",
                "Ein Reaktionszeit-Messer"]
    box_h = 14 * mm + len(projects) * 7 * mm + 22 * mm
    c.setFillColor(CREAM)
    c.roundRect(x2, y - box_h, cw, box_h + 2 * mm, 3 * mm, fill=1, stroke=0)
    yy = y - 6 * mm
    p.text(x2 + 5 * mm, yy, "Sieben Projekte", 12.5, "Segoe-Bold", BLUE)
    yy -= 7 * mm
    for it in projects:
        c.setFillColor(ORANGE)
        c.circle(x2 + 6.5 * mm, yy + 1.2 * mm, 1.3 * mm, fill=1, stroke=0)
        yy = p.para(x2 + 10 * mm, yy, cw - 14 * mm, it, 9.5) - 1.1 * mm
    p.para(x2 + 5 * mm, yy - 3 * mm, cw - 10 * mm,
           "Volta rechnet jede Schaltung wirklich aus – nichts ist vorher aufgezeichnet. Die Abweichung zu den "
           "Formeln bleibt unter 2 %.", 9, color=GREY)
    p.footer(4)


def page_school_series(p: Page, ver: str):
    p.band("Für Schule und Eltern", "Sicher, werbefrei, ohne Konto – wie Volta 1 und 2")
    c, W, H = p.c, p.W, p.H
    x1 = 18 * mm
    y = H - 40 * mm
    items = [
        "Kein Konto, keine Werbung, keine Käufe in der App",
        "Keine Datenerhebung, keine Internetverbindung, keine Berechtigungen",
        "Die Senderanlage sendet nur eigene Aufnahmen in der App – kein Empfang echter Sender",
        "Einstellungen mit PIN: Freigaben, Profile, Ton im Klassenzimmer",
        "Bedienelemente ab 48 dp, Farbe nie allein, Vorlesen der Texte",
        "Sprachen Deutsch, Français, English, Lëtzebuergesch",
        "Sicherheit: nur Kleinspannung aus Batterie und Pult – auch der Dimmer",
    ]
    box = 12 * mm + len(items) * 6.6 * mm
    c.setFillColor(LIGHT)
    c.roundRect(x1, y - box, W - 36 * mm, box + 2 * mm, 3 * mm, fill=1, stroke=0)
    yy = y - 6 * mm
    for it in items:
        c.setFillColor(GREEN)
        c.circle(x1 + 6.5 * mm, yy + 1.2 * mm, 1.3 * mm, fill=1, stroke=0)
        yy = p.para(x1 + 10 * mm, yy, W - 50 * mm, it, 10) - 1.6 * mm
    y = y - box - 10 * mm
    p.text(x1, y, "Die Reihe", 14, "Segoe-Bold", BLUE)
    y -= 6 * mm
    series = [
        ("Volta 1", "8–12 Jahre", "Stromkreis, Schalter, Leuchtdioden, Messen, Sensoren, Transistor, Module",
         "Testphase 0.9", NAVY),
        ("Volta 2", "11–14 Jahre", "Oszilloskop, Funktionsgenerator, Kennlinien, Verstärker, Operationsverstärker, "
         "Timer-Baustein 555, Infrarot", "Testphase", TEAL),
        ("Volta 3", "13–16 Jahre", "Schwingkreis und Radio, Stromversorgung, Thyristor, Verstärkerklassen, "
         "Messverstärker, Logik und Zähler", "in Entwicklung", PURPLE),
        ("Zuse 1", "12–16 Jahre", "Vom Bit über Gatter, Rechenwerk und Speicher zum selbst gebauten "
         "4-Bit-Rechner – und ihn programmieren", "in Entwicklung", colors.HexColor("#0E7490")),
    ]
    for name, age, text, status, col in series:
        c.setFillColor(col)
        c.roundRect(x1, y - 17 * mm, 34 * mm, 18 * mm, 3 * mm, fill=1, stroke=0)
        p.text(x1 + 17 * mm, y - 6.5 * mm, name, 13, "Segoe-Bold", colors.white, "c")
        p.text(x1 + 17 * mm, y - 12.5 * mm, age, 8.5, "Segoe", colors.white, "c")
        p.chip(x1 + 39 * mm, y - 3.5 * mm, status, LIGHT, NAVY, 8.5)
        p.para(x1 + 39 * mm, y - 10.5 * mm, W - 78 * mm, text, 9.5)
        y -= 21 * mm
    y -= 3 * mm
    c.setFillColor(CREAM)
    c.roundRect(x1, y - 40 * mm, W - 36 * mm, 42 * mm, 3 * mm, fill=1, stroke=0)
    c.drawImage(str(base.TICK / "tick_motivierend.png"), W - 52 * mm, y - 38 * mm, 32 * mm, 32 * mm, mask="auto")
    p.text(x1 + 6 * mm, y - 7 * mm, "Kontakt", 13, "Segoe-Bold", BLUE)
    yy = y - 14 * mm
    for line in ("DC Software Engineering · Daniel Zeimentz", "Aufm Rothenhügel 26 · 54666 Irrel · Deutschland",
                 "info@dc-software-engineering.com", "dc-software-engineering.com"):
        p.text(x1 + 6 * mm, yy, line, 10.5)
        yy -= 5.8 * mm
    p.para(x1 + 6 * mm, yy - 1 * mm, W - 90 * mm,
           f"Stand: Version {ver}, in Entwicklung. Volta 3 baut auf Volta 1 und 2 auf; gerne zeigen wir es in einer "
           "Vorführung.", 9, color=GREY)
    p.footer(5)


def build(lang: str, force: bool) -> int:
    base.LANG = lang
    base.FEHLT.clear()
    take_screenshots(force)
    missing = [n for n in SCENES if not img(n).exists()]
    if missing:
        print("Bilder fehlen:", ", ".join(missing))
        return 1
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / f"Volta3-Produktflyer-{lang}.pdf"
    c = pdfcanvas.Canvas(str(path), pagesize=A4)
    c.setTitle(T("Volta 3 – Das Elektronik-Labor · Produktinformation"))
    c.setAuthor("DC Software Engineering")
    ver = base.version()
    st, total = stages()
    if lang != "de":  # Stufentitel und Fragen aus den Inhalten der Sprache
        tr = json.loads((ROOT / "content" / "volta3" / "i18n" / f"{lang}.json").read_text(encoding="utf-8"))
        tr = tr.get("texte", tr)
        de = json.loads((ROOT / "content" / "volta3" / "i18n" / "de.json").read_text(encoding="utf-8"))
        de = de.get("texte", de)
        for k, v in de.items():
            if k.startswith("stufe_") and k in tr:
                base.TEXTE.setdefault(lang, {})[v] = tr[k]
    for draw in (lambda p: page_cover(p, ver, total, len(st)), page_instruments, lambda p: page_path(p, st),
                 page_tools, lambda p: page_school_series(p, ver)):
        draw(Page(c))
        c.showPage()
    if base.FEHLT:
        print(f"Uebersetzung fehlt ({lang}):")
        for f in sorted(base.FEHLT):
            print("   ", repr(f))
        return 1
    c.save()
    print("Flyer:", path)
    return 0


def main() -> int:
    base.fonts()
    for tables in (TEXTE2, TEXTE3):
        for lang, table in tables.items():
            base.TEXTE.setdefault(lang, {}).update(table)
    for rules in (MUSTER2, MUSTER3):
        for lang, r in rules.items():
            base.MUSTER.setdefault(lang, []).extend(r)
    langs = [sys.argv[sys.argv.index("--sprache") + 1]] if "--sprache" in sys.argv else ["de", "en"]
    for lang in langs:
        if build(lang, "--bilder" in sys.argv):
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
