#!/usr/bin/env python3
"""Erzeugt die App-Symbole aus app/resources/icons/volta.svg und zuse/resources/icons/zuse1.svg (Quellen, von Hand
gepflegt).

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


def apple_icons(Image) -> None:
    """iPadOS: App-Symbol 1024 px ohne Transparenz (App-Store-Vorgabe) auf dem Navy des Startbildschirms,
    Startbild-Logo in 1x/2x/3x; macOS: volta.icns. Quelle ist das Volta-Symbol in 1024 px."""
    big = Image.open(ICONS / "volta_1024.png").convert("RGBA")
    navy = (10, 37, 64, 255)  # wie LaunchScreen.storyboard
    ios = ROOT / "packaging" / "ios" / "Assets.xcassets"
    flat = Image.new("RGBA", big.size, navy)
    flat.alpha_composite(big)
    flat.convert("RGB").save(ios / "AppIcon.appiconset" / "AppIcon-1024.png")
    for name, size in (("LaunchLogo.png", 192), ("LaunchLogo@2x.png", 384), ("LaunchLogo@3x.png", 576)):
        big.resize((size, size), Image.LANCZOS).save(ios / "LaunchLogo.imageset" / name)
    mac = ROOT / "packaging" / "macos"
    mac.mkdir(parents=True, exist_ok=True)
    big.save(mac / "volta.icns", sizes=[(16, 16), (32, 32), (64, 64), (128, 128), (256, 256), (512, 512),
                                        (1024, 1024)])


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
    for icons, name, android in ((ICONS, "volta", "android"),
                                 (ROOT / "zuse" / "resources" / "icons", "zuse1", "android-zuse")):
        if subprocess.run([str(exe), str(icons / f"{name}.svg"), str(icons)] + [str(s) for s in SIZES],
                          env=env).returncode != 0:
            return 1
        base = Image.open(icons / f"{name}_256.png")
        # Pillow verkleinert selbst; fuer kleine Groessen die von Qt gerenderten (schaerferen) Bilder verwenden
        frames = [Image.open(icons / f"{name}_{s}.png").convert("RGBA") for s in ICO_SIZES]
        base.save(icons / f"{name}.ico", sizes=[(s, s) for s in ICO_SIZES], append_images=frames)
        # Android-Startsymbole (packaging/<android>/res/mipmap-*/ic_launcher.png)
        big = Image.open(icons / f"{name}_1024.png").convert("RGBA")
        for density, size in {"mdpi": 48, "hdpi": 72, "xhdpi": 96, "xxhdpi": 144, "xxxhdpi": 192}.items():
            target = ROOT / "packaging" / android / "res" / f"mipmap-{density}"
            target.mkdir(parents=True, exist_ok=True)
            big.resize((size, size), Image.LANCZOS).save(target / "ic_launcher.png")
    apple_icons(Image)
    print("Symbole erzeugt: app/resources/icons, zuse/resources/icons, packaging/android*/res, packaging/ios, "
          "packaging/macos")
    return 0


if __name__ == "__main__":
    sys.exit(main())
