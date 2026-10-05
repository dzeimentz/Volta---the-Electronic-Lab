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
    shutil.copy2(exe, TARGET / exe_name)
    if IS_WINDOWS:
        deploy = shutil.which("windeployqt", path=env["PATH"]) or str(qt_root / "bin" / "windeployqt.exe")
        run([deploy, "--release", "--qmldir", str(ROOT / "app" / "qml"), "--no-translations",
             "--no-system-d3d-compiler", "--no-opengl-sw",
             # nicht benoetigt: Netzwerkinformation, QML-Debugger (kein Netz, docs/privacy-safety.md)
             "--skip-plugin-types", "networkinformation,qmltooling,tls",
             str(TARGET / exe_name)], env)
    content = TARGET / "content"
    if content.exists():
        shutil.rmtree(content)
    shutil.copytree(ROOT / "content", content, ignore=shutil.ignore_patterns("schema"))
    size_mb = sum(f.stat().st_size for f in TARGET.rglob("*") if f.is_file()) / 1e6
    print(f"Release: {TARGET / exe_name} ({size_mb:.0f} MB mit Laufzeit und Inhalten)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
