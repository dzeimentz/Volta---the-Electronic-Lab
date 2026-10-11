#!/usr/bin/env python3
"""Baut Volta 1 als Release (Desktop) und legt ein lauffaehiges Programm im Ordner Release/ ab.

Aufruf (aus dem Repository-Wurzelverzeichnis):
    python3 tools/deploy_release.py

Ablauf: Preset desktop-release konfigurieren (beim ersten Mal) und bauen, volta1(.exe) nach Release/ kopieren,
Qt-Laufzeit mit windeployqt daneben legen (nur Windows) und die Inhaltspakete nach Release/content kopieren
(die App sucht ihre Inhalte zuerst neben der ausfuehrbaren Datei). Release/ ist nicht im Repository (.gitignore).
Nur fuer den lokalen Gebrauch: nichts wird signiert, hochgeladen oder veroeffentlicht.
"""
from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_all import IS_WINDOWS, ROOT, environment, tail  # noqa: E402

PRESET = "desktop-release"
TARGET = ROOT / "Release"


def run(cmd: list[str], env: dict[str, str]) -> None:
    proc = subprocess.run(cmd, cwd=ROOT, env=env, capture_output=True, text=True, encoding="utf-8",
                          errors="replace")
    if proc.returncode != 0:
        print(f"$ {' '.join(cmd)}\n{tail((proc.stdout or '') + (proc.stderr or ''), 40)}")
        raise SystemExit(1)


def main() -> int:
    env, qt_root, use_mingw = environment()
    if qt_root is None:
        print("Qt nicht gefunden (QT_ROOT_DESKTOP setzen).")
        return 1
    configure = ["cmake", "--preset", PRESET]
    if use_mingw and not (ROOT / "build" / PRESET / "CMakeCache.txt").exists():
        configure.append("-DCMAKE_CXX_COMPILER=g++")
    run(configure, env)
    run(["cmake", "--build", "--preset", PRESET], env)

    exe_name = "volta1.exe" if IS_WINDOWS else "volta1"
    exe = ROOT / "build" / PRESET / "app" / exe_name
    if not exe.exists():
        print(f"{exe} fehlt nach dem Bauen.")
        return 1
    TARGET.mkdir(exist_ok=True)
    try:
        shutil.copy2(exe, TARGET / exe_name)
    except PermissionError:
        print(f"{TARGET / exe_name} ist in Benutzung - bitte Volta beenden und erneut aufrufen.")
        return 1
    if IS_WINDOWS:
        deploy = shutil.which("windeployqt", path=env["PATH"]) or str(qt_root / "bin" / "windeployqt.exe")
        run([deploy, "--release", "--qmldir", str(ROOT / "app" / "qml"), "--no-translations",
             "--no-system-d3d-compiler", "--no-opengl-sw",
             # nicht benoetigt: Netzwerkinformation, QML-Debugger (kein Netz, docs/privacy-safety.md)
             "--skip-plugin-types", "networkinformation,qmltooling,tls",
             str(TARGET / exe_name)], env)
        # Tonausgabe ueber das Windows-Backend: FFmpeg-Plugin und -Bibliotheken (koennten Netzprotokolle) und das
        # Test-Plugin der Sprachausgabe werden nicht ausgeliefert (docs/adr/0009-tonausgabe-und-vorlesen.md)
        unwanted = [TARGET / "multimedia" / "ffmpegmediaplugin.dll", TARGET / "texttospeech" / "qtexttospeech_mock.dll"]
        for pattern in ("avcodec-*.dll", "avformat-*.dll", "avutil-*.dll", "swresample-*.dll", "swscale-*.dll"):
            unwanted += list(TARGET.glob(pattern))
        # Stil Basic ist fest eingestellt (main.cpp): andere Controls-Stile, NativeStyle und die virtuelle Tastatur
        # werden nicht gebraucht (Paketgroesse, docs/platforms.md)
        for style in ("Fusion", "Imagine", "Material", "Universal", "FluentWinUI3", "Windows"):
            unwanted += list(TARGET.glob(f"Qt6QuickControls2{style}*.dll"))
            if (TARGET / "qml" / "QtQuick" / "Controls" / style).exists():
                shutil.rmtree(TARGET / "qml" / "QtQuick" / "Controls" / style)
        for folder in (TARGET / "qml" / "QtQuick" / "NativeStyle", TARGET / "qml" / "QtQuick" / "VirtualKeyboard"):
            if folder.exists():
                shutil.rmtree(folder)
        unwanted += list(TARGET.glob("Qt6VirtualKeyboard*.dll")) + [
            TARGET / "platforminputcontexts" / "qtvirtualkeyboardplugin.dll",
            TARGET / "imageformats" / "qgif.dll", TARGET / "imageformats" / "qico.dll"]
        for f in unwanted:
            if f.exists():
                f.unlink()
    content = TARGET / "content"
    if content.exists():
        shutil.rmtree(content)
    shutil.copytree(ROOT / "content", content, ignore=shutil.ignore_patterns("schema"))
    size_mb = sum(f.stat().st_size for f in TARGET.rglob("*")
                  if f.is_file() and "android" not in f.relative_to(TARGET).parts) / 1e6
    print(f"Release: {TARGET / exe_name} ({size_mb:.0f} MB mit Laufzeit und Inhalten, ohne Android-APK)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
