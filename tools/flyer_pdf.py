#!/usr/bin/env python3
"""Produkt-Flyer Volta 1 als PDF (deutsch), fuer Schulen, Eltern und Gespraeche mit Partnern:

    python3 tools/flyer_pdf.py            # Release/Flyer/Volta1-Produktflyer-de.pdf
    python3 tools/flyer_pdf.py --bilder   # vorher die Bildschirmfotos neu aufnehmen

Fuenf Seiten A4: Titel mit Tablet-Ansicht und Eckdaten, Forschen im Labor, Messen und Verstehen, Schule und Eltern,
Plattformen und Reihe. Bildschirmfotos 1920 x 1200 (Fenster 1280 x 800, Skalierung 1,5) ueber die Entwicklerhilfen
der Desktop-Ausgabe; Plattformstand ehrlich nach docs/platforms.md und docs/roadmap.md. Benoetigt reportlab, Pillow
und Segoe UI. Version aus CMakeLists.txt.
"""
from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path

from PIL import Image as PILImage, ImageDraw
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas as pdfcanvas

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "Release" / "Flyer"
SHOTS = OUT / "bilder"
EXE = ROOT / "build" / "desktop-debug" / "app" / ("volta1.exe" if os.name == "nt" else "volta1")
FONTS = Path("C:/Windows/Fonts")
TICK = ROOT / "app" / "resources" / "tick"

NAVY = colors.HexColor("#0A2540")
BLUE = colors.HexColor("#1E5FC2")
SKY = colors.HexColor("#4A8CCC")
AMBER = colors.HexColor("#FFB07A")
ORANGE = colors.HexColor("#FF6B1A")
YELLOW = colors.HexColor("#FFD23F")
CREAM = colors.HexColor("#FFF8E6")
GREY = colors.HexColor("#5A6573")
LIGHT = colors.HexColor("#EEF3F8")
GREEN = colors.HexColor("#2E7D32")

SCENES = {
    "start": {},
    "blinker": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.4.10", "VOLTA_SCREENSHOT_S": "6"},
    "aufbauhilfe": {"VOLTA_DEMO_GUIDE": "1", "VOLTA_DEMO_EXP": "volta1.1.1", "VOLTA_SCREENSHOT_S": "4"},
    "messgeraet": {"VOLTA_DEMO_METER": "v"},
    "zeitschreiber": {"VOLTA_DEMO_RECORDER": "1", "VOLTA_SCREENSHOT_S": "6"},
    "schaltplan": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.1.9", "VOLTA_DEMO_PLAN": "1",
                   "VOLTA_SCREENSHOT_S": "5"},
    "blick_hinein": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.0.1", "VOLTA_DEMO_INSIDE": "BL1",
                     "VOLTA_SCREENSHOT_S": "6"},
    "fehlersuche": {"VOLTA_DEMO_TOOLBOX": "4", "VOLTA_SCREENSHOT_S": "4"},
    "umwelt": {"VOLTA_DEMO_SENSORS": "1", "VOLTA_DEMO_FLYOUT": "1", "VOLTA_SCREENSHOT_S": "4"},
    "aufgabe_qr": {"VOLTA_DEMO_TASK": "qr"},
    "vorfuehrung": {"VOLTA_DEMO_SHOW": "1"},
    "hilfe": {"VOLTA_DEMO_HELP": "1"},
    "forscherheft": {"VOLTA_DEMO_JOURNAL": "1"},
}


def version() -> str:
    m = re.search(r"project\(Volta\s+VERSION\s+([0-9.]+)", (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"))
    return m.group(1) if m else "0"


def take_screenshots(force: bool):
    SHOTS.mkdir(parents=True, exist_ok=True)
    for name, extra in SCENES.items():
        target = SHOTS / f"{name}.png"
        if target.exists() and not force:
            continue
        env = dict(os.environ)
        env.update({"VOLTA_NO_SPLASH": "1", "VOLTA_DEMO_LANG": "de", "QT_SCALE_FACTOR": "1.5",
                    "VOLTA_SCREENSHOT": str(target)})
        env.update(extra)
        subprocess.run([str(EXE)], env=env, timeout=60, check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print(f"  Bild {name}: {'ok' if target.exists() else 'FEHLT'}")


def tablet(src: Path, dst: Path):
    """Bildschirmfoto in einen gezeichneten Tablet-Rahmen setzen (dunkler Rand, abgerundet, Kamerapunkt)."""
    shot = PILImage.open(src).convert("RGB")
    w, h = shot.size
    b = 70
    frame = PILImage.new("RGBA", (w + 2 * b, h + 2 * b), (0, 0, 0, 0))
    d = ImageDraw.Draw(frame)
    d.rounded_rectangle([0, 0, w + 2 * b - 1, h + 2 * b - 1], radius=90, fill=(28, 32, 38, 255),
                        outline=(70, 78, 88, 255), width=6)
    d.ellipse([b // 2 - 9, (h + 2 * b) // 2 - 9, b // 2 + 9, (h + 2 * b) // 2 + 9], fill=(55, 62, 72, 255))
    frame.paste(shot, (b, b))
    frame.save(dst)


# ---------------------------------------------------------------- Zeichenhilfen

class Page:
    def __init__(self, c):
        self.c = c
        self.W, self.H = A4

    def text(self, x, y, s, size=10, font="Segoe", color=NAVY, anchor="l"):
        c = self.c
        c.setFont(font, size)
        c.setFillColor(color)
        {"l": c.drawString, "r": c.drawRightString, "c": c.drawCentredString}[anchor](x, y, s)

    def para(self, x, y, width, s, size=10, leading=None, font="Segoe", color=NAVY):
        """Einfacher Zeilenumbruch; gibt die y-Position unter dem Absatz zurueck."""
        leading = leading or size * 1.38
        c = self.c
        c.setFont(font, size)
        c.setFillColor(color)
        words, line = s.split(), ""
        for wd in words:
            test = (line + " " + wd).strip()
            if pdfmetrics.stringWidth(test, font, size) > width and line:
                c.drawString(x, y, line)
                y -= leading
                line = wd
            else:
                line = test
        if line:
            c.drawString(x, y, line)
            y -= leading
        return y

    def image(self, path, x, y, w=None, h=None, border=True):
        iw, ih = PILImage.open(path).size
        if w and not h:
            h = w * ih / iw
        if h and not w:
            w = h * iw / ih
        self.c.drawImage(str(path), x, y, w, h, mask="auto")
        if border:
            self.c.setStrokeColor(colors.HexColor("#B8C2CC"))
            self.c.setLineWidth(0.6)
            self.c.rect(x, y, w, h, stroke=1, fill=0)
        return w, h

    def band(self, title, subtitle=None, color=NAVY):
        """Kopfband mit Seitentitel und duennem Schaltplanstrich."""
        c = self.c
        c.setFillColor(color)
        c.rect(0, self.H - 30 * mm, self.W, 30 * mm, fill=1, stroke=0)
        c.setStrokeColor(colors.HexColor("#2C5A8A"))
        c.setLineWidth(0.8)
        y = self.H - 26 * mm
        c.line(0, y, self.W, y)
        for i, x in enumerate(range(30, int(self.W), 70)):
            c.rect(x, y - 3, 18, 6, stroke=1, fill=0) if i % 2 == 0 else c.circle(x + 9, y, 2, stroke=0, fill=1)
        self.text(18 * mm, self.H - 15 * mm, title, 22, "Segoe-Bold", colors.white)
        if subtitle:
            self.text(18 * mm, self.H - 21.5 * mm, subtitle, 11, "Segoe", AMBER)

    def footer(self, n):
        self.text(18 * mm, 10 * mm, "Volta 1 – Das Elektronik-Labor · DC Software Engineering · dc-software-engineering.com",
                  8, "Segoe", GREY)
        self.text(self.W - 18 * mm, 10 * mm, f"{n}", 8, "Segoe", GREY, "r")

    def feature(self, x, y, w, title, body, img=None, img_h=None):
        """Kasten mit Bild (oben), Titel und Text; gibt die Unterkante zurueck."""
        c = self.c
        top = y
        if img:
            iw, ih = PILImage.open(img).size
            h = img_h or w * ih / iw
            self.image(img, x, y - h, w, h)
            y -= h + 5 * mm
        self.text(x, y, title, 12.5, "Segoe-Bold", BLUE)
        y -= 5.5 * mm
        y = self.para(x, y, w, body, 9.5)
        return y

    def chip(self, x, y, label, fill=YELLOW, color=NAVY, size=9.5):
        w = pdfmetrics.stringWidth(label, "Segoe-Bold", size) + 7 * mm
        self.c.setFillColor(fill)
        self.c.roundRect(x, y - 2.2 * mm, w, 7 * mm, 3.5 * mm, fill=1, stroke=0)
        self.text(x + 3.5 * mm, y, label, size, "Segoe-Bold", color)
        return w


def fonts():
    pdfmetrics.registerFont(TTFont("Segoe", str(FONTS / "segoeui.ttf")))
    pdfmetrics.registerFont(TTFont("Segoe-Bold", str(FONTS / "segoeuib.ttf")))
    pdfmetrics.registerFont(TTFont("Segoe-Italic", str(FONTS / "segoeuii.ttf")))


def img(name):
    return SHOTS / f"{name}.png"


# ---------------------------------------------------------------- Seiten

def page_cover(p: Page, ver: str):
    c, W, H = p.c, p.W, p.H
    # Hintergrund: Verlauf Navy -> Blau (wie Fensterleiste und Store-Grafik)
    steps = 60
    for i in range(steps):
        t = i / (steps - 1)
        col = colors.Color(0.04 + 0.12 * t, 0.15 + 0.30 * t, 0.25 + 0.45 * t)
        c.setFillColor(col)
        c.rect(0, H * (1 - (i + 1) / steps), W, H / steps + 1, fill=1, stroke=0)
    logo = ROOT / "app" / "resources" / "icons" / "volta_256.png"
    c.drawImage(str(logo), 18 * mm, H - 40 * mm, 22 * mm, 22 * mm, mask="auto")
    p.text(45 * mm, H - 27 * mm, "Volta 1", 34, "Segoe-Bold", colors.white)
    p.text(45 * mm, H - 36 * mm, "Das Elektronik-Labor", 18, "Segoe-Bold", YELLOW)
    p.text(18 * mm, H - 52 * mm, "Bauteile stecken, Leitungen ziehen, einschalten – und sehen, was wirklich passiert.",
           13, "Segoe", colors.white)
    p.text(18 * mm, H - 59 * mm, "Ein virtueller Elektronik-Experimentierkasten für Kinder von 8 bis 12 Jahren.",
           11, "Segoe", colors.HexColor("#CFE3F7"))
    # Tablet mit laufendem Experiment
    framed = OUT / "bilder" / "_tablet_blinker.png"
    tablet(img("blinker"), framed)
    tw = W - 36 * mm
    iw, ih = PILImage.open(framed).size
    th = tw * ih / iw
    c.drawImage(str(framed), 18 * mm, H - 68 * mm - th, tw, th, mask="auto")
    # Tick winkt neben den Eckdaten (unter dem Tablet)
    tk = TICK / "tick_freundlich.png"
    c.drawImage(str(tk), W - 48 * mm, H - 68 * mm - th - 50 * mm, 36 * mm, 36 * mm, mask="auto")
    # Eckdaten als Kacheln
    y = H - 68 * mm - th - 14 * mm
    facts = [("61", "Experimente in 5 Stufen"), ("21 + 4", "Bauteile und Module"),
             ("4", "Sprachen: DE · FR · EN · LU"), ("0", "Daten, Werbung, Käufe – offline")]
    x = 18 * mm
    for big, small in facts:
        c.setFillColor(colors.Color(1, 1, 1, alpha=0.12))
        c.roundRect(x, y - 13 * mm, 33 * mm, 20 * mm, 3 * mm, fill=1, stroke=0)
        p.text(x + 16.5 * mm, y - 1 * mm, big, 18, "Segoe-Bold", YELLOW, "c")
        p.para(x + 2.5 * mm, y - 6.5 * mm, 28 * mm, small, 8, 9.5, "Segoe", colors.white)
        x += 36 * mm
    # Fuer wen und wo
    yb = y - 30 * mm
    p.text(18 * mm, yb, "Für Kinder, Eltern und Schulen", 12, "Segoe-Bold", colors.white)
    # Kacheln umbrechen, rechts bleibt Platz fuer Tick
    x, yc = 18 * mm, yb - 9 * mm
    for label in ("Lernpfad mit Forscherfrage", "Ehrliche Simulation", "Tick hilft", "Aufgaben per QR-Code",
                  "Vorführung für die Klasse"):
        wch = pdfmetrics.stringWidth(label, "Segoe-Bold", 9) + 7 * mm
        if x + wch > W - 52 * mm:
            x, yc = 18 * mm, yc - 9 * mm
        x += p.chip(x, yc, label, colors.Color(1, 1, 1, alpha=0.18), colors.white, 9) + 2.5 * mm
    p.text(18 * mm, yc - 12 * mm, "Android-Tablet und Windows · iPad, Mac und Linux folgen", 11, "Segoe-Bold", YELLOW)
    p.text(18 * mm, 14 * mm, f"Version {ver} (Testphase) · Oktober 2026 · DC Software Engineering, Irrel",
           8.5, "Segoe", colors.HexColor("#CFE3F7"))


def page_lab(p: Page):
    p.band("Forschen wie im echten Labor", "Echte Schaltungssimulation – nichts ist vorher festgelegt")
    W, H = p.W, p.H
    x1, x2, cw = 18 * mm, W / 2 + 4 * mm, W / 2 - 22 * mm
    y = H - 38 * mm
    yl = p.feature(x1, y, cw, "Stecken, ziehen, einschalten",
                   "Bauteile kommen aus dem Bauteilfach auf die Grundplatte; was im selben Kontaktfeld steckt, ist "
                   "verbunden. Leitungen zieht man mit dem Finger von Feld zu Feld. Mit »Betrieb« fließt Strom – "
                   "zwei Finger zoomen, »Rückgängig« holt jeden Schritt zurück.", img("aufbauhilfe"))
    yr = p.feature(x2, y, cw, "Physik, die ehrlich ist",
                   "Volta rechnet jede Schaltung aus, auch selbst erfundene: Lämpchen glühen heller oder dunkler, "
                   "Motoren drehen sich, Bauteile werden warm – und eine Leuchtdiode ohne Schutzwiderstand brennt "
                   "durch. Ein Tippen auf »Rückgängig« macht sie wieder heil.", img("blinker"))
    y = min(yl, yr) - 6 * mm
    yl = p.feature(x1, y, cw, "Der Lernpfad",
                   "61 Experimente in fünf Stufen, jedes mit einer Forscherfrage: Vermutung wählen, aufbauen, "
                   "beobachten, verstehen. Stufe für Stufe kommen neue Bauteile dazu – vom Lämpchen bis zum "
                   "Transistor, Relais und Zeitmodul.", img("start"))
    yr = p.feature(x2, y, cw, "Tick hilft",
                   "Das Maskottchen Tick stellt die Forscherfrage, zeigt Schritt für Schritt, wohin jedes Teil "
                   "gehört, und gibt Tipps, wenn es nicht weitergeht. Eine Kurzanleitung und eine Hilfe mit Tick "
                   "sind immer einen Fingertipp entfernt.", img("hilfe"))
    # Die fuenf Stufen
    y = min(yl, yr) - 8 * mm
    stages = [("0", "Auftakt", "Wie bringe ich etwas zum Blinken?", "1"),
              ("1", "Strom fließt im Kreis", "Wann leuchtet das Lämpchen?", "10"),
              ("2", "Mehrere Wege", "Was passiert, wenn ich ein zweites Lämpchen einbaue?", "20"),
              ("3", "Messen und verstehen", "Wie viel Strom fließt eigentlich?", "15"),
              ("4", "Kleine Elektronik", "Wie kann ein Strom einen anderen steuern?", "15")]
    c = p.c
    c.setFillColor(LIGHT)
    c.roundRect(x1, y - 50 * mm, W - 36 * mm, 52 * mm, 3 * mm, fill=1, stroke=0)
    p.text(x1 + 5 * mm, y - 6 * mm, "Die fünf Stufen", 12.5, "Segoe-Bold", BLUE)
    yy = y - 14 * mm
    for nr, title, q, n in stages:
        c.setFillColor(NAVY)
        c.circle(x1 + 9 * mm, yy + 1.3 * mm, 3.2 * mm, fill=1, stroke=0)
        p.text(x1 + 9 * mm, yy, nr, 10, "Segoe-Bold", colors.white, "c")
        p.text(x1 + 15 * mm, yy, title, 10, "Segoe-Bold")
        p.text(x1 + 62 * mm, yy, q, 9.5, "Segoe-Italic", GREY)
        p.text(W - 23 * mm, yy, f"{n} Exp.", 9.5, "Segoe-Bold", NAVY, "r")
        yy -= 7.5 * mm
    p.footer(2)


def page_measure(p: Page):
    p.band("Messen und verstehen", "Werkzeuge wie im Elektronik-Labor – kindgerecht erklärt")
    W, H = p.W, p.H
    x1, x2, cw = 18 * mm, W / 2 + 4 * mm, W / 2 - 22 * mm
    y = H - 38 * mm
    yl = p.feature(x1, y, cw, "Dreifach-Messgerät",
                   "Spannung, Strom und Widerstand – digital oder mit Zeiger, neben der Grundplatte oder oben im "
                   "Pult. Spannungsanzeigen an jedem Feld und laufende Pfeile zeigen, wohin der Strom fließt.",
                   img("messgeraet"))
    yr = p.feature(x2, y, cw, "Zeitschreiber",
                   "Zwei Kanäle zeichnen Spannungen über die Zeit auf: wie sich ein Kondensator füllt, wie ein "
                   "Blinker kippt, wie ein Sensor auf Licht oder Wärme reagiert.", img("zeitschreiber"))
    y = min(yl, yr) - 6 * mm
    yl = p.feature(x1, y, cw, "Schaltplan und »Blick hinein«",
                   "Jeder Aufbau erscheint auf Wunsch als Schaltplan mit Normsymbolen. Module wie das Blinkmodul "
                   "lassen sich öffnen: Innen arbeiten echte Transistoren – mit den Spannungen, die gerade "
                   "gerechnet werden.", img("blick_hinein"))
    yr = p.feature(x2, y, cw, "Werkzeugkasten und Fehlersuche",
                   "Farbcode, E-Reihen, Bauteillexikon, Formelrechner mit Rechenweg und eine Teileliste mit "
                   "Aufbaukarte zum Nachbauen. Die Fehlersuche findet Kurzschlüsse, offene Kreise und falsch "
                   "gepolte Bauteile – und fragt nach, statt zu verraten.", img("fehlersuche"))
    # Licht, Waerme, Wasser: Bild links, Text rechts
    y = min(yl, yr) - 8 * mm
    w, h = p.image(img("umwelt"), x1, y - cw * 1200 / 1920, cw)
    yy = y - 5 * mm
    p.text(x2, yy, "Licht, Wärme, Wasser", 12.5, "Segoe-Bold", BLUE)
    p.para(x2, yy - 6 * mm, cw,
           "Fotowiderstand, Heißleiter und Feuchtesonde reagieren auf ihre Umgebung: Taschenlampe, Föhn und "
           "Wassertropfen zieht man direkt auf das Bauteil, das Raumlicht stellt man auf hell, Dämmerung oder "
           "dunkel. So entstehen Nachtlicht, Hitzealarm und Wassermelder.", 9.5)
    p.footer(3)


def page_school(p: Page):
    p.band("Für Schule und Eltern", "Sicher, werbefrei, ohne Konto – und bereit für den Unterricht")
    W, H = p.W, p.H
    x1, x2, cw = 18 * mm, W / 2 + 4 * mm, W / 2 - 22 * mm
    y = H - 38 * mm
    yl = p.feature(x1, y, cw, "Aufgaben per QR-Code",
                   "Die Lehrkraft wählt Experimente oder gibt ihren Aufbau mit – Volta zeigt einen großen QR-Code "
                   "für den Beamer oder einen kurzen Code zum Eintippen. Keine Verbindung zwischen den Geräten, keine "
                   "Rückmeldung an einen Server.", img("aufgabe_qr"), img_h=(cw * 1200 / 1920))
    yr = p.feature(x2, y, cw, "Vorführung und Präsentation",
                   "Zehn fertige Aufbauten aus allen Stufen für die Klasse oder ein Gespräch, dazu ein "
                   "Präsentationsmodus für Beamer und Tafel. Beides läuft ohne Spuren: Fortschritt und Forscherheft "
                   "bleiben unberührt.", img("vorfuehrung"))
    y = min(yl, yr) - 6 * mm
    yl = p.feature(x1, y, cw, "Forscherheft",
                   "Vermutungen, Beobachtungen, eigene Notizen und Fotos vom echten Aufbau – das Forscherheft "
                   "wächst mit jedem Experiment. Mehrere Kinder an einem Gerät bekommen eigene Forscherprofile "
                   "(Symbol und Spitzname, kein Klarname).", img("forscherheft"))
    # Rechts: Liste statt Bild
    c = p.c
    box_h = (y - yl) + 2 * mm
    c.setFillColor(LIGHT)
    c.roundRect(x2, yl - 2 * mm, cw, box_h, 3 * mm, fill=1, stroke=0)
    yy = y - 6 * mm
    p.text(x2 + 5 * mm, yy, "Elternbereich und Datenschutz", 12.5, "Segoe-Bold", BLUE)
    yy -= 7 * mm
    items = [
        "Kein Konto, keine Werbung, keine Käufe in der App",
        "Keine Datenerhebung, keine Internetverbindung, keine Berechtigungen",
        "Elternbereich mit PIN: Freigaben, Profile, Ton im Klassenzimmer",
        "Verwaltete Geräte: Vorgaben der Schule (Sprache, Stufen, Ton)",
        "Bedienelemente ab 48 dp, Farbe nie allein, Vorlesen der Texte",
        "Sprachen Deutsch, Français, English, Lëtzebuergesch",
        "Sicherheit: nur Batterien bis 9 V, zerstörende Versuche »Nur in der App!«",
    ]
    for it in items:
        c.setFillColor(GREEN)
        c.circle(x2 + 6.5 * mm, yy + 1.2 * mm, 1.3 * mm, fill=1, stroke=0)
        yy = p.para(x2 + 10 * mm, yy, cw - 14 * mm, it, 9.5) - 1.8 * mm
    # Jetzt in echt
    y = min(yl, yy) - 8 * mm
    c.setFillColor(CREAM)
    c.roundRect(x1, y - 34 * mm, W - 36 * mm, 36 * mm, 3 * mm, fill=1, stroke=0)
    c.drawImage(str(TICK / "tick_erklaerend.png"), W - 50 * mm, y - 33 * mm, 30 * mm, 30 * mm, mask="auto")
    p.text(x1 + 5 * mm, y - 6 * mm, "Jetzt in echt", 12.5, "Segoe-Bold", BLUE)
    p.para(x1 + 5 * mm, y - 12.5 * mm, W - 80 * mm,
           "Zu jedem Experiment gibt es eine Teileliste und eine Aufbaukarte als PDF zum Ausdrucken – damit lässt "
           "sich der Versuch mit echten Bauteilen nachbauen (nur Batterien bis 9 Volt; zerstörende Versuche gibt "
           "es nur in der App). Dazu eine Kurzanleitung in der App sowie eine ausführliche Anleitung und eine "
           "Kurzanleitung als PDF.", 9.5)
    p.footer(4)


def device(c, x, y, kind, w, h):
    """Einfache Geraeteumrisse (keine Markenzeichen): Tablet, Laptop, Monitor."""
    c.setStrokeColor(NAVY)
    c.setLineWidth(1.4)
    c.setFillColor(colors.HexColor("#DCE6F2"))
    if kind == "tablet":
        c.roundRect(x, y, w, h, 3 * mm, fill=1, stroke=1)
        c.setFillColor(colors.white)
        c.rect(x + 2.5 * mm, y + 2.5 * mm, w - 5 * mm, h - 5 * mm, fill=1, stroke=0)
    elif kind == "laptop":
        c.roundRect(x + 3 * mm, y + 3 * mm, w - 6 * mm, h - 3 * mm, 1.5 * mm, fill=1, stroke=1)
        c.setFillColor(colors.white)
        c.rect(x + 5 * mm, y + 5 * mm, w - 10 * mm, h - 7 * mm, fill=1, stroke=0)
        c.setFillColor(colors.HexColor("#DCE6F2"))
        c.roundRect(x, y, w, 3 * mm, 1 * mm, fill=1, stroke=1)
    else:  # Monitor
        c.roundRect(x, y + 4 * mm, w, h - 4 * mm, 1.5 * mm, fill=1, stroke=1)
        c.setFillColor(colors.white)
        c.rect(x + 2 * mm, y + 6 * mm, w - 4 * mm, h - 8 * mm, fill=1, stroke=0)
        c.line(x + w / 2, y + 4 * mm, x + w / 2, y + 1 * mm)
        c.line(x + w / 2 - 6 * mm, y + 1 * mm, x + w / 2 + 6 * mm, y + 1 * mm)


def page_platforms(p: Page):
    p.band("Plattformen", "Ein Programm – auf Tablet und Computer")
    c, W, H = p.c, p.W, p.H
    rows = [
        ("tablet", "Android-Tablet", "Testversion verfügbar",
         "Android 8 und neuer, Tablets ab 10 Zoll empfohlen, Finger und Stift. 64 und 32 Bit (arm64, armv7), "
         "ohne Berechtigungen. Geplant über Google Play, auch für Schulen (verwaltete Geräte).", GREEN),
        ("monitor", "Windows", "Testversion verfügbar",
         "Windows 10 und 11 (64 Bit), Maus, Tastatur und Touchscreen. Eigene Fensterleiste im Volta-Design. "
         "Installationspaket (MSIX/MSI) und Microsoft Store in Vorbereitung.", GREEN),
        ("tablet", "iPad", "in Vorbereitung",
         "iPadOS, Finger und Stift gleichwertig, alle Ausrichtungen. Paketierung vorbereitet; Veröffentlichung "
         "im App Store (Kinderkategorie, Apple School Manager) folgt.", ORANGE),
        ("laptop", "Mac", "geplant",
         "macOS auf Apple Silicon aus demselben Programmcode, mit Maus, Trackpad und Tastatur.", BLUE),
        ("monitor", "Linux", "geplant",
         "Linux 64 Bit als AppImage, für Schulrechner und Tafel-PCs; verwaltete Einstellungen über "
         "/etc/xdg.", BLUE),
    ]
    y = H - 42 * mm
    for kind, name, status, text, col in rows:
        c.setFillColor(LIGHT)
        c.roundRect(18 * mm, y - 30 * mm, W - 36 * mm, 32 * mm, 3 * mm, fill=1, stroke=0)
        device(c, 23 * mm, y - 25 * mm, kind, 30 * mm, 21 * mm)
        p.text(60 * mm, y - 6 * mm, name, 15, "Segoe-Bold", NAVY)
        p.chip(60 * mm + pdfmetrics.stringWidth(name, "Segoe-Bold", 15) + 4 * mm, y - 6 * mm, status, col,
               colors.white, 8.5)
        p.para(60 * mm, y - 13 * mm, W - 84 * mm, text, 9.5)
        y -= 36 * mm
    p.para(18 * mm, y - 1 * mm, W - 36 * mm,
           "Auf allen Plattformen gleich: dieselben Experimente, dieselbe ehrliche Simulation, Bedienung mit Finger, "
           "Stift oder Maus, kein Netz und keine Datenerhebung. Fortschritt, Forscherheft und eigene Aufbauten "
           "bleiben auf dem Gerät und lassen sich als Datei mitnehmen.", 9.5)
    p.footer(5)


def page_series(p: Page, ver: str):
    p.band("Die Reihe und der Weg dorthin", "Volta wächst mit – Zuse führt in die Computertechnik")
    c, W, H = p.c, p.W, p.H
    series = [
        ("Volta 1", "8–12 Jahre", "Stromkreis, Schalter, Leuchtdioden, Messen, Sensoren, Transistor, Module",
         "Testphase 0.9", NAVY),
        ("Volta 2", "11–14 Jahre", "Oszilloskop, Funktionsgenerator, hörbare Simulation, Operationsverstärker, "
         "Timer-Baustein 555", "Planung", colors.HexColor("#0F6E7A")),
        ("Volta 3", "13–16 Jahre", "Schwingkreis und Radio, Stromversorgung, Messverstärker, digitale "
         "Grundschaltungen", "Planung", colors.HexColor("#5B2A6E")),
        ("Zuse 1", "12–16 Jahre", "Vom Bit über Gatter, Rechenwerk und Speicher zum selbst gebauten "
         "4-Bit-Rechner – und ihn programmieren", "Konzept", colors.HexColor("#0E7490")),
    ]
    y = H - 42 * mm
    for name, age, text, status, col in series:
        c.setFillColor(col)
        c.roundRect(18 * mm, y - 22 * mm, 38 * mm, 24 * mm, 3 * mm, fill=1, stroke=0)
        p.text(37 * mm, y - 8 * mm, name, 15, "Segoe-Bold", colors.white, "c")
        p.text(37 * mm, y - 15 * mm, age, 9, "Segoe", colors.white, "c")
        p.chip(62 * mm, y - 4 * mm, status, LIGHT, NAVY, 8.5)
        p.para(62 * mm, y - 12 * mm, W - 82 * mm, text, 10)
        y -= 30 * mm
    # Kontakt
    y -= 4 * mm
    c.setFillColor(CREAM)
    c.roundRect(18 * mm, y - 52 * mm, W - 36 * mm, 54 * mm, 3 * mm, fill=1, stroke=0)
    tk = TICK / "tick_motivierend.png"
    c.drawImage(str(tk), W - 60 * mm, y - 50 * mm, 38 * mm, 38 * mm, mask="auto")
    p.text(24 * mm, y - 8 * mm, "Kontakt", 14, "Segoe-Bold", BLUE)
    yy = y - 16 * mm
    for line in ("DC Software Engineering · Daniel Zeimentz",
                 "Aufm Rothenhügel 26 · 54666 Irrel · Deutschland",
                 "info@dc-software-engineering.com",
                 "dc-software-engineering.com"):
        p.text(24 * mm, yy, line, 11)
        yy -= 6.5 * mm
    p.para(24 * mm, yy - 2 * mm, W - 90 * mm,
           f"Stand: Version {ver}, Testphase. Erprobung mit Kindern und Schulen läuft; Veröffentlichung zuerst für "
           "Android-Tablets und Windows. Gerne zeigen wir Volta in einer Vorführung.", 9.5, color=GREY)
    p.footer(6)


def main() -> int:
    take_screenshots("--bilder" in sys.argv)
    missing = [n for n in SCENES if not img(n).exists()]
    if missing:
        print("Bilder fehlen:", ", ".join(missing))
        return 1
    fonts()
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / "Volta1-Produktflyer-de.pdf"
    c = pdfcanvas.Canvas(str(path), pagesize=A4)
    c.setTitle("Volta 1 – Das Elektronik-Labor · Produktinformation")
    c.setAuthor("DC Software Engineering")
    ver = version()
    for draw in (lambda p: page_cover(p, ver), page_lab, page_measure, page_school, page_platforms,
                 lambda p: page_series(p, ver)):
        draw(Page(c))
        c.showPage()
    c.save()
    print("Flyer:", path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
