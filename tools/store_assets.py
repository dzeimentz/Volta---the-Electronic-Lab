#!/usr/bin/env python3
"""Grafiken fuer den Store-Eintrag (docs/store/README.md) nach Release/Store/:

    python3 tools/store_assets.py            # Symbol, Feature-Grafik und Screenshots (DE, EN, FR, LB)
    python3 tools/store_assets.py --ohne-screenshots

- icon-512.png: App-Symbol 512 x 512 (aus app/resources/icons/volta_512.png)
- feature-<sprache>.png: Feature-Grafik 1024 x 500 mit Tick, Name und drei Merkmalen
- screenshots/<sprache>/NN-<szene>.png: 1920 x 1200 (Fenster 1280 x 800, Skalierung 1,5) ueber die
  Entwicklerhilfen VOLTA_SCREENSHOT/VOLTA_DEMO_* der Desktop-Ausgabe (build/desktop-debug/app/volta1.exe)

Benoetigt Pillow und die Schrift Segoe UI (Windows).
"""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "Release" / "Store"
EXE = ROOT / "build" / "desktop-debug" / "app" / ("volta1.exe" if os.name == "nt" else "volta1")
FONTS = Path("C:/Windows/Fonts")

TEXTS = {
    "de": ("Das Elektronik-Labor", ["61 Experimente mit echter Simulation", "Tick hilft beim Forschen",
                                    "Ohne Werbung, ohne Internet"]),
    "en": ("The Electronics Lab", ["61 experiments with real simulation", "Tick helps you explore",
                                   "No ads, no internet needed"]),
    "fr": ("Le labo d'électronique", ["61 expériences, vraie simulation", "Tick t'aide à chercher",
                                      "Sans pub, sans Internet"]),
    "lb": ("Den Elektronik-Labo", ["61 Experimenter mat richteger Simulatioun", "Den Tick hëlleft beim Fuerschen",
                                   "Ouni Reklamm, ouni Internet"]),
}

# Szenen: Dateiname -> Umgebungsvariablen der Entwicklerhilfen
SCENES = [
    ("01-start", {}),
    ("02-experiment", {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.0.1", "VOLTA_SCREENSHOT_S": "5"}),
    ("03-messgeraet", {"VOLTA_DEMO_METER": "v"}),
    ("04-fehlersuche", {"VOLTA_DEMO_TOOLBOX": "4", "VOLTA_SCREENSHOT_S": "4"}),
    ("05-schaltplan", {"VOLTA_DEMO_SCHEMATIC": "1", "VOLTA_SCREENSHOT_S": "4"}),
    ("06-blick-hinein", {"VOLTA_DEMO_SOLVE": "1", "VOLTA_DEMO_EXP": "volta1.0.1", "VOLTA_DEMO_INSIDE": "BL1",
                         "VOLTA_SCREENSHOT_S": "6"}),
    ("07-zeitschreiber", {"VOLTA_DEMO_RECORDER": "1", "VOLTA_SCREENSHOT_S": "6"}),
    ("08-hilfe", {"VOLTA_DEMO_HELP": "1"}),
]


def font(size: int, bold=False):
    return ImageFont.truetype(str(FONTS / ("segoeuib.ttf" if bold else "segoeui.ttf")), size)


def feature(lang: str, path: Path):
    w, h = 1024, 500
    img = Image.new("RGB", (w, h))
    px = img.load()
    top, bottom = (10, 37, 64), (40, 115, 176)  # Navy -> Akzentblau
    for y in range(h):
        t = y / (h - 1)
        c = tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3))
        for x in range(w):
            px[x, y] = c
    tick = Image.open(ROOT / "app" / "resources" / "tick" / "tick_freundlich.png").convert("RGBA")
    tick.thumbnail((400, 400), Image.LANCZOS)
    img.paste(tick, (40, (h - tick.height) // 2 + 10), tick)
    d = ImageDraw.Draw(img)
    subtitle, points = TEXTS[lang]
    x = 470
    d.text((x, 70), "Volta 1", font=font(96, True), fill=(255, 255, 255))
    d.text((x + 4, 190), subtitle, font=font(40, True), fill=(255, 210, 63))
    y = 270
    for p in points:
        d.ellipse((x + 4, y + 14, x + 18, y + 28), fill=(255, 210, 63))
        d.text((x + 32, y), p, font=font(30), fill=(255, 255, 255))
        y += 56
    img.save(path)


def screenshots(lang: str, folder: Path):
    folder.mkdir(parents=True, exist_ok=True)
    for name, extra in SCENES:
        env = dict(os.environ)
        env.update({"VOLTA_NO_SPLASH": "1", "VOLTA_DEMO_LANG": lang, "QT_SCALE_FACTOR": "1.5",
                    "VOLTA_SCREENSHOT": str(folder / f"{name}.png")})
        env.update(extra)
        subprocess.run([str(EXE)], env=env, timeout=60, check=False)
        print(f"  {lang}/{name}: {'ok' if (folder / f'{name}.png').exists() else 'FEHLT'}")


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    icon = Image.open(ROOT / "app" / "resources" / "icons" / "volta_512.png").convert("RGBA")
    icon.resize((512, 512), Image.LANCZOS).save(OUT / "icon-512.png")
    for lang in TEXTS:
        feature(lang, OUT / f"feature-{lang}.png")
    print("Symbol und Feature-Grafiken:", OUT)
    if "--ohne-screenshots" not in sys.argv:
        if not EXE.exists():
            print("Desktop-Ausgabe fehlt:", EXE)
            return 1
        for lang in TEXTS:
            screenshots(lang, OUT / "screenshots" / lang)
    return 0


if __name__ == "__main__":
    sys.exit(main())
