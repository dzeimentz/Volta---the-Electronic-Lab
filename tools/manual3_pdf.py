#!/usr/bin/env python3
"""Anleitung Volta 3 als PDF (deutsch) fuer Kinder, Eltern und Lehrkraefte:

    python3 tools/manual3_pdf.py            # Release/Anleitung/Volta3-Anleitung-de.pdf
    python3 tools/manual3_pdf.py --bilder   # vorher die Bildschirmfotos neu aufnehmen (wie der Flyer)

Beschreibt, was in Volta 3 neu ist (die Bedienung aus Volta 1 und 2 bleibt gleich und steht in der Hilfe der App):
Frequenzgang-Schreiber, Lupe, Logikanalysator, Generator mit Modulation und Wobbeln, Senderanlage, Werkzeugkasten,
Lernpfad (Stufen aus content/volta3), Sicherheit und der Teil fuer Erwachsene. Bilder: dieselben Bildschirmfotos wie
der Flyer (tools/flyer3_pdf.py, Release/Flyer/bilder-volta3/de). Benoetigt reportlab, Pillow und Segoe UI.
"""
from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image as PILImage
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.platypus import (Image, KeepTogether, PageBreak, Paragraph, SimpleDocTemplate, Spacer, Table,
                                TableStyle)

sys.path.insert(0, str(Path(__file__).resolve().parent))
import flyer_pdf as base  # noqa: E402
import flyer3_pdf as flyer  # noqa: E402

ROOT = base.ROOT
OUT = ROOT / "Release" / "Anleitung"
NAVY = base.NAVY
PURPLE = flyer.PURPLE
TICK = base.TICK

H1 = ParagraphStyle("h1", fontName="Segoe-Bold", fontSize=18, leading=22, textColor=PURPLE, spaceBefore=6,
                    spaceAfter=6)
H2 = ParagraphStyle("h2", fontName="Segoe-Bold", fontSize=13, leading=16, textColor=NAVY, spaceBefore=8,
                    spaceAfter=4)
BODY = ParagraphStyle("body", fontName="Segoe", fontSize=10.5, leading=14.5, spaceAfter=4)
BULLET = ParagraphStyle("bullet", parent=BODY, leftIndent=12, bulletIndent=2, spaceAfter=2)
SMALL = ParagraphStyle("small", parent=BODY, fontSize=8.5, leading=11, textColor=base.GREY)
TITLE = ParagraphStyle("title", fontName="Segoe-Bold", fontSize=30, leading=34, textColor=PURPLE)
SUB = ParagraphStyle("sub", fontName="Segoe-Bold", fontSize=15, leading=19, textColor=NAVY)


def bullets(items):
    return [Paragraph(t, BULLET, bulletText="•") for t in items]


def picture(name, width=170 * mm):
    path = flyer.img(name)
    w, h = PILImage.open(path).size
    return Image(str(path), width, width * h / w)


def tick(mood, size=26 * mm):
    return Image(str(TICK / f"tick_{mood}.png"), size, size)


def says(mood, text):
    """Tick mit Sprechtext nebeneinander."""
    t = Table([[tick(mood), Paragraph(text, BODY)]], colWidths=[30 * mm, 140 * mm])
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
                           ("BACKGROUND", (1, 0), (1, 0), colors.HexColor("#EEF5FF")),
                           ("BOX", (1, 0), (1, 0), 0.8, base.BLUE),
                           ("LEFTPADDING", (1, 0), (1, 0), 8), ("RIGHTPADDING", (1, 0), (1, 0), 8),
                           ("TOPPADDING", (1, 0), (1, 0), 6), ("BOTTOMPADDING", (1, 0), (1, 0), 6)]))
    return t


def chapter(title, mood, say, points, image=None):
    out = [Paragraph(title, H1), says(mood, say), Spacer(1, 3 * mm)]
    out += bullets(points)
    if image:
        out += [Spacer(1, 3 * mm), picture(image)]
    return [KeepTogether(out[:3]), *out[3:]]


def footer(canvas, doc):
    canvas.saveState()
    canvas.setFont("Segoe", 8)
    canvas.setFillColor(base.GREY)
    canvas.drawString(18 * mm, 10 * mm, "Volta 3 – Das Elektronik-Labor · Anleitung · DC Software Engineering")
    canvas.drawRightString(A4[0] - 18 * mm, 10 * mm, str(doc.page))
    canvas.restoreState()


def story(ver: str):
    st, total = flyer.stages()
    s = [Spacer(1, 20 * mm), Paragraph("Volta 3", TITLE), Paragraph("Das Elektronik-Labor · Anleitung", SUB),
         Spacer(1, 6 * mm),
         Paragraph(f"Für 13 bis 16 Jahre · {total} Experimente in {len(st)} Stufen · Version {ver} (Entwicklung)", BODY),
         Spacer(1, 8 * mm),
         says("freundlich", "Hallo, ich bin Tick! Volta 3 baut auf Volta 1 und 2 auf. Stecken, Leitungen ziehen, "
                            "Messen, Oszilloskop und Generator funktionieren genauso. Hier erkläre ich dir, was neu "
                            "ist. Alles andere findest du in der Hilfe der App: Tippe auf das Fragezeichen."),
         Spacer(1, 6 * mm), picture("start", 150 * mm),
         Spacer(1, 6 * mm), Paragraph("Was ist neu?", H2)]
    s += bullets(["Der <b>Frequenzgang-Schreiber</b> zeigt, welche Frequenzen deine Schaltung durchlässt.",
                  "Die <b>Lupe</b> im Oszilloskop dehnt schnelle Vorgänge ×10, ×100 oder ×1000.",
                  "Der <b>Logikanalysator</b> zeigt bis zu acht digitale Signale untereinander.",
                  "Der <b>Generator</b> kann einen Ton aufprägen (Modulation) und seine Frequenz durchlaufen lassen "
                  "(Wobbeln).",
                  "Die <b>Senderanlage</b> im Pult sendet eigene Aufnahmen – dein erstes Radio empfängt sie.",
                  "Neue Bauteile und Module: Drehkondensator, Spulen, Thyristor, Triac, MOSFET, Sensoren, MW- und "
                  "UKW-Modul, Gatter-, Zähler-, Dekadenzähler- und Anzeige-Modul."])
    s.append(PageBreak())
    s += chapter("Frequenzgang-Schreiber", "neugierig",
                 "Welche Frequenzen lässt deine Schaltung durch? Der Frequenzgang-Schreiber zeigt es dir für alle "
                 "Frequenzen auf einmal.",
                 ["Verbinde den Generator G mit dem Eingang deiner Schaltung. Stecke den Tastkopf K1 an den Ausgang.",
                  "Tippe in der Werkzeugleiste auf »dB«. Die Kurve reicht von 1 Hz bis 10 MHz. Oben ist viel "
                  "Spannung, unten wenig. »Phase« zeigt zusätzlich, wie weit der Ausgang dem Eingang hinterherläuft.",
                  "Die Marken f₀ und fg zeigen die Resonanz und die Grenzfrequenz. Rechts stehen sie als Zahl, dazu "
                  "Bandbreite und Güte.",
                  "Grenzfrequenz heißt: Dort ist die Spannung 3 dB unter dem Höchstwert, also etwa 71 %.",
                  "Schaltet deine Schaltung, statt gleichmäßig zu verstärken (zum Beispiel mit Leuchtdioden oder "
                  "Logik), zeige ich einen Hinweis statt einer Kurve. Miss solche Schaltungen mit dem Oszilloskop."],
                 "frequenzgang")
    s.append(PageBreak())
    s += chapter("Die Lupe", "erklaerend",
                 "Manches geht so schnell, dass du es im Oszilloskop nicht erkennen kannst. Dann nimm die Lupe.",
                 ["Tippe im Oszilloskop bei »Zeit« auf die Lupe: ×10, ×100, ×1000 und wieder aus.",
                  "Gedehnte Bilder haben einen gestrichelten Rahmen und tragen den Faktor, zum Beispiel »×10 gedehnt«.",
                  "Die Zeit auf dem Schirm bleibt die echte Zeit. Volta rechnet nur in feineren Schritten. Darum läuft "
                  "die Simulation mit Lupe langsamer als in Wirklichkeit."],
                 "lupe")
    s.append(PageBreak())
    s += chapter("Logikanalysator", "neugierig",
                 "In der Digitaltechnik zählt nur H oder L. Der Logikanalysator zeigt dir viele Signale untereinander.",
                 ["Stecke Logik-Tastköpfe LA1 bis LA8 auf Felder. Jeder bekommt eine Zeile. H liegt oben, L unten, "
                  "dazwischen das gestreifte Band »?«. Rechts steht der Pegel als Buchstabe.",
                  "»fein« zeigt 20 ms – da siehst du, wie ein Taster prellt. »grob« zeigt 5 s.",
                  "<b>Trigger:</b> Wähle je Kanal H, L, ↑ (steigende) oder ↓ (fallende Flanke). Mehrere Kanäle "
                  "zusammen sind ein Muster. Das Bild steht dann still, der Auslöser liegt bei T.",
                  "<b>Zeitmarken:</b> Tippe auf die Spuren für A und dann für B. Die Marken rasten an Flanken ein. "
                  "Oben steht der Abstand in µs oder ms und 1 ÷ Δt als Frequenz.",
                  "<b>Schwellen:</b> Ab welcher Spannung H oder L gilt, stellst du in Schritten von 10 % der "
                  "Batteriespannung ein. Sie gelten nur für die Anzeige – die Bausteine schalten bei ihren eigenen."],
                 "logik")
    s.append(PageBreak())
    s += chapter("Generator, Senderanlage und Radio", "motivierend",
                 "Der Generator kann in Volta 3 zwei neue Dinge, und im Pult steht eine kleine Senderanlage.",
                 ["<b>Modulation:</b> Der Regler macht die Kurve im Takt eines Tons (100, 200 oder 500 Hz) größer und "
                  "kleiner. So arbeitet ein Mittelwellensender.",
                  "<b>Wobbeln:</b> Die Frequenz steigt immer wieder vom eingestellten Wert bis zum 10-, 100- oder "
                  "1000-Fachen (höchstens 10 kHz), in 1, 5 oder 10 Sekunden. Hängt ein Schwingkreis am Generator, "
                  "wird die Kurve im Oszilloskop dort am größten, wo er in Resonanz ist.",
                  "<b>Senderanlage:</b> Hole sie aus dem Gerätefach ins Pult. Ihr Feld ANT ist die Antenne. Im "
                  "Senderfenster schaltest du die Sender ein und aus.",
                  "Die Sender spielen nur eigene Aufnahmen in Volta. Es gibt keinen Empfang echter Sender und keine "
                  "Verbindung ins Internet.",
                  "Mit Spule und Drehkondensator stimmst du ab. Der Drehknopf erscheint, wenn du den Drehkondensator "
                  "antippst."],
                 "radio")
    s.append(PageBreak())
    s.append(Paragraph("Werkzeugkasten", H1))
    s.append(says("erklaerend", "Im Werkzeugkasten liegen meine Helfer. In Volta 3 sind neue dazugekommen."))
    s.append(Paragraph("Formelrechner – immer mit Rechenweg", H2))
    s += bullets(["<b>Schwingkreis:</b> f₀ = 1 ÷ (2 · π · √(L · C)), dazu der Blindwiderstand bei f₀.",
                  "<b>Bandbreite und Güte:</b> B = fo − fu, f₀ = √(fu · fo), Q = f₀ ÷ B.",
                  "<b>Wirkungsgrad:</b> η = Pab ÷ Pzu und der Verlust, der zu Wärme wird.",
                  "<b>Pegel in dB:</b> 20 · lg(Ua ÷ Ue) – und zurück von dB zur Spannung.",
                  "<b>Tastverhältnis:</b> Anteil der Zeit oben, Frequenz und Mittelwert.",
                  "<b>Frequenzteiler:</b> f ÷ n und was an den Ausgängen von Zähler und Dekadenzähler herauskommt."])
    s.append(Paragraph("Lexikon", H2))
    s += bullets(["Alle Bauteile und Module mit Schaltzeichen.",
                  "Neu sind Begriffe: Modulation, Resonanz, Wirkungsgrad, Pegel, Flanke und Prellen."])
    s.append(Paragraph("Fehlersuche", H2))
    s.append(Paragraph("Tippe auf »Schaltung prüfen«. Ich stelle dir Fragen zu den Stellen, die mir auffallen – "
                       "die Lösung findest du selbst. In Volta 3 achte ich zusätzlich auf:", BODY))
    s += bullets(["einen Thyristor, der gezündet wird, aber weniger als den Haltestrom bekommt,",
                  "ein MOSFET-Gate, das nur an einem Taster oder Kondensator hängt,",
                  "eine Relais- oder Magnetspule, die ein Transistor schaltet, ohne Bauteil parallel zur Spule,",
                  "einen offenen Eingang am Gatter- oder Zähler-Modul,",
                  "einen Taster direkt am Takteingang eines Zählers (er prellt),",
                  "einen Schwingkreis direkt am Generator, ohne Widerstand dazwischen."])
    s.append(Paragraph("Zahlensysteme", H2))
    s.append(Paragraph("Dieselbe Zahl binär, dezimal und hexadezimal in einer Zeile.", BODY))
    s.append(PageBreak())
    s.append(Paragraph("Der Lernpfad", H1))
    rows = [["Stufe", "Thema", "Forscherfrage", "Exp."]]
    for nr, title, q, n in st:
        rows.append([nr, Paragraph(title, BODY), Paragraph(q, BODY), str(n)])
    t = Table(rows, colWidths=[14 * mm, 52 * mm, 90 * mm, 14 * mm], repeatRows=1)
    t.setStyle(TableStyle([("FONT", (0, 0), (-1, 0), "Segoe-Bold", 10), ("FONT", (0, 1), (-1, -1), "Segoe", 10),
                           ("BACKGROUND", (0, 0), (-1, 0), PURPLE), ("TEXTCOLOR", (0, 0), (-1, 0), colors.white),
                           ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, colors.HexColor("#F4EEF7")]),
                           ("VALIGN", (0, 0), (-1, -1), "MIDDLE"), ("ALIGN", (0, 0), (0, -1), "CENTER"),
                           ("ALIGN", (3, 0), (3, -1), "CENTER"),
                           ("GRID", (0, 0), (-1, -1), 0.4, colors.HexColor("#C9B6D3"))]))
    s += [t, Spacer(1, 4 * mm)]
    s.append(Paragraph("Die Stufen werden der Reihe nach freigeschaltet. Die meisten Stufen enden mit einem Projekt. Viele "
                       "Versuche bauen aufeinander auf: Mit »Aufbau übernehmen« setzt du einen fertigen Aufbau aus "
                       "dem letzten Experiment ein.", BODY))
    s.append(Paragraph("Sicher experimentieren", H1))
    s.append(says("warnend", "Mit echten Bauteilen gilt immer: Sicherheit zuerst!"))
    s += bullets(["Experimentiere nur mit Batterien bis 9 Volt und Kleinspannung – niemals an der Steckdose. Auch "
                  "der Lampendimmer in Volta 3 arbeitet nur mit Kleinspannung aus dem Pult.",
                  "Spulen und Relais erzeugen beim Abschalten hohe Spannungsspitzen. Fass die Anschlüsse nicht an.",
                  "Versuche mit »Nur in der App!« machen ein Bauteil kaputt. Die probierst du nur hier aus.",
                  "Bauteile können heiß werden – besonders Spannungsregler und Leistungstransistoren."])
    s.append(Paragraph("Für Eltern und Lehrkräfte", H1))
    s += bullets(["Kein Konto, keine Werbung, keine Käufe in der App, keine Datenerhebung, keine Internetverbindung.",
                  "Die Einstellungen liegen hinter einer PIN: Freigaben der Stufen, Profile, Ton im Klassenzimmer.",
                  "Bedienelemente ab 48 dp, Farbe ist nie die einzige Information, alle Texte können vorgelesen werden.",
                  "Sprachen: Deutsch, Français, English, Lëtzebuergesch.",
                  "Volta rechnet jede Schaltung wirklich aus. Die Abweichung zu den Formeln bleibt unter 2 %."])
    s += [Spacer(1, 6 * mm),
          Paragraph("DC Software Engineering · Daniel Zeimentz · Aufm Rothenhügel 26 · 54666 Irrel · Deutschland · "
                    "info@dc-software-engineering.com", SMALL)]
    return s


def main() -> int:
    base.fonts()
    base.LANG = "de"
    flyer.take_screenshots("--bilder" in sys.argv)
    missing = [n for n in ("start", "frequenzgang", "lupe", "logik", "radio") if not flyer.img(n).exists()]
    if missing:
        print("Bilder fehlen:", ", ".join(missing))
        return 1
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / "Volta3-Anleitung-de.pdf"
    doc = SimpleDocTemplate(str(path), pagesize=A4, leftMargin=20 * mm, rightMargin=20 * mm, topMargin=18 * mm,
                            bottomMargin=18 * mm, title="Volta 3 – Anleitung", author="DC Software Engineering")
    doc.build(story(base.version()), onFirstPage=footer, onLaterPages=footer)
    print("Anleitung:", path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
