#!/usr/bin/env python3
"""Prueft die Inhaltspakete von Volta (nur Standardbibliothek).

Aufruf:
    python3 tools/validate_content.py            # alle Baende
    python3 tools/validate_content.py volta1     # ein Band

Rueckgabewert 0 bei fehlerfreien Paketen (Warnungen erlaubt), sonst 1.
Siehe docs/content-format.md.
"""
from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CONTENT = ROOT / "content"

FIELD = re.compile(r"^[A-E](1[0-2]|[1-9])$")
ANCHOR = re.compile(
    r"^([A-E](1[0-2]|[1-9])|P:(\+9V|0V|P1a|P1s|P1b|P2a|P2s|P2b|S1|S2|LS1|LS2)|MESS:(COM|VOHM|A)|[A-Z]+[0-9]+\.[a-z]+)$"
)
KEY = re.compile(r"^[a-z][a-z0-9_]*$")
EXP_ID = re.compile(r"^(volta1|volta2|volta3|zuse1)\.[0-9]+\.[0-9]+$")
COLORS = {"rot", "schwarz", "blau", "gelb", "gruen", "weiss"}
QUANTITIES = {"strom_mA", "spannung_V", "leistung_mW", "helligkeit", "drehzahl", "frequenz_Hz", "temperatur_C"}
STATES = {"intakt", "ueberlastet", "zerstoert"}
REQUIRED = ["id", "band", "stufe", "titel_key", "zerstoerend", "bauteile_frei", "frage_key",
            "vermutungen", "vorlage", "erfolg", "hilfen", "erklaerung_key", "jetzt_in_echt_key"]
LANGS = ["de", "fr", "en", "lb"]
FORBIDDEN = re.compile(r"kosmos", re.IGNORECASE)


class Report:
    def __init__(self) -> None:
        self.errors: list[str] = []
        self.warnings: list[str] = []

    def error(self, where: str, msg: str) -> None:
        self.errors.append(f"FEHLER   {where}: {msg}")

    def warn(self, where: str, msg: str) -> None:
        self.warnings.append(f"WARNUNG  {where}: {msg}")


def load_json(path: Path, rep: Report):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        rep.error(str(path.relative_to(ROOT)), f"JSON nicht lesbar: {exc}")
        return None


def collect_keys(exp: dict) -> set[str]:
    keys: set[str] = set()
    for k in ("titel_key", "frage_key", "erklaerung_key", "mehr_wissen_key", "jetzt_in_echt_key"):
        if isinstance(exp.get(k), str):
            keys.add(exp[k])
    for k in ("vermutungen", "hilfen", "schritte", "weiterforschen"):
        for v in exp.get(k, []) or []:
            if isinstance(v, str):
                keys.add(v)
    return keys


def check_condition(cond, where: str, part_ids: set[str], rep: Report) -> None:
    if not isinstance(cond, dict) or not cond:
        rep.error(where, "Erfolgsbedingung muss ein nicht leeres Objekt sein")
        return
    if "alle" in cond or "eine" in cond:
        items = cond.get("alle", cond.get("eine"))
        if not isinstance(items, list) or not items:
            rep.error(where, "'alle'/'eine' braucht eine nicht leere Liste")
            return
        for i, c in enumerate(items):
            check_condition(c, f"{where}[{i}]", part_ids, rep)
        return
    if "nicht" in cond:
        check_condition(cond["nicht"], f"{where}.nicht", part_ids, rep)
        return
    ref = cond.get("bauteil")
    if ref is not None and ref not in part_ids:
        rep.error(where, f"Bauteil '{ref}' ist nicht in der Vorlage")
    if "feld" in cond and not ANCHOR.match(str(cond["feld"])):
        rep.error(where, f"ungueltiges Feld '{cond['feld']}'")
    if "groesse" in cond:
        if cond["groesse"] not in QUANTITIES:
            rep.error(where, f"unbekannte Groesse '{cond['groesse']}'")
        rng = cond.get("zwischen")
        if rng is not None:
            if not (isinstance(rng, list) and len(rng) == 2 and rng[0] < rng[1]):
                rep.error(where, "'zwischen' braucht [min, max] mit min < max")
        elif "min" not in cond and "max" not in cond:
            rep.error(where, "Groessenbedingung ohne 'zwischen', 'min' oder 'max'")
    elif "zustand" in cond:
        if cond["zustand"] not in STATES:
            rep.error(where, f"unbekannter Zustand '{cond['zustand']}'")
    else:
        rep.error(where, "Bedingung ohne 'groesse', 'zustand', 'alle', 'eine' oder 'nicht'")


def check_experiment(path: Path, exp: dict, band: str, catalog: dict, rep: Report) -> set[str]:
    where = str(path.relative_to(ROOT))
    for k in REQUIRED:
        if k not in exp:
            rep.error(where, f"Pflichtfeld '{k}' fehlt")
    if not EXP_ID.match(str(exp.get("id", ""))):
        rep.error(where, f"ungueltige id '{exp.get('id')}'")
    elif exp["id"].split(".")[0] != band:
        rep.error(where, f"id passt nicht zum Band '{band}'")
    expected_name = ".".join(str(exp.get("id", "")).split(".")[1:]) + ".json"
    if path.name != expected_name:
        rep.error(where, f"Dateiname sollte '{expected_name}' sein")
    if exp.get("band") != band:
        rep.error(where, "Feld 'band' passt nicht zum Verzeichnis")
    if not isinstance(exp.get("zerstoerend"), bool):
        rep.error(where, "'zerstoerend' muss true oder false sein")

    stufe = exp.get("stufe", 0)
    types = catalog["types"]
    for t in exp.get("bauteile_frei", []):
        if t != "leitung" and t not in types and t not in catalog["modules"]:
            rep.error(where, f"Bauteiltyp '{t}' fehlt im Katalog")

    tpl = exp.get("vorlage", {}) or {}
    part_ids: set[str] = set()
    used_sockets: dict[str, int] = {}
    for p in tpl.get("bauteile", []):
        pid = p.get("id", "?")
        if pid in part_ids:
            rep.error(where, f"Bauteil-ID '{pid}' doppelt")
        part_ids.add(pid)
        typ = p.get("typ")
        spec = types.get(typ)
        if spec is None:
            rep.error(where, f"Bauteil {pid}: Typ '{typ}' unbekannt")
            continue
        if spec.get("ab_band") == band and spec.get("ab_stufe", 0) > stufe:
            rep.warn(where, f"Bauteil {pid}: Typ '{typ}' erst ab Stufe {spec['ab_stufe']} freigeschaltet")
        felder = p.get("felder")
        if isinstance(felder, dict):
            missing = set(spec["anschluesse"]) - set(felder)
            extra = set(felder) - set(spec["anschluesse"])
            if missing:
                rep.error(where, f"Bauteil {pid}: Anschluesse fehlen {sorted(missing)}")
            if extra:
                rep.error(where, f"Bauteil {pid}: unbekannte Anschluesse {sorted(extra)}")
            fields = list(felder.values())
        elif isinstance(felder, list):
            fields = felder
            if len(fields) != len(spec["anschluesse"]):
                rep.error(where, f"Bauteil {pid}: {len(spec['anschluesse'])} Felder erwartet")
        else:
            rep.error(where, f"Bauteil {pid}: 'felder' fehlt")
            continue
        for f in fields:
            if not FIELD.match(str(f)):
                rep.error(where, f"Bauteil {pid}: ungueltiges Feld '{f}'")
            used_sockets[f] = used_sockets.get(f, 0) + 1
        if len(fields) == 2 and all(FIELD.match(str(f)) for f in fields):
            (r1, c1), (r2, c2) = (ord(fields[0][0]), int(fields[0][1:])), (ord(fields[1][0]), int(fields[1][1:]))
            span = abs(r1 - r2) + abs(c1 - c2)
            if (r1 != r2 and c1 != c2) or span not in (1, 2):
                rep.error(where, f"Bauteil {pid}: nur waagrecht/senkrecht mit Spannweite 1 oder 2 Feldern")
        if "platine_2x2" in spec.get("spannweite", []) and isinstance(felder, dict) and len(fields) == 4:
            lage = spec.get("lage", spec["anschluesse"])
            try:
                (r0, c0) = (ord(felder[lage[0]][0]), int(felder[lage[0]][1:]))
                expected = [(r0, c0), (r0, c0 + 1), (r0 + 1, c0), (r0 + 1, c0 + 1)]
                got = [(ord(felder[k][0]), int(felder[k][1:])) for k in lage]
                ok = got == expected or got == list(reversed(expected))  # auch umgedreht (halbe Drehung)
            except (KeyError, ValueError, IndexError):
                ok = False
            if not ok:
                rep.error(where, f"Bauteil {pid}: Platine braucht einen 2x2-Block in der Lage {lage}")
        if spec.get("varianten") and "wert" in p:
            volts = [v.get("u_nenn_V") for v in spec["varianten"]]
            if p["wert"] not in volts:
                rep.error(where, f"Bauteil {pid}: Nennspannung {p['wert']} V nicht im Katalog {volts}")
        if typ == "widerstand" and "wert" not in p:
            rep.error(where, f"Bauteil {pid}: Widerstand ohne 'wert'")
    for f, n in used_sockets.items():
        if n > 5:
            rep.error(where, f"Feld {f}: {n} Bauteilbeine, maximal 5 Steckplaetze")

    for m in tpl.get("module", []):
        if m.get("typ") not in catalog["modules"]:
            rep.error(where, f"Modul '{m.get('id')}': Typ '{m.get('typ')}' unbekannt")
        part_ids.add(m.get("id", "?"))

    for i, w in enumerate(tpl.get("leitungen", [])):
        for end in ("von", "nach"):
            if not ANCHOR.match(str(w.get(end, ""))):
                rep.error(where, f"Leitung {i}: ungueltiger Anschluss '{w.get(end)}'")
        if w.get("von") == w.get("nach"):
            rep.error(where, f"Leitung {i}: Anfang und Ende gleich")
        if "farbe" in w and w["farbe"] not in COLORS:
            rep.error(where, f"Leitung {i}: unbekannte Farbe '{w['farbe']}'")
    pult = tpl.get("pult_stellung", {}) or {}
    for k in pult:
        if k not in ("batterie_V", "schalter", "p1", "p2"):
            rep.error(where, f"pult_stellung: unbekannter Schluessel '{k}'")
    if "batterie_V" in pult and pult["batterie_V"] not in (3, 4.5, 9):
        rep.error(where, "pult_stellung: batterie_V muss 3, 4.5 oder 9 sein")
    for k in ("p1", "p2"):
        if k in pult and not (isinstance(pult[k], (int, float)) and 0 <= pult[k] <= 1):
            rep.error(where, f"pult_stellung: {k} muss zwischen 0 und 1 liegen")
    for a in tpl.get("anzeigen", []):
        if not ANCHOR.match(str(a)):
            rep.error(where, f"Spannungsanzeige an ungueltigem Feld '{a}'")

    check_condition(exp.get("erfolg"), f"{where}:erfolg", part_ids, rep)

    keys = collect_keys(exp)
    for k in keys:
        if not KEY.match(k):
            rep.error(where, f"ungueltiger Schluessel '{k}'")
    return keys


def load_catalog(rep: Report) -> dict:
    data = load_json(CONTENT / "catalog" / "bauteile.json", rep) or {}
    types = {b["typ"]: b for b in data.get("bauteile", [])}
    modules = {m["typ"]: m for m in data.get("module", [])}
    keys = {b.get("lexikon_key") for b in data.get("bauteile", []) + data.get("module", []) if b.get("lexikon_key")}
    return {"types": types, "modules": modules, "keys": keys}


def check_band(band_dir: Path, catalog: dict, rep: Report) -> int:
    band = band_dir.name
    pkg = load_json(band_dir / "paket.json", rep)
    if pkg is None:
        return 0
    if pkg.get("band") != band:
        rep.error(f"content/{band}/paket.json", "Feld 'band' passt nicht zum Verzeichnis")
    used: set[str] = set(catalog["keys"])
    stages_path = band_dir / "stufen.json"
    if stages_path.exists():
        stages = load_json(stages_path, rep) or {}
        nrs = [st.get("nr") for st in stages.get("stufen", [])]
        if nrs != sorted(set(nrs)) or any(not isinstance(n, int) for n in nrs):
            rep.error(f"content/{band}/stufen.json", "Stufennummern muessen eindeutig und aufsteigend sein")
        for st in stages.get("stufen", []):
            for k in ("titel_key", "frage_key"):
                if not isinstance(st.get(k), str) or not KEY.match(st[k]):
                    rep.error(f"content/{band}/stufen.json", f"Stufe {st.get('nr')}: '{k}' fehlt oder ungueltig")
                else:
                    used.add(st[k])
    count = 0
    for path in sorted((band_dir / "experiments").glob("*.json")):
        exp = load_json(path, rep)
        if exp is None:
            continue
        count += 1
        used |= check_experiment(path, exp, band, catalog, rep)

    texts: dict[str, dict] = {}
    for lang in LANGS:
        p = band_dir / "i18n" / f"{lang}.json"
        if p.exists():
            texts[lang] = load_json(p, rep) or {}
        elif lang == pkg.get("quellsprache", "de"):
            rep.error(f"content/{band}/i18n", f"Quellsprache '{lang}' fehlt")
    src = texts.get(pkg.get("quellsprache", "de"), {})
    for k in sorted(used):
        if k not in src:
            rep.error(f"content/{band}/i18n/de.json", f"Schluessel '{k}' fehlt")
    for lang in LANGS:
        if lang == "de":
            continue
        missing = [k for k in used if k not in texts.get(lang, {})]
        if missing:
            rep.warn(f"content/{band}/i18n/{lang}.json", f"{len(missing)} Uebersetzungen fehlen")
    for lang, tx in texts.items():
        for k, v in tx.items():
            if isinstance(v, str) and FORBIDDEN.search(v):
                rep.error(f"content/{band}/i18n/{lang}.json", f"verbotener Begriff in '{k}'")
    return count


def main(argv: list[str]) -> int:
    rep = Report()
    catalog = load_catalog(rep)
    bands = [CONTENT / b for b in argv[1:]] or [
        d for d in sorted(CONTENT.iterdir()) if d.is_dir() and (d / "paket.json").exists()
    ]
    total = 0
    for b in bands:
        if not (b / "paket.json").exists():
            rep.error(str(b.relative_to(ROOT)), "kein Inhaltspaket (paket.json fehlt)")
            continue
        total += check_band(b, catalog, rep)
    for line in rep.errors + rep.warnings:
        print(line)
    print(f"{total} Experiment(e) geprueft: {len(rep.errors)} Fehler, {len(rep.warnings)} Warnung(en).")
    return 1 if rep.errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
