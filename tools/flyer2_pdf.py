#!/usr/bin/env python3
"""Produkt-Flyer Volta 2 als PDF (deutsch und englisch), fuer Schulen, Eltern und Gespraeche mit Partnern:

    python3 tools/flyer2_pdf.py            # Release/Flyer/Volta2-Produktflyer-de.pdf und -en.pdf
    python3 tools/flyer2_pdf.py --sprache en
    python3 tools/flyer2_pdf.py --bilder   # vorher die Bildschirmfotos neu aufnehmen

Gleiche Gestaltung und Zeichenhilfen wie der Flyer von Volta 1 (tools/flyer_pdf.py); Uebersetzungen in
tools/flyer2_texte.py (Schluessel = deutscher Text), fehlende Uebersetzungen brechen ab. Bildschirmfotos ueber die
Entwicklerhilfen der Volta-2-Ausgabe (build/volta2-debug), je Sprache (VOLTA_DEMO_LANG).

Fuenf Seiten A4: Titel, Messgeraete, Lernpfad, Projekte und Schaltplan, Schule/Reihe/Kontakt. Inhalte ehrlich nach
dem Stand der App (Experimente und Stufen aus content/volta2, Plattformstand wie Volta 1).
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

ROOT = base.ROOT
OUT = base.OUT
SHOTS = OUT / "bilder-volta2"
EXE = ROOT / "build" / "volta2-debug" / "app" / ("volta2.exe" if os.name == "nt" else "volta2")
NAVY, BLUE, YELLOW, AMBER, GREY, LIGHT, CREAM, GREEN, ORANGE = (base.NAVY, base.BLUE, base.YELLOW, base.AMBER,
                                                                 base.GREY, base.LIGHT, base.CREAM, base.GREEN,
                                                                 base.ORANGE)
TEAL = colors.HexColor("#0F6E7A")  # Farbe von Volta 2 in der Reihe
T = base.T

SCENES = {
    "start": {},
    "oszilloskop": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta2.4.4", "VOLTA_SCREENSHOT_S": "7"},
    "generator": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta2.2.2", "VOLTA_DEMO_GENERATOR": "1",
                  "VOLTA_SCREENSHOT_S": "6"},
    "kennlinie": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta2.1.4", "VOLTA_DEMO_TRACER": "1",
                  "VOLTA_SCREENSHOT_S": "8"},
    "netzteil": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta2.3.7", "VOLTA_DEMO_PLAN": "1",
                 "VOLTA_SCREENSHOT_S": "6"},
    "emitter": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta2.4.3", "VOLTA_DEMO_PLAN": "1",
                "VOLTA_SCREENSHOT_S": "6"},
    "blick_hinein": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta2.6.3", "VOLTA_DEMO_INSIDE": "TI1",
                     "VOLTA_SCREENSHOT_S": "6"},
    "uebernehmen": {"VOLTA_DEMO_GUIDE": "1", "VOLTA_DEMO_EXP": "volta2.4.3", "VOLTA_DEMO_DONE": "volta2.4.2",
                    "VOLTA_SCREENSHOT_S": "4"},
    "geraetefach": {"VOLTA_DEMO_FREELAB": "1", "VOLTA_DEMO_DESKTRAY": "1", "VOLTA_SCREENSHOT_S": "4"},
}


def take_screenshots(force: bool):
    (SHOTS / base.LANG).mkdir(parents=True, exist_ok=True)
    for name, extra in SCENES.items():
        target = SHOTS / base.LANG / f"{name}.png"
        if target.exists() and not force:
            continue
        env = dict(os.environ)
        env.update({"VOLTA_NO_SPLASH": "1", "VOLTA_DEMO_LANG": base.LANG, "QT_SCALE_FACTOR": "1.5",
                    "VOLTA_SCREENSHOT": str(target)})
        env.update(extra)
        subprocess.run([str(EXE)], env=env, timeout=90, check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print(f"  Bild {base.LANG}/{name}: {'ok' if target.exists() else 'FEHLT'}")


def img(name):
    return SHOTS / base.LANG / f"{name}.png"


def stages():
    """Stufen (Titel, Frage) und Zahl der Experimente je Stufe aus content/volta2 (deutsche Quelle)."""
    de = json.loads((ROOT / "content" / "volta2" / "i18n" / "de.json").read_text(encoding="utf-8"))
    de = de.get("texte", de)
    count = Counter()
    for f in glob.glob(str(ROOT / "content" / "volta2" / "experiments" / "*.json")):
        count[json.loads(Path(f).read_text(encoding="utf-8"))["id"].split(".")[1]] += 1
    st = json.loads((ROOT / "content" / "volta2" / "stufen.json").read_text(encoding="utf-8"))["stufen"]
    return [(str(s["nr"]), de[s["titel_key"]], de[s["frage_key"]], count[str(s["nr"])]) for s in st], sum(count.values())


class Page(base.Page):
    def footer(self, n):
        self.text(18 * mm, 10 * mm, "Volta 2 – Das Elektronik-Labor · DC Software Engineering · dc-software-engineering.com",
                  8, "Segoe", GREY)
        self.text(self.W - 18 * mm, 10 * mm, f"{n}", 8, "Segoe", GREY, "r")


# ---------------------------------------------------------------- Seiten

def page_cover(p: Page, ver: str, total: int):
    c, W, H = p.c, p.W, p.H
    steps = 60
    for i in range(steps):  # Verlauf Navy -> Petrol (Farbe von Volta 2)
        t = i / (steps - 1)
        c.setFillColor(colors.Color(0.04 + 0.02 * t, 0.15 + 0.30 * t, 0.25 + 0.27 * t))
        c.rect(0, H * (1 - (i + 1) / steps), W, H / steps + 1, fill=1, stroke=0)
    logo = ROOT / "app" / "resources" / "icons" / "volta_256.png"
    c.drawImage(str(logo), 18 * mm, H - 40 * mm, 22 * mm, 22 * mm, mask="auto")
    p.text(45 * mm, H - 27 * mm, "Volta 2", 34, "Segoe-Bold", colors.white)
    p.text(45 * mm, H - 36 * mm, "Das Elektronik-Labor", 18, "Segoe-Bold", YELLOW)
    p.text(18 * mm, H - 52 * mm, "Messen, sehen, hören: Oszilloskop, Generator und Verstärker – ehrlich gerechnet.",
           13, "Segoe", colors.white)
    p.text(18 * mm, H - 59 * mm, "Der zweite Band des virtuellen Elektronik-Experimentierkastens, für 11 bis 14 Jahre.",
           11, "Segoe", colors.HexColor("#CFE3F7"))
    framed = SHOTS / base.LANG / "_tablet_oszilloskop.png"
    base.tablet(img("oszilloskop"), framed)
    tw = W - 36 * mm
    iw, ih = PILImage.open(framed).size
    th = tw * ih / iw
    c.drawImage(str(framed), 18 * mm, H - 68 * mm - th, tw, th, mask="auto")
    c.drawImage(str(base.TICK / "tick_freundlich.png"), W - 48 * mm, H - 68 * mm - th - 50 * mm, 36 * mm, 36 * mm,
                mask="auto")
    y = H - 68 * mm - th - 14 * mm
    facts = [(str(total), "Experimente in 9 Stufen"), ("15 + 2", "neue Bauteile und Module"),
             ("4", "Sprachen: DE · FR · EN · LU"), ("0", "Daten, Werbung, Käufe – offline")]
    x = 18 * mm
    for big, small in facts:
        c.setFillColor(colors.Color(1, 1, 1, alpha=0.12))
        c.roundRect(x, y - 13 * mm, 33 * mm, 20 * mm, 3 * mm, fill=1, stroke=0)
        p.text(x + 16.5 * mm, y - 1 * mm, big, 18, "Segoe-Bold", YELLOW, "c")
        p.para(x + 2.5 * mm, y - 6.5 * mm, 28 * mm, small, 8, 9.5, "Segoe", colors.white)
        x += 36 * mm
    yb = y - 30 * mm
    p.text(18 * mm, yb, "Neu in Volta 2", 12, "Segoe-Bold", colors.white)
    x, yc = 18 * mm, yb - 9 * mm
    for label in ("Oszilloskop", "Funktionsgenerator", "Kennlinienschreiber", "Hörbare Simulation",
                  "Schaltplan nach Norm", "Aufbau übernehmen"):
        wch = pdfmetrics.stringWidth(T(label), "Segoe-Bold", 9) + 7 * mm
        if x + wch > W - 52 * mm:
            x, yc = 18 * mm, yc - 9 * mm
        x += p.chip(x, yc, label, colors.Color(1, 1, 1, alpha=0.18), colors.white, 9) + 2.5 * mm
    p.text(18 * mm, yc - 12 * mm, "Android-Tablet und Windows · iPad, Mac und Linux folgen", 11, "Segoe-Bold", YELLOW)
    p.text(18 * mm, 14 * mm, f"Version {ver} (Testphase) · November 2026 · DC Software Engineering, Irrel",
           8.5, "Segoe", colors.HexColor("#CFE3F7"))


def page_instruments(p: Page):
    p.band("Messgeräte wie im echten Labor", "Was Volta 1 begonnen hat, wird jetzt sichtbar und hörbar")
    W, H = p.W, p.H
    x1, x2, cw = 18 * mm, W / 2 + 4 * mm, W / 2 - 22 * mm
    y = H - 38 * mm
    yl = p.feature(x1, y, cw, "Oszilloskop",
                   "Zwei Kanäle mit Tastköpfen direkt auf der Grundplatte. Zeit, Volt je Kästchen, Lage und Trigger "
                   "stellt man wie am echten Gerät ein – oder tippt auf »Auto« je Kanal, Zeit und Trigger. Dazu "
                   "Rollbetrieb und XY-Darstellung. Es schaltet mit dem Hauptschalter ein und aus.",
                   img("oszilloskop"))
    yr = p.feature(x2, y, cw, "Funktionsgenerator",
                   "Sinus, Dreieck und Rechteck von 1 Hz bis 10 kHz, mit einstellbarer Spitzenspannung und "
                   "Gleichanteil. Was er erzeugt, rechnet Volta durch die ganze Schaltung – und über den "
                   "Lautsprecher im Pult hört man das Ergebnis.",
                   img("generator"))
    y = min(yl, yr) - 6 * mm
    yl = p.feature(x1, y, cw, "Kennlinienschreiber",
                   "Er fährt die Spannung an einem Bauteil hoch und zeichnet den Strom dazu: die Gerade des "
                   "Widerstands, die Kurve der Glühlampe, den Knick der Diode. So wird sichtbar, was die Formeln "
                   "aus Volta 1 meinen – und wo sie aufhören zu gelten.",
                   img("kennlinie"))
    yr = p.feature(x2, y, cw, "Pult nach Bedarf",
                   "Neue Geräte kommen erst ins Pult, wenn sie gebraucht werden: zweite Batterie, Wechselquelle, "
                   "Generator und Kennlinienschreiber. Mit »+« holt man sie aus dem Gerätefach, mit »−« nimmt man "
                   "sie wieder heraus. Leuchten zeigen, was eingeschaltet ist.",
                   img("geraetefach"))
    p.footer(2)


def page_path(p: Page, st):
    p.band("Der Lernpfad", "Neun Stufen – vom genauen Messen bis zur Lichtschranke")
    c, W, H = p.c, p.W, p.H
    x1 = 18 * mm
    y = H - 38 * mm
    rows = len(st)
    box = 14 * mm + rows * 7.2 * mm
    c.setFillColor(LIGHT)
    c.roundRect(x1, y - box, W - 36 * mm, box + 2 * mm, 3 * mm, fill=1, stroke=0)
    p.text(x1 + 5 * mm, y - 6 * mm, "Die neun Stufen", 12.5, "Segoe-Bold", BLUE)
    yy = y - 14 * mm
    for nr, title, q, n in st:
        c.setFillColor(TEAL)
        c.circle(x1 + 9 * mm, yy + 1.3 * mm, 3.2 * mm, fill=1, stroke=0)
        p.text(x1 + 9 * mm, yy, nr, 10, "Segoe-Bold", colors.white, "c")
        p.text(x1 + 15 * mm, yy, title, 10, "Segoe-Bold")
        size = 9.5  # lange Fragen kleiner, damit die Zahl der Experimente frei bleibt
        while pdfmetrics.stringWidth(T(q), "Segoe-Italic", size) > W - 23 * mm - 16 * mm - (x1 + 82 * mm):
            size -= 0.25
        p.text(x1 + 82 * mm, yy, q, size, "Segoe-Italic", GREY)
        p.text(W - 23 * mm, yy, f"{n} Exp.", 9.5, "Segoe-Bold", NAVY, "r")
        yy -= 7.2 * mm
    y = y - box - 8 * mm
    x2, cw = W / 2 + 4 * mm, W / 2 - 22 * mm
    yl = p.feature(x1, y, cw, "Aufbau übernehmen",
                   "Viele Versuche bauen aufeinander auf. Wer den Arbeitspunkt geschafft hat, findet ihn im nächsten "
                   "Experiment als beschrifteten Knopf in der Aufbaubeschreibung: ein Tippen setzt den fertigen "
                   "Aufbau ein, nur das Neue kommt dazu. Alles bleibt rückgängig machbar.",
                   img("uebernehmen"))
    yr = p.feature(x2, y, cw, "Tick erklärt weiter",
                   "Jedes Experiment beginnt mit einer Forscherfrage und einer Vermutung. Tick zeigt Schritt für "
                   "Schritt, wohin jedes Teil gehört, gibt Tipps und erklärt am Ende, warum es so ist. Ein »?« an "
                   "jedem Gerät öffnet eine kurze Anleitung.",
                   img("start"))
    p.footer(3)


def page_projects(p: Page):
    p.band("Projekte, Schaltpläne, Blick hinein", "Vom einzelnen Versuch zum fertigen Gerät")
    c, W, H = p.c, p.W, p.H
    x1, x2, cw = 18 * mm, W / 2 + 4 * mm, W / 2 - 22 * mm
    y = H - 38 * mm
    yl = p.feature(x1, y, cw, "Schaltplan nach Norm",
                   "Jeder Aufbau erscheint als Schaltplan mit Normsymbolen. Bekannte Grundschaltungen zeichnet Volta "
                   "so, wie sie in jedem Buch stehen: den Brückengleichrichter als Raute, die Emitterschaltung mit "
                   "dem Transistor in der Mitte, die Darlingtonschaltung. Der Plan lässt sich vergrößern und "
                   "verkleinern.",
                   img("netzteil"))
    yr = p.feature(x2, y, cw, "Blick in den 555",
                   "Der Timer-Baustein 555 lässt sich öffnen: Ein Blockbild zeigt Spannungsteiler, zwei Vergleicher "
                   "und das Flipflop – mit den Spannungen, die gerade gerechnet werden. Blink- und Tonmodul aus "
                   "Volta 1 baut man nach und sieht, wie sie innen arbeiten.",
                   img("blick_hinein"))
    y = min(yl, yr) - 6 * mm
    yl = p.feature(x1, y, cw, "Der Transistor als Verstärker",
                   "Arbeitspunkt einstellen, Emitterschaltung aufbauen, übersteuern und wieder sauber machen – am "
                   "Oszilloskop sieht man Eingang und Ausgang übereinander. Am Ende steht ein Audioverstärker.",
                   img("emitter"))
    # Projekte als Liste
    projects = ["Ein Netzteil mit Gleichrichter, Elko und 5-Volt-Regler", "Ein Audioverstärker mit Gegentakt-Endstufe",
                "Ein Klangregler mit Operationsverstärkern", "Ein Reaktionstester mit dem 555",
                "Ein kleiner Synthesizer", "Eine Alarmanlage mit Lichtschranke"]
    box_h = (y - yl) + 2 * mm
    c.setFillColor(CREAM)
    c.roundRect(x2, yl - 2 * mm, cw, box_h, 3 * mm, fill=1, stroke=0)
    yy = y - 6 * mm
    p.text(x2 + 5 * mm, yy, "Sechs Projekte", 12.5, "Segoe-Bold", BLUE)
    yy -= 7 * mm
    for it in projects:
        c.setFillColor(ORANGE)
        c.circle(x2 + 6.5 * mm, yy + 1.2 * mm, 1.3 * mm, fill=1, stroke=0)
        yy = p.para(x2 + 10 * mm, yy, cw - 14 * mm, it, 9.5) - 1.1 * mm
    p.para(x2 + 5 * mm, yy - 2 * mm, cw - 10 * mm,
           "Volta rechnet jede Schaltung wirklich aus – nichts ist vorher aufgezeichnet. Wer genau hinschauen "
           "will, nimmt die Zeitlupe.", 9, color=GREY)
    p.footer(4)


def page_school_series(p: Page, ver: str):
    p.band("Für Schule und Eltern", "Sicher, werbefrei, ohne Konto – wie schon bei Volta 1")
    c, W, H = p.c, p.W, p.H
    x1 = 18 * mm
    y = H - 40 * mm
    items = [
        "Kein Konto, keine Werbung, keine Käufe in der App",
        "Keine Datenerhebung, keine Internetverbindung, keine Berechtigungen",
        "Aufgaben per QR-Code und Forscherheft wie in Volta 1",
        "Einstellungen mit PIN: Freigaben, Profile, Ton im Klassenzimmer",
        "Bedienelemente ab 48 dp, Farbe nie allein, Vorlesen der Texte",
        "Sprachen Deutsch, Français, English, Lëtzebuergesch",
        "Sicherheit: nur Kleinspannung aus Batterie und Pult",
    ]
    box = 12 * mm + len(items) * 6.6 * mm
    c.setFillColor(LIGHT)
    c.roundRect(x1, y - box, W - 36 * mm, box + 2 * mm, 3 * mm, fill=1, stroke=0)
    yy = y - 6 * mm
    for it in items:
        c.setFillColor(GREEN)
        c.circle(x1 + 6.5 * mm, yy + 1.2 * mm, 1.3 * mm, fill=1, stroke=0)
        yy = p.para(x1 + 10 * mm, yy, W - 50 * mm, it, 10) - 1.6 * mm
    # Reihe
    y = y - box - 10 * mm
    p.text(x1, y, "Die Reihe", 14, "Segoe-Bold", BLUE)
    y -= 6 * mm
    series = [
        ("Volta 1", "8–12 Jahre", "Stromkreis, Schalter, Leuchtdioden, Messen, Sensoren, Transistor, Module",
         "Testphase 0.9", NAVY),
        ("Volta 2", "11–14 Jahre", "Oszilloskop, Funktionsgenerator, Kennlinien, Verstärker, Operationsverstärker, "
         "Timer-Baustein 555, Infrarot", "Testphase", TEAL),
        ("Volta 3", "13–16 Jahre", "Schwingkreis und Radio, Stromversorgung, Messverstärker, digitale "
         "Grundschaltungen", "Konzept", colors.HexColor("#5B2A6E")),
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
    # Kontakt
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
           f"Stand: Version {ver}, Testphase. Volta 2 baut auf Volta 1 auf; gerne zeigen wir es in einer Vorführung.",
           9, color=GREY)
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
    path = OUT / f"Volta2-Produktflyer-{lang}.pdf"
    c = pdfcanvas.Canvas(str(path), pagesize=A4)
    c.setTitle(T("Volta 2 – Das Elektronik-Labor · Produktinformation"))
    c.setAuthor("DC Software Engineering")
    ver = base.version()
    st, total = stages()
    if lang != "de":  # Stufentitel und Fragen aus den Inhalten der Sprache
        tr = json.loads((ROOT / "content" / "volta2" / "i18n" / f"{lang}.json").read_text(encoding="utf-8"))
        tr = tr.get("texte", tr)
        de = json.loads((ROOT / "content" / "volta2" / "i18n" / "de.json").read_text(encoding="utf-8"))
        de = de.get("texte", de)
        for k, v in de.items():
            if k.startswith("stufe_") and k in tr:
                base.TEXTE.setdefault(lang, {})[v] = tr[k]
    for draw in (lambda p: page_cover(p, ver, total), page_instruments, lambda p: page_path(p, st), page_projects,
                 lambda p: page_school_series(p, ver)):
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
    for lang, table in TEXTE2.items():
        base.TEXTE.setdefault(lang, {}).update(table)
    for lang, rules in MUSTER2.items():
        base.MUSTER.setdefault(lang, []).extend(rules)
    langs = [sys.argv[sys.argv.index("--sprache") + 1]] if "--sprache" in sys.argv else ["de", "en"]
    for lang in langs:
        if build(lang, "--bilder" in sys.argv):
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
