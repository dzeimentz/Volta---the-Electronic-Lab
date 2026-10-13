#!/usr/bin/env python3
"""Anleitungen als PDF fuer Kinder, Eltern und Lehrkraefte (deutsch):

    python3 tools/manual_pdfs.py            # beide PDFs nach Release/Anleitung/
    python3 tools/manual_pdfs.py --bilder   # vorher die Bildschirmfotos neu aufnehmen
    python3 tools/manual_pdfs.py --app-bilder   # nur die Bilder der Kurzanleitung in der App (QuickGuide.qml)

- Volta1-Anleitung-de.pdf: ausfuehrliche Anleitung (alle Bereiche der App, Elternbereich, Sicherheit, Bauteile)
- Volta1-Kurzanleitung-de.pdf: zwei Seiten zum Ausdrucken (erstes Experiment, Bedienung auf einen Blick)

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
    (1, (66, 12)), (2, (282, 12)), (3, (412, 12)), (4, (500, 12)), (5, (577, 12)), (6, (663, 12)),
    (7, (724, 12)), (8, (783, 12)), (9, (838, 12)), (10, (990, 28)), (11, (195, 140)), (12, (905, 95)),
    (13, (870, 215)), (14, (640, 480)), (15, (918, 330)), (16, (875, 465)), (17, (1255, 130)),
    (18, (1255, 712)), (19, (700, 760)),
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
              P("Teilen sich mehrere Kinder ein Gerät, können Eltern oder Lehrkräfte im Elternbereich "
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
                     ["6", "V A Ω", "Messgerät ein- und ausblenden"],
                     ["7", "Schaltzeichen", "Schaltplan des Aufbaus zeigen"],
                     ["8", "Kurve", "Zeitschreiber (ab Stufe 3)"],
                     ["9", "Farbpunkt", "Farbe für neue Leitungen"],
                     ["10", "A− A+, Flagge, ?, i, Menü", "Schriftgröße, Sprache, Hilfe, Info, Einstellungen"],
                     ["11", "Bauteilfach / Modulfach", "Bauteile und Module zum Herausziehen"],
                     ["12", "Bedienpult", "Batterie mit Hauptschalter und Spannungswahl, Drehregler P1 und P2, "
                      "Schalter S, Lautsprecher"],
                     ["13", "Grundplatte", "Kontaktfelder A1 bis E12 – hier wird gesteckt"],
                     ["14", "Modulleiste", "Plätze M1 bis M16 für Module (z. B. das Blinkmodul)"],
                     ["15", "+ und −", "mehr oder weniger Felder je Reihe"],
                     ["16", "Mülleimer", "Ausgewähltes entfernen oder alles abbauen"],
                     ["17", "Experimentfeld", "Frage, Vermutung, Aufbau, Tipps – einklappbar mit »"],
                     ["18", "Fertig", "wenn die Schaltung funktioniert"],
                     ["19", "Licht, Wärme, Wasser", "Taschenlampe, Föhn, Wassertropfen und Raumlicht für "
                      "Sensoren (ab Stufe 3)"]], [12, 48, 110])]
    story += [P("Die Grundplatte", "h2"),
              P("Jedes Kontaktfeld hat fünf Steckplätze. <b>Alles, was im selben Feld steckt, ist miteinander "
                "verbunden.</b> Die Felder heißen nach Reihe und Spalte: A1, A2 … bis E12. Über der Grundplatte "
                "liegt die Pultleiste mit den Anschlüssen des Bedienpults:"),
              table([["Feld", "führt zu"],
                     ["+9V und 0V", "Plus- und Minuspol der Batterie (die Beschriftung zeigt die gewählte "
                      "Spannung: 3 V, 4,5 V oder 9 V)"],
                     ["P1a, P1s, P1b / P2a, P2s, P2b", "Drehregler P1 und P2: die beiden Enden (a, b) und der "
                      "Schleifer (s)"],
                     ["S1, S2", "Schalter S"],
                     ["LS1, LS2", "Lautsprecher"]], [55, 115]),
              P("Mit den runden Tasten <b>+</b> und <b>−</b> rechts neben der Grundplatte wählst du 8, 10 oder "
                "12 Felder je Reihe. Ist im Menü »Steckfeld-Breite automatisch« eingeschaltet, passt Volta die "
                "Breite selbst an: im Lernpfad 8, im freien Labor 12 Felder; mit dem Messgerät daneben 10, mit dem "
                "Messgerät im Pult 12.")]

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
                      "die Farbe mit dem Farbpunkt oben (9).",
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
              tick_box("erklaerend", "<b>Merke:</b> Strom braucht einen geschlossenen Weg – von +9V durch die "
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
                "nächste auf. Eltern und Lehrkräfte können im Elternbereich alles sofort freigeben."),
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
    story += bullets(["Langer Druck auf ein Feld öffnet das Feldmenü. Dort schaltest du eine <b>Spannungsanzeige</b> "
                      "ein: Bis zu vier grüne Balken zeigen, wie viel Spannung an diesem Feld anliegt.",
                      "Kleine gelbe Pfeile auf Leitungen und Bauteilen zeigen, <b>wohin der Strom fließt</b> "
                      "(abschaltbar im Menü)."])
    story += [P("Schaltplan", "h2"),
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
                      "<b>Kein geschlossener Weg?</b> Führt ein Weg von +9V durch die Bauteile zurück nach 0V?",
                      "<b>Leuchtdiode falsch herum?</b> Leuchtdioden haben eine Richtung: Das lange Bein kommt zu "
                      "Plus.",
                      "<b>Ein Bein steckt allein in einem Feld?</b> Dann ist es mit nichts verbunden.",
                      "<b>Bauteil durchgebrannt?</b> Zu viel Strom – meist fehlt ein Vorwiderstand. Mit "
                      "»Rückgängig« oder »Reparieren« im Bauteilmenü ist es wieder heil.",
                      "<b>Sicherung durchgebrannt?</b> Ein Kurzschluss: Plus und Minus sind direkt verbunden. "
                      "Kurzschluss beseitigen und »Sicherung tauschen« an der Batterie antippen."])

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
                     ["Elternbereich …", "Einstellungen für Erwachsene (mit PIN)"]], [58, 112]),
              shot("hilfe", 140, "Die Hilfe mit Tick: Kapitel oben antippen", crop=(230, 0, 1050, 800))]

    # 11 Eltern und Lehrkraefte
    story += chapter("11  Für Eltern und Lehrkräfte")
    story += [P("Der Elternbereich", "h2"),
              P("Den Elternbereich öffnen Sie über das Menü (drei Striche) → »Elternbereich …« oder in der Hilfe "
                "unter »Für Erwachsene«. Er ist durch eine PIN geschützt; die <b>Start-PIN ist 1234</b>. Bitte "
                "legen Sie gleich eine eigene PIN mit 4 bis 8 Ziffern fest. Die PIN schützt auch alle Links aus "
                "der App heraus. Am Computer lässt sie sich mit der Tastatur eingeben."),
              shot("elternbereich", 150, "Der Elternbereich (Ausschnitt)", crop=(250, 0, 1030, 800)),
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
              P("Wählen Sie im Elternbereich unter »Aufgabe für die Klasse« Experimente aus dem Lernpfad und/oder "
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
                "freigegebene Stufen, Ton und die Zahl der Forscherprofile vorgeben. Solche Einstellungen sind im "
                "Elternbereich gekennzeichnet und lassen sich dort nicht ändern."),
              P("Datenschutz", "h2"),
              P("Volta erhebt keine Daten. Es gibt kein Konto, keine Werbung, keine Käufe in der App und keine "
                "Verbindung ins Internet; Kamera und Mikrofon werden nicht verwendet. Fortschritt, Forscherheft, "
                "Fotos, Aufbau und Einstellungen liegen nur im privaten Speicher der App auf dem Gerät und werden "
                "beim Deinstallieren gelöscht."),
              P("So rechnet Volta", "h2"),
              P("Was in Volta passiert, ergibt sich aus einer echten Schaltungssimulation, nicht aus vorbereiteten "
                "Ergebnissen. Deshalb verhalten sich auch selbst erfundene Schaltungen plausibel. Die wenigen "
                "Vereinfachungen (etwa beim Tonmodul oder bei Spulen) sind im Elternbereich unter »So rechnet "
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
              P("Kontakt", "h2"),
              P("DC Software Engineering, Daniel Zeimentz, Aufm Rothenhügel 26, 54666 Irrel (Deutschland)<br/>"
                "Web: dc-software-engineering.com · E-Mail: info@dc-software-engineering.com"),
              tick_box("froehlich", "Viel Spaß beim Forschen! Und denk dran: Wer sich irrt, lernt am meisten.")]

    doc = Doc(path, "Anleitung")
    doc.multiBuild(story)


# ---------------------------------------------------------------- Kurzanleitung

def short_manual(path):
    story = [P("Kurzanleitung", "h1"),
             P("In sechs Schritten zum ersten Experiment", "h2")]
    annotated(SHOTS / "aufbauhilfe.png", OUT / "bilder" / "_kurz_nummern.png", QUICK_MARKS)
    w, h = PILImage.open(OUT / "bilder" / "_kurz_nummern.png").size
    story += [_frame(Image(str(OUT / "bilder" / "_kurz_nummern.png"), width=170 * mm, height=170 * mm * h / w)),
              Spacer(1, 6)]
    steps = [["1", "<b>Bauteil holen</b> – Ziehe ein Bauteil aus dem Bauteilfach auf die Grundplatte. Alles im "
                   "selben Kontaktfeld ist verbunden. Zweimal tippen dreht ein Bauteil."],
             ["2", "<b>Leitungen ziehen</b> – Ziehe mit dem Finger von Feld zu Feld. Plus wird rot, Minus schwarz."],
             ["3", "<b>Schritt für Schritt</b> – Lies die Frage, wähle deine Vermutung und lass dir mit »Schritt "
                   "für Schritt bauen« jedes Teil zeigen. Die Zielfelder blinken blau."],
             ["4", "<b>Einschalten</b> – Tippe auf »Stecken«: Die Taste wird zu »Betrieb«, Strom fließt. Zum "
                   "Umbauen wieder antippen."],
             ["5", "<b>Fertig</b> – Funktioniert es, tippe auf »Fertig« und wähle, was du beobachtet hast. Tick "
                   "erklärt dir, was passiert ist."],
             ["6", "<b>Hilfe</b> – Kommst du nicht weiter: »Ich brauche einen Tipp«. Ist etwas kaputt: "
                   "»Rückgängig« macht es wieder heil."]]
    t = Table([[P(f"<font size=14><b>{n}</b></font>", "center"), P(txt, "cell")] for n, txt in steps],
              colWidths=[11 * mm, 159 * mm])
    t.setStyle(TableStyle([("BACKGROUND", (0, 0), (0, -1), YELLOW), ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
                           ("GRID", (0, 0), (-1, -1), 0.5, colors.HexColor("#C9D1DA")),
                           ("TOPPADDING", (0, 0), (-1, -1), 3), ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]))
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
                     ["V A Ω", "Messgerät (Spannung, Strom, Widerstand)"],
                     ["Schaltzeichen", "Schaltplan"],
                     ["Kurve", "Zeitschreiber (ab Stufe 3)"],
                     ["A− A+ · Flagge", "Schriftgröße · Sprache (DE, FR, EN, LU)"],
                     ["? · i · drei Striche", "Hilfe mit Tick · Info · Einstellungen und Elternbereich"]],
                    [45, 125]),
              tick_box("warnend", "<b>Mit echten Bauteilen:</b> nur Batterien bis 9 Volt, nie an die Steckdose! "
                       "Versuche mit »Nur in der App!« nicht in echt nachbauen. Bauteile können heiß werden.",
                       bg=WARN, border=colors.HexColor("#E57373")),
              P("Für Eltern und Lehrkräfte", "h2")]
    story += bullets(["Elternbereich: Menü (drei Striche) → »Elternbereich …«, <b>Start-PIN 1234</b> – bitte gleich "
                      "ändern.",
                      "Dort: Freigaben, Forscherprofile, Ton im Klassenzimmer, Aufgaben per QR-Code oder Code zum "
                      "Eintippen, Präsentationsmodus.",
                      "Kein Internet, kein Konto, keine Werbung, keine Datenerhebung.",
                      "Ausführliche Anleitung: Volta1-Anleitung-de.pdf"])
    doc = Doc(path, "Kurzanleitung")
    doc.addPageTemplates([])
    doc.pageTemplates = [doc.pageTemplates[1]]
    doc.build(story)


# Kurzanleitung: Nummern der sechs Schritte im Bild »aufbauhilfe« (1280 x 800); 1 zeigt allgemein auf das Bauteilfach,
# weil dessen Reihenfolge von der Sprache abhaengt
QUICK_MARKS = [(1, (196, 140)), (2, (222, 205)), (3, (1255, 455)), (4, (412, 12)), (5, (1255, 712)), (6, (1255, 562))]
APP_GUIDE = ROOT / "app" / "resources" / "guide"


def app_images():
    """Bild der Kurzanleitung in der App je Sprache: app/resources/guide/kurz_<sprache>.jpg (1024 x 640)."""
    APP_GUIDE.mkdir(parents=True, exist_ok=True)
    for lang in ("de", "en", "fr", "lb"):
        raw = OUT / "bilder" / f"_app_{lang}.png"
        env = dict(os.environ)
        env.update({"VOLTA_NO_SPLASH": "1", "VOLTA_DEMO_LANG": lang, "VOLTA_SCREENSHOT": str(raw)})
        env.update(SCENES["aufbauhilfe"])
        subprocess.run([str(EXE)], env=env, timeout=60, check=False,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        annotated(raw, raw, QUICK_MARKS)
        PILImage.open(raw).convert("RGB").resize((1024, 640), PILImage.LANCZOS).save(
            APP_GUIDE / f"kurz_{lang}.jpg", quality=82, optimize=True)
        print(f"  App-Bild {lang}: {(APP_GUIDE / f'kurz_{lang}.jpg').stat().st_size // 1024} KB")


def main() -> int:
    global S
    if "--app-bilder" in sys.argv:
        app_images()
        return 0
    take_screenshots("--bilder" in sys.argv)
    missing = [n for n in SCENES if not (SHOTS / f"{n}.png").exists()]
    if missing:
        print("Bilder fehlen:", ", ".join(missing))
        return 1
    fonts()
    S = styles()
    long_manual(OUT / "Volta1-Anleitung-de.pdf")
    short_manual(OUT / "Volta1-Kurzanleitung-de.pdf")
    print("Anleitungen:", OUT)
    return 0


if __name__ == "__main__":
    sys.exit(main())
