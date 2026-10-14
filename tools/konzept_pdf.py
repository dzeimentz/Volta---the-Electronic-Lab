#!/usr/bin/env python3
"""Erzeugt ein Konzept-PDF aus einer Markdown-Quelle (z. B. docs/Konzept_Zuse.md).

Aufruf (aus dem Repository-Wurzelverzeichnis):
    python3 tools/konzept_pdf.py docs/Konzept_Zuse.md [ausgabe.pdf]

Ohne Ausgabe: <quelle ohne .md>_v<Version>.pdf neben der Quelle (Version aus der Metadatentabelle).
Verstanden wird der Markdown-Umfang der Konzepte: »# Titel«, Untertitel in _..._, Metadatentabelle (erste Tabelle
mit leerer Kopfzeile, wird zum Deckblatt), »##«/»###«-Ueberschriften (mit Inhaltsverzeichnis), Absaetze,
Aufzaehlungen (- und 1.), Tabellen, Bilder ![Unterschrift](pfad), Codebloecke (```), **fett** und `code`.
Layout nach der Konzeptvorlage (ReportLab): Deckblatt, Inhaltsverzeichnis, Kopf- und Fusszeile, Seitenzahlen.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_JUSTIFY, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import cm
from reportlab.lib.utils import ImageReader
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, Frame, HRFlowable, Image, PageBreak, PageTemplate,
                                Paragraph, Preformatted, Spacer, Table, TableStyle)
from reportlab.platypus.tableofcontents import TableOfContents

PRIMARY = colors.HexColor("#1A3A5C")
ACCENT = colors.HexColor("#2E7DBE")
LIGHT_GREY = colors.HexColor("#F4F4F4")
MID_GREY = colors.HexColor("#CCCCCC")
TEXT_DARK = colors.HexColor("#1C1C1C")
CONTENT_WIDTH = 17 * cm

# ---- Schriften: Arial/Consolas (Umlaute, ≥, Pfeile), Symbole aus Segoe UI Symbol; sonst Helvetica ----
FONT, FONT_BOLD, FONT_ITALIC, FONT_MONO, FONT_SYMBOL = "Helvetica", "Helvetica-Bold", "Helvetica-Oblique", "Courier", None
_WIN = Path("C:/Windows/Fonts")
if (_WIN / "arial.ttf").exists():
    pdfmetrics.registerFont(TTFont("Arial", str(_WIN / "arial.ttf")))
    pdfmetrics.registerFont(TTFont("Arial-Bold", str(_WIN / "arialbd.ttf")))
    pdfmetrics.registerFont(TTFont("Arial-Italic", str(_WIN / "ariali.ttf")))
    pdfmetrics.registerFont(TTFont("Arial-BoldItalic", str(_WIN / "arialbi.ttf")))
    pdfmetrics.registerFontFamily("Arial", normal="Arial", bold="Arial-Bold", italic="Arial-Italic",
                                  boldItalic="Arial-BoldItalic")
    FONT, FONT_BOLD, FONT_ITALIC = "Arial", "Arial-Bold", "Arial-Italic"
    if (_WIN / "consola.ttf").exists():
        pdfmetrics.registerFont(TTFont("Consolas", str(_WIN / "consola.ttf")))
        FONT_MONO = "Consolas"
    if (_WIN / "seguisym.ttf").exists():
        pdfmetrics.registerFont(TTFont("SegoeSymbol", str(_WIN / "seguisym.ttf")))
        FONT_SYMBOL = "SegoeSymbol"
SYMBOL_CHARS = "✓✗★◀▶●○"


def styles() -> dict[str, ParagraphStyle]:
    def ps(name, **kw):
        base = dict(fontName=FONT, fontSize=10, leading=14, textColor=TEXT_DARK)
        base.update(kw)
        return ParagraphStyle(name, **base)
    return {
        "cover_title": ps("cover_title", fontName=FONT_BOLD, fontSize=28, leading=34, textColor=PRIMARY, spaceAfter=6),
        "cover_sub": ps("cover_sub", fontSize=14, leading=18, textColor=ACCENT, spaceAfter=4),
        "h1": ps("h1", fontName=FONT_BOLD, fontSize=16, leading=20, textColor=PRIMARY, spaceBefore=16, spaceAfter=4,
                 keepWithNext=True),
        "h2": ps("h2", fontName=FONT_BOLD, fontSize=12.5, leading=16, textColor=PRIMARY, spaceBefore=10,
                 spaceAfter=4, keepWithNext=True),
        "body": ps("body", alignment=TA_JUSTIFY, spaceAfter=6),
        "bullet": ps("bullet", leftIndent=14, bulletIndent=4, spaceAfter=3),
        "caption": ps("caption", fontName=FONT_ITALIC, fontSize=8, leading=11, textColor=colors.grey,
                      alignment=TA_CENTER, spaceAfter=8),
        "th": ps("th", fontName=FONT_BOLD, fontSize=8.5, leading=11, textColor=colors.white),
        "td": ps("td", fontSize=8.5, leading=11),
        "code": ps("code", fontName=FONT_MONO, fontSize=8.5, leading=11, backColor=LIGHT_GREY, borderPadding=6,
                   leftIndent=6, rightIndent=6, spaceBefore=4, spaceAfter=8),
        "toc1": ps("toc1", fontName=FONT_BOLD, fontSize=10.5, leading=15, leftIndent=0),
        "toc2": ps("toc2", fontSize=9.5, leading=13, leftIndent=16),
    }


def inline(text: str) -> str:
    """Markdown-Inline nach ReportLab-Markup: Sonderzeichen maskieren, **fett**, `code`, Symbolschrift."""
    text = text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    text = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", text)
    text = re.sub(r"`([^`]+)`", lambda m: f'<font name="{FONT_MONO}">{m.group(1)}</font>', text)
    if FONT_SYMBOL:
        text = "".join(f'<font name="{FONT_SYMBOL}">{c}</font>' if c in SYMBOL_CHARS else c for c in text)
    return text


def split_row(line: str) -> list[str]:
    return [c.strip() for c in line.strip().strip("|").split("|")]


def make_table(rows: list[list[str]], st) -> Table:
    ncol = max(len(r) for r in rows)
    rows = [r + [""] * (ncol - len(r)) for r in rows]
    # Spaltenbreiten nach Textlaenge (gedeckelt), mindestens 1,6 cm
    weights = [min(max(len(r[c]) for r in rows), 60) + 6 for c in range(ncol)]
    total = sum(weights)
    # Mindestbreite: laengstes Wort der Spalte passt ungetrennt (8,5 pt, fett in der Kopfzeile)
    longest = [max((pdfmetrics.stringWidth(w, FONT_BOLD, 8.5) for r in rows for w in re.sub(r"[*`]", "", r[c]).split()),
                   default=0) + 12 for c in range(ncol)]
    widths = [max(1.6 * cm, longest[c], CONTENT_WIDTH * weights[c] / total) for c in range(ncol)]
    scale = CONTENT_WIDTH / sum(widths)
    widths = [w * scale for w in widths]
    data = [[Paragraph(inline(c), st["th"]) for c in rows[0]]]
    data += [[Paragraph(inline(c), st["td"]) for c in r] for r in rows[1:]]
    t = Table(data, colWidths=widths, repeatRows=1)
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), PRIMARY),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, LIGHT_GREY]),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("GRID", (0, 0), (-1, -1), 0.5, MID_GREY),
        ("LEFTPADDING", (0, 0), (-1, -1), 5), ("RIGHTPADDING", (0, 0), (-1, -1), 5),
        ("TOPPADDING", (0, 0), (-1, -1), 3), ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
    ]))
    return t


def make_image(path: Path, caption: str, st) -> list:
    if not path.exists():
        return [Paragraph(inline(f"[Bild fehlt: {path.name}]"), st["caption"])]
    w, h = ImageReader(str(path)).getSize()
    max_w, max_h = 15 * cm, 9 * cm
    if w <= 500 and h <= 500:  # Symbole und Figuren klein lassen
        max_w, max_h = 3.5 * cm, 4.5 * cm
    f = min(max_w / w, max_h / h)
    # Bild und Unterschrift als ein Block: eine vorangehende Ueberschrift wandert mit auf die naechste Seite
    block = Table([[Image(str(path), w * f, h * f)], [Paragraph(inline(caption), st["caption"])]],
                  colWidths=[CONTENT_WIDTH])
    block.setStyle(TableStyle([("ALIGN", (0, 0), (-1, -1), "CENTER"), ("LEFTPADDING", (0, 0), (-1, -1), 0),
                               ("RIGHTPADDING", (0, 0), (-1, -1), 0), ("TOPPADDING", (0, 0), (-1, -1), 2),
                               ("BOTTOMPADDING", (0, 0), (-1, -1), 0)]))
    return [block]


class KonzeptDoc(BaseDocTemplate):
    def __init__(self, filename: str, meta: dict[str, str], **kw):
        super().__init__(filename, pagesize=A4, leftMargin=2 * cm, rightMargin=2 * cm, topMargin=2.5 * cm,
                         bottomMargin=2.5 * cm, **kw)
        self.meta = meta
        frame = Frame(self.leftMargin, self.bottomMargin, self.width, self.height, id="f")
        self.addPageTemplates([PageTemplate(id="cover", frames=[frame]),
                               PageTemplate(id="page", frames=[frame], onPage=self.header_footer)])

    def header_footer(self, c, doc):
        w, h = A4
        c.saveState()
        c.setStrokeColor(ACCENT)
        c.setLineWidth(0.8)
        c.line(2 * cm, h - 1.8 * cm, w - 2 * cm, h - 1.8 * cm)
        c.line(2 * cm, 1.8 * cm, w - 2 * cm, 1.8 * cm)
        c.setFont(FONT, 8)
        c.setFillColor(colors.grey)
        m = self.meta
        c.drawString(2 * cm, h - 1.5 * cm, f"{m.get('title', '')}  |  v{m.get('Version', '')}  |  {m.get('Status', '')}")
        c.drawRightString(w - 2 * cm, h - 1.5 * cm, m.get("Datum", ""))
        c.drawString(2 * cm, 1.2 * cm, "Vertraulich")
        c.drawCentredString(w / 2, 1.2 * cm, m.get("Autor", ""))
        c.drawRightString(w - 2 * cm, 1.2 * cm, f"Seite {doc.page}")
        c.restoreState()

    def afterFlowable(self, flowable):
        if isinstance(flowable, Paragraph) and flowable.style.name in ("h1", "h2"):
            level = 0 if flowable.style.name == "h1" else 1
            key = f"h{id(flowable)}"
            self.canv.bookmarkPage(key)
            self.notify("TOCEntry", (level, flowable.getPlainText(), self.page, key))


def build(src: Path, out: Path | None) -> Path:
    st = styles()
    lines = src.read_text(encoding="utf-8").splitlines()
    meta: dict[str, str] = {}
    cover: list = []
    story: list = []
    i = 0
    title = subtitle = ""
    meta_done = False

    seen_h1 = False

    def flush_para(buf: list[str]):
        if buf and not meta_done:  # Hinweise vor der Metadatentabelle gelten nur der Markdown-Fassung
            buf.clear()
        if buf:
            story.append(Paragraph(inline(" ".join(s.strip() for s in buf)), st["body"]))
            buf.clear()

    para: list[str] = []
    while i < len(lines):
        line = lines[i]
        s = line.strip()
        if s.startswith("# ") and not title:
            title = s[2:].strip()
            i += 1
            continue
        if re.fullmatch(r"_[^_].*_", s) and not subtitle and not story:
            subtitle = s.strip("_")
            i += 1
            continue
        if s.startswith("```"):
            flush_para(para)
            code = []
            i += 1
            while i < len(lines) and not lines[i].strip().startswith("```"):
                code.append(lines[i])
                i += 1
            story.append(Preformatted("\n".join(code), st["code"]))
            i += 1
            continue
        if s.startswith("|"):
            flush_para(para)
            rows = []
            while i < len(lines) and lines[i].strip().startswith("|"):
                r = split_row(lines[i])
                if not all(re.fullmatch(r":?-{2,}:?", c) for c in r):
                    rows.append(r)
                i += 1
            if not meta_done and rows and all(c == "" for c in rows[0]):  # Metadaten -> Deckblatt
                meta = {r[0]: r[1] for r in rows[1:] if len(r) >= 2}
                meta_done = True
                continue
            story.append(make_table(rows, st))
            story.append(Spacer(1, 6))
            continue
        m = re.fullmatch(r"!\[(.*?)\]\((.+?)\)", s)
        if m:
            flush_para(para)
            img = make_image((src.parent / m.group(2)).resolve(), m.group(1), st)
            # Bilder vor dem ersten Abschnitt (Symbol) gehoeren aufs Deckblatt
            (story if seen_h1 else cover).extend(img)
            i += 1
            continue
        if s.startswith("## "):
            flush_para(para)
            seen_h1 = True
            story.append(Paragraph(inline(s[3:]), st["h1"]))
            rule = HRFlowable(width="100%", thickness=1.2, color=ACCENT, spaceAfter=6)
            rule.keepWithNext = True  # Kapitelanfang nicht allein am Seitenende
            story.append(rule)
            i += 1
            continue
        if s.startswith("### "):
            flush_para(para)
            story.append(Paragraph(inline(s[4:]), st["h2"]))
            i += 1
            continue
        mb = re.match(r"^(\s*)(- |\d+\. )(.*)$", line)
        if mb:
            flush_para(para)
            indent = len(mb.group(1))
            marker = "•" if mb.group(2) == "- " else mb.group(2).strip()
            text = [mb.group(3)]
            i += 1
            while i < len(lines) and lines[i].startswith(" " * (indent + 2)) and lines[i].strip() \
                    and not re.match(r"^\s*(- |\d+\. )", lines[i]):
                text.append(lines[i].strip())
                i += 1
            bstyle = ParagraphStyle(f"b{indent}", parent=st["bullet"], leftIndent=14 + indent * 7,
                                    bulletIndent=4 + indent * 7)
            story.append(Paragraph(inline(" ".join(text)), bstyle, bulletText=marker))
            continue
        if not s:
            flush_para(para)
            i += 1
            continue
        para.append(line)
        i += 1
    flush_para(para)

    meta["title"] = title
    version = meta.get("Version", "0")
    if out is None:
        out = src.with_name(f"{src.stem}_v{version}.pdf")

    head = [Spacer(1, 2.5 * cm), HRFlowable(width="100%", thickness=3, color=ACCENT, spaceAfter=12),
            Paragraph(inline(title), st["cover_title"])]
    if subtitle:
        head.append(Paragraph(inline(subtitle), st["cover_sub"]))
    head += [HRFlowable(width="100%", thickness=1, color=LIGHT_GREY, spaceAfter=12), Spacer(1, 0.4 * cm)]
    meta_rows = [[Paragraph(f"<b>{inline(k)}</b>", st["td"]), Paragraph(inline(v), st["td"])]
                 for k, v in meta.items() if k != "title"]
    mt = Table(meta_rows, colWidths=[4 * cm, 13 * cm])
    mt.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LINEBELOW", (0, 0), (-1, -2), 0.3, MID_GREY),
                            ("TOPPADDING", (0, 0), (-1, -1), 4), ("BOTTOMPADDING", (0, 0), (-1, -1), 4)]))
    toc = TableOfContents()
    toc.levelStyles = [st["toc1"], st["toc2"]]
    from reportlab.platypus.doctemplate import NextPageTemplate
    flow = head + [mt, Spacer(1, 0.8 * cm)] + cover + [NextPageTemplate("page"), PageBreak(),
                                                         Paragraph("Inhalt", ParagraphStyle("tochead", parent=st["h1"])),
                                                         HRFlowable(width="100%", thickness=1.2, color=ACCENT,
                                                                    spaceAfter=6), toc, PageBreak()] + story
    doc = KonzeptDoc(str(out), meta, title=title, author=meta.get("Autor", ""), subject=subtitle)
    doc.multiBuild(flow)
    return out


def main() -> int:
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    src = Path(sys.argv[1])
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else None
    result = build(src, out)
    print(f"PDF: {result}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
