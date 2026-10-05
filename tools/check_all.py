#!/usr/bin/env python3
"""Lokale Gesamtpruefung von Volta (Ersatz fuer CI, nur Standardbibliothek).

Aufruf (aus dem Repository-Wurzelverzeichnis):
    python3 tools/check_all.py            # Inhalte, Kern (Debug), App mit Modell- und Inhaltstests (Debug)
    python3 tools/check_all.py --quick    # nur Inhalte und Kern
    python3 tools/check_all.py --bench    # zusaetzlich Kern im Release-Build mit Leistungsmessung

Rueckgabewert 0, wenn alle Pruefungen bestanden sind, sonst 1.

Qt wird ueber die Umgebungsvariable QT_ROOT_DESKTOP gefunden; fehlt sie, wird unter Windows
C:/Qt/6.11.0/mingw_64 versucht. Ist eine MinGW-Installation von Qt vorhanden (C:/Qt/Tools/mingw1310_64), wird sie
fuer Kern und App verwendet. Ohne Qt wird die App-Pruefung uebersprungen (und als solche gemeldet).
"""
from __future__ import annotations

import argparse
import os
import platform
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
IS_WINDOWS = platform.system() == "Windows"
DEFAULT_QT_WINDOWS = Path("C:/Qt/6.11.0/mingw_64")
MINGW_WINDOWS = Path("C:/Qt/Tools/mingw1310_64/bin")


class Result:
    def __init__(self, name: str, ok: bool | None, seconds: float, detail: str = "") -> None:
        self.name = name
        self.ok = ok  # None = uebersprungen
        self.seconds = seconds
        self.detail = detail


def environment() -> tuple[dict[str, str], Path | None, bool]:
    env = dict(os.environ)
    qt_root = Path(env["QT_ROOT_DESKTOP"]) if env.get("QT_ROOT_DESKTOP") else None
    if qt_root is None and IS_WINDOWS and DEFAULT_QT_WINDOWS.exists():
        qt_root = DEFAULT_QT_WINDOWS
    if qt_root is not None and not qt_root.exists():
        qt_root = None
    use_mingw = IS_WINDOWS and MINGW_WINDOWS.exists()
    paths = []
    if use_mingw:
        paths.append(str(MINGW_WINDOWS))
    if qt_root is not None:
        paths.append(str(qt_root / "bin"))
        env["QT_ROOT_DESKTOP"] = str(qt_root)
    if paths:
        env["PATH"] = os.pathsep.join(paths + [env.get("PATH", "")])
    return env, qt_root, use_mingw


def run(cmd: list[str], env: dict[str, str]) -> tuple[bool, str]:
    proc = subprocess.run(cmd, cwd=ROOT, env=env, capture_output=True, text=True, encoding="utf-8",
                          errors="replace")
    return proc.returncode == 0, (proc.stdout or "") + (proc.stderr or "")


def tail(text: str, lines: int = 25) -> str:
    return "\n".join(text.strip().splitlines()[-lines:])


def cmake_steps(preset: str, env: dict[str, str], use_mingw: bool, test: bool = True) -> tuple[bool, str]:
    configure = ["cmake", "--preset", preset]
    if use_mingw and not (ROOT / "build" / preset / "CMakeCache.txt").exists():
        configure.append("-DCMAKE_CXX_COMPILER=g++")
    for cmd in (configure, ["cmake", "--build", "--preset", preset]):
        ok, out = run(cmd, env)
        if not ok:
            return False, f"$ {' '.join(cmd)}\n{tail(out)}"
    if test:
        ok, out = run(["ctest", "--preset", preset, "--output-on-failure"], env)
        summary = next((l for l in out.splitlines() if "tests passed" in l), "")
        if not ok:
            return False, f"$ ctest --preset {preset}\n{tail(out, 40)}"
        return True, summary.strip()
    return True, ""


def timed(name: str, fn) -> Result:
    t0 = time.monotonic()
    ok, detail = fn()
    return Result(name, ok, time.monotonic() - t0, detail)


def main() -> int:
    parser = argparse.ArgumentParser(description="Lokale Gesamtpruefung von Volta")
    parser.add_argument("--quick", action="store_true", help="nur Inhalte und Kern")
    parser.add_argument("--bench", action="store_true", help="zusaetzlich Leistungsmessung (Release)")
    args = parser.parse_args()

    env, qt_root, use_mingw = environment()
    results: list[Result] = []

    def content() -> tuple[bool, str]:
        ok, out = run([sys.executable, "tools/validate_content.py"], env)
        last = out.strip().splitlines()[-1] if out.strip() else ""
        return ok, last if ok else tail(out)

    results.append(timed("Inhaltspakete (validate_content.py)", content))
    results.append(timed("Kern Debug (core-only)", lambda: cmake_steps("core-only", env, use_mingw)))

    if not args.quick:
        if qt_root is None:
            results.append(Result("App Debug (desktop-debug)", None, 0.0, "uebersprungen: Qt nicht gefunden "
                                                                          "(QT_ROOT_DESKTOP setzen)"))
        else:
            results.append(timed("App Debug (desktop-debug)", lambda: cmake_steps("desktop-debug", env, use_mingw)))

    if args.bench:
        def bench() -> tuple[bool, str]:
            ok, detail = cmake_steps("core-release", env, use_mingw)
            if not ok:
                return ok, detail
            exe = ROOT / "build" / "core-release" / "core" / "bench" / ("volta_core_bench.exe" if IS_WINDOWS
                                                                        else "volta_core_bench")
            ok, out = run([str(exe)], env)
            lines = [l.strip() for l in out.splitlines() if "1000 Schritte" in l or "Einzelschritt" in l]
            return ok, " | ".join(lines) if ok else tail(out)
        results.append(timed("Kern Release + Leistungsmessung (core-release)", bench))

    print("\nVolta - Gesamtpruefung")
    print("=" * 72)
    failed = False
    for r in results:
        state = "UEBERSPRUNGEN" if r.ok is None else ("OK" if r.ok else "FEHLER")
        failed = failed or r.ok is False
        print(f"{state:<14}{r.name:<48}{r.seconds:6.1f} s")
        if r.detail:
            for line in r.detail.splitlines():
                print(f"{'':14}{line}")
    print("=" * 72)
    print("Ergebnis: " + ("FEHLER" if failed else "alles bestanden"))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
