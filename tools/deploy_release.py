#!/usr/bin/env python3
"""Baut Volta 1 als Release (Desktop) und legt ein lauffaehiges Programm im Ordner Release/ ab.

Aufruf (aus dem Repository-Wurzelverzeichnis):
    python3 tools/deploy_release.py           # Volta 1 -> Release/
    python3 tools/deploy_release.py --zuse    # Zuse 1 (Prototyp) -> ../ZUSE/Release/ (Projektordner der Zuse-Linie)

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
ZUSE = "--zuse" in sys.argv
TARGET = ROOT.parent / "ZUSE" / "Release" if ZUSE else ROOT / "Release"
APP_DIR, APP_NAME, QML_DIR = ("zuse", "zuse1", "zuse/qml") if ZUSE else ("app", "volta1", "app/qml")


def run(cmd: list[str], env: dict[str, str]) -> None:
    proc = subprocess.run(cmd, cwd=ROOT, env=env, capture_output=True, text=True, encoding="utf-8",
                          errors="replace")
    if proc.returncode != 0:
        print(f"$ {' '.join(cmd)}\n{tail((proc.stdout or '') + (proc.stderr or ''), 40)}")
        raise SystemExit(1)


def prune_macos(app: Path) -> None:
    """Wie unter Windows nur Benoetigtes ausliefern: Ton ueber das Darwin-Backend (ohne FFmpeg), Stil Basic,
    kein Netz (networkinformation, tls), keine Datenbank; danach Frameworks entfernen, die nichts mehr laedt."""
    contents = app / "Contents"
    plugins, frameworks, qml = contents / "PlugIns", contents / "Frameworks", contents / "Resources" / "qml"
    unwanted = [plugins / "multimedia" / "libffmpegmediaplugin.dylib",
                plugins / "texttospeech" / "libqtexttospeech_mock.dylib",
                plugins / "networkinformation", plugins / "tls", plugins / "sqldrivers",
                plugins / "imageformats" / "libqgif.dylib", plugins / "imageformats" / "libqico.dylib",
                plugins / "quick" / "libqtquickcontrols2nativestyleplugin.dylib",
                plugins / "quick" / "libqmllocalstorageplugin.dylib",
                plugins / "quick" / "libqmlxmllistmodelplugin.dylib",
                plugins / "quick" / "libparticlesplugin.dylib",
                qml / "QtQuick" / "NativeStyle", qml / "QtQuick" / "LocalStorage", qml / "QtQuick" / "Particles",
                qml / "QtQml" / "XmlListModel", qml / "QtQuick" / "Controls" / "designer"]
    unwanted += list(frameworks.glob("libav*.dylib")) + list(frameworks.glob("libsw*.dylib"))
    for style in ("Fusion", "Imagine", "Material", "Universal", "FluentWinUI3", "iOS", "macOS"):
        unwanted += list(plugins.glob(f"quick/libqtquickcontrols2{style.lower()}style*plugin.dylib"))
        unwanted.append(qml / "QtQuick" / "Controls" / style)
    for f in unwanted:
        if f.is_dir() and not f.is_symlink():
            shutil.rmtree(f)
        elif f.exists() or f.is_symlink():
            f.unlink()
    # Frameworks behalten, die vom Programm oder einem verbliebenen Plugin (auch mittelbar) geladen werden
    def deps(binary: Path) -> set[str]:
        out = subprocess.run(["otool", "-L", str(binary)], capture_output=True, text=True).stdout
        return {line.split()[0].split("/")[-1] for line in out.splitlines()[1:] if "@" in line}
    binaries = {f"{fw.stem}": fw / fw.stem for fw in frameworks.glob("*.framework")}
    binaries.update({lib.name: lib for lib in frameworks.glob("*.dylib")})
    pending = [contents / "MacOS" / app.stem] + list(plugins.rglob("*.dylib"))
    needed: set[str] = set()
    while pending:
        for name in deps(pending.pop()):
            if name in binaries and name not in needed:
                needed.add(name)
                pending.append(binaries[name])
    for name, binary in binaries.items():
        if name not in needed:
            target = binary.parent if binary.parent.suffix == ".framework" else binary
            shutil.rmtree(target) if target.is_dir() else target.unlink()


def deploy_macos(env: dict[str, str]) -> int:
    """macOS: App-Bundle (<name>.app, Inhalte eingebettet) nach Release/ kopieren und die Qt-Laufzeit mit
    macdeployqt hineinlegen. Nicht signiert: Weitergabe an andere Macs erst nach Signierung und Notarisierung."""
    bundle = ROOT / "build" / PRESET / APP_DIR / f"{APP_NAME}.app"
    if not bundle.exists():
        print(f"{bundle} fehlt nach dem Bauen.")
        return 1
    TARGET.mkdir(parents=True, exist_ok=True)
    target = TARGET / bundle.name
    if target.exists():
        shutil.rmtree(target)
    shutil.copytree(bundle, target, symlinks=True)
    deploy = shutil.which("macdeployqt", path=env.get("PATH")) or str(
        Path(env.get("QT_ROOT_DESKTOP", "")) / "bin" / "macdeployqt")
    run([deploy, str(target), f"-qmldir={ROOT / QML_DIR}"], env)
    prune_macos(target)
    # Nach dem Entfernen neu ad-hoc signieren (Apple Silicon startet nur signierte Programme; kein Zertifikat)
    run(["codesign", "--force", "--deep", "--sign", "-", str(target)], env)
    size_mb = sum(f.stat().st_size for f in target.rglob("*") if f.is_file() and not f.is_symlink()) / 1e6
    print(f"Release: {target} ({size_mb:.0f} MB mit Laufzeit und Inhalten, nicht signiert)")
    return 0


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

    if sys.platform == "darwin":
        return deploy_macos(env)
    exe_name = f"{APP_NAME}.exe" if IS_WINDOWS else APP_NAME
    exe = ROOT / "build" / PRESET / APP_DIR / exe_name
    if not exe.exists():
        print(f"{exe} fehlt nach dem Bauen.")
        return 1
    TARGET.mkdir(parents=True, exist_ok=True)
    try:
        shutil.copy2(exe, TARGET / exe_name)
    except PermissionError:
        print(f"{TARGET / exe_name} ist in Benutzung - bitte das Programm beenden und erneut aufrufen.")
        return 1
    if IS_WINDOWS:
        deploy = shutil.which("windeployqt", path=env["PATH"]) or str(qt_root / "bin" / "windeployqt.exe")
        run([deploy, "--release", "--qmldir", str(ROOT / QML_DIR), "--no-translations",
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
    if ZUSE:  # Stufen sind in die App eingebettet (Qt-Ressource), keine Inhalte daneben
        size_mb = sum(f.stat().st_size for f in TARGET.rglob("*")
                      if f.is_file() and "android" not in f.relative_to(TARGET).parts) / 1e6
        print(f"Release: {TARGET / exe_name} ({size_mb:.0f} MB mit Laufzeit, ohne Android-APK)")
        return 0
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
