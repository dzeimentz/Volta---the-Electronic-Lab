#!/usr/bin/env python3
"""Erzeugt die Lizenzangaben fuer die Seite »Lizenzen« der App (app/resources/licenses/).

Aufruf (aus dem Repository-Wurzelverzeichnis, nach deploy_release.py und build_android.py):
    python3 tools/make_licenses.py

Quellen, alle lokal (kein Netz):
- Windows: die DLLs in Release/ (Qt, MinGW-Laufzeit); Android: die Bibliotheken der Test-APK in Release/android/
  abzueglich der Muster, die tools/build_android.py entfernt.
- Qt-Stuecklisten (SBOM, SPDX-JSON) der Qt-Installationen: zu jeder ausgelieferten Bibliothek das Qt-Paket und
  ueber DEPENDS_ON die darin enthaltene Drittsoftware (FreeType, HarfBuzz, libpng, zlib, PCRE2 ...).
- Lizenztexte aus den LICENSES-Ordnern der Qt-Quellen, der MinGW-Werkzeugkette und dem NDK.

Ergebnis: licenses.json (Bestandteile mit Version, Lizenz, Copyright, Plattformen) und texts/<SPDX-ID>.txt.
Bricht ab, wenn eine ausgelieferte Bibliothek nur unter GPL bzw. kommerziell steht (nicht LGPL).
"""
from __future__ import annotations

import glob
import json
import re
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from build_android import UNWANTED  # noqa: E402

RELEASE = ROOT / "Release"
OUT = ROOT / "app" / "resources" / "licenses"
QT_WIN = Path("C:/Qt/6.11.0/mingw_64")
QT_ANDROID = Path("C:/Qt/6.11.2/android_arm64_v8a")
QT_SRC = Path("C:/Qt/6.11.2/Src")
MINGW = Path("C:/Qt/Tools/mingw1310_64/licenses")
NDK_GLOB = str(Path.home() / "AppData/Local/Android/Sdk/ndk/*/NOTICE.toolchain")

LGPL = "LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only"
SKIP = re.compile(r"(Test|Bootstrap|TestInternals|QSQLite|DBus|Platform_Attribution|xserverhelper|wintab)", re.I)


def version() -> str:
    text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    return re.search(r"project\(\s*\w+\s+VERSION\s+([\d.]+)", text).group(1)


def shipped_windows() -> set[str]:
    return {p.name for p in RELEASE.rglob("*.dll")}


def shipped_android() -> set[str]:
    apks = sorted((RELEASE / "android").glob(f"Volta1-{version()}-arm64-debug.apk"))
    if not apks:
        sys.exit("Test-APK fehlt: zuerst python tools/build_android.py")
    with zipfile.ZipFile(apks[0]) as z:
        names = {Path(n).name for n in z.namelist() if n.endswith(".so")}
    return {n for n in names if not any(re.search(p, n) for p in UNWANTED)}


def clean_name(name: str) -> str:
    name = re.sub(r"^.*_Attribution_", "", name)
    name = re.sub(r"^Bundled", "", name)
    return re.sub(r"(Private|16bits|12bits)$", "", name)


def scan(sbom_dir: Path, shipped: set[str], qt_version: str):
    """Qt-Bibliotheken und Drittsoftware zu den ausgelieferten Dateien."""
    modules: dict[str, str] = {}  # Paketname -> Lizenz
    third: dict[tuple, dict] = {}
    found: set[str] = set()
    for f in sorted(sbom_dir.glob("*.spdx.json")):
        doc = json.loads(f.read_text(encoding="utf-8"))
        pkgs = {p["SPDXID"]: p for p in doc["packages"]}
        files = {x["SPDXID"]: Path(x["fileName"]).name for x in doc.get("files", [])}
        deps: dict[str, list[str]] = {}
        start: list[str] = []
        for r in doc["relationships"]:
            a, b, t = r["spdxElementId"], r["relatedSpdxElement"], r["relationshipType"]
            if t == "CONTAINS" and a in pkgs and files.get(b) in shipped:
                start.append(a)
                found.add(files[b])
            elif t == "DEPENDS_ON":
                deps.setdefault(a, []).append(b)
        seen: set[str] = set()
        todo = list(start)
        while todo:
            pid = todo.pop()
            if pid in seen or pid not in pkgs:
                continue
            seen.add(pid)
            todo += deps.get(pid, [])
        for pid in seen:
            p = pkgs[pid]
            lic = p.get("licenseConcluded", "NOASSERTION")
            if "-qt-module-" in pid or "-qt-plugin-" in pid:
                if pid in start:
                    modules[p["name"]] = lic
            elif "3rdparty" in pid and lic != "NOASSERTION" and not SKIP.search(p["name"]):
                key = (clean_name(p["name"]).lower(), p.get("versionInfo", ""))
                cr = p.get("copyrightText", "")
                third[key] = {"name": clean_name(p["name"]), "version": p.get("versionInfo", ""), "license": lic,
                              "copyright": "" if cr == "NOASSERTION" else cr.strip()}
    bad = sorted(n for n, lic in modules.items() if "LGPL" not in lic)
    if bad:
        sys.exit(f"FEHLER: nicht LGPL-lizenzierte Qt-Teile ausgeliefert: {bad}")
    return modules, third, found


def license_ids(expr: str) -> list[str]:
    """Benoetigte Lizenztexte: bei OR die erste Nicht-GPL-Wahl (bzw. LGPL), bei AND alle."""
    expr = expr.replace("(", "").replace(")", "")
    out: list[str] = []
    for part in expr.split(" AND "):
        choices = [c.strip().split(" WITH ")[0] for c in part.split(" OR ")]
        choices = [c for c in choices if c != "LicenseRef-Qt-Commercial"]
        pick = next((c for c in choices if not c.startswith("GPL")), choices[0])
        out.append(pick)
    return out


def license_text(spdx: str) -> str:
    for p in [QT_SRC / "LICENSES" / f"{spdx}.txt", *sorted(QT_SRC.glob(f"*/LICENSES/{spdx}.txt"))]:
        if p.exists():
            return p.read_text(encoding="utf-8", errors="replace")
    sys.exit(f"Lizenztext fehlt: {spdx}")


def main() -> int:
    win, android = shipped_windows(), shipped_android()
    qt_win, third_win, found_win = scan(QT_WIN / "sbom", win, "6.11.0")
    qt_and, third_and, found_and = scan(QT_ANDROID / "sbom", android, "6.11.2")
    unknown_win = sorted(n for n in win if n.startswith("Qt6") and n not in found_win)
    unknown_and = sorted(n for n in android if "Qt6" in n and n not in found_and)
    if unknown_win or unknown_and:
        sys.exit(f"FEHLER: Qt-Bibliotheken ohne Stueckliste: {unknown_win + unknown_and}")

    components = [
        {"name": "Qt", "version": "6.11.0 (Windows), 6.11.2 (Android)", "license": "LGPL-3.0-only",
         "copyright": "Copyright (C) The Qt Company Ltd. and other contributors.", "platforms": ["windows", "android"],
         "texts": ["LGPL-3.0-only", "GPL-3.0-only"], "kind": "qt",
         "modules": {"windows": sorted(qt_win), "android": sorted(qt_and)}},
        {"name": "QR Code generator library (qrcodegen)", "version": "", "license": "MIT",
         "copyright": "Copyright (c) Project Nayuki.", "platforms": ["windows", "android"], "texts": ["qrcodegen"],
         "kind": "main"},
        {"name": "GCC runtime (libstdc++, libgcc)", "version": "13.1.0", "license": "GPL-3.0 WITH GCC-exception-3.1",
         "copyright": "Copyright (C) Free Software Foundation, Inc.", "platforms": ["windows"],
         "texts": ["GCC-exception-3.1", "GPL-3.0-only"], "kind": "main"},
        {"name": "MinGW-w64 runtime, winpthreads", "version": "", "license": "MIT, ZPL-2.1, public domain",
         "copyright": "Copyright (c) 2009, 2010, 2011 by the mingw-w64 project", "platforms": ["windows"],
         "texts": ["mingw-w64", "winpthreads"], "kind": "main"},
        {"name": "LLVM libc++", "version": "", "license": "Apache-2.0 WITH LLVM-exception",
         "copyright": "Copyright (c) LLVM Project contributors.", "platforms": ["android"], "texts": ["LLVM"],
         "kind": "main"},
        {"name": "AndroidX Core", "version": "1.17.0", "license": "Apache-2.0",
         "copyright": "Copyright (C) The Android Open Source Project", "platforms": ["android"], "texts": ["Apache-2.0"],
         "kind": "main"},
    ]
    merged: dict[tuple, dict] = {}
    for plat, third in (("windows", third_win), ("android", third_and)):
        for (key, _), e in third.items():
            m = merged.setdefault((key, e["version"]), {**e, "platforms": [], "kind": "qt3rd",
                                                         "texts": license_ids(e["license"])})
            if plat not in m["platforms"]:
                m["platforms"].append(plat)
    # gleiche Software in mehreren Teilpaketen (z. B. zlib in Gui und Network) nur einmal
    unique: dict[tuple, dict] = {}
    for e in sorted(merged.values(), key=lambda e: (e["name"].lower(), e["version"])):
        k = (e["name"].lower(), e["version"], e["license"])
        if k in unique:
            unique[k]["platforms"] = sorted(set(unique[k]["platforms"]) | set(e["platforms"]))
        else:
            unique[k] = e
    components += list(unique.values())

    texts_dir = OUT / "texts"
    if texts_dir.exists():
        for f in texts_dir.glob("*.txt"):
            f.unlink()
    texts_dir.mkdir(parents=True, exist_ok=True)
    needed = sorted({t for c in components for t in c["texts"]})
    fixed = {
        "qrcodegen": lambda: re.search(r"/\*(.*?)\*/", (ROOT / "third_party/qrcodegen/qrcodegen.hpp")
                                       .read_text(encoding="utf-8"), re.S).group(1).replace("\n * ", "\n")
                                       .replace("\n *", "\n").strip(),
        "GCC-exception-3.1": lambda: (MINGW / "gcc" / "COPYING.RUNTIME").read_text(encoding="utf-8", errors="replace"),
        "mingw-w64": lambda: (MINGW / "mingw-w64" / "COPYING.MinGW-w64-runtime.txt")
                     .read_text(encoding="utf-8", errors="replace"),
        "winpthreads": lambda: (MINGW / "winpthreads" / "COPYING").read_text(encoding="utf-8", errors="replace"),
        "LLVM": lambda: llvm_notice(),
    }
    for t in needed:
        text = fixed[t]() if t in fixed else license_text(t)
        (texts_dir / f"{t}.txt").write_bytes(text.replace("\r\n", "\n").strip().encode("utf-8") + b"\n")
    for c in components:
        c["platforms"] = sorted(c["platforms"])
    (OUT / "licenses.json").write_bytes(
        json.dumps({"components": components, "texts": needed}, ensure_ascii=False, indent=1).encode("utf-8") + b"\n")
    print(f"{len(components)} Bestandteile, {len(needed)} Lizenztexte -> {OUT}")
    return 0


def llvm_notice() -> str:
    ndk = sorted(glob.glob(NDK_GLOB))
    if not ndk:
        sys.exit("NDK-NOTICE.toolchain fehlt")
    text = Path(ndk[-1]).read_text(encoding="utf-8", errors="replace")
    start = text.index("The LLVM Project is under the Apache License v2.0 with LLVM Exceptions:")
    end = text.index("Software from third parties included in the LLVM Project:", start)
    return text[start:end]


if __name__ == "__main__":
    sys.exit(main())
