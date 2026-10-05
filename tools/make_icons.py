#!/usr/bin/env python3
"""Erzeugt die App-Symbole aus app/resources/icons/volta.svg (Quelle, von Hand gepflegt).

Aufruf (aus dem Repository-Wurzelverzeichnis):
    python3 tools/make_icons.py            # Desktop (PNG 16-1024 px, volta.ico) und Mobil (Android, iOS)
    python3 tools/make_icons.py --mobil    # nur Android und iOS

Ablauf: tools/render_icon.cpp mit dem Qt der Desktop-Installation uebersetzen (nach build/tools/) und das SVG in
PNGs rendern; ohne Qt wird ersatzweise cairosvg verwendet (pip install cairosvg), falls vorhanden. Mit Pillow
entstehen daraus das Windows-Symbol volta.ico und die Mobilsaetze:

- Android (packaging/android/res): ic_launcher und ic_launcher_round (48 dp, mdpi-xxxhdpi) fuer Startprogramme ohne
  adaptive Symbole sowie ic_launcher_foreground (108 dp) als Vordergrund des adaptiven Symbols. Hintergrund
  (Farbverlauf), mipmap-anydpi-v26 und Startbildschirm sind XML-Dateien und werden von Hand gepflegt.
- iOS (packaging/ios/Assets.xcassets): AppIcon 1024 px ohne Transparenz (iOS rundet selbst ab) und LaunchLogo
  (@1x-@3x) fuer den Startbildschirm (LaunchScreen.storyboard).

Die Mobilvarianten werden aus dem SVG abgeleitet: Vordergrund = Motiv ohne Grundflaeche, verkleinert in die sichere
Zone des adaptiven Symbols (66 von 108 dp); iOS = Grundflaeche randlos. Ergebnisse werden eingecheckt; nur nach
Aenderungen am SVG neu erzeugen.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_all import IS_WINDOWS, ROOT, environment  # noqa: E402

ICONS = ROOT / "app" / "resources" / "icons"
SVG = ICONS / "volta.svg"
SIZES = [16, 24, 32, 48, 64, 128, 256, 512, 1024]
ICO_SIZES = [16, 24, 32, 48, 64, 128, 256]

ANDROID_RES = ROOT / "packaging" / "android" / "res"
# Android-Dichten: Faktor zu mdpi (1 dp = 1 px)
DENSITIES = {"mdpi": 1.0, "hdpi": 1.5, "xhdpi": 2.0, "xxhdpi": 3.0, "xxxhdpi": 4.0}
LAUNCHER_DP = 48
ADAPTIVE_DP = 108
IOS_ASSETS = ROOT / "packaging" / "ios" / "Assets.xcassets"
LAUNCH_LOGO_PT = 192
NAVY = (10, 37, 64)  # Theme.navy, Grund fuer das iOS-Symbol ohne Transparenz

# Grundflaeche des Symbols (abgerundetes Quadrat mit Farbverlauf)
BG_RECT = re.compile(r'\s*<rect[^>]*fill="url\(#bg\)"[^>]*/>')
# Motiv in die sichere Zone des adaptiven Symbols: Mittelpunkt des Motivs (512, 530), weitester Punkt (Stecker)
# ca. 500 px davon entfernt; sichere Zone = Kreis mit 33/108 der Kantenlaenge = 313 px -> Faktor 0,62
FOREGROUND_TRANSFORM = "translate(512 512) scale(0.62) translate(-512 -530)"


def derived_svgs(source: str) -> dict[str, str]:
    """Varianten des Symbols: 'foreground' (ohne Grundflaeche, verkleinert), 'fullbleed' (Grundflaeche randlos)."""
    if not BG_RECT.search(source):
        raise ValueError("Grundflaeche (rect mit fill=url(#bg)) im SVG nicht gefunden")
    no_bg = BG_RECT.sub("", source, count=1)
    head, sep, body = no_bg.partition("</defs>")
    body, _, _ = body.rpartition("</svg>")
    foreground = f'{head}{sep}\n  <g transform="{FOREGROUND_TRANSFORM}">{body}  </g>\n</svg>\n'
    fullbleed = BG_RECT.sub('\n  <rect x="0" y="0" width="1024" height="1024" fill="url(#bg)"/>', source, count=1)
    return {"foreground": foreground, "fullbleed": fullbleed}


class Renderer:
    """Rendert ein SVG in quadratische PNGs: bevorzugt mit Qt Svg (render_icon), sonst mit cairosvg."""

    def __init__(self) -> None:
        self.exe: Path | None = None
        self.env: dict[str, str] = {}
        self.name = ""

    def setup(self) -> bool:
        env, qt_root, _ = environment()
        if qt_root is not None:
            out_dir = ROOT / "build" / "tools"
            out_dir.mkdir(parents=True, exist_ok=True)
            exe = out_dir / ("render_icon.exe" if IS_WINDOWS else "render_icon")
            inc = qt_root / "include"
            cmd = ["g++", "-std=c++17", "-O1", str(ROOT / "tools" / "render_icon.cpp"), "-o", str(exe),
                   f"-I{inc}", f"-I{inc / 'QtCore'}", f"-I{inc / 'QtGui'}", f"-I{inc / 'QtSvg'}",
                   f"-L{qt_root / 'lib'}", "-lQt6Svg", "-lQt6Gui", "-lQt6Core"]
            if subprocess.run(cmd, env=env).returncode != 0:
                return False
            self.exe, self.env, self.name = exe, env, "Qt Svg"
            return True
        try:
            import cairosvg  # noqa: F401
        except ImportError:
            print("Qt nicht gefunden (QT_ROOT_DESKTOP setzen) und cairosvg fehlt (pip install cairosvg).")
            return False
        self.name = "cairosvg (Ersatz ohne Qt)"
        return True

    def render(self, svg: Path, sizes: list[int], out_dir: Path) -> bool:
        """Schreibt <out_dir>/volta_<groesse>.png je Groesse."""
        if self.exe is not None:
            return subprocess.run([str(self.exe), str(svg), str(out_dir)] + [str(s) for s in sizes],
                                  env=self.env).returncode == 0
        import cairosvg
        for s in sizes:
            cairosvg.svg2png(url=str(svg), write_to=str(out_dir / f"volta_{s}.png"), output_width=s,
                             output_height=s)
        return True


def make_desktop(renderer: Renderer) -> bool:
    from PIL import Image
    if not renderer.render(SVG, SIZES, ICONS):
        return False
    base = Image.open(ICONS / "volta_256.png")
    # Pillow verkleinert selbst; fuer kleine Groessen die gerenderten (schaerferen) Bilder verwenden
    frames = [Image.open(ICONS / f"volta_{s}.png").convert("RGBA") for s in ICO_SIZES]
    base.save(ICONS / "volta.ico", sizes=[(s, s) for s in ICO_SIZES], append_images=frames)
    print(f"Desktop-Symbole erzeugt in {ICONS}")
    return True


def make_mobile(renderer: Renderer) -> bool:
    from PIL import Image, ImageDraw
    variants = derived_svgs(SVG.read_text(encoding="utf-8"))
    with tempfile.TemporaryDirectory() as tmp_name:
        tmp = Path(tmp_name)
        rendered: dict[str, Path] = {}
        for name, text in [("plain", SVG.read_text(encoding="utf-8"))] + list(variants.items()):
            svg = tmp / f"{name}.svg"
            svg.write_text(text, encoding="utf-8")
            out = tmp / name
            out.mkdir()
            if not renderer.render(svg, [1024], out):
                return False
            rendered[name] = out / "volta_1024.png"

        def scaled(name: str, px: int) -> Image.Image:
            # ein Rendering in 1024 px, verkleinert mit Lanczos (gleichmaessig ueber alle Dichten)
            return Image.open(rendered[name]).convert("RGBA").resize((px, px), Image.LANCZOS)

        # Android: Startsymbol (eckig, rund) und Vordergrund des adaptiven Symbols je Dichte
        for density, factor in DENSITIES.items():
            folder = ANDROID_RES / f"mipmap-{density}"
            folder.mkdir(parents=True, exist_ok=True)
            px = round(LAUNCHER_DP * factor)
            scaled("plain", px).save(folder / "ic_launcher.png", optimize=True)
            round_icon = scaled("fullbleed", px)
            mask = Image.new("L", (px * 4, px * 4), 0)
            ImageDraw.Draw(mask).ellipse((0, 0, px * 4 - 1, px * 4 - 1), fill=255)
            round_icon.putalpha(mask.resize((px, px), Image.LANCZOS))
            round_icon.save(folder / "ic_launcher_round.png", optimize=True)
            scaled("foreground", round(ADAPTIVE_DP * factor)).save(folder / "ic_launcher_foreground.png",
                                                                    optimize=True)
        print(f"Android-Symbole erzeugt in {ANDROID_RES}")

        # iOS: App-Symbol ohne Transparenz (App Store), Logo fuer den Startbildschirm
        icon_dir = IOS_ASSETS / "AppIcon.appiconset"
        icon_dir.mkdir(parents=True, exist_ok=True)
        flat = Image.new("RGB", (1024, 1024), NAVY)
        full = scaled("fullbleed", 1024)
        flat.paste(full, mask=full.getchannel("A"))
        flat.save(icon_dir / "AppIcon-1024.png", optimize=True)
        logo_dir = IOS_ASSETS / "LaunchLogo.imageset"
        logo_dir.mkdir(parents=True, exist_ok=True)
        for scale in (1, 2, 3):
            suffix = "" if scale == 1 else f"@{scale}x"
            scaled("foreground", LAUNCH_LOGO_PT * scale).save(logo_dir / f"LaunchLogo{suffix}.png", optimize=True)
        print(f"iOS-Symbole erzeugt in {IOS_ASSETS}")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description="App-Symbole aus volta.svg erzeugen")
    parser.add_argument("--mobil", action="store_true", help="nur Android und iOS (Desktop-Symbole bleiben)")
    args = parser.parse_args()
    try:
        from PIL import Image  # noqa: F401
    except ImportError:
        print("Pillow fehlt (pip install pillow).")
        return 1
    renderer = Renderer()
    if not renderer.setup():
        return 1
    print(f"Renderer: {renderer.name}")
    if not args.mobil and not make_desktop(renderer):
        return 1
    return 0 if make_mobile(renderer) else 1


if __name__ == "__main__":
    sys.exit(main())
