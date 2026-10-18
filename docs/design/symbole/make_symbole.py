"""Symbole der Reihe: Volta/Zuse/MCU Explorer je Band 1-3, Band als LED-Balkenanzeige (docs/design/symbole/README.md).

Aufruf aus dem Repository: python docs/design/symbole/make_symbole.py
Grundlagen: app/resources/icons/volta.svg und zuse/resources/icons/zuse1.svg in der Fassung mit Binaer-LEDs (main);
MCU wird hier gezeichnet. Rendern mit build/tools/render_icon.exe (tools/make_icons.py baut es).
"""
import re
import subprocess
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

REPO = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
OUT.mkdir(parents=True, exist_ok=True)
RENDER = REPO / "build" / "tools" / ("render_icon.exe" if __import__("os").name == "nt" else "render_icon")


def bar(y, n, on, off, seg_w=88, seg_h=58, gap=20, glow=None, housing="#0B0F14"):
    """Klassische LED-Balkenanzeige: Gehaeuse mit drei rechteckigen Segmenten, die ersten n leuchten."""
    total = 3 * seg_w + 2 * gap
    x0 = 512 - total / 2
    pad = 16
    s = (f'<rect x="{x0 - pad}" y="{y - pad}" width="{total + 2 * pad}" height="{seg_h + 2 * pad}" rx="14" '
         f'fill="{housing}" stroke="#000" stroke-opacity="0.35" stroke-width="4"/>')
    for i in range(3):
        x = x0 + i * (seg_w + gap)
        lit = i < n
        if lit and glow:
            s += f'<rect x="{x - 8}" y="{y - 8}" width="{seg_w + 16}" height="{seg_h + 16}" rx="10" fill="{glow}" opacity="0.30"/>'
        s += f'<rect x="{x}" y="{y}" width="{seg_w}" height="{seg_h}" rx="6" fill="{on if lit else off}"/>'
        if lit:  # Glanzlicht oben
            s += f'<rect x="{x + 8}" y="{y + 6}" width="{seg_w - 16}" height="{seg_h * 0.28:.0f}" rx="4" fill="#FFFFFF" opacity="0.35"/>'
    return s


def volta(n):
    svg = (REPO / "app/resources/icons/volta.svg").read_text(encoding="utf-8")
    # fuenf Steckplaetze -> drei Band-LEDs im Kontaktfeld
    svg = re.sub(r'<g fill="#6E6553">.*?</g>',
                 "<g>" + bar(824, n, "#FFC23A", "#4A3B1C", seg_w=84, seg_h=52, gap=18, glow="#FFD45A") + "</g>",
                 svg, flags=re.S)
    return svg


def zuse(n):
    svg = (REPO / "zuse/resources/icons/zuse1.svg").read_text(encoding="utf-8")
    svg = re.sub(r'<!-- Bandnummer binaer: 001 -->.*?(?=</svg>)',
                 "<!-- Band als LED-Balkenanzeige -->" + bar(800, n, "#22D3EE", "#16323A", glow="#22D3EE") + "\n",
                 svg, flags=re.S)
    return svg


def mcu(n):
    g, gd, acc = "#34D399", "#0B3B24", "#34D399"
    pins = "".join(f'<rect x="{x - 18}" y="132" width="36" height="36" rx="6" fill="#C9A45C"/>'
                   for x in range(276, 800, 68))
    return f'''<?xml version="1.0" encoding="UTF-8"?>
<!-- Entwurf MCU Explorer: gruene Platine mit Steckleiste; das M ist eine Leiterbahn, deren obere Enden in die
     Steckleiste fuehren (»an die Pins angeschlossen«); eine LED-Balkenanzeige mit drei Segmenten zeigt den Band. -->
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1024 1024" width="1024" height="1024">
  <defs>
    <linearGradient id="bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0" stop-color="#166534"/>
      <stop offset="1" stop-color="#052E16"/>
    </linearGradient>
  </defs>
  <rect x="32" y="32" width="960" height="960" rx="216" fill="url(#bg)"/>
  <!-- Steckleiste -->
  <rect x="226" y="108" width="572" height="84" rx="16" fill="#111827"/>
  {pins}
  <!-- Zuleitungen von den Pins zum M -->
  <g stroke="{gd}" stroke-width="40" stroke-linecap="round" fill="none">
    <path d="M276 170 L276 300"/><path d="M748 170 L748 300"/>
  </g>
  <g stroke="{acc}" stroke-width="22" stroke-linecap="round" fill="none">
    <path d="M276 170 L276 300"/><path d="M748 170 L748 300"/>
  </g>
  <!-- M als Leiterbahn -->
  <polyline points="276,712 276,316 512,560 748,316 748,712" fill="none" stroke="{gd}" stroke-width="96"
            stroke-linecap="round" stroke-linejoin="round"/>
  <polyline points="276,712 276,316 512,560 748,316 748,712" fill="none" stroke="{g}" stroke-width="64"
            stroke-linecap="round" stroke-linejoin="round"/>
  <!-- Loetpunkte an den Enden -->
  <g fill="#052E16" stroke="{acc}" stroke-width="18">
    <circle cx="276" cy="712" r="40"/><circle cx="748" cy="712" r="40"/>
  </g>
  <!-- Band-LEDs -->
  {bar(808, n, acc, "#123B27", glow=acc)}
</svg>
'''


tiles = []
for name, fn in (("volta", volta), ("zuse", zuse), ("mcu", mcu)):
    row = []
    for n in (1, 2, 3):
        p = OUT / f"{name}{n}.svg"
        p.write_text(fn(n), encoding="utf-8")
        subprocess.run([str(RENDER), str(p), str(OUT), "256"], check=True)
        row.append(OUT / f"{name}{n}_256.png")
    tiles.append(row)

W, H = 3 * 290 + 220, 3 * 320 + 60
sheet = Image.new("RGB", (W, H), "white")
d = ImageDraw.Draw(sheet)
try:
    font = ImageFont.truetype("arialbd.ttf", 26)
    small = ImageFont.truetype("arial.ttf", 22)
except OSError:
    font = small = ImageFont.load_default()
labels = ["Volta", "Zuse", "MCU Explorer"]
for c in range(3):
    d.text((220 + c * 290 + 100, 18), f"Band {c + 1}", fill=(90, 100, 115), font=small)
for r, row in enumerate(tiles):
    d.text((20, 60 + r * 320 + 110), labels[r], fill=(20, 33, 61), font=font)
    for c, png in enumerate(row):
        im = Image.open(png).convert("RGBA")
        sheet.paste(im, (220 + c * 290, 60 + r * 320), im)
        d.text((220 + c * 290 + 70, 60 + r * 320 + 262), f"{labels[r]} {c + 1}", fill=(90, 100, 115), font=small)
sheet.save(OUT / "uebersicht.png")
print(OUT / "uebersicht.png")
