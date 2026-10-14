#!/usr/bin/env python3
"""Schneidet das Maskottchen Zetti (Zuse 1) aus dem Entwurfsbogen und stellt die Posen frei.

Aufruf (aus dem Repository-Wurzelverzeichnis):
    python3 tools/make_zetti.py [bogen]

Quelle: Entwurfsbogen »Zuse 1 - Maskottchen: Zetti« (1254 x 1254 px, mit einem KI-Bildwerkzeug erzeugt, siehe
docs/partner/rechte-und-lizenzen.md), Vorgabe ../Gfx/Zetti/zetti_bogen_chatgpt.webp (nicht im Repository).
Ergebnis: zuse/resources/zetti/zetti_<pose>.png (freigestellt, hoechstens 466 px, RGBA). Nur nach einem neuen Bogen erneut ausfuehren.
"""
from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT.parent / "Gfx" / "Zetti" / "zetti_bogen_chatgpt.webp"
TARGET = ROOT / "zuse" / "resources" / "zetti"

# Pose -> Ausschnitt (links, oben, rechts, unten) im Bogen; ganze Figur oben, Mimik (Kopf) unten
BOXES = {
    "zeigend": (40, 140, 505, 845),
    "bauen": (445, 565, 650, 840),
    "erklaerend": (650, 565, 855, 840),
    "nachdenklich": (850, 565, 1035, 840),
    "losgehts": (1030, 565, 1245, 840),
    "freundlich": (28, 925, 268, 1165),
    "begeistert": (268, 925, 508, 1165),
    "ueberrascht": (508, 925, 748, 1165),
    "gruebelnd": (748, 925, 990, 1165),
    "froehlich": (990, 925, 1232, 1165),
}
# Fremde Teile im Ausschnitt (Koordinaten im Ausschnitt), werden vor dem Freistellen weiss uebermalt
CLEAN = {"zeigend": [(400, 420, 465, 705)]}
TOLERANCE = 16  # Farbabstand zum Hintergrund (klein, damit das Leuchten der Antenne bleibt)
MAX_SIZE = 466


def cut_out(img: Image.Image) -> Image.Image:
    """Hintergrund vom Rand aus (zusammenhaengend) transparent machen; Figur bleibt unberuehrt."""
    # Der Bogen ist stellenweise schon transparent: fuer das Fuellen auf Weiss legen, Alpha am Ende uebernehmen
    rgba = img.convert("RGBA")
    white = Image.new("RGBA", rgba.size, (255, 255, 255, 255))
    rgb = Image.alpha_composite(white, rgba).convert("RGB")
    mask_key = (255, 0, 255)
    w, h = rgb.size
    seeds = [(x, 0) for x in range(0, w, 8)] + [(x, h - 1) for x in range(0, w, 8)]
    seeds += [(0, y) for y in range(0, h, 8)] + [(w - 1, y) for y in range(0, h, 8)]
    for s in seeds:
        px = rgb.getpixel(s)
        if px == mask_key or sum(px) < 600:  # schon gefuellt oder dunkel (Figur am Rand)
            continue
        ImageDraw.floodfill(rgb, s, mask_key, thresh=TOLERANCE)
    # Alpha: 0, wo die Fuellfarbe steht, sonst 255
    diff = ImageChops.difference(rgb, Image.new("RGB", rgb.size, mask_key)).convert("L")
    out = Image.alpha_composite(white, rgba)
    out.putalpha(ImageChops.multiply(diff.point(lambda v: 0 if v == 0 else 255), rgba.getchannel("A")))
    return out.crop(out.getbbox())


def main() -> int:
    source = Path(sys.argv[1]) if len(sys.argv) > 1 else SOURCE
    if not source.exists():
        print(f"Bogen fehlt: {source}")
        return 1
    sheet = Image.open(source).convert("RGBA")
    TARGET.mkdir(parents=True, exist_ok=True)
    for name, box in BOXES.items():
        part = sheet.crop(box)
        draw = ImageDraw.Draw(part)
        for rect in CLEAN.get(name, []):
            draw.rectangle(rect, fill=(255, 255, 255, 0))
        fig = cut_out(part)
        fig.thumbnail((MAX_SIZE, MAX_SIZE), Image.LANCZOS)
        fig.save(TARGET / f"zetti_{name}.png", optimize=True)
    print(f"{len(BOXES)} Posen nach {TARGET}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
