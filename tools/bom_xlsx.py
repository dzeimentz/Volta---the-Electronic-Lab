#!/usr/bin/env python3
"""Stueckliste (BOM) fuer Volta 1, 2 und 3 als Excel-Datei.

Quelle sind die Musteraufbauten (vorlage) aller Experimente: Je Bauteil und Wert zaehlt, wie viele Stueck ein
einzelnes Experiment hoechstens gleichzeitig braucht - so viele muessten in einem echten Kasten liegen. Dazu
Module, Pultgeraete, Werkzeuge und Leitungen je Farbe. Ausgabe: Release/Stueckliste/Volta_Stueckliste.xlsx

    python tools/bom_xlsx.py [ausgabe.xlsx]
"""
import collections
import glob
import json
import os
import re
import sys

from openpyxl import Workbook
from openpyxl.styles import Alignment, Border, Font, PatternFill, Side
from openpyxl.utils import get_column_letter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BANDS = [("volta1", "Volta 1", "8–12 Jahre"), ("volta2", "Volta 2", "11–14 Jahre"), ("volta3", "Volta 3", "13–16 Jahre")]

THEMES = {"licht": "Licht", "schalten": "Schalten", "bewegung": "Bewegung und Ton", "widerstaende": "Widerstände und Sensoren",
          "halbleiter": "Dioden und Transistoren", "speicher": "Kondensatoren und Spulen", "messen": "Messzubehör"}
THEME_ORDER = list(THEMES.values())

# Vorschlag fuer ein echtes Bauteil (Kennwerte wie im Katalog content/catalog/bauteile.json)
REAL = {
    "widerstand": "Kohleschicht 0,25 W, Reihe E12",
    "gluehlampe": "Glühlampe, Nennwert siehe Wert",
    "taster": "Drucktaster, Schließer", "wechselschalter": "Kippschalter, Umschalter", "reedkontakt": "Reedkontakt, Schließer",
    "motor": "Kleinmotor 3–6 V (Ra ≈ 8 Ω)", "summer": "Summer, ca. 2,4 kHz", "piezo": "Piezo-Schallgeber, ca. 3,2 kHz",
    "elektromagnet": "Elektromagnet, Spule ≈ 100 Ω", "relais": "Kleinrelais, Spule ≈ 180 Ω",
    "led_rot": "LED 5 mm rot, 20 mA", "led_gruen": "LED 5 mm grün, 20 mA", "led_gelb": "LED 5 mm gelb, 20 mA",
    "led_blau": "LED 5 mm blau, 20 mA", "led_weiss": "LED 5 mm weiß, 20 mA", "ir_led": "IR-LED 940 nm, 50 mA",
    "diode_si": "Si-Diode, z. B. 1N4148 / 1N4001", "diode_ge": "Germaniumdiode, z. B. AA118",
    "zdiode_4v7": "Z-Diode 4,7 V, 0,5 W", "zdiode_5v6": "Z-Diode 5,6 V, 0,5 W",
    "kondensator": "Folie/Keramik, Wert siehe Spalte", "elko": "Elektrolytkondensator 16 V",
    "drehkondensator": "Drehkondensator 10–500 pF",
    "fotowiderstand": "LDR, ≈ 10 kΩ bei 10 lx", "heissleiter": "NTC 10 kΩ, B 3950", "feuchtesonde": "Feuchtesonde, zwei Elektroden",
    "fototransistor": "NPN-Fototransistor", "optokoppler": "Optokoppler, z. B. PC817 / 4N25",
    "transistor_npn": "NPN-Kleinsignal, z. B. BC547 (β ≈ 200)", "transistor_pnp": "PNP-Kleinsignal, z. B. BC557 (β ≈ 250)",
    "leistung_npn": "NPN-Leistung, z. B. BD139", "leistung_pnp": "PNP-Leistung, z. B. BD140",
    "mosfet_n": "N-MOSFET Logic-Level, z. B. BS170", "mosfet_p": "P-MOSFET Logic-Level, z. B. BS250",
    "thyristor": "Thyristor für Kleinspannung, z. B. BT169", "triac": "Triac für Kleinspannung, z. B. Z0103",
    "diac": "Diac (Modell mit 6 V Kippspannung, didaktisch)", "spannungsregler_5v": "Festspannungsregler 5 V, z. B. 78L05",
    "trimmer_10k": "Trimmpotentiometer 10 kΩ", "trimmer_100k": "Trimmpotentiometer 100 kΩ",
    "spule_100mh": "Festinduktivität 100 mH", "spule_1h": "Spule 1 H", "spule_220uh": "Antennenspule 220 µH",
    "spule_1mh": "Festinduktivität 1 mH", "spule_10mh": "Festinduktivität 10 mH",
    "drossel_10mh": "Speicherdrossel 10 mH", "drossel_1mh": "Drossel 1 mH", "uebertrager": "Übertrager 2 × 10 mH, k ≈ 0,98",
    "kraftsensor": "Wägezelle, Vollbrücke 4 × 1 kΩ", "hallsensor": "Linearer Hall-Sensor, z. B. SS49E",
    "kristallhoerer": "Kristallohrhörer (hochohmig)",
    "messstelle_v": "Spannungsmessstelle (Anschluss Messgerät)", "messstelle_a": "Strommessstelle (Anschluss Messgerät)",
    "messstelle_ohm": "Widerstandsmessstelle (Anschluss Messgerät)", "tastkopf": "Tastkopf Oszilloskop",
    "logiktastkopf": "Logik-Tastkopf (Logikanalysator)",
}
MODULE_REAL = {
    "blinkmodul": "Astabile Kippstufe, 2 Transistoren, 2 × 22 µF", "tonmodul": "Melodie-Baustein",
    "zeitmodul": "Monoflop (Zeitschalter)", "verstaerkermodul": "Zweistufiger Transistorverstärker",
    "opmodul": "Operationsverstärker", "timer555": "Timer NE555", "mwmodul": "MW-Empfänger mit Transistorstufe",
    "ukwmodul": "UKW-Empfänger mit Endstufe", "gattermodul": "4 × NAND mit Schmitt-Trigger (wie 4093)",
    "zaehlermodul": "4-Bit-Binärzähler (wie 4520)", "dekadenzaehler": "Dekadenzähler (wie 4017)",
    "anzeigemodul": "Decoder (wie 4511) + 7-Segment, gem. Kathode, 7 × 680 Ω",
}
DESK = [  # (Schluessel, Name, echtes Gegenstueck)
    ("batterie", "Batterie 1 (Pult)", "Batteriehalter 1,5 / 3 / 4,5 / 9 V"),
    ("batterie2", "Batterie 2 (Pult, symmetrische Versorgung)", "zweiter Batteriehalter 4,5 / 9 V"),
    ("schalter", "Schalter S (Pult)", "Kippschalter"), ("p1", "Drehregler P1 (Pult)", "Potentiometer"),
    ("p2", "Drehregler P2 (Pult)", "Potentiometer"), ("lautsprecher", "Lautsprecher (Pult)", "Kleinlautsprecher 8 Ω"),
    ("messgeraet", "Messgerät / Anzeigegerät", "Multimeter"), ("wechselquelle", "Wechselquelle (Pult)", "Kleinspannungs-Trafo 6/9 V~"),
    ("generator", "Funktionsgenerator (Pult)", "Funktionsgenerator"), ("oszilloskop", "Oszilloskop", "Oszilloskop 2 Kanäle"),
    ("kennlinienschreiber", "Kennlinienschreiber (Pult)", "Kennlinienschreiber"),
    ("frequenzgang", "Frequenzgang-Schreiber", "Netzwerkanalysator / Wobbelgenerator"),
    ("logikanalysator", "Logikanalysator", "Logikanalysator 8 Kanäle"), ("senderanlage", "Senderanlage (Pult)", "Prüfsender MW"),
]
TOOLS = {"taschenlampe": "Taschenlampe", "foehn": "Föhn", "wasser": "Wasserglas", "karte": "Abdeckkarte",
         "gewicht500": "Gewicht 500 g", "magnet": "Magnet"}
COLORS = {"rot": "rot", "schwarz": "schwarz", "blau": "blau", "gelb": "gelb", "gruen": "grün", "weiss": "weiß"}


def names():
    """Deutsche Anzeigenamen aus app/qml/Texts.qml (partName/moduleName)."""
    src = open(os.path.join(ROOT, "app", "qml", "Texts.qml"), encoding="utf-8").read()
    return dict(re.findall(r'case "([a-z0-9_]+)": return qsTr\("([^"]+)"\)', src))


def num(v, decimals=None):
    if decimals is None:
        decimals = 0 if abs(v - round(v)) < 1e-9 else (1 if abs(v * 10 - round(v * 10)) < 1e-6 else 2)
    return f"{v:.{decimals}f}".replace(".", ",")


def si(v, unit):
    for f, p in ((1e6, "M"), (1e3, "k"), (1.0, ""), (1e-3, "m"), (1e-6, "µ"), (1e-9, "n"), (1e-12, "p")):
        if abs(v) >= f * 0.9995:
            return f"{num(v / f)} {p}{unit}"
    return f"{num(v)} {unit}"


def value_text(typ, wert):
    if wert is None:
        return ""
    if typ == "widerstand":
        return si(wert, "Ω")
    if typ in ("kondensator", "elko", "drehkondensator"):
        return si(wert, "F")
    if typ == "gluehlampe":
        return f"{num(wert)} V"
    return num(wert)


def collect(band, catalog):
    parts = collections.defaultdict(lambda: {"max": 0, "exps": [], "typ": "", "wert": None})
    modules = collections.defaultdict(lambda: {"max": 0, "exps": []})
    desk = collections.defaultdict(list)
    tools = collections.defaultdict(list)
    wires = collections.defaultdict(int)
    wires_total = 0
    volts = set()
    files = glob.glob(os.path.join(ROOT, "content", band, "experiments", "*.json"))
    key_of = lambda f: tuple(int(x) for x in os.path.basename(f)[:-5].split("."))
    texts = json.load(open(os.path.join(ROOT, "content", band, "i18n", "de.json"), encoding="utf-8"))
    for f in sorted(files, key=key_of):
        e = json.load(open(f, encoding="utf-8"))
        exp, v = e["id"].split(".", 1)[1], e["vorlage"]
        count = collections.Counter()
        for p in v.get("bauteile", []):
            w = p.get("wert")
            if p["typ"] == "widerstand" and w is None:
                w = 1000.0
            count[(p["typ"], w)] += 1
        for (typ, w), n in count.items():
            k = f"{typ}|{value_text(typ, w)}"
            parts[k].update(typ=typ, wert=w)
            parts[k]["max"] = max(parts[k]["max"], n)
            parts[k]["exps"].append(exp)
        mc = collections.Counter(m["typ"] for m in v.get("module", []))
        for typ, n in mc.items():
            modules[typ]["max"] = max(modules[typ]["max"], n)
            modules[typ]["exps"].append(exp)
        for t in {w["typ"] for w in v.get("werkzeuge", [])}:
            tools[t].append(exp)
        wc = collections.Counter(l.get("farbe", "rot") for l in v.get("leitungen", []))
        for c, n in wc.items():
            wires[c] = max(wires[c], n)
        wires_total = max(wires_total, sum(wc.values()))
        ps = v.get("pult_stellung", {})
        eps = {x for l in v.get("leitungen", []) for x in (l["von"], l["nach"]) if x.startswith("P:")}
        types = {p["typ"] for p in v.get("bauteile", [])}
        used = set()
        if eps & {"P:+9V"} or "batterie_V" in ps: used.add("batterie")
        if eps & {"P:+B2", "P:-B2"} or "batterie2_V" in ps: used.add("batterie2")
        if eps & {"P:S1", "P:S2"}: used.add("schalter")
        if any(x.startswith("P:P1") for x in eps): used.add("p1")
        if any(x.startswith("P:P2") for x in eps): used.add("p2")
        if eps & {"P:LS1", "P:LS2"}: used.add("lautsprecher")
        if eps & {"P:G"}: used.add("generator")
        if eps & {"P:W~a", "P:W~b"}: used.add("wechselquelle")
        if eps & {"P:KS+", "P:KS-"}: used.add("kennlinienschreiber")
        if eps & {"P:ANT"}: used.add("senderanlage")
        if "oszilloskop" in ps or "tastkopf" in types: used.add("oszilloskop")
        if "logiktastkopf" in types: used.add("logikanalysator")
        if "messart" in ps or "anzeigegeraet" in ps or types & {"messstelle_v", "messstelle_a", "messstelle_ohm"}:
            used.add("messgeraet")
        if band == "volta3" and "frequenzgang" in " ".join(texts.get(f"exp3_{exp.replace('.', '_')}_{s}", "")
                                                             for s in ("s1", "s2", "s3", "s4", "s5", "s6", "frage")).lower():
            used.add("frequenzgang")
        if band == "volta3" and exp == "0.1":
            used.add("frequenzgang")
        for u in used:
            desk[u].append(exp)
        if "batterie_V" in ps:
            volts.add(ps["batterie_V"])
    return parts, modules, desk, tools, wires, wires_total, volts, len(files)


HEAD_FILL = PatternFill("solid", fgColor="1F3A5F")
SUB_FILL = PatternFill("solid", fgColor="E6ECF2")
BAND_FILL = {"Volta 1": "2E7D32", "Volta 2": "1565C0", "Volta 3": "A4266B"}
THIN = Side(style="thin", color="B5C2CF")
BORDER = Border(left=THIN, right=THIN, top=THIN, bottom=THIN)
F = "Arial"


def style_header(ws, row, ncol, fill=HEAD_FILL):
    for c in range(1, ncol + 1):
        cell = ws.cell(row=row, column=c)
        cell.font = Font(name=F, bold=True, color="FFFFFF")
        cell.fill = fill
        cell.alignment = Alignment(vertical="center", wrap_text=True)
        cell.border = BORDER


def body(cell, bold=False, color="000000"):
    cell.font = Font(name=F, bold=bold, color=color)
    cell.border = BORDER
    cell.alignment = Alignment(vertical="top", wrap_text=True)


def exp_list(exps):
    return ", ".join(exps) if len(exps) <= 6 else ", ".join(exps[:6]) + f" … (+{len(exps) - 6})"


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "Release", "Stueckliste", "Volta_Stueckliste.xlsx")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    catalog = json.load(open(os.path.join(ROOT, "content", "catalog", "bauteile.json"), encoding="utf-8"))
    theme_of = {b["typ"]: THEMES.get(b.get("thema"), "Sonstiges") for b in catalog["bauteile"]}
    nm = names()
    wb = Workbook()
    ov = wb.active
    ov.title = "Übersicht"
    band_sheets = []
    all_keys = {}
    for band, title, age in BANDS:
        parts, modules, desk, tools, wires, wires_total, volts, nexp = collect(band, catalog)
        ws = wb.create_sheet(title)
        band_sheets.append((title, ws, nexp))
        ws["A1"] = f"Stückliste {title} ({age}) – {nexp} Experimente"
        ws["A1"].font = Font(name=F, bold=True, size=14, color=BAND_FILL[title])
        ws["A2"] = ("Menge = höchste Stückzahl, die ein einzelnes Experiment gleichzeitig braucht (Musteraufbau). "
                    "Quelle: content/" + band + "/experiments, erzeugt mit tools/bom_xlsx.py.")
        ws["A2"].font = Font(name=F, italic=True, color="4A5866")
        hdr = ["Pos.", "Schlüssel", "Gruppe", "Bauteil", "Wert", "Menge", "in Exp.", "Experimente", "Vorschlag echtes Bauteil"]
        r = 4
        for c, h in enumerate(hdr, 1):
            ws.cell(row=r, column=c, value=h)
        style_header(ws, r, len(hdr), PatternFill("solid", fgColor=BAND_FILL[title]))
        rows = []
        for k, p in parts.items():
            grp = theme_of.get(p["typ"], "Sonstiges")
            rows.append((THEME_ORDER.index(grp) if grp in THEME_ORDER else 99, grp, nm.get(p["typ"], p["typ"]),
                         p["wert"] or 0, k, value_text(p["typ"], p["wert"]), p["max"], p["exps"], REAL.get(p["typ"], "")))
        rows.sort(key=lambda x: (x[0], x[2], x[3]))
        pos = 0
        first = r + 1
        for _, grp, name, _, k, val, qty, exps, real in rows:
            r += 1
            pos += 1
            vals = [pos, k, grp, name, val, qty, len(exps), exp_list(exps), real]
            for c, v in enumerate(vals, 1):
                body(ws.cell(row=r, column=c, value=v), color="0000FF" if c == 6 else "000000")
            all_keys.setdefault(k, (grp, name, val, real))
        for typ in sorted(modules, key=lambda t: nm.get(t, t)):
            m = modules[typ]
            r += 1
            pos += 1
            k = f"modul|{typ}"
            vals = [pos, k, "Module", nm.get(typ, typ), "", m["max"], len(m["exps"]), exp_list(m["exps"]), MODULE_REAL.get(typ, "")]
            for c, v in enumerate(vals, 1):
                body(ws.cell(row=r, column=c, value=v), color="0000FF" if c == 6 else "000000")
            all_keys.setdefault(k, ("Module", nm.get(typ, typ), "", MODULE_REAL.get(typ, "")))
        for c, n in sorted(wires.items(), key=lambda x: list(COLORS).index(x[0]) if x[0] in COLORS else 9):
            r += 1
            pos += 1
            k = f"leitung|{c}"
            vals = [pos, k, "Leitungen", f"Leitung {COLORS.get(c, c)}", "", n, "", "", "Steckbrücke / Laborleitung"]
            for cc, v in enumerate(vals, 1):
                body(ws.cell(row=r, column=cc, value=v), color="0000FF" if cc == 6 else "000000")
            all_keys.setdefault(k, ("Leitungen", f"Leitung {COLORS.get(c, c)}", "", "Steckbrücke / Laborleitung"))
        last_part = r
        r += 1
        ws.cell(row=r, column=4, value="Stück gesamt (Bauteile, Module, Leitungen)")
        ws.cell(row=r, column=6, value=f"=SUM(F{first}:F{last_part})")
        for c in (4, 6):
            body(ws.cell(row=r, column=c), bold=True)
        r += 1
        ws.cell(row=r, column=4, value=f"Höchstzahl Leitungen in einem Experiment (alle Farben): {wires_total}")
        ws.cell(row=r, column=4).font = Font(name=F, italic=True, color="4A5866")
        # Pult und Geraete
        r += 2
        ws.cell(row=r, column=1, value="Bedienpult, Geräte und Werkzeuge (in der App virtuell, für einen echten Kasten das Gegenstück)")
        ws.cell(row=r, column=1).font = Font(name=F, bold=True, size=12)
        r += 1
        for c, h in enumerate(["", "Schlüssel", "Gruppe", "Gerät / Werkzeug", "", "Menge", "in Exp.", "Experimente", "Echtes Gegenstück"], 1):
            ws.cell(row=r, column=c, value=h)
        style_header(ws, r, 9, PatternFill("solid", fgColor="4A5866"))
        for key, name, real in DESK:
            if key not in desk:
                continue
            r += 1
            note = real + (f" (genutzt: {', '.join(num(x) + ' V' for x in sorted(volts))})" if key == "batterie" and volts else "")
            vals = ["", f"pult|{key}", "Pult und Geräte", name, "", 1, len(desk[key]), exp_list(desk[key]), note]
            for c, v in enumerate(vals, 1):
                body(ws.cell(row=r, column=c, value=v))
            all_keys.setdefault(f"pult|{key}", ("Pult und Geräte", name, "", real))
        for t in sorted(tools):
            r += 1
            vals = ["", f"werkzeug|{t}", "Werkzeuge", TOOLS.get(t, t), "", 1, len(tools[t]), exp_list(tools[t]), "Haushalt / Zubehör"]
            for c, v in enumerate(vals, 1):
                body(ws.cell(row=r, column=c, value=v))
            all_keys.setdefault(f"werkzeug|{t}", ("Werkzeuge", TOOLS.get(t, t), "", "Haushalt / Zubehör"))
        for c, w in enumerate([6, 26, 22, 30, 11, 8, 8, 34, 44], 1):
            ws.column_dimensions[get_column_letter(c)].width = w
        ws.column_dimensions["B"].hidden = True
        ws.freeze_panes = "A5"
        ws.sheet_view.zoomScale = 100

    # Reihe gesamt: Menge je Band ueber SUMIFS, Kasten fuer die ganze Reihe = groesste Menge
    gs = wb.create_sheet("Reihe gesamt")
    gs["A1"] = "Stückliste der ganzen Reihe – Menge je Band und Kasten für alle drei Bände"
    gs["A1"].font = Font(name=F, bold=True, size=14, color="1F3A5F")
    gs["A2"] = ("Spalten Volta 1–3 holen die Mengen aus den Bandblättern (SUMIFS über den Schlüssel). "
                "»Reihe« = größte Menge, weil ein Kasten für alle Bände jedes Teil nur so oft braucht wie das anspruchsvollste Experiment.")
    gs["A2"].font = Font(name=F, italic=True, color="4A5866")
    hdr = ["Schlüssel", "Gruppe", "Bauteil / Gerät", "Wert", "Volta 1", "Volta 2", "Volta 3", "Reihe", "Vorschlag echtes Bauteil"]
    r = 4
    for c, h in enumerate(hdr, 1):
        gs.cell(row=r, column=c, value=h)
    style_header(gs, r, len(hdr))
    order = THEME_ORDER + ["Sonstiges", "Module", "Leitungen", "Pult und Geräte", "Werkzeuge"]
    first = r + 1
    for k, (grp, name, val, real) in sorted(all_keys.items(), key=lambda kv: (order.index(kv[1][0]) if kv[1][0] in order else 99, kv[1][1], kv[0])):
        r += 1
        gs.cell(row=r, column=1, value=k)
        gs.cell(row=r, column=2, value=grp)
        gs.cell(row=r, column=3, value=name)
        gs.cell(row=r, column=4, value=val)
        for i, (title, _, _) in enumerate(band_sheets):
            gs.cell(row=r, column=5 + i, value=f"=SUMIFS('{title}'!$F:$F,'{title}'!$B:$B,$A{r})")
        gs.cell(row=r, column=8, value=f"=MAX(E{r}:G{r})")
        gs.cell(row=r, column=9, value=real)
        for c in range(1, 10):
            body(gs.cell(row=r, column=c), bold=(c == 8), color="008000" if 5 <= c <= 7 else "000000")
            if 5 <= c <= 8:
                gs.cell(row=r, column=c).number_format = "0;-0;–"
    last = r
    r += 1
    gs.cell(row=r, column=3, value="Stück gesamt (ohne Pult und Werkzeuge)")
    for c, col in ((5, "E"), (6, "F"), (7, "G"), (8, "H")):
        gs.cell(row=r, column=c, value=f'=SUMPRODUCT(({col}{first}:{col}{last})*(LEFT($A{first}:$A{last},5)<>"pult|")'
                                       f'*(LEFT($A{first}:$A{last},9)<>"werkzeug|"))')
    for c in (3, 5, 6, 7, 8):
        body(gs.cell(row=r, column=c), bold=True)
    for c, w in enumerate([26, 22, 30, 11, 9, 9, 9, 9, 44], 1):
        gs.column_dimensions[get_column_letter(c)].width = w
    gs.column_dimensions["A"].hidden = True
    gs.freeze_panes = "A5"

    # Uebersicht
    ov["A1"] = "Volta – Stücklisten (BOM) für Volta 1, 2 und 3"
    ov["A1"].font = Font(name=F, bold=True, size=16, color="1F3A5F")
    ov["A2"] = "DC Software Engineering · Stand der Inhalte: Zweig volta3 · erzeugt mit tools/bom_xlsx.py"
    ov["A2"].font = Font(name=F, italic=True, color="4A5866")
    r = 4
    for c, h in enumerate(["Band", "Experimente", "Positionen", "Stück (Bauteile, Module, Leitungen)"], 1):
        ov.cell(row=r, column=c, value=h)
    style_header(ov, r, 4)
    for title, ws, nexp in band_sheets:
        r += 1
        ov.cell(row=r, column=1, value=title)
        ov.cell(row=r, column=2, value=nexp)
        ov.cell(row=r, column=3, value=f"=COUNT('{title}'!$A:$A)")
        ov.cell(row=r, column=4, value=f"=SUMIFS('{title}'!$F:$F,'{title}'!$D:$D,\"Stück gesamt (Bauteile, Module, Leitungen)\")")
        for c in range(1, 5):
            body(ov.cell(row=r, column=c), bold=(c == 1), color="0000FF" if c == 2 else ("008000" if c > 2 else "000000"))
    r += 1
    ov.cell(row=r, column=1, value="Kasten für die ganze Reihe")
    ov.cell(row=r, column=3, value=f"=COUNTIFS('Reihe gesamt'!$H:$H,\">0\",'Reihe gesamt'!$B:$B,\"<>Pult und Geräte\",'Reihe gesamt'!$B:$B,\"<>Werkzeuge\")")
    ov.cell(row=r, column=4, value=f"='Reihe gesamt'!H{last + 1}")
    for c in range(1, 5):
        body(ov.cell(row=r, column=c), bold=True, color="008000" if c > 2 else "000000")
    notes = [
        "So ist die Liste zu lesen",
        "• Menge: höchste Stückzahl desselben Bauteils mit demselben Wert in einem einzelnen Experiment (Musteraufbau). "
        "So viele müssten in einem echten Kasten liegen; über alle Experimente zusammen wird nicht addiert.",
        "• Widerstände und Kondensatoren sind nach Wert getrennt. Ein Widerstand ohne Wert im Experiment zählt als 1 kΩ (Vorgabe der App).",
        "• Module sind in der App fertige Platinen (Modulleiste); die Spalte »Vorschlag« nennt die Schaltung bzw. einen vergleichbaren Baustein.",
        "• Pult, Geräte und Werkzeuge sind in der App virtuell (Bedienpult, Gerätefach). Für einen echten Kasten steht das Gegenstück daneben; "
        "Messgerät, Oszilloskop, Generator, Frequenzgang-Schreiber und Logikanalysator wären externe Messgeräte.",
        "• Messstellen und Tastköpfe sind Anschlüsse für die Messgeräte, keine eigenen Bauteile im Handel.",
        "• Volta 3 setzt Volta 2 nicht als Kauf voraus; jede Bandliste ist für sich vollständig. Das Blatt »Reihe gesamt« zeigt den Bedarf für einen Kasten aller drei Bände.",
        "• Vorschläge für echte Bauteile sind Beispiele mit passenden Kennwerten (Katalog der App), keine verbindliche Bestellliste. "
        "»Nur in der App«-Versuche (Zerstörung, hohe Spannungen) sind nicht für den echten Aufbau gedacht.",
        "Farben: blau = aus den Experimenten gezählt, grün = Verweis auf ein anderes Blatt, schwarz = Text/Berechnung.",
    ]
    r += 2
    for i, t in enumerate(notes):
        ov.cell(row=r + i, column=1, value=t)
        ov.cell(row=r + i, column=1).font = Font(name=F, bold=(i == 0), color="1E2B38" if i else "1F3A5F")
        ov.merge_cells(start_row=r + i, start_column=1, end_row=r + i, end_column=4)
        ov.cell(row=r + i, column=1).alignment = Alignment(wrap_text=True, vertical="top")
        ov.row_dimensions[r + i].height = 15 if i == 0 else 32
    for c, w in enumerate([30, 14, 14, 40], 1):
        ov.column_dimensions[get_column_letter(c)].width = w
    wb.calculation.fullCalcOnLoad = True  # Excel rechnet die Formeln beim Oeffnen
    wb.save(out)
    print(out)


if __name__ == "__main__":
    main()
