#!/usr/bin/env python3
"""Uebersetzungsbogen als PDF fuer Menschen, die nicht im Repository arbeiten (docs/i18n.md).

Aufruf (aus dem Repository-Wurzelverzeichnis):
    python3 tools/translation_pdfs.py            # erzeugt beide PDFs in Release/Uebersetzung/
    python3 tools/translation_pdfs.py --import-lb <ausgefuellt.pdf>   # luxemburgische Eingaben uebernehmen

1. Volta1-Texte-LU.pdf: zwei Spalten - links der deutsche Text, rechts ein Eingabefeld fuer Luxemburgisch. Die Felder
   lassen sich in jedem PDF-Programm mit Formularen (Acrobat Reader, Edge, Firefox, ...) ausfuellen und speichern.
   Vorhandene Uebersetzungen sind vorbelegt.
2. Volta1-Texte-DE-EN-FR.pdf: drei Spalten Deutsch, Englisch, Franzoesisch zum Gegenlesen (Anmerkungen mit der
   Kommentarfunktion des PDF-Programms).

Quellen: Inhalte content/volta1/i18n/<sprache>.json (Experimente, Stufen, Lexikon) und Oberflaeche
i18n/volta_<sprache>.ts (Quelltext Deutsch). Gleiche Oberflaechentexte erscheinen nur einmal.
Feldnamen: "c_<schluessel>" (Inhalte) bzw. "u_<sha1 des deutschen Textes>" (Oberflaeche); --import-lb schreibt
content/volta1/i18n/lb.json und traegt die Oberflaechentexte in i18n/volta_lb.ts ein.
"""
from __future__ import annotations

import hashlib
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONTENT = ROOT / "content" / "volta1" / "i18n"
TS = ROOT / "i18n"
OUT = ROOT / "Release" / "Uebersetzung"
FONTS = Path("C:/Windows/Fonts")
LETTERS = re.compile(r"[A-Za-zÄÖÜäöüß]{2,}")


def content_texts(lang: str) -> dict[str, str]:
    path = CONTENT / f"{lang}.json"
    return json.loads(path.read_text(encoding="utf-8")) if path.exists() else {}


def ui_texts(lang: str) -> dict[str, str]:
    """deutscher Quelltext -> Uebersetzung (nur fertige; ohne verschwundene Eintraege)."""
    out: dict[str, str] = {}
    for ctx in ET.parse(TS / f"volta_{lang}.ts").getroot().findall("context"):
        for m in ctx.findall("message"):
            tr = m.find("translation")
            src = m.find("source").text or ""
            if tr.get("type") in ("vanished", "obsolete"):
                continue
            out.setdefault(src, "" if tr.get("type") == "unfinished" else (tr.text or ""))
    return out


def ui_sources() -> list[str]:
    seen: list[str] = []
    for ctx in ET.parse(TS / "volta_de.ts").getroot().findall("context"):
        for m in ctx.findall("message"):
            tr = m.find("translation")
            src = m.find("source").text or ""
            if tr.get("type") in ("vanished", "obsolete") or not LETTERS.search(src) or src in seen:
                continue
            seen.append(src)
    return seen


def ui_field(src: str) -> str:
    return "u_" + hashlib.sha1(src.encode("utf-8")).hexdigest()[:12]


def sections() -> list[tuple[str, list[tuple[str, str, str]]]]:
    """[(Ueberschrift, [(feldname, kennung, deutscher Text)])]"""
    de = content_texts("de")
    groups: dict[str, list] = {"Stufen": [], "Experimente": [], "Lexikon": []}
    for key, text in de.items():
        kind = "Stufen" if key.startswith("stufe") else "Lexikon" if key.startswith("lex") else "Experimente"
        groups[kind].append(("c_" + key, key, text))
    ui = [(ui_field(s), "Oberfläche", s) for s in ui_sources()]
    return [("Teil A – Stufen", groups["Stufen"]), ("Teil B – Experimente", groups["Experimente"]),
            ("Teil C – Bauteillexikon", groups["Lexikon"]), ("Teil D – Bedienoberfläche", ui)]


# ------------------------------------------------------------------ PDF
def setup_fonts():
    from reportlab.pdfbase import pdfmetrics
    from reportlab.pdfbase.ttfonts import TTFont
    pdfmetrics.registerFont(TTFont("Segoe", str(FONTS / "segoeui.ttf")))
    pdfmetrics.registerFont(TTFont("SegoeB", str(FONTS / "segoeuib.ttf")))


def esc(text: str) -> str:
    return text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\n", "<br/>")


class Sheet:
    """Seitenaufbau mit Kopf, Fusszeile und Zeilen, die nicht ueber den Seitenrand laufen."""

    def __init__(self, path: Path, title: str, intro: str):
        from reportlab.lib.pagesizes import A4
        from reportlab.pdfgen import canvas
        self.c = canvas.Canvas(str(path), pagesize=A4)
        self.c.setTitle(title)
        self.c.setAuthor("DC Software Engineering")
        self.w, self.h = A4
        self.margin = 36
        self.title = title
        self.page = 0
        self.new_page()
        self.y -= self.paragraph(intro, 10, self.w - 2 * self.margin, self.margin, self.y) + 10

    def new_page(self):
        if self.page:
            self.c.showPage()
        self.page += 1
        c = self.c
        c.setFillColorRGB(0.04, 0.15, 0.25)
        c.rect(self.margin, self.h - self.margin - 22, self.w - 2 * self.margin, 22, fill=1, stroke=0)
        c.setFillColorRGB(1, 1, 1)
        c.setFont("SegoeB", 11)
        c.drawString(self.margin + 8, self.h - self.margin - 15, self.title)
        c.drawRightString(self.w - self.margin - 8, self.h - self.margin - 15, f"Seite {self.page}")
        self.y = self.h - self.margin - 34

    def style(self, size: float, bold=False, color=(0.12, 0.17, 0.22)):
        from reportlab.lib.colors import Color
        from reportlab.lib.styles import ParagraphStyle
        return ParagraphStyle("s", fontName="SegoeB" if bold else "Segoe", fontSize=size, leading=size * 1.25,
                              textColor=Color(*color))

    def para(self, text: str, size: float, bold=False, color=(0.12, 0.17, 0.22)):
        from reportlab.platypus import Paragraph
        return Paragraph(esc(text), self.style(size, bold, color))

    def paragraph(self, text, size, width, x, y, bold=False, color=(0.12, 0.17, 0.22)) -> float:
        p = self.para(text, size, bold, color)
        _, h = p.wrap(width, 10000)
        p.drawOn(self.c, x, y - h)
        return h

    def need(self, h: float):
        if self.y - h < self.margin + 10:
            self.new_page()

    def heading(self, text: str):
        self.need(40)
        self.y -= 8
        self.y -= self.paragraph(text, 13, self.w - 2 * self.margin, self.margin, self.y, bold=True,
                                 color=(0.04, 0.15, 0.25)) + 6

    def save(self):
        self.c.save()


def make_lb(path: Path):
    sheet = Sheet(path, "Volta 1 – Texte auf Luxemburgisch",
                  "Bitte tragen Sie rechts neben jedem deutschen Text die luxemburgische Fassung ein und speichern Sie "
                  "die Datei (z. B. mit Acrobat Reader: Datei › Speichern). Platzhalter wie %1 und %2 stehen für "
                  "Zahlen oder Namen und bleiben unverändert. Leere Felder zeigt Volta auf Deutsch an. Die kleine graue "
                  "Kennung dient nur der Zuordnung.")
    c = sheet.c
    lb_content = content_texts("lb")
    lb_ui = ui_texts("lb")
    col = (sheet.w - 2 * sheet.margin - 12) / 2
    for title, rows in sections():
        sheet.heading(f"{title} ({len(rows)} Texte)")
        for field, key, text in rows:
            p = sheet.para(text, 9.5)
            _, h = p.wrap(col - 6, 10000)
            k = sheet.para(key, 6.5, color=(0.5, 0.5, 0.5))
            _, kh = k.wrap(col - 6, 100)
            row = max(h + kh + 6, 22)
            sheet.need(row + 4)
            y = sheet.y
            k.drawOn(c, sheet.margin + 3, y - kh)
            p.drawOn(c, sheet.margin + 3, y - kh - 2 - h)
            prefill = lb_content.get(key, "") if field.startswith("c_") else lb_ui.get(text, "")
            c.acroForm.textfield(name=field, tooltip=key if field.startswith("c_") else text[:80],
                                 x=sheet.margin + col + 12, y=y - row + 2, width=col, height=row - 2,
                                 fontName="Helvetica", fontSize=9, borderWidth=0.6,
                                 borderColor=_rgb(0.61, 0.56, 0.47), fillColor=_rgb(1, 0.99, 0.95),
                                 textColor=_rgb(0.1, 0.1, 0.1), forceBorder=True, maxlen=4000,
                                 fieldFlags="multiline" if row > 24 else "", value=prefill)
            c.setStrokeColorRGB(0.9, 0.88, 0.84)
            c.line(sheet.margin, y - row - 1, sheet.w - sheet.margin, y - row - 1)
            sheet.y = y - row - 4
    sheet.save()


def make_de_en_fr(path: Path):
    sheet = Sheet(path, "Volta 1 – Texte Deutsch · Englisch · Französisch",
                  "Zum Gegenlesen: links der deutsche Ausgangstext, daneben Englisch und Französisch, wie sie in der "
                  "App erscheinen. Korrekturen bitte als Kommentar oder Hervorhebung im PDF-Programm anbringen. %1, %2 "
                  "sind Platzhalter für Zahlen oder Namen.")
    c = sheet.c
    en_c, fr_c = content_texts("en"), content_texts("fr")
    en_u, fr_u = ui_texts("en"), ui_texts("fr")
    gap = 8
    col = (sheet.w - 2 * sheet.margin - 2 * gap) / 3
    xs = [sheet.margin + i * (col + gap) for i in range(3)]
    for title, rows in sections():
        sheet.heading(f"{title} ({len(rows)} Texte)")
        sheet.need(20)
        for i, name in enumerate(("Deutsch", "English", "Français")):
            sheet.paragraph(name, 9, col, xs[i], sheet.y, bold=True, color=(0.16, 0.45, 0.69))
        sheet.y -= 16
        for field, key, text in rows:
            if field.startswith("c_"):
                cells = [text, en_c.get(key, ""), fr_c.get(key, "")]
            else:
                cells = [text, en_u.get(text, ""), fr_u.get(text, "")]
            paras = [sheet.para(t if t else "– fehlt –", 8.5, color=(0.12, 0.17, 0.22) if t else (0.7, 0.2, 0.1))
                     for t in cells]
            k = sheet.para(key, 6, color=(0.5, 0.5, 0.5))
            _, kh = k.wrap(col, 100)
            hs = [p.wrap(col, 10000)[1] for p in paras]
            row = max(hs) + kh + 6
            sheet.need(row + 2)
            y = sheet.y
            k.drawOn(c, xs[0], y - kh)
            for i, p in enumerate(paras):
                p.drawOn(c, xs[i], y - kh - 2 - hs[i])
            c.setStrokeColorRGB(0.9, 0.88, 0.84)
            c.line(sheet.margin, y - row, sheet.w - sheet.margin, y - row)
            sheet.y = y - row - 3
    sheet.save()


def _rgb(r, g, b):
    from reportlab.lib.colors import Color
    return Color(r, g, b)


# ------------------------------------------------------------------ Import
def import_lb(pdf: Path) -> int:
    from pypdf import PdfReader
    fields = PdfReader(str(pdf)).get_fields() or {}
    values = {name: (f.get("/V") or "").strip() for name, f in fields.items()}
    de = content_texts("de")
    lb = content_texts("lb")
    changed = 0
    for key in de:
        v = values.get("c_" + key, "")
        if v and lb.get(key) != v:
            lb[key] = v
            changed += 1
    ordered = {k: lb[k] for k in de if k in lb}
    (CONTENT / "lb.json").write_text(json.dumps(ordered, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    # Oberflaeche: alle Kontexte mit gleichem Quelltext
    path = TS / "volta_lb.ts"
    tree = ET.parse(path)
    for ctx in tree.getroot().findall("context"):
        for m in ctx.findall("message"):
            src = m.find("source").text or ""
            v = values.get(ui_field(src), "")
            tr = m.find("translation")
            if not v or tr.get("type") in ("vanished", "obsolete"):
                continue
            if tr.text != v or tr.get("type") == "unfinished":
                tr.text = v
                tr.attrib.pop("type", None)
                changed += 1
    tree.write(path, encoding="utf-8", xml_declaration=True)
    s = path.read_text(encoding="utf-8").replace("<?xml version='1.0' encoding='utf-8'?>",
                                                 '<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE TS>', 1)
    path.write_text(s if s.endswith("\n") else s + "\n", encoding="utf-8", newline="\n")
    return changed


def main() -> int:
    if len(sys.argv) >= 3 and sys.argv[1] == "--import-lb":
        n = import_lb(Path(sys.argv[2]))
        print(f"{n} luxemburgische Texte uebernommen (content/volta1/i18n/lb.json, i18n/volta_lb.ts)")
        return 0
    setup_fonts()
    OUT.mkdir(parents=True, exist_ok=True)
    make_lb(OUT / "Volta1-Texte-LU.pdf")
    make_de_en_fr(OUT / "Volta1-Texte-DE-EN-FR.pdf")
    n = sum(len(r) for _, r in sections())
    print(f"{n} Texte -> {OUT / 'Volta1-Texte-LU.pdf'} und {OUT / 'Volta1-Texte-DE-EN-FR.pdf'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
