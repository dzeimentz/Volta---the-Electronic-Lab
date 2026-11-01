#!/usr/bin/env python3
"""Anleitungen als PDF fuer Kinder, Eltern und Lehrkraefte (deutsch):

    python3 tools/manual_pdfs.py            # beide PDFs nach Release/Anleitung/
    python3 tools/manual_pdfs.py --bilder   # vorher die Bildschirmfotos neu aufnehmen
    python3 tools/manual_pdfs.py --app-bilder   # nur die Schrittbilder der Kurzanleitung (App und PDF)
    python3 tools/manual_pdfs.py --app-bilder-volta2   # Kapitelbilder der Hilfe ab Volta 2 (aus volta2.exe)

- Volta1-Anleitung-de.pdf: ausfuehrliche Anleitung (alle Bereiche der App, Einstellungen, Sicherheit, Bauteile)
- Volta1-Kurzanleitung-de.pdf: drei Seiten zum Ausdrucken (acht Schritte mit Bildern, Bedienung auf einen Blick)

Bilder: Bildschirmfotos der Desktop-Ausgabe (build/desktop-debug/app/volta1.exe, 1280 x 800) ueber die
Entwicklerhilfen VOLTA_SCREENSHOT/VOLTA_DEMO_*; sie liegen in Release/Anleitung/bilder/de/ und werden nur mit
--bilder (oder wenn sie fehlen) neu erzeugt. Tick aus app/resources/tick/. Benoetigt reportlab, Pillow und Segoe UI.
"""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

from PIL import Image as PILImage, ImageDraw, ImageFont
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, Frame, Image, KeepTogether, NextPageTemplate,
                                PageBreak, PageTemplate, Paragraph, Spacer, Table, TableStyle)
from reportlab.platypus.tableofcontents import TableOfContents

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "Release" / "Anleitung"
SHOTS = OUT / "bilder" / "de"
EXE = ROOT / "build" / "desktop-debug" / "app" / ("volta1.exe" if os.name == "nt" else "volta1")
TICK = ROOT / "app" / "resources" / "tick"
FONTS = Path("C:/Windows/Fonts")


def _version() -> str:
    import re
    m = re.search(r"project\(Volta\s+VERSION\s+([0-9.]+)", (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"))
    return m.group(1) if m else "0"


VERSION = _version()

NAVY = colors.HexColor("#0A2540")
BLUE = colors.HexColor("#1E5FC2")
YELLOW = colors.HexColor("#FFD23F")
CREAM = colors.HexColor("#FFF8E6")
WARN = colors.HexColor("#FDECEA")
GREY = colors.HexColor("#5A6573")

# Bildschirmfotos: Name -> Entwicklerhilfen
SCENES = {
    "start": {},
    "profile": {"VOLTA_DEMO_PROFILES": "1"},
    "freies_labor": {"VOLTA_DEMO_FREELAB": "1"},
    "aufbauhilfe": {"VOLTA_DEMO_GUIDE": "1", "VOLTA_DEMO_EXP": "volta1.1.1", "VOLTA_SCREENSHOT_S": "4"},
    "geloest": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.1.1", "VOLTA_SCREENSHOT_S": "5"},
    "messgeraet": {"VOLTA_DEMO_METER": "v"},
    "schaltplan": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.2.4", "VOLTA_DEMO_PLAN": "1",
                   "VOLTA_SCREENSHOT_S": "5"},
    "zeitschreiber": {"VOLTA_DEMO_RECORDER": "1", "VOLTA_SCREENSHOT_S": "6"},
    "blick_hinein": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.0.1", "VOLTA_DEMO_INSIDE": "BL1",
                     "VOLTA_SCREENSHOT_S": "6"},
    "umwelt": {"VOLTA_DEMO_SENSORS": "1", "VOLTA_DEMO_FLYOUT": "1", "VOLTA_SCREENSHOT_S": "4"},
    "farbcode": {"VOLTA_DEMO_TOOLBOX": "0", "VOLTA_SCREENSHOT_S": "4"},
    "fehlersuche": {"VOLTA_DEMO_TOOLBOX": "4", "VOLTA_SCREENSHOT_S": "4"},
    "teileliste": {"VOLTA_DEMO_TOOLBOX": "5", "VOLTA_DEMO_EXP": "volta1.1.1", "VOLTA_SCREENSHOT_S": "4"},
    "forscherheft": {"VOLTA_DEMO_JOURNAL": "1"},
    "hilfe": {"VOLTA_DEMO_HELP": "1"},
    "pin": {"VOLTA_DEMO_PARENT": "gate"},
    "elternbereich": {"VOLTA_DEMO_PARENT": "area"},
    "aufgabe_qr": {"VOLTA_DEMO_TASK": "qr"},
    "spannung_auto": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.1.1", "VOLTA_DEMO_VOLTAGE_AUTO": "1",
                      "VOLTA_SCREENSHOT_S": "5"},
    "pultinfo": {"VOLTA_DEMO_FREELAB": "1", "VOLTA_DEMO_DESKINFO": "p1", "VOLTA_SCREENSHOT_S": "4"},
}


def take_screenshots(force: bool):
    SHOTS.mkdir(parents=True, exist_ok=True)
    for name, extra in SCENES.items():
        target = SHOTS / f"{name}.png"
        if target.exists() and not force:
            continue
        env = dict(os.environ)
        env.update({"VOLTA_NO_SPLASH": "1", "VOLTA_DEMO_LANG": "de", "VOLTA_SCREENSHOT": str(target)})
        env.update(extra)
        subprocess.run([str(EXE)], env=env, timeout=60, check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print(f"  Bild {name}: {'ok' if target.exists() else 'FEHLT'}")


# ---------------------------------------------------------------- Beschriftete Bilder (Nummern im Bild)

# Labor mit geloestem Experiment 1.1 (1280 x 800): Nummer -> Bildpunkt (rechts oben am jeweiligen Element)
LAB_MARKS = [
    (1, (66, 12)), (2, (282, 12)), (3, (412, 12)), (4, (500, 12)), (5, (577, 12)), (6, (640, 12)),
    (7, (705, 12)), (8, (770, 12)), (9, (830, 12)), (10, (890, 12)), (11, (990, 28)), (12, (195, 140)),
    (13, (905, 95)), (14, (870, 215)), (15, (640, 480)), (16, (918, 330)), (17, (875, 465)), (18, (1255, 130)),
    (19, (1255, 712)), (20, (700, 760)),
]


def annotated(src: Path, dst: Path, marks, scale_from=(1280, 800)):
    """Setzt nummerierte gelbe Kreise; Koordinaten im Bezugsbild scale_from."""
    img = PILImage.open(src).convert("RGB")
    sx, sy = img.width / scale_from[0], img.height / scale_from[1]
    d = ImageDraw.Draw(img)
    f = ImageFont.truetype(str(FONTS / "segoeuib.ttf"), 17)
    r = 14
    for n, (x, y) in marks:
        cx, cy = x * sx, y * sy
        d.ellipse((cx - r, cy - r, cx + r, cy + r), fill=(255, 210, 63), outline=(10, 37, 64), width=3)
        d.text((cx, cy), str(n), font=f, fill=(10, 37, 64), anchor="mm")
    img.save(dst)


# ---------------------------------------------------------------- Satz

def fonts():
    pdfmetrics.registerFont(TTFont("Segoe", str(FONTS / "segoeui.ttf")))
    pdfmetrics.registerFont(TTFont("Segoe-Bold", str(FONTS / "segoeuib.ttf")))
    pdfmetrics.registerFont(TTFont("Segoe-Italic", str(FONTS / "segoeuii.ttf")))
    pdfmetrics.registerFont(TTFont("Segoe-Symbol", str(FONTS / "seguisym.ttf")))
    from reportlab.pdfbase.pdfmetrics import registerFontFamily
    registerFontFamily("Segoe", normal="Segoe", bold="Segoe-Bold", italic="Segoe-Italic", boldItalic="Segoe-Bold")


def styles():
    s = {}
    s["body"] = ParagraphStyle("body", fontName="Segoe", fontSize=10.5, leading=15, spaceAfter=5, textColor=NAVY)
    s["small"] = ParagraphStyle("small", parent=s["body"], fontSize=9, leading=12, textColor=GREY)
    s["caption"] = ParagraphStyle("caption", parent=s["small"], alignment=TA_CENTER, spaceBefore=2, spaceAfter=10)
    s["h1"] = ParagraphStyle("h1", fontName="Segoe-Bold", fontSize=20, leading=25, textColor=NAVY, spaceBefore=4,
                             spaceAfter=10)
    s["h2"] = ParagraphStyle("h2", fontName="Segoe-Bold", fontSize=13.5, leading=18, textColor=BLUE, spaceBefore=10,
                             spaceAfter=4, keepWithNext=1)
    s["h1plain"] = ParagraphStyle("h1plain", parent=s["h1"]) # wie h1, aber nicht im Inhaltsverzeichnis
    s["bullet"] = ParagraphStyle("bullet", parent=s["body"], leftIndent=14, bulletIndent=3, spaceAfter=3)
    s["cell"] = ParagraphStyle("cell", parent=s["body"], fontSize=9.5, leading=13, spaceAfter=0)
    s["cellb"] = ParagraphStyle("cellb", parent=s["cell"], fontName="Segoe-Bold")
    s["toc1"] = ParagraphStyle("toc1", fontName="Segoe", fontSize=11, leading=18, textColor=NAVY, leftIndent=8)
    s["title"] = ParagraphStyle("title", fontName="Segoe-Bold", fontSize=34, leading=40, textColor=NAVY,
                                alignment=TA_CENTER)
    s["subtitle"] = ParagraphStyle("subtitle", fontName="Segoe-Bold", fontSize=17, leading=22,
                                   textColor=colors.HexColor("#C98A00"), alignment=TA_CENTER)
    s["center"] = ParagraphStyle("center", parent=s["body"], alignment=TA_CENTER)
    return s


S: dict = {}


def P(text, style="body"):
    return Paragraph(text, S[style])


def bullets(items, style="bullet"):
    return [Paragraph(t, S[style], bulletText="•") for t in items]


def shot(name, width_mm=165, caption=None, crop=None):
    path = SHOTS / f"{name}.png"
    if crop:  # (links, oben, rechts, unten) in Bildpunkten
        cropped = OUT / "bilder" / f"_{name}_{'_'.join(map(str, crop))}.png"
        PILImage.open(path).crop(crop).save(cropped)
        path = cropped
    w, h = PILImage.open(path).size
    img = Image(str(path), width=width_mm * mm, height=width_mm * mm * h / w)
    img.hAlign = "CENTER"
    parts = [_frame(img)]
    if caption:
        parts.append(P(caption, "caption"))
    else:
        parts.append(Spacer(1, 8))
    return KeepTogether(parts)


def _frame(img):
    t = Table([[img]], style=TableStyle([("BOX", (0, 0), (-1, -1), 0.6, colors.HexColor("#B8C2CC")),
                                         ("LEFTPADDING", (0, 0), (-1, -1), 0), ("RIGHTPADDING", (0, 0), (-1, -1), 0),
                                         ("TOPPADDING", (0, 0), (-1, -1), 0), ("BOTTOMPADDING", (0, 0), (-1, -1), 0)]))
    t.hAlign = "CENTER"
    return t


def tick_box(mood, text, width_mm=170, bg=CREAM, border=colors.HexColor("#E8C766")):
    """Tick mit Sprechtext (Merke-Kasten)."""
    pic = TICK / f"tick_{mood}.png"
    w, h = PILImage.open(pic).size
    img = Image(str(pic), width=20 * mm, height=20 * mm * h / w, mask="auto")
    t = Table([[img, P(text)]], colWidths=[24 * mm, (width_mm - 24) * mm])
    t.setStyle(TableStyle([("BACKGROUND", (0, 0), (-1, -1), bg), ("BOX", (0, 0), (-1, -1), 1, border),
                           ("VALIGN", (0, 0), (-1, -1), "MIDDLE"), ("LEFTPADDING", (0, 0), (-1, -1), 6),
                           ("RIGHTPADDING", (0, 0), (-1, -1), 8), ("TOPPADDING", (0, 0), (-1, -1), 5),
                           ("BOTTOMPADDING", (0, 0), (-1, -1), 5)]))
    return KeepTogether([t, Spacer(1, 8)])


def table(rows, widths_mm, head=True):
    data = [[P(c, "cellb" if (head and i == 0) or j == 0 else "cell") for j, c in enumerate(r)]
            for i, r in enumerate(rows)]
    t = Table(data, colWidths=[w * mm for w in widths_mm], repeatRows=1 if head else 0)
    st = [("GRID", (0, 0), (-1, -1), 0.5, colors.HexColor("#C9D1DA")), ("VALIGN", (0, 0), (-1, -1), "TOP"),
          ("LEFTPADDING", (0, 0), (-1, -1), 5), ("RIGHTPADDING", (0, 0), (-1, -1), 5),
          ("TOPPADDING", (0, 0), (-1, -1), 3), ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]
    if head:
        st.append(("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#E3ECF7")))
    t.setStyle(TableStyle(st))
    return KeepTogether([t, Spacer(1, 8)]) if len(rows) < 14 else t


class Doc(BaseDocTemplate):
    def __init__(self, path, title, short=False):
        super().__init__(str(path), pagesize=A4, leftMargin=20 * mm, rightMargin=20 * mm, topMargin=18 * mm,
                         bottomMargin=18 * mm, title=title, author="DC Software Engineering",
                         subject="Volta 1 – Das Elektronik-Labor")
        self.doc_title = title
        frame = Frame(self.leftMargin, self.bottomMargin, self.width, self.height, id="f")
        self.addPageTemplates([PageTemplate("cover", [frame], onPage=lambda c, d: None),
                               PageTemplate("page", [frame], onPage=self.decorate)])

    def decorate(self, c, d):
        c.saveState()
        c.setFillColor(NAVY)
        c.rect(0, A4[1] - 9 * mm, A4[0], 9 * mm, fill=1, stroke=0)
        c.setFillColor(colors.white)
        c.setFont("Segoe-Bold", 9)
        c.drawString(20 * mm, A4[1] - 6 * mm, "Volta 1 – Das Elektronik-Labor")
        c.setFont("Segoe", 9)
        c.drawRightString(A4[0] - 20 * mm, A4[1] - 6 * mm, self.doc_title)
        c.setFillColor(GREY)
        c.setFont("Segoe", 8.5)
        c.drawString(20 * mm, 10 * mm, "DC Software Engineering · dc-software-engineering.com")
        c.drawRightString(A4[0] - 20 * mm, 10 * mm, f"Seite {d.page}")
        c.restoreState()

    def afterFlowable(self, flowable):
        if isinstance(flowable, Paragraph) and flowable.style.name == "h1":
            text = flowable.getPlainText()
            key = "h1-" + text.split()[0] # Kapitelnummer bzw. erstes Wort: gleich in jedem Durchlauf
            self.canv.bookmarkPage(key)
            self.canv.addOutlineEntry(text, key, level=0)
            self.notify("TOCEntry", (0, text, self.page, key))


def chapter(title):
    """Jedes Kapitel beginnt auf einer neuen Seite."""
    return [PageBreak(), P(title, "h1")]


def cover(title, subtitle, mood="freundlich"):
    pic = TICK / f"tick_{mood}.png"
    w, h = PILImage.open(pic).size
    tick = Image(str(pic), width=75 * mm, height=75 * mm * h / w, mask="auto")
    logo = Image(str(ROOT / "app" / "resources" / "icons" / "volta_256.png"), width=28 * mm, height=28 * mm,
                 mask="auto")
    return [Spacer(1, 18 * mm), logo, Spacer(1, 8 * mm), P("Volta 1", "title"), Spacer(1, 2 * mm),
            P("Das Elektronik-Labor", "subtitle"), Spacer(1, 10 * mm), tick, Spacer(1, 10 * mm),
            P(f"<font size=20><b>{title}</b></font>", "center"), Spacer(1, 3 * mm), P(subtitle, "center"),
            Spacer(1, 18 * mm),
            P(f"Version {VERSION} · Oktober 2026 · für Android-Tablets und Windows<br/>"
              "DC Software Engineering, Daniel Zeimentz, Aufm Rothenhügel 26, 54666 Irrel · info@dc-software-engineering.com", "small")]


# ---------------------------------------------------------------- Ausfuehrliche Anleitung

def long_manual(path):
    story = cover("Anleitung", "für Forscherinnen und Forscher von 8 bis 12 Jahren – und für Eltern und Lehrkräfte")
    story[-1].style = S["caption"]
    toc = TableOfContents()
    toc.levelStyles = [S["toc1"]]
    story += [NextPageTemplate("page"), PageBreak(), P("Inhalt", "h1plain"), toc]

    # 1 Willkommen
    story += chapter("1  Willkommen bei Volta")
    story += [P("Volta ist ein Elektronik-Labor auf dem Tablet oder Computer. Du steckst Bauteile auf eine "
                "Grundplatte, verbindest sie mit Leitungen und schaltest ein. Was dann passiert, rechnet Volta "
                "wirklich aus – wie in einer echten Schaltung: Lämpchen leuchten, Motoren drehen sich, Summer "
                "brummen. Und wenn zu viel Strom fließt, brennt auch einmal etwas durch. Keine Sorge: Mit "
                "»Rückgängig« ist alles sofort wieder heil."),
              tick_box("freundlich", "<b>Hallo, ich bin Tick!</b> Ich begleite dich durch die Experimente, stelle "
                       "dir Forscherfragen, gebe Tipps und helfe dir, wenn etwas nicht klappt."),
              P("Was du in Volta findest", "h2")]
    story += bullets(["<b>Lernpfad</b> – 61 Experimente in fünf Stufen. Jedes beginnt mit einer Forscherfrage. "
                      "Stufe für Stufe kommen neue Bauteile dazu.",
                      "<b>Freies Labor</b> – hier baust du, was du willst, mit allen freigeschalteten Bauteilen.",
                      "<b>Forscherheft</b> – deine Vermutungen, Beobachtungen, Notizen und Fotos.",
                      "<b>Werkzeugkasten</b> – Farbcode, E-Reihen, Bauteillexikon, Formelrechner, Fehlersuche "
                      "und Teileliste mit Aufbaukarte zum Ausdrucken.",
                      "<b>Messgerät, Schaltplan und Zeitschreiber</b> – damit siehst du, was in der Schaltung "
                      "vorgeht."])
    story += [P("Was du brauchst", "h2"),
              P("Ein Android-Tablet (ab etwa 10 Zoll empfohlen) oder einen Windows-Computer. Volta braucht kein "
                "Internet, kein Konto und keine Anmeldung. Alles, was du machst, bleibt auf deinem Gerät.")]

    # 2 Erste Schritte
    story += chapter("2  Erste Schritte")
    story += [P("Nach dem Start siehst du die Startseite. Oben begrüßt dich Tick, darunter liegen die Bereiche "
                "von Volta und der Lernpfad."),
              shot("start", caption="Die Startseite: Aufgabe der Lehrkraft, freies Labor, Forscherheft und der "
                                    "Lernpfad mit seinen Stufen"),
              P("So geht es los", "h2")]
    story += bullets(["Tippe auf <b>»Zeig mir, wie Volta funktioniert«</b> (das rote Fragezeichen). Tick erklärt "
                      "dir in kurzen Kapiteln alles Wichtige.",
                      "Tippe im Lernpfad auf das erste Experiment <b>»Dein erstes Blinklicht«</b> (Stufe 0).",
                      "Später bringt dich <b>»Weitermachen«</b> direkt zu deinem letzten Experiment."])
    story += [P("Forscherprofile", "h2"),
              P("Teilen sich mehrere Kinder ein Gerät, können Eltern oder Lehrkräfte in den Einstellungen "
                "Forscherprofile einschalten. Dann fragt Volta beim Start: »Wer forscht heute?« Jedes Kind wählt "
                "sein Symbol und hat seinen eigenen Fortschritt, sein Forscherheft und seinen Aufbau."),
              shot("profile", 120, "Profilauswahl mit drei Forscherprofilen", crop=(0, 0, 1280, 420))]

    # 3 Der Bildschirm im Labor
    story += chapter("3  Der Bildschirm im Labor")
    annotated(SHOTS / "geloest.png", OUT / "bilder" / "_labor_nummern.png", LAB_MARKS)
    w, h = PILImage.open(OUT / "bilder" / "_labor_nummern.png").size
    story += [_frame(Image(str(OUT / "bilder" / "_labor_nummern.png"), width=170 * mm, height=170 * mm * h / w)),
              P("Das Labor mit einem Experiment aus Stufe 1", "caption"),
              table([["Nr.", "Was", "Wozu"],
                     ["1", "Haus", "zurück zur Startseite"],
                     ["2", "Experiment wählen", "zeigt die Stufe und öffnet die Liste aller Experimente"],
                     ["3", "Stecken / Betrieb", "»Stecken«: aufbauen und umbauen. »Betrieb«: der Hauptschalter "
                      "ist an, Strom fließt, Taster und Schalter lassen sich bedienen."],
                     ["4", "Pfeile", "Rückgängig und Wiederholen"],
                     ["5", "Werkzeugkasten", "Farbcode, E-Reihen, Lexikon, Rechner, Fehlersuche, Teileliste"],
                     ["6", "Grüne Balken", "Spannungsanzeige auf allen belegten Feldern ein- und ausschalten"],
                     ["7", "V A Ω", "Messgerät ein- und ausblenden"],
                     ["8", "Schaltzeichen", "Schaltplan des Aufbaus zeigen"],
                     ["9", "Kurve", "Zeitschreiber (ab Stufe 3)"],
                     ["10", "Farbpunkt", "Farbe für neue Leitungen"],
                     ["11", "A− A+, Flagge, ?, i, Menü", "Schriftgröße, Sprache, Hilfe, Info, Einstellungen"],
                     ["12", "Bauteilfach / Modulfach", "Bauteile und Module zum Herausziehen"],
                     ["13", "Bedienpult", "Batterie mit Hauptschalter und Spannungswahl, Drehregler P1 und P2, "
                      "Schalter S, Lautsprecher"],
                     ["14", "Grundplatte", "Kontaktfelder A1 bis E12 – hier wird gesteckt"],
                     ["15", "Modulleiste", "Plätze M1 bis M16 für Module (z. B. das Blinkmodul)"],
                     ["16", "+ und −", "mehr oder weniger Felder je Reihe"],
                     ["17", "Mülleimer", "Ausgewähltes entfernen oder alles abbauen"],
                     ["18", "Experimentfeld", "Frage, Vermutung, Aufbau, Tipps – einklappbar mit »"],
                     ["19", "Fertig", "wenn die Schaltung funktioniert"],
                     ["20", "Licht, Wärme, Wasser", "Taschenlampe, Föhn, Wassertropfen und Raumlicht für "
                      "Sensoren (ab Stufe 3)"]], [12, 48, 110])]
    story += [P("Die Grundplatte", "h2"),
              P("Jedes Kontaktfeld hat fünf Steckplätze. <b>Alles, was im selben Feld steckt, ist miteinander "
                "verbunden.</b> Die Felder heißen nach Reihe und Spalte: A1, A2 … bis E12. Über der Grundplatte "
                "liegt die Pultleiste mit den Anschlüssen des Bedienpults:"),
              table([["Feld", "führt zu"],
                     ["V+ und 0V", "Plus- und Minuspol der Batterie (oben an der Batterie wählst du 3 V, 4,5 V "
                      "oder 9 V)"],
                     ["P1a, P1s, P1b / P2a, P2s, P2b", "Drehregler P1 und P2: die beiden Enden (a, b) und der "
                      "Schleifer (s)"],
                     ["S1, S2", "Schalter S"],
                     ["LS1, LS2", "Lautsprecher"]], [55, 115]),
              P("Mit den runden Tasten <b>+</b> und <b>−</b> rechts neben der Grundplatte wählst du 8, 10 oder "
                "12 Felder je Reihe. Ist im Menü »Steckfeld-Breite automatisch« eingeschaltet, passt Volta die "
                "Breite selbst an: im Lernpfad 8, im freien Labor 12 Felder; mit dem Messgerät daneben 10, mit dem "
                "Messgerät im Pult 12."),
              P("Das »?« am Pult", "h2"),
              P("Batterie, Drehregler P1 und P2, Schalter und Lautsprecher haben oben rechts in ihrem Rahmen ein "
                "kleines <b>»?«</b>. Tippe darauf: Volta erklärt, was das Bauteil macht, was jedes seiner Felder "
                "bedeutet und wie man es anschließt – zum Beispiel den Drehregler als Spannungsteiler."),
              shot("pultinfo", 120, "Das »?« am Drehregler P1 erklärt das Bauteil und seine drei Felder")]

    # 4 Aufbauen
    story += chapter("4  Aufbauen: Bauteile und Leitungen")
    story += [P("Bauteile stecken", "h2")]
    story += bullets(["Ziehe ein Bauteil aus dem <b>Bauteilfach</b> links auf die Grundplatte. Seine Beine landen "
                      "in Kontaktfeldern; die blinkenden Felder zeigen beim Ziehen, wohin es kommt.",
                      "Ein gestecktes Bauteil verschiebst du, indem du es ziehst.",
                      "<b>Zweimal schnell tippen</b> (am Computer: Doppelklick) dreht ein Bauteil.",
                      "<b>Lange drücken</b> (am Computer: Rechtsklick) öffnet das Bauteilmenü: Drehen, kurz oder "
                      "lang stecken, Wert oder Farbe wählen, Polung umdrehen, Blick hinein, Entfernen.",
                      "<b>Module</b> wie das Blinkmodul liegen im <b>Modulfach</b> (Reiter oben im Fach). Sie "
                      "gehören in die Modulleiste unter der Grundplatte."])
    story += [P("Leitungen ziehen", "h2")]
    story += bullets(["Ziehe mit dem Finger von einem Feld zu einem anderen – schon liegt dort eine Leitung.",
                      "Leitungen an Plus werden automatisch rot, an Minus schwarz. Für andere Leitungen wählst du "
                      "die Farbe mit dem Farbpunkt oben (10).",
                      "Ein Leitungsende kannst du anfassen und in ein anderes Feld umstecken.",
                      "Lange auf eine Leitung drücken: Farbe ändern oder Leitung entfernen."])
    story += [P("Rückgängig, Löschen, Abbauen", "h2")]
    story += bullets(["Der gebogene Pfeil nach links macht den letzten Schritt rückgängig, der Pfeil nach rechts "
                      "holt ihn zurück. Das gilt für jedes Stecken, Verschieben, Drehen und Löschen.",
                      "Auch ein durchgebranntes Bauteil wird mit »Rückgängig« wieder heil.",
                      "Der <b>Mülleimer</b> entfernt, was ausgewählt ist. Du kannst Bauteile auch auf ihn ziehen. "
                      "Ist nichts ausgewählt, baut er nach einer Rückfrage alles ab."])
    story += [P("Stecken und Betrieb", "h2"),
              P("Oben links wechselst du zwischen <b>»Stecken«</b> und <b>»Betrieb«</b>. »Betrieb« schaltet den "
                "Hauptschalter der Batterie ein: Jetzt fließt Strom, und du kannst Taster, Schalter und Regler "
                "bedienen. Zum Umbauen geht es zurück auf »Stecken«. Den Hauptschalter kannst du auch direkt an "
                "der Batterie im Bedienpult umlegen."),
              tick_box("erklaerend", "<b>Merke:</b> Strom braucht einen geschlossenen Weg – von V+ durch die "
                       "Bauteile zurück nach 0V. Ist der Weg irgendwo unterbrochen, passiert nichts.")]

    # 5 Experiment
    story += chapter("5  Ein Experiment durchführen")
    story += [P("Jedes Experiment im Lernpfad läuft in denselben Schritten ab. Das Experimentfeld rechts führt "
                "dich hindurch."),
              table([["Schritt", "Was du tust"],
                     ["1  Frage", "Lies die Forscherfrage."],
                     ["2  Vermutung", "Wähle, was du glaubst. Es gibt kein Falsch – Forscher vermuten erst und "
                      "prüfen dann. Deine Vermutung kommt ins Forscherheft."],
                     ["3  Aufbau", "Baue nach der Anleitung auf. <b>»Schritt für Schritt bauen«</b> zeigt dir jedes "
                      "Teil einzeln: Die Zielfelder blinken blau. Kommst du nicht weiter, hilft <b>»Ich brauche "
                      "einen Tipp«</b>."],
                     ["4  Ausprobieren", "Schalte ein (»Betrieb«) und probiere aus. Wenn alles steckt und "
                      "funktioniert, sagt Tick Bescheid."],
                     ["5  Fertig", "Tippe auf <b>»Fertig«</b> und wähle, was du beobachtet hast."],
                     ["6  Erklärung", "Tick erklärt, was passiert ist. Unter <b>»Mehr wissen«</b> steht mehr dazu, "
                      "unter <b>»Jetzt in echt«</b>, wie du es mit echten Bauteilen nachbauen kannst. "
                      "<b>»Weiterforschen«</b> gibt dir eine Idee für einen eigenen Versuch."]], [35, 135]),
              shot("aufbauhilfe", caption="Schritt für Schritt bauen: Tick zeigt das nächste Teil, die Zielfelder "
                                          "A1 und A2 sind blau markiert"),
              shot("geloest", caption="Geschafft: Das Lämpchen leuchtet, die kleinen Pfeile zeigen, wohin der "
                                      "Strom fließt"),
              P("Weitere Hilfen im Experimentfeld", "h2")]
    story += bullets(["Die <b>Lautsprecher-Taste</b> oben rechts im Experimentfeld liest dir die Texte vor.",
                      "Mit <b>»</b> klappst du das Experimentfeld ein und hast mehr Platz zum Bauen.",
                      "Experimente mit dem Schild <b>»Nur in der App!«</b> machen absichtlich ein Bauteil kaputt. "
                      "Die probierst du nur in Volta aus – nie mit echten Bauteilen."])
    story += [P("Der Lernpfad", "h2"),
              P("Die Stufen öffnen sich nacheinander: Hast du die Experimente einer Stufe geschafft, geht die "
                "nächste auf. Eltern und Lehrkräfte können in den Einstellungen alles sofort freigeben."),
              table([["Stufe", "Thema", "Forscherfrage", "Exp."],
                     ["0", "Auftakt", "Wie bringe ich etwas zum Blinken?", "1"],
                     ["1", "Strom fließt im Kreis", "Wann leuchtet das Lämpchen?", "10"],
                     ["2", "Mehrere Wege", "Was passiert, wenn ich ein zweites Lämpchen einbaue?", "20"],
                     ["3", "Messen und verstehen", "Wie viel Strom fließt eigentlich?", "15"],
                     ["4", "Kleine Elektronik", "Wie kann ein Strom einen anderen steuern?", "15"]],
                    [14, 46, 96, 14])]

    # 6 Messen und Beobachten
    story += chapter("6  Messen und beobachten")
    story += [P("Das Messgerät", "h2"),
              P("<b>V A Ω</b> in der oberen Leiste holt das Messgerät. Mit dem Drehschalter wählst du, was du "
                "misst:"),
              table([["Stellung", "misst", "so wird angeschlossen"],
                     ["V", "Spannung in Volt", "COM und V/Ω an die beiden Stellen, zwischen denen du misst "
                      "(parallel)"],
                     ["A", "Strom in Ampere bzw. Milliampere", "COM und A so, dass der Strom durch das "
                      "Messgerät fließen muss (in den Stromkreis hinein)"],
                     ["Ω", "Widerstand in Ohm", "COM und V/Ω an das Bauteil – bei ausgeschaltetem Strom"]],
                    [20, 50, 100]),
              P("Der Schalter <b>digital/analog</b> wechselt zwischen Ziffernanzeige und Zeiger. Über das Menü "
                "kannst du das Messgerät neben die Grundplatte oder oben ins Pult setzen; seine Buchsen COM, V/Ω "
                "und A sind Kontaktfelder wie alle anderen."),
              shot("messgeraet", caption="Das Messgerät zeigt die Spannung der Batterie: 9,00 V"),
              P("Spannungsanzeige und Stromfluss", "h2")]
    story += bullets(["Der Knopf mit den <b>grünen Balken</b> oben in der Leiste schaltet die <b>Spannungsanzeige</b> "
                      "auf allen belegten Feldern der Grundplatte ein: Bis zu vier grüne Balken zeigen, wie viel "
                      "Spannung an jedem Feld anliegt, an dem ein Bauteil oder eine Leitung steckt. Volle Balken "
                      "heißen viel Spannung, leere Umrisse fast keine. Ab Werk ist der Knopf aus – probiere ihn aus!",
                      "Für ein einzelnes Feld – auch am Pult – geht es mit langem Druck auf das Feld: Im Feldmenü "
                      "schaltest du dort eine Spannungsanzeige ein.",
                      "Kleine gelbe Pfeile auf Leitungen und Bauteilen zeigen, <b>wohin der Strom fließt</b> "
                      "(abschaltbar im Menü)."])
    story += [shot("spannung_auto", 120, "Knopf mit den grünen Balken eingeschaltet: A1 hat Spannung, A2 fast keine",
                   crop=(262, 0, 940, 540)),
              P("Schaltplan", "h2"),
              P("Das Schaltzeichen-Symbol zeigt deinen Aufbau als Schaltplan – so, wie ihn Elektroniker zeichnen. "
                "Er wächst mit, während du baust."),
              shot("schaltplan", caption="Aufbau und Schaltplan nebeneinander"),
              P("Zeitschreiber (ab Stufe 3)", "h2"),
              P("Der Zeitschreiber zeichnet Spannungen über die Zeit auf, zum Beispiel beim Laden eines "
                "Kondensators. Wähle im Feldmenü »Zeitschreiber Kanal 1« (oder 2): Dann wird die Spannung an diesem "
                "Feld aufgezeichnet. Ein Kanal kann auch das Messgerät mitschreiben."),
              shot("zeitschreiber", caption="Der Zeitschreiber zeigt, wie sich ein Elko auflädt"),
              P("Blick hinein", "h2"),
              P("Module wie das Blinkmodul sind kleine Schaltungen in einem Gehäuse. Im Menü eines Moduls öffnet "
                "<b>»Blick hinein«</b> seinen Schaltplan – mit den Spannungen und Strömen, die Volta gerade "
                "ausrechnet."),
              shot("blick_hinein", caption="Blick in das Blinkmodul: zwei Transistoren kippen hin und her",
                   crop=(225, 95, 1055, 775)),
              P("Licht, Wärme, Wasser (ab Stufe 3)", "h2"),
              P("Fotowiderstand, Heißleiter und Feuchtesonde reagieren auf ihre Umgebung. Unten in der Mitte "
                "klappt die Leiste <b>»Licht, Wärme, Wasser«</b> auf: Ziehe die <b>Taschenlampe</b>, den "
                "<b>Föhn</b> oder einen <b>Wassertropfen</b> auf ein Bauteil. Mit dem Raumlicht stellst du hell, "
                "Dämmerung oder dunkel ein."),
              shot("umwelt", caption="Sensoren mit Taschenlampe, Föhn und Wassertropfen")]

    # 7 Werkzeugkasten
    story += chapter("7  Der Werkzeugkasten")
    story += [P("Der Koffer in der oberen Leiste öffnet den Werkzeugkasten. Wische über die Reiter oder tippe auf "
                "den blauen Pfeil, um alle zu sehen."),
              table([["Reiter", "Was er kann"],
                     ["Farbcode", "Was bedeuten die Ringe auf einem Widerstand? Ringe wählen oder einen Wert "
                      "eingeben – auch für einen ausgewählten Widerstand."],
                     ["E-Reihen", "Welche Widerstandswerte gibt es zu kaufen?"],
                     ["Lexikon", "Was macht welches Bauteil? Mit Bild, Schaltzeichen und Erklärung."],
                     ["Rechner", "Formeln mit Rechenweg, z. B. das ohmsche Gesetz (ab Stufe 3)."],
                     ["Fehlersuche", "Tick prüft deinen Aufbau und stellt Fragen zu Stellen, die ihm auffallen "
                      "(ab Stufe 2)."],
                     ["Teileliste", "Was du für ein Experiment oder deinen Aufbau brauchst – auch als "
                      "<b>Aufbaukarte</b> (PDF) zum Ausdrucken und Nachbauen mit echten Teilen."]], [32, 138]),
              shot("farbcode", caption="Farbcode: 470 Ω mit den Ringen Gelb, Violett, Braun, Gold"),
              shot("teileliste", caption="Teileliste zum Experiment – »Aufbaukarte speichern (PDF)« erzeugt eine "
                                         "Karte zum Ausdrucken")]

    # 8 Wenn etwas nicht klappt
    story += chapter("8  Wenn etwas nicht klappt")
    story += [P("Es leuchtet nichts, oder ein Bauteil ist kaputtgegangen? Dann meldet sich Tick mit einem kleinen "
                "Fenster. <b>»Zur Fehlersuche«</b> öffnet die Fehlersuche, ein Tipp daneben schließt das Fenster."),
              shot("fehlersuche", caption="Die Fehlersuche hat zwei Stellen gefunden; Tick fragt nach"),
              P("Typische Ursachen", "h2")]
    story += bullets(["<b>Hauptschalter aus?</b> Erst im »Betrieb« fließt Strom.",
                      "<b>Kein geschlossener Weg?</b> Führt ein Weg von V+ durch die Bauteile zurück nach 0V?",
                      "<b>Leuchtdiode falsch herum?</b> Leuchtdioden haben eine Richtung: Das lange Bein kommt zu "
                      "Plus.",
                      "<b>Ein Bein steckt allein in einem Feld?</b> Dann ist es mit nichts verbunden.",
                      "<b>Bauteil durchgebrannt?</b> Zu viel Strom – meist fehlt ein Vorwiderstand. Mit "
                      "»Rückgängig« oder »Reparieren« im Bauteilmenü ist es wieder heil.",
                      "<b>Sicherung durchgebrannt?</b> Ein Kurzschluss: Plus und Minus sind direkt verbunden. "
                      "Kurzschluss beseitigen und »Sicherung tauschen« an der Batterie antippen.",
                      "<b>Drehregler durchgebrannt?</b> Auch die Drehregler im Pult vertragen nur eine begrenzte "
                      "Leistung. Wird einer zu heiß, bekommt sein Knopf einen orangen Rand; brennt er durch, wird er "
                      "grau, es raucht, und darunter steht »kaputt – antippen«. Antippen repariert ihn. Abhilfe: "
                      "einen Vorwiderstand einbauen oder nicht ganz an den Anschlag drehen."])

    # 9 Forscherheft
    story += chapter("9  Das Forscherheft")
    story += [P("Im Forscherheft sammelt Volta zu jedem Experiment deine Vermutung, deine Beobachtung und ob die "
                "Vermutung gestimmt hat. Dazu kannst du eigene Notizen schreiben (»Ins Forscherheft schreiben«) "
                "und mit <b>»Foto hinzufügen«</b> Bilder anheften – zum Beispiel ein Foto von deinem echten Aufbau. "
                "Volta verkleinert das Foto und entfernt dabei Aufnahmeort und Kameradaten."),
              shot("forscherheft", 150, "Ein Eintrag im Forscherheft mit Vermutung und Notiz",
                   crop=(0, 0, 1280, 330))]

    # 10 Einstellungen
    story += chapter("10  Einstellungen")
    story += [P("Oben rechts stellst du Volta so ein, wie es dir gefällt:")]
    story += bullets(["<b>A− und A+</b> machen die Schrift kleiner oder größer.",
                      "Die <b>Flagge</b> wählt die Sprache: Deutsch, Französisch, Englisch oder Luxemburgisch.",
                      "Das <b>?</b> öffnet die Hilfe mit Tick, das <b>i</b> zeigt Infos über Volta.",
                      "Das <b>Menü mit den drei Strichen</b> enthält die Einstellungen:"])
    story += [table([["Einstellung", "Wirkung"],
                     ["Stromfluss anzeigen", "kleine Pfeile zeigen die Stromrichtung"],
                     ["Ton über Lautsprecher", "Summer, Lautsprecher und Klänge hörbar"],
                     ["Leitungsfarbe automatisch", "Leitungen an Plus rot, an Minus schwarz"],
                     ["Steckfeld-Breite automatisch", "Lernpfad 8, freies Labor 12 Felder; Messgerät daneben 10, "
                      "im Pult 12"],
                     ["Kontaktfelder silbern", "silberne Kontaktfelder; ausgeschaltet messingfarben"],
                     ["Animationen reduzieren", "weniger Bewegung auf dem Bildschirm"],
                     ["Messgerät anzeigen, Felder je Reihe", "wie die Tasten in der Leiste"],
                     ["Weitere Einstellungen …", "Einstellungen für Erwachsene (mit PIN)"]], [58, 112]),
              shot("hilfe", 140, "Die Hilfe mit Tick: Kapitel oben antippen", crop=(230, 0, 1050, 800))]

    # 11 Eltern und Lehrkraefte
    story += chapter("11  Für Eltern und Lehrkräfte")
    story += [P("Die Einstellungen", "h2"),
              P("Die Einstellungen öffnen Sie über das Menü (drei Striche) → »Weitere Einstellungen …« oder in der "
                "Hilfe unter »Für Erwachsene«. Sie sind durch eine PIN geschützt; die <b>Start-PIN ist 1234</b>. Bitte "
                "legen Sie gleich eine eigene PIN mit 4 bis 8 Ziffern fest. Unter »Allgemein« lässt sich die "
                "PIN-Abfrage für die Einstellungen abschalten (nur nach Eingabe der PIN); alle Links aus der App "
                "heraus bleiben immer hinter der PIN. Am Computer lässt sie sich mit der Tastatur eingeben."),
              shot("elternbereich", 150, "Die Einstellungen (Ausschnitt)", crop=(250, 0, 1030, 800)),
              table([["Bereich", "Was Sie dort einstellen"],
                     ["PIN", "eigene PIN festlegen (wird nur als Prüfwert gespeichert)"],
                     ["Ton im Klassenzimmer", "Normal, nur mit Kopfhörern oder aus – gilt auch für das Vorlesen"],
                     ["Maskottchen Tick", "Tick ein- oder ausblenden; ohne Tick erscheinen dieselben Hinweise als "
                      "einfache Kästen"],
                     ["Aufgabe für die Klasse", "Experimente und/oder den aktuellen Aufbau als QR-Code ausgeben"],
                     ["Präsentationsmodus", "Volta bildschirmfüllend für Beamer und Tafel"],
                     ["Freigaben", "alle Stufen und Bauteile sofort freigeben"],
                     ["Forscherprofile", "mehrere Kinder an einem Gerät, je mit Symbol und Spitzname"],
                     ["Mitnehmen", "Profil als Datei speichern/laden; Fortschritt als QR-Code"],
                     ["Fortschritt und Forscherheft", "zurücksetzen bzw. leeren"],
                     ["Datenschutz, So rechnet Volta", "was gespeichert wird; welche Vereinfachungen die "
                      "Simulation enthält"]], [52, 118])]
    story += [P("Aufgaben für die Klasse", "h2"),
              P("Wählen Sie in den Einstellungen unter »Aufgabe für die Klasse« Experimente aus dem Lernpfad und/oder "
                "geben Sie den Aufbau mit, der gerade im Labor steckt – etwa mit dem Titel »Finde den Fehler« und "
                "einem kurzen Aufgabentext. <b>»QR-Code zeigen«</b> zeigt einen großen Code für den Beamer:")]
    story += bullets(["Die Kinder scannen den QR-Code mit der <b>Kamera-App</b> ihres Tablets; Volta öffnet sich "
                      "und zeigt die Aufgabe.",
                      "Ohne Kamera: Bis zu vier Experimente (ohne Text und Aufbau) passen auch in einen <b>Code "
                      "zum Eintippen</b> wie 2H31-0005. Die Kinder tippen auf der Startseite auf »Aufgabe« → "
                      "»Code der Lehrkraft eingeben«.",
                      "Die Aufgabe erscheint auf der Startseite der Kinder, bis eine neue kommt oder sie entfernt "
                      "wird. Erledigte Experimente bekommen einen Haken.",
                      "Übertragen wird nur die Aufgabe selbst – es gibt keine Verbindung zwischen den Geräten und "
                      "keine Rückmeldung an die Lehrkraft."])
    story += [shot("aufgabe_qr", 140, "QR-Code und Code zum Eintippen für eine Aufgabe mit zwei Experimenten",
                   crop=(0, 60, 1280, 760)),
              P("Präsentationsmodus", "h2"),
              P("Für die Arbeit am Beamer zeigt sich Volta bildschirmfüllend mit etwas größerer Schrift; alle "
                "Stufen sind offen. Fortschritt, Forscherheft und Laborstand des Geräts bleiben unberührt – was "
                "Sie in der Präsentation aufbauen, wird beim Beenden verworfen."),
              P("Verwaltete Geräte in der Schule", "h2"),
              P("Auf Tablets, die eine Schule über eine Geräteverwaltung (MDM) betreut, kann die Schule Sprache, "
                "freigegebene Stufen, Ton und die Zahl der Forscherprofile vorgeben. Solche Vorgaben sind "
                "in den Einstellungen gekennzeichnet und lassen sich dort nicht ändern."),
              P("Datenschutz", "h2"),
              P("Volta erhebt keine Daten. Es gibt kein Konto, keine Werbung, keine Käufe in der App und keine "
                "Verbindung ins Internet; Kamera und Mikrofon werden nicht verwendet. Fortschritt, Forscherheft, "
                "Fotos, Aufbau und Einstellungen liegen nur im privaten Speicher der App auf dem Gerät und werden "
                "beim Deinstallieren gelöscht."),
              P("So rechnet Volta", "h2"),
              P("Was in Volta passiert, ergibt sich aus einer echten Schaltungssimulation, nicht aus vorbereiteten "
                "Ergebnissen. Deshalb verhalten sich auch selbst erfundene Schaltungen plausibel. Die wenigen "
                "Vereinfachungen (etwa beim Tonmodul oder bei Spulen) sind in den Einstellungen unter »So rechnet "
                "Volta« offengelegt.")]

    # 12 Sicherheit
    story += chapter("12  Sicher experimentieren mit echten Bauteilen")
    story += [tick_box("warnend", "<b>Sicherheit zuerst!</b> Volta macht Lust auf echte Elektronik. Dabei gilt "
                       "immer:", bg=WARN, border=colors.HexColor("#E57373"))]
    story += bullets(["Experimentiere nur mit <b>Batterien bis 9 Volt</b> – niemals an der Steckdose!",
                      "Versuche mit <b>»Nur in der App!«</b> machen ein Bauteil kaputt. Für sie gibt es keine "
                      "Aufbaukarte; probiere sie nur in Volta aus.",
                      "Bauteile können heiß werden. Fass sie nicht an, solange Strom fließt.",
                      "Batterien nicht kurzschließen: Plus und Minus nie direkt verbinden.",
                      "Kleine Bauteile gehören nicht in die Hände kleiner Geschwister."])

    # 13 Bauteile
    story += chapter("13  Bauteile und Module")
    story += [table([["Bauteil", "Was es macht"],
                     ["Glühlämpchen", "leuchtet, wenn Strom hindurchfließt; wird dabei warm"],
                     ["Leuchtdiode (rot, grün, gelb, blau)", "leuchtet nur in einer Richtung (langes Bein zu "
                      "Plus); braucht einen Vorwiderstand"],
                     ["Widerstand", "bremst den Strom; der Wert steht im Farbcode"],
                     ["Taster, Wechselschalter, Reedkontakt", "schließen oder öffnen einen Stromkreis – der "
                      "Reedkontakt mit einem Magneten"],
                     ["Motor", "dreht sich; die Richtung hängt von der Polung ab"],
                     ["Summer, Piezo-Schallgeber", "geben einen Ton aus"],
                     ["Diode", "lässt Strom nur in eine Richtung durch"],
                     ["Kondensator, Elko", "speichern elektrische Ladung; der Elko hat eine Polung"],
                     ["Fotowiderstand", "leitet besser, je heller es ist"],
                     ["Heißleiter", "leitet besser, je wärmer er ist"],
                     ["Feuchtesonde", "leitet, wenn sie nass wird"],
                     ["Elektromagnet, Relais", "ein Strom erzeugt Magnetkraft; das Relais schaltet damit einen "
                      "zweiten Stromkreis"],
                     ["Transistor (NPN)", "ein kleiner Strom steuert einen großen – das Herz der Elektronik"]],
                    [62, 108]),
              table([["Modul", "Was es macht"],
                     ["Blinkmodul", "schaltet seinen Ausgang im Takt ein und aus"],
                     ["Tonmodul", "spielt Töne und Melodien"],
                     ["Zeitmodul", "hält den Ausgang nach einem Druck auf »start« einige Sekunden eingeschaltet"],
                     ["Verstärkermodul", "verstärkt einen winzigen Eingangsstrom etwa 40 000-fach"]], [62, 108])]

    # 14 Tastatur und Kontakt
    story += chapter("14  Am Computer und Kontakt")
    story += [table([["Taste", "Wirkung"],
                     ["Strg+Z", "Rückgängig"],
                     ["Strg+Y oder Strg+Umschalt+Z", "Wiederholen"],
                     ["Entf oder Rücktaste", "Ausgewähltes entfernen"],
                     ["Rechtsklick", "Menü eines Bauteils, einer Leitung oder eines Feldes"],
                     ["Doppelklick", "Bauteil drehen"]], [62, 108]),
              P("Lizenzen", "h2"),
              P("Volta ist mit freier Software gebaut, vor allem mit Qt. Im Info-Fenster (<b>i</b> oben rechts) "
                "zeigt <b>»Lizenzen anzeigen«</b> alle Fremdbestandteile mit Version und Lizenz; ein Tippen auf einen "
                "Eintrag öffnet den Lizenztext im englischen Original. Alles ist in der App gespeichert, es braucht "
                "kein Internet."),
              P("Kontakt", "h2"),
              P("DC Software Engineering, Daniel Zeimentz, Aufm Rothenhügel 26, 54666 Irrel (Deutschland)<br/>"
                "Web: dc-software-engineering.com · E-Mail: info@dc-software-engineering.com"),
              tick_box("froehlich", "Viel Spaß beim Forschen! Und denk dran: Wer sich irrt, lernt am meisten.")]

    doc = Doc(path, "Anleitung")
    doc.multiBuild(story)


# ---------------------------------------------------------------- Kurzanleitung

def short_manual(path):
    # dieselben acht Schritte und Bilder wie in der App (QuickGuide.qml), je Seite vier
    steps = [("Experiment aussuchen", "Tippe auf der Startseite im Lernpfad auf ein Experiment. Fang ganz oben bei "
                                      "Stufe 0 an."),
             ("Frage lesen und raten", "Tick stellt dir eine Forscherfrage. Was glaubst du? Tippe auf deine Antwort. "
                                       "Raten ist erlaubt!"),
             ("Bauteil holen", "Zieh das Bauteil mit dem Finger aus dem Bauteilfach auf die Felder. Alles, was im "
                               "selben Feld steckt, ist verbunden."),
             ("Leitungen ziehen", "Zieh mit dem Finger von einem Feld zum anderen. Eine Leitung von Plus (V+) wird "
                                  "rot, eine von Minus (0V) schwarz."),
             ("Strom einschalten", "Tippe an der Batterie auf den Hauptschalter, bis »Ein« dasteht. Oben steht jetzt "
                                   "»Betrieb«. Zum Umbauen schaltest du wieder aus."),
             ("Hinschauen und »Fertig«", "Leuchtet das Lämpchen? Dann hat es geklappt! Tippe auf »Fertig«."),
             ("Sagen, was du gesehen hast", "Tippe an, was passiert ist. Danach erklärt dir Tick, warum es so ist."),
             ("Hilfe holen", "»Schritt für Schritt bauen« zeigt dir jedes Teil. »Ich brauche einen Tipp« gibt dir "
                             "einen Hinweis. Ging etwas schief? Der Pfeil oben macht es wieder rückgängig.")]
    story = []
    for page in range(2):
        story += ([P("Kurzanleitung", "h1"), P("In acht Schritten zum ersten Experiment", "h2")] if page == 0
                  else [P("Kurzanleitung – weiter", "h1")])
        rows = []
        for n in range(page * 4 + 1, page * 4 + 5):
            title, text = steps[n - 1]
            img = Image(str(OUT / "bilder" / f"_schritt{n}_de.png"), width=92 * mm, height=46 * mm)
            rows.append([P(f"<font size=16><b>{n}</b></font>", "center"),
                         [P(f"<font size=13><b>{title}</b></font>", "body"), P(text, "body")], img])
        t = Table(rows, colWidths=[11 * mm, 66 * mm, 93 * mm])
        t.setStyle(TableStyle([("BACKGROUND", (0, 0), (0, -1), YELLOW), ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
                               ("GRID", (0, 0), (-1, -1), 0.5, colors.HexColor("#C9D1DA")),
                               ("TOPPADDING", (0, 0), (-1, -1), 5), ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
                               ("LEFTPADDING", (2, 0), (2, -1), 0), ("RIGHTPADDING", (2, 0), (2, -1), 0)]))
        story += [t, PageBreak()]

    story += [P("Bedienung auf einen Blick", "h1"),
              table([["So geht's", "Finger / Maus"],
                     ["Bauteil stecken oder verschieben", "ziehen"],
                     ["Bauteil drehen", "zweimal schnell tippen / Doppelklick"],
                     ["Menü (Drehen, Farbe, Wert, Löschen …)", "lange drücken / Rechtsklick"],
                     ["Leitung ziehen", "von Feld zu Feld ziehen"],
                     ["Leitung umstecken", "Leitungsende anfassen und ziehen"],
                     ["Etwas entfernen", "auswählen und Mülleimer antippen oder darauf ziehen"],
                     ["Rückgängig / Wiederholen", "Pfeile oben / Strg+Z, Strg+Y"],
                     ["Strom ein / aus", "»Stecken« ↔ »Betrieb« oben links"]], [85, 85]),
              table([["Oben in der Leiste", ""],
                     ["Haus", "Startseite"],
                     ["Koffer", "Werkzeugkasten: Farbcode, E-Reihen, Lexikon, Rechner, Fehlersuche, Teileliste"],
                     ["Grüne Balken", "Spannungsanzeige auf allen belegten Feldern"],
                     ["V A Ω", "Messgerät (Spannung, Strom, Widerstand)"],
                     ["Schaltzeichen", "Schaltplan"],
                     ["Kurve", "Zeitschreiber (ab Stufe 3)"],
                     ["A− A+ · Flagge", "Schriftgröße · Sprache (DE, FR, EN, LU)"],
                     ["? · i · drei Striche", "Hilfe mit Tick · Info · Einstellungen (weitere mit PIN)"]],
                    [45, 125]),
              tick_box("warnend", "<b>Mit echten Bauteilen:</b> nur Batterien bis 9 Volt, nie an die Steckdose! "
                       "Versuche mit »Nur in der App!« nicht in echt nachbauen. Bauteile können heiß werden.",
                       bg=WARN, border=colors.HexColor("#E57373")),
              P("Für Eltern und Lehrkräfte", "h2")]
    story += bullets(["Einstellungen: Menü (drei Striche) → »Weitere Einstellungen …«, <b>Start-PIN 1234</b> – bitte gleich "
                      "ändern.",
                      "Dort: Freigaben, Forscherprofile, Ton im Klassenzimmer, Aufgaben per QR-Code oder Code zum "
                      "Eintippen, Präsentationsmodus.",
                      "Kein Internet, kein Konto, keine Werbung, keine Datenerhebung.",
                      "Ausführliche Anleitung: Volta1-Anleitung-de.pdf"])
    doc = Doc(path, "Kurzanleitung")
    doc.addPageTemplates([])
    doc.pageTemplates = [doc.pageTemplates[1]]
    doc.build(story)


# ---------------------------------------------------------------- Bilder der Kurzanleitung (App und PDF)

# Je Schritt ein vergroesserter Ausschnitt des Labors mit orangem Rahmen und Pfeil, je Sprache aufgenommen:
# app/resources/guide/schritt<n>_<sprache>.jpg (QuickGuide.qml) und Release/Anleitung/bilder/_schritt<n>_de.png (PDF).
# Koordinaten im Bild 1280 x 800 (Aufbau: 1280 x 1000); Knoepfe im Seitenfeld rechts werden gesucht, weil ihre Lage
# von der Textlaenge der Sprache abhaengt.
GUIDE_SCENES = {
    "start": {"VOLTA_SCREENSHOT_S": "3"},
    "frage": {"VOLTA_DEMO_QUESTION": "1", "VOLTA_SCREENSHOT_S": "4"},
    "aufbau": {"VOLTA_DEMO_QUESTION": "2", "VOLTA_DEMO_SIZE": "1280x1000", "VOLTA_SCREENSHOT_S": "4"},
    "hilfe": {"VOLTA_DEMO_GUIDE": "1", "VOLTA_DEMO_EXP": "volta1.1.1", "VOLTA_SCREENSHOT_S": "4"},
    "geloest": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.1.1", "VOLTA_SCREENSHOT_S": "5"},
    "beobachten": {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.1.1", "VOLTA_DEMO_FINISH": "2",
                   "VOLTA_SCREENSHOT_S": "6"},
    "messgeraet": {"VOLTA_DEMO_METER": "v", "VOLTA_SCREENSHOT_S": "4"},
    "werkzeug": {"VOLTA_DEMO_TOOLBOX": "0", "VOLTA_SCREENSHOT_S": "4"},
    "fehlersuche": {"VOLTA_DEMO_TOOLBOX": "4", "VOLTA_SCREENSHOT_S": "4"},
}
APP_GUIDE = ROOT / "app" / "resources" / "guide"
APP_HELP = ROOT / "app" / "resources" / "help"
GUIDE_STEPS = 8
GUIDE_SIZE = (880, 440)
GUIDE_JOIN = {6}   # Schritte mit Pfeil zwischen den Ausschnitten (sonst nur Abstand)
ORANGE = (240, 120, 0)


def panel_buttons(img, top=100):
    """Knoepfe im Seitenfeld rechts (Antworten, Hilfen): [(oben, unten)] am blauen Rand bei x = 958."""
    lines = []
    for y in range(top, img.height - 110):
        r, g, b = img.getpixel((958, y))
        if not (r < 70 and 100 < g < 135 and b > 160):
            continue
        if lines and y - lines[-1][1] <= 2:
            lines[-1][1] = y
        else:
            lines.append([y, y])
    return [(lines[i][0] - 3, lines[i + 1][1] + 3) for i in range(0, len(lines) - 1, 2)]


def _step_panels(n, shots):
    """Ausschnitte eines Schritts: [(Szene, (links, oben, rechts, unten), [Markierungen])]; Markierung
    ("box", (x0, y0, x1, y1)) oder ("arrow", (x0, y0), (x1, y1)) im Bild der Szene."""
    if n == 1:   # Experiment im Lernpfad antippen
        return [("start", (16, 496, 700, 716), [("box", (36, 607, 334, 699)), ("arrow", (580, 653), (350, 653))])]
    if n == 2:   # Frage lesen, Vermutung antippen
        b = panel_buttons(shots["frage"], 280)
        return [("frage", (944, 108, 1280, b[-1][1] + 16), [("box", (950, b[0][0] - 2, 1270, b[-1][1] + 2))])]
    if n == 3:   # Bauteil aus dem Fach auf die blinkenden Felder ziehen
        return [("hilfe", (0, 228, 830, 540), [("box", (5, 240, 123, 309)), ("box", (265, 343, 487, 437)),
                                               ("arrow", (128, 286), (258, 376))])]
    if n == 4:   # Leitungen: rot von V+, schwarz von 0V
        return [("geloest", (266, 236, 512, 470), [("box", (283, 243, 356, 316)), ("box", (355, 243, 428, 316))])]
    if n == 5:   # Hauptschalter ein, oben steht »Betrieb«
        return [("geloest", (262, 0, 620, 322), [("box", (293, 1, 417, 55)), ("box", (298, 170, 422, 218)),
                                                ("arrow", (284, 170), (300, 62))])]
    if n == 6:   # Laempchen leuchtet -> »Fertig«
        return [("geloest", (266, 330, 492, 470), [("box", (334, 350, 416, 428))]),
                ("geloest", (944, 700, 1280, 796), [("box", (950, 709, 1270, 769))])]
    if n == 7:   # Beobachtung antippen
        b = panel_buttons(shots["beobachten"], 200)
        return [("beobachten", (944, 108, 1280, b[-1][1] + 16), [("box", (950, b[0][0] - 2, 1270, b[-1][1] + 2))])]
    if n == 8:   # Schritt fuer Schritt, Tipp, Rueckgaengig
        b = panel_buttons(shots["aufbau"], 520)[-2:]
        return [("geloest", (419, 0, 483, 60), [("box", (423, 1, 477, 55))]),
                ("aufbau", (944, b[0][0] - 22, 1280, b[-1][1] + 22),
                 [("box", (950, y0 - 2, 1270, y1 + 2)) for y0, y1 in b])]
    raise ValueError(n)


# Hilfe (HelpDialog.qml): ein Bild je Kapitel, ruhiger als die Kurzanleitung (kleiner, wenige Rahmen)
HELP_SIZE = (760, 300)
HELP_JOIN = {"werkzeug"}


def _help_panels(key, shots):
    if key == "willkommen":   # Startseite: freies Labor, Forscherheft, Lernpfad
        return [("start", (6, 240, 980, 552), [("box", (340, 247, 644, 355)), ("box", (22, 369, 326, 477)),
                                                 ("box", (12, 486, 150, 542))])]
    if key == "stecken":      # Bauteilfach -> Felder
        return [("hilfe", (0, 228, 830, 540), [("box", (5, 240, 123, 309)), ("box", (265, 343, 487, 437)),
                                               ("arrow", (128, 286), (258, 376))])]
    if key == "leitungen":
        return [("geloest", (266, 236, 512, 470), [("box", (283, 243, 356, 316)), ("box", (355, 243, 428, 316))])]
    if key == "pult":         # Spannung waehlen, Hauptschalter
        return [("geloest", (262, 78, 930, 330), [("box", (294, 114, 412, 170)), ("box", (298, 170, 422, 218))])]
    if key == "rueckgaengig":  # Pfeile oben, Muelleimer
        return [("geloest", (419, 0, 543, 60), [("box", (423, 1, 537, 55))]),
                ("geloest", (740, 566, 815, 640), [("box", (747, 573, 808, 634))])]
    if key == "messen":       # Spannungsanzeige, V A Ohm, Schaltplan, Zeitschreiber; Messgeraet
        return [("messgeraet", (600, 0, 852, 60), [("box", (602, 1, 658, 55)), ("box", (662, 1, 729, 55)),
                                                    ("box", (734, 1, 789, 55)), ("box", (794, 1, 849, 55))]),
                ("messgeraet", (1100, 262, 1262, 476), [])]
    if key == "werkzeug":     # Koffer in der Leiste -> Werkzeugkasten
        return [("werkzeug", (420, 0, 740, 60), [("box", (542, 1, 598, 55))]),
                ("werkzeug", (822, 62, 1280, 340), [])]
    if key == "experiment":   # Frage und Vermutung
        b = panel_buttons(shots["frage"], 280)
        return [("frage", (944, 108, 1280, b[-1][1] + 16), [("box", (950, b[0][0] - 2, 1270, b[-1][1] + 2))])]
    if key == "fehler":       # Fehlersuche: pruefen, Fragen von Tick
        return [("fehlersuche", (822, 62, 1280, 445), [("box", (833, 176, 1268, 228))])]
    if key == "einstellungen":  # oben rechts: Schrift, Sprache, Hilfe, Kurzanleitung, Info, Menue
        return [("start", (890, 0, 1280, 58), [])]
    raise ValueError(key)


HELP_KEYS = ["willkommen", "stecken", "leitungen", "pult", "rueckgaengig", "messen", "werkzeug", "experiment",
             "fehler", "einstellungen"]


def _arrow(d, a, b, width, color):
    import math
    ang = math.atan2(b[1] - a[1], b[0] - a[0])
    head = width * 2.6
    base = (b[0] - head * math.cos(ang), b[1] - head * math.sin(ang))
    d.line([a, base], fill=color, width=round(width))
    side = (head * 0.62 * math.sin(ang), head * 0.62 * math.cos(ang))
    d.polygon([b, (base[0] + side[0], base[1] - side[1]), (base[0] - side[0], base[1] + side[1])], fill=color)


def guide_image(n, shots, dst):
    """Ein Schrittbild der Kurzanleitung."""
    compose(_step_panels(n, shots), shots, dst, GUIDE_SIZE, n in GUIDE_JOIN)


def help_image(key, shots, dst):
    """Ein Kapitelbild der Hilfe."""
    compose(_help_panels(key, shots), shots, dst, HELP_SIZE, key in HELP_JOIN)


def compose(panels, shots, dst, size, join):
    """Ausschnitte nebeneinander auf hellem Grund, vergroessert, mit Rahmen und Pfeilen."""
    k = 3   # Ueberabtastung fuer glatte Linien
    W, H = size
    gap, pad = (90 if join else 40), 20
    widths = sum(c[2] - c[0] for _, c, _ in panels)
    heights = max(c[3] - c[1] for _, c, _ in panels)
    s = min((W - 2 * pad - gap * (len(panels) - 1)) / widths, (H - 2 * pad) / heights, 2.2)
    out = PILImage.new("RGB", (W * k, H * k), (255, 248, 230))
    d = ImageDraw.Draw(out)
    x = (W - widths * s - gap * (len(panels) - 1)) / 2
    places = []
    for scene, (x0, y0, x1, y1), marks in panels:
        w, h = (x1 - x0) * s, (y1 - y0) * s
        y = (H - h) / 2
        crop = shots[scene].crop((x0, y0, x1, y1)).resize((round(w * k), round(h * k)), PILImage.LANCZOS)
        out.paste(crop, (round(x * k), round(y * k)))
        d.rounded_rectangle([(x - 2) * k, (y - 2) * k, (x + w + 2) * k, (y + h + 2) * k], radius=6 * k,
                            outline=(10, 37, 64), width=2 * k)

        def to(px, py, x=x, y=y, x0=x0, y0=y0):
            return ((x + (px - x0) * s) * k, (y + (py - y0) * s) * k)
        for m in marks:
            if m[0] == "box":
                a, b = to(*m[1][:2]), to(*m[1][2:])
                for col, wid in (((255, 255, 255), 9), (ORANGE, 5)):
                    d.rounded_rectangle([a, b], radius=8 * k, outline=col, width=wid * k)
            else:
                a, b = to(*m[1]), to(*m[2])
                _arrow(d, a, b, 13 * k, (255, 255, 255))
                _arrow(d, a, b, 7 * k, ORANGE)
        places.append((x, w))
        x += w + gap
    if join:
        for (ax, aw), (bx, _) in zip(places, places[1:]):
            _arrow(d, ((ax + aw + 14) * k, H / 2 * k), ((bx - 14) * k, H / 2 * k), 8 * k, ORANGE)
    img = out.resize(size, PILImage.LANCZOS)
    if str(dst).endswith(".jpg"):
        img.save(dst, quality=84, optimize=True)
    else:
        img.save(dst)


def guide_shots(lang):
    folder = OUT / "bilder" / f"kurz_{lang}"
    folder.mkdir(parents=True, exist_ok=True)
    shots = {}
    for name, extra in GUIDE_SCENES.items():
        target = folder / f"{name}.png"
        env = dict(os.environ)
        env.update({"VOLTA_NO_SPLASH": "1", "VOLTA_DEMO_LANG": lang, "VOLTA_SCREENSHOT": str(target)})
        env.update(extra)
        subprocess.run([str(EXE)], env=env, timeout=60, check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        shots[name] = PILImage.open(target).convert("RGB")
    return shots


def app_images():
    """Schrittbilder der Kurzanleitung und Kapitelbilder der Hilfe je Sprache fuer die App, deutsch auch fuers PDF."""
    APP_GUIDE.mkdir(parents=True, exist_ok=True)
    APP_HELP.mkdir(parents=True, exist_ok=True)
    for lang in ("de", "en", "fr", "lb"):
        shots = guide_shots(lang)
        for key in HELP_KEYS:
            help_image(key, shots, APP_HELP / f"{key}_{lang}.jpg")
            if lang == "de":
                help_image(key, shots, OUT / "bilder" / f"_hilfe_{key}_de.png")
        size = 0
        for n in range(1, GUIDE_STEPS + 1):
            dst = APP_GUIDE / f"schritt{n}_{lang}.jpg"
            guide_image(n, shots, dst)
            size += dst.stat().st_size
            if lang == "de":
                guide_image(n, shots, OUT / "bilder" / f"_schritt{n}_de.png")
        print(f"  App-Bilder {lang}: {size // 1024} KB")


# Hilfe ab Volta 2 (HelpDialog.qml, Band volta2): eigene Kapitelbilder aus volta2.exe (build/volta2-debug, Worktree
# volta-volta2) fuer die Kapitel, deren Bild aus Volta 1 nicht passt, und die neuen Kapitel
EXE2 = ROOT / "build" / "volta2-debug" / "app" / ("volta2.exe" if os.name == "nt" else "volta2")
APP_HELP2 = ROOT / "app" / "resources" / "help2"
HELP2_SCENES = {
    "start": {"VOLTA_SCREENSHOT_S": "3"},
    "hilfe": {"VOLTA_DEMO_GUIDE": "1", "VOLTA_DEMO_EXP": "volta2.1.1", "VOLTA_SCREENSHOT_S": "4"},
    "pult": {"VOLTA_DEMO_FREELAB": "1", "VOLTA_DEMO_DESK": "batterie2,wechselquelle,generator,kennlinienschreiber",
             "VOLTA_SCREENSHOT_S": "4"},
    "messen": {"VOLTA_DEMO_FREELAB": "1", "VOLTA_DEMO_GALLERY": "5", "VOLTA_SCREENSHOT_S": "5"},
    "oszi": {"VOLTA_DEMO_FREELAB": "1", "VOLTA_DEMO_GALLERY": "6", "VOLTA_SCREENSHOT_S": "6"},
    "ton": {"VOLTA_DEMO_FREELAB": "1", "VOLTA_DEMO_GALLERY": "8", "VOLTA_SCREENSHOT_S": "5"},
}
HELP2_KEYS = ["willkommen", "stecken", "pult", "messen", "oszilloskop", "ton"]
HELP2_JOIN = {"stecken", "oszilloskop"}


def _help2_panels(key):
    if key == "willkommen":   # Startseite: freies Labor, Forscherheft, Lernpfad mit Stufe 0
        return [("start", (6, 240, 1270, 710), [("box", (342, 249, 642, 353)), ("box", (24, 371, 324, 475)),
                                                  ("box", (24, 543, 1256, 708))])]
    if key == "stecken":      # Gruppe »Messen« im Bauteilfach -> Zielfelder auf der Grundplatte
        return [("hilfe", (0, 180, 790, 440), [("box", (3, 289, 125, 363)), ("box", (272, 200, 420, 240)),
                                               ("arrow", (124, 326), (300, 246))])]
    if key == "pult":         # Batterie 2, Wechselquelle, Generator, Kennlinie; Plus fuers Geraetefach
        return [("pult", (272, 72, 1212, 272), [("box", (403, 77, 537, 269)), ("box", (665, 77, 799, 269)),
                                                ("box", (928, 184, 980, 236))])]
    if key == "messen":       # V A Ohm, Messstelle V1 auf der Grundplatte, Doppelmessgeraet
        return [("messen", (660, 0, 732, 58), [("box", (664, 2, 728, 54))]),
                ("messen", (272, 262, 432, 590), [("box", (296, 462, 394, 530))]),
                ("messen", (1100, 262, 1262, 582), [])]
    if key == "oszilloskop":  # Tastkoepfe K1, K2 -> Kurven-Knopf -> Schirm
        return [("oszi", (430, 196, 622, 252), [("box", (447, 201, 489, 239)), ("box", (559, 201, 601, 239))]),
                ("oszi", (790, 0, 854, 58), [("box", (796, 2, 848, 54))]),
                ("oszi", (258, 564, 548, 798), [])]
    if key == "ton":          # Lautsprecher mit Tonanzeige, Schildkroete mit Stufe
        return [("ton", (540, 84, 662, 266), [("box", (548, 150, 654, 180))]),
                ("ton", (850, 0, 936, 58), [("box", (856, 2, 932, 54))])]
    raise ValueError(key)


def app_images_volta2():
    """Kapitelbilder der Hilfe ab Volta 2 je Sprache (Bildschirmfotos aus volta2.exe)."""
    APP_HELP2.mkdir(parents=True, exist_ok=True)
    for lang in ("de", "en", "fr", "lb"):
        folder = OUT / "bilder" / f"volta2_{lang}"
        folder.mkdir(parents=True, exist_ok=True)
        shots = {}
        for name, extra in HELP2_SCENES.items():
            target = folder / f"{name}.png"
            env = dict(os.environ)
            env.update({"VOLTA_NO_SPLASH": "1", "VOLTA_DEMO_LANG": lang, "VOLTA_SCREENSHOT": str(target)})
            env.update(extra)
            subprocess.run([str(EXE2)], env=env, timeout=60, check=False,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            shots[name] = PILImage.open(target).convert("RGB")
        size = 0
        for key in HELP2_KEYS:
            dst = APP_HELP2 / f"{key}_{lang}.jpg"
            compose(_help2_panels(key), shots, dst, HELP_SIZE, key in HELP2_JOIN)
            size += dst.stat().st_size
        print(f"  Hilfebilder Volta 2 {lang}: {size // 1024} KB")


def main() -> int:
    global S
    if "--app-bilder" in sys.argv:
        app_images()
        return 0
    if "--app-bilder-volta2" in sys.argv:
        app_images_volta2()
        return 0
    take_screenshots("--bilder" in sys.argv)
    missing = [n for n in SCENES if not (SHOTS / f"{n}.png").exists()]
    if missing:
        print("Bilder fehlen:", ", ".join(missing))
        return 1
    if not all((OUT / "bilder" / f"_schritt{n}_de.png").exists() for n in range(1, GUIDE_STEPS + 1)):
        app_images()
    fonts()
    S = styles()
    long_manual(OUT / "Volta1-Anleitung-de.pdf")
    short_manual(OUT / "Volta1-Kurzanleitung-de.pdf")
    print("Anleitungen:", OUT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
