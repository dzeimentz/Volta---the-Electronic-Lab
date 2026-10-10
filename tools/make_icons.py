#!/usr/bin/env python3
"""Erzeugt die App-Symbole aus app/resources/icons/volta.svg (Quelle, von Hand gepflegt).

Aufruf (aus dem Repository-Wurzelverzeichnis):
    python3 tools/make_icons.py

Ablauf: tools/render_icon.cpp mit dem Qt der Desktop-Installation uebersetzen (nach build/tools/), das SVG in PNGs
(16-1024 px) rendern und daraus mit Pillow das Windows-Symbol volta.ico bauen. Ergebnisse liegen neben dem SVG und
werden eingecheckt; nur nach Aenderungen am SVG neu erzeugen.
"""
from __future__ import annotations

import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_all import IS_WINDOWS, ROOT, environment  # noqa: E402

ICONS = ROOT / "app" / "resources" / "icons"
SIZES = [16, 24, 32, 48, 64, 128, 256, 512, 1024]
ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]


def main() -> int:
    try:
        from PIL import Image
    except ImportError:
        print("Pillow fehlt (pip install pillow).")
        return 1
    env, qt_root, _ = environment()
    if qt_root is None:
        print("Qt nicht gefunden (QT_ROOT_DESKTOP setzen).")
        return 1
    out_dir = ROOT / "build" / "tools"
    out_dir.mkdir(parents=True, exist_ok=True)
    exe = out_dir / ("render_icon.exe" if IS_WINDOWS else "render_icon")
    inc = qt_root / "include"
    cmd = ["g++", "-std=c++17", "-O1", str(ROOT / "tools" / "render_icon.cpp"), "-o", str(exe),
           f"-I{inc}", f"-I{inc / 'QtCore'}", f"-I{inc / 'QtGui'}", f"-I{inc / 'QtSvg'}",
           f"-L{qt_root / 'lib'}", "-lQt6Svg", "-lQt6Gui", "-lQt6Core"]
    if subprocess.run(cmd, env=env).returncode != 0:
        return 1
    if subprocess.run([str(exe), str(ICONS / "volta.svg"), str(ICONS)] + [str(s) for s in SIZES],
                      env=env).returncode != 0:
        return 1
    base = Image.open(ICONS / "volta_256.png")
    # Pillow verkleinert selbst; fuer kleine Groessen die von Qt gerenderten (schaerferen) Bilder verwenden
    frames = [Image.open(ICONS / f"volta_{s}.png").convert("RGBA") for s in ICO_SIZES]
    base.save(ICONS / "volta.ico", sizes=[(s, s) for s in ICO_SIZES], append_images=frames)
    # Android-Startsymbole (packaging/android/res/mipmap-*/ic_launcher.png)
    big = Image.open(ICONS / "volta_1024.png").convert("RGBA")
    for name, size in {"mdpi": 48, "hdpi": 72, "xhdpi": 96, "xxhdpi": 144, "xxxhdpi": 192}.items():
        target = ROOT / "packaging" / "android" / "res" / f"mipmap-{name}"
        target.mkdir(parents=True, exist_ok=True)
        big.resize((size, size), Image.LANCZOS).save(target / "ic_launcher.png")
    print(f"Symbole erzeugt in {ICONS} und packaging/android/res")
    return 0


if __name__ == "__main__":
    sys.exit(main())
