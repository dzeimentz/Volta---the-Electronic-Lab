#!/usr/bin/env python3
"""Baut die Android-Test-APK von Volta 1 (arm64, Debug-Signatur) und spielt sie auf angeschlossene Tablets, oder das
Paket fuer Google Play (AAB, arm64 + armv7, unsigniert).

Aufruf (aus dem Repository-Wurzelverzeichnis):
    python3 tools/build_android.py            # bauen, Release/android/Volta1-<version>-arm64-debug.apk
    python3 tools/build_android.py --install  # zusaetzlich auf alle per adb angeschlossenen Geraete spielen
    python3 tools/build_android.py --armv7    # Test-APK fuer aeltere 32-Bit-Tablets (...-armv7-debug.apk)
    python3 tools/build_android.py --aab      # Release/android/Volta1-<version>-release-unsigned.aab
    python3 tools/build_android.py --zuse     # Zuse-Prototyp Z0: ../ZUSE/Release/android/Zuse1-Prototyp-arm64-debug.apk

AAB: Release-Bau fuer arm64-v8a und armeabi-v7a in einem Paket (build/android-release, QT_ANDROID_ABIS), Gradle
bundleRelease ohne Signatur. Signieren mit dem eigenen Upload-Schluessel ist Sache des Inhabers (CLAUDE.md: keine
Releases signieren, nichts hochladen), z. B.
    jarsigner -keystore <upload.jks> Volta1-<version>-release-unsigned.aab <alias>

Ablauf (docs/platforms.md): cmake-Preset android-arm64 bauen, androiddeployqt (ohne Gradle) legt die Bibliotheken
ab, danach werden nicht benoetigte Qt-Teile entfernt (FFmpeg, ungenutzte Controls-Stile, virtuelle Tastatur, TLS,
Netzinformation, seltene Bildformate - Ziel < 80 MB), Gradle baut die APK. Gradle-Ausgabe geht in eine Datei: in
einer Pipe haelt der Gradle-Dienst sie offen und der Aufruf haengt. Zum Schluss Pruefung, dass die APK keine
Berechtigungen anfordert (ausser der internen AndroidX-Signaturberechtigung).
Nur fuer Tests: Debug-Signatur, nichts wird signiert, hochgeladen oder veroeffentlicht.

Umgebung: Qt for Android (QT_ROOT_ANDROID), Desktop-Qt gleicher Version (QT_HOST_PATH), Android-SDK mit NDK und
JDK - Vorgaben unten, per Umgebungsvariable ueberschreibbar.
"""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ZUSE = "--zuse" in sys.argv  # Zuse-Prototyp (eigener Build-Ordner, nur arm64-Test-APK)
AAB = "--aab" in sys.argv and not ZUSE
ARMV7 = "--armv7" in sys.argv and not AAB and not ZUSE
PRESET = "android-armv7" if ARMV7 else "android-arm64"
BUILD = ROOT / "build" / ("android-zuse" if ZUSE else "android-release" if AAB else PRESET)
APP_DIR, APP_TARGET = ("zuse", "zuse_app") if ZUSE else ("app", "volta_app")
ANDROID_BUILD = BUILD / APP_DIR / "android-build"
LIBS = ANDROID_BUILD / "libs" # je ABI ein Unterordner
PACKAGE = "com.dcsoftwareengineering.zuse1" if ZUSE else "com.dcsoftwareengineering.volta1"
ACTIVITY = "org.qtproject.qt.android.bindings.QtActivity"

# Bibliotheken, die Volta nicht braucht (Muster auf Dateinamen in libs/<abi>)
UNWANTED = [
    r"^libav(codec|format|util|device|filter)\.so$", r"^libsw(scale|resample)\.so$",  # FFmpeg: nur Tonausgabe
    r"ffmpegmediaplugin",
    r"QuickControls2(Fusion|Imagine|Material|Universal|FluentWinUI3)",                 # Stil Basic fest
    r"Controls_(Fusion|Imagine|Material|Universal|FluentWinUI3)",
    r"VirtualKeyboard", r"virtualkeyboard",                                            # Android-Tastatur des Systems
    r"plugins_tls_", r"plugins_networkinformation_",                                   # kein Netz
    r"imageformats_q(gif|icns|ico|tga|tiff|wbmp|webp)_", r"vectorimageformats_",
]


def environment() -> dict[str, str]:
    env = dict(os.environ)
    sdk = env.get("ANDROID_SDK_ROOT") or str(Path(env.get("LOCALAPPDATA", "")) / "Android" / "Sdk")
    env.setdefault("ANDROID_SDK_ROOT", sdk.replace("\\", "/"))
    env.setdefault("ANDROID_NDK_ROOT", env["ANDROID_SDK_ROOT"] + "/ndk/27.2.12479018")
    env.setdefault("JAVA_HOME", "C:/Program Files/Android/openjdk/jdk-21.0.8")
    env.setdefault("QT_ROOT_ANDROID", "C:/Qt/6.11.2/android_arm64_v8a")
    if ARMV7: # eigene Qt-Ausgabe je ABI (das AAB holt sich armv7 selbst ueber QT_ANDROID_ABIS)
        env["QT_ROOT_ANDROID"] = env["QT_ROOT_ANDROID"].replace("android_arm64_v8a", "android_armv7")
    env.setdefault("QT_HOST_PATH", "C:/Qt/6.11.2/mingw_64")
    extra = ["C:/Qt/Tools/Ninja", "C:/Qt/Tools/CMake_64/bin"]
    env["PATH"] = os.pathsep.join(extra + [env.get("PATH", "")])
    return env


def run(cmd: list[str], env: dict[str, str], log: Path, cwd: Path = ROOT) -> None:
    with open(log, "w", encoding="utf-8", errors="replace") as out:
        proc = subprocess.run(cmd, cwd=cwd, env=env, stdout=out, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
    if proc.returncode != 0:
        print(f"Fehler bei: {' '.join(cmd)} (Protokoll: {log})")
        print("".join(open(log, encoding="utf-8", errors="replace").readlines()[-30:]))
        raise SystemExit(1)


def prune_libs_xml(path: Path) -> None:
    """Qt laedt die in libs.xml genannten Bibliotheken beim Start: entfernte duerfen dort nicht mehr stehen."""
    text = path.read_text(encoding="utf-8")

    def wanted(name: str) -> bool:
        file = name if name.endswith(".so") else f"lib{name}.so"
        return not any(re.search(p, file) for p in UNWANTED)

    def item(m: re.Match) -> str:
        abi, libs = m.group(1), m.group(2)
        kept = [lib for lib in libs.split(":") if wanted(lib)]
        return f"<item>{abi};{':'.join(kept)}</item>" if kept else ""

    text = re.sub(r"<item>([^;<]+);([^<]*)</item>", item, text)
    text = re.sub(r"\n\s*\n", "\n", text)  # Leerzeilen der entfernten Eintraege
    path.write_text(text, encoding="utf-8")


def version() -> str:
    text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    m = re.search(r"project\(Volta\s+VERSION\s+([0-9.]+)", text)
    return m.group(1) if m else "0"


def bundle(env: dict[str, str]) -> int:
    """Gradle bundleRelease ohne Signatur; prueft Architekturen und Berechtigungen im fertigen AAB."""
    gradlew = "gradlew.bat" if sys.platform == "win32" else "./gradlew"
    aab_in = ANDROID_BUILD / "build" / "outputs" / "bundle" / "release" / "android-build-release.aab"
    aab_in.unlink(missing_ok=True)
    run([str(ANDROID_BUILD / gradlew), "bundleRelease", "--no-daemon", "--console=plain"], env,
        BUILD / "gradle.log", cwd=ANDROID_BUILD)
    target = ROOT / "Release" / "android"
    target.mkdir(parents=True, exist_ok=True)
    aab = target / f"Volta1-{version()}-release-unsigned.aab"
    shutil.copy2(aab_in, aab)
    import zipfile
    with zipfile.ZipFile(aab) as z:
        names = z.namelist()
        manifest = z.read("base/manifest/AndroidManifest.xml")
    abis = sorted({n.split("/")[2] for n in names if n.startswith("base/lib/")})
    signed = any(n.startswith("META-INF/") and n.endswith((".RSA", ".EC", ".DSA")) for n in names)
    # Manifest im AAB ist protobuf: nur Elemente »uses-permission« zaehlen (android.permission.DUMP schuetzt den
    # Empfaenger des AndroidX-ProfileInstaller und ist keine Anforderung); interne Signaturberechtigung ausgenommen
    perms = []
    for m in re.finditer(rb"uses-permission", manifest):
        found = re.search(rb"[A-Za-z0-9_.]+\.[A-Z_]{3,}", manifest[m.end():m.end() + 300])
        name = found.group(0).decode() if found else "?"
        if not name.endswith("DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION"):
            perms.append(name)
    if perms:
        print(f"FEHLER: Das AAB fordert Berechtigungen an: {perms}")
        return 1
    print(f"AAB: {aab} ({aab.stat().st_size / 2**20:.1f} MB), ABIs {', '.join(abis)}, "
          f"{'SIGNIERT?!' if signed else 'unsigniert'}, keine Berechtigungen")
    return 0


def main() -> int:
    env = environment()
    BUILD.mkdir(parents=True, exist_ok=True)
    if not (BUILD / "CMakeCache.txt").exists():
        extra = ["-B", str(BUILD), "-DQT_ANDROID_ABIS=arm64-v8a;armeabi-v7a"] if AAB else []
        if ZUSE:
            extra = ["-B", str(BUILD), "-DVOLTA_BUILD_ZUSE=ON"]
        run(["cmake", "--preset", PRESET] + extra, env, BUILD / "configure.log")
    run(["cmake", "--build", str(BUILD)] + (["--target", APP_TARGET, f"{APP_TARGET}_prepare_apk_dir"] if ZUSE else []), env, BUILD / "native.log")
    deploy = str(Path(env["QT_HOST_PATH"]) / "bin" / "androiddeployqt")
    run([deploy, "--input", str(BUILD / APP_DIR / f"android-{APP_TARGET}-deployment-settings.json"),
         "--output", str(ANDROID_BUILD), "--android-platform", "android-36", "--jdk", env["JAVA_HOME"],
         "--no-gdbserver"] + (["--release", "--aab"] if AAB else []), env, BUILD / "deploy.log")

    removed = 0
    for f in sorted(LIBS.glob("*/*.so")):
        if any(re.search(p, f.name) for p in UNWANTED):
            removed += f.stat().st_size
            f.unlink()
    prune_libs_xml(ANDROID_BUILD / "res" / "values" / "libs.xml")
    print(f"Entfernt: {removed / 2**20:.1f} MB nicht benoetigter Bibliotheken")

    gradlew = "gradlew.bat" if sys.platform == "win32" else "./gradlew"
    if AAB:
        return bundle(env)
    apk_in = ANDROID_BUILD / "build" / "outputs" / "apk" / "debug" / "android-build-debug.apk"
    # Gradle packt sonst inkrementell in die vorhandene APK und laesst geaenderte Eintraege als Luecken stehen
    # (Datei wuchs so um rund 30 MB ohne neuen Inhalt): jedes Mal frisch packen
    apk_in.unlink(missing_ok=True)
    shutil.rmtree(ANDROID_BUILD / "build" / "intermediates" / "incremental" / "packageDebug", ignore_errors=True)
    run([str(ANDROID_BUILD / gradlew), "assembleDebug", "--no-daemon", "--console=plain"], env, BUILD / "gradle.log",
        cwd=ANDROID_BUILD)
    target = (ROOT.parent / "ZUSE" / "Release" if ZUSE else ROOT / "Release") / "android"
    target.mkdir(parents=True, exist_ok=True)
    apk = target / ("Zuse1-Prototyp-arm64-debug.apk" if ZUSE
                    else f"Volta1-{version()}-{'armv7' if ARMV7 else 'arm64'}-debug.apk")
    shutil.copy2(apk_in, apk)

    sdk = Path(env["ANDROID_SDK_ROOT"])
    aapt = next((sdk / "build-tools").glob("*/aapt*"), None)
    if aapt:
        badging = subprocess.run([str(aapt), "dump", "badging", str(apk)], capture_output=True, text=True,
                                 encoding="utf-8", errors="replace").stdout
        perms = [p for p in re.findall(r"uses-permission: name='([^']+)'", badging)
                 if not p.endswith("DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION")]
        if perms:
            print(f"FEHLER: Die APK fordert Berechtigungen an: {perms}")
            return 1
    print(f"APK: {apk} ({apk.stat().st_size / 2**20:.1f} MB), keine Berechtigungen")

    if "--install" in sys.argv:
        adb = str(sdk / "platform-tools" / "adb")
        devices = subprocess.run([adb, "devices"], capture_output=True, text=True).stdout.splitlines()[1:]
        for line in devices:
            parts = line.split()
            if len(parts) != 2 or parts[1] != "device":
                continue
            serial = parts[0]
            res = subprocess.run([adb, "-s", serial, "install", "-r", str(apk)], capture_output=True, text=True)
            print(f"{serial}: {(res.stdout + res.stderr).strip().splitlines()[-1]}")
            time.sleep(3)  # das System muss die App erst registrieren
            subprocess.run([adb, "-s", serial, "shell", "am", "start", "-a", "android.intent.action.MAIN", "-c",
                            "android.intent.category.LAUNCHER", "-n", f"{PACKAGE}/{ACTIVITY}"], capture_output=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
