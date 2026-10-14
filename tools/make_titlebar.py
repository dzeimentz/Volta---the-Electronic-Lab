#!/usr/bin/env python3
"""Schaltplanmuster fuer die eigene Fensterleiste der Windows-Ausgabe (TitleBar.qml, Variante C):

    python3 tools/make_titlebar.py   # -> app/resources/brand/titlebar_pattern.png

Eine kachelbare Periode (115 x 40 dp) in doppelter Aufloesung (230 x 80 px), dunkelgrau und halbtransparent auf
durchsichtigem Grund: zwei Leiterbahnen, Widerstand, senkrechtes Bauteil (Kondensator, Diode oder Widerstand im
Wechsel ueber drei Perioden), Knoten und Abzweige. Der Farbverlauf kommt aus QML. Benoetigt Pillow.
"""
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "app" / "resources" / "brand" / "titlebar_pattern.png"
K = 2                 # Aufloesung (2 px je dp)
TILE, H = 115, 40     # eine Periode in dp
COLOR = (34, 38, 46, 120)


def draw_period(d, x0, kind):
    s = lambda v: int(round(v * K))
    y1, y2 = 11, 29
    w = 1 * K
    d.line([(s(x0), s(y1)), (s(x0 + TILE), s(y1))], fill=COLOR, width=w)
    d.line([(s(x0), s(y2)), (s(x0 + TILE), s(y2))], fill=COLOR, width=w)
    d.rectangle([s(x0 + 14), s(y1 - 4), s(x0 + 34), s(y1 + 4)], outline=COLOR, width=w)
    xv = x0 + 60
    d.line([(s(xv), s(y1)), (s(xv), s(16))], fill=COLOR, width=w)
    d.line([(s(xv), s(24)), (s(xv), s(y2))], fill=COLOR, width=w)
    if kind == 0:    # Kondensator
        d.line([(s(xv - 7), s(17)), (s(xv + 7), s(17))], fill=COLOR, width=2 * K)
        d.line([(s(xv - 7), s(23)), (s(xv + 7), s(23))], fill=COLOR, width=2 * K)
    elif kind == 1:  # Diode
        d.polygon([(s(xv - 6), s(16)), (s(xv + 6), s(16)), (s(xv), s(23))], outline=COLOR)
        d.line([(s(xv - 6), s(23)), (s(xv + 6), s(23))], fill=COLOR, width=w)
    else:            # Widerstand senkrecht
        d.rectangle([s(xv - 4), s(15), s(xv + 4), s(25)], outline=COLOR, width=w)
    for y in (y1, y2):
        d.ellipse([s(xv - 2), s(y - 2), s(xv + 2), s(y + 2)], fill=COLOR)
    d.line([(s(x0 + 88), s(y2)), (s(x0 + 88), s(H))], fill=COLOR, width=w)
    d.line([(s(x0 + 100), s(0)), (s(x0 + 100), s(y1))], fill=COLOR, width=w)
    d.ellipse([s(x0 + 98), s(y1 - 2), s(x0 + 102), s(y1 + 2)], fill=COLOR)


def main():
    img = Image.new("RGBA", (3 * TILE * K, H * K), (0, 0, 0, 0))  # drei Perioden: Kondensator, Diode, Widerstand
    d = ImageDraw.Draw(img)
    for i in range(3):
        draw_period(d, i * TILE, i)
    img.save(OUT, optimize=True)
    print(OUT, img.size)


if __name__ == "__main__":
    main()
