"""Erzeugt zuse/content/stufen.json (Stufen von Zuse 1). Texte DE/EN; Wahrheitstabellen werden, wo moeglich,
aus Python-Funktionen berechnet. Aufruf: python tools/zuse_stufen.py"""
import json
from pathlib import Path


def L(de, en):
    return {"de": de, "en": en}


def inp(n, de, en, art="kipp", zustand=None):
    e = {"name": n, "bezeichnung": L(de, en), "art": art}
    if zustand:
        e["zustand"] = zustand
    return e


def out(n, de, en, zustand=None):
    e = {"name": n, "bezeichnung": L(de, en)}
    if zustand:
        e["zustand"] = zustand
    return e


def Z(de0, de1, en0, en1):
    """Zustaende in Alltagssprache fuer 0 und 1 (Entdeckerstufen: Situationskarten statt Nullen und Einsen)"""
    return {"de": [de0, de1], "en": [en0, en1]}


def sol(parts, wires):
    return {"bausteine": [{"id": i, "typ": t, "x": x, "y": y} for i, t, x, y in parts],
            "verbindungen": [list(w) for w in wires]}


def table(n_in, fn):
    """Wahrheitstabelle: fn(bits) -> Ausgaenge als String; erster Eingang ist das hoechste Bit."""
    rows = []
    for r in range(2 ** n_in):
        bits = [(r >> (n_in - 1 - i)) & 1 for i in range(n_in)]
        rows.append(fn(bits))
    return rows


def full_add(bits):
    total = sum(bits)
    return f"{total & 1}{total >> 1}"  # Ausgaenge S, C


def add4(bits):
    a = int("".join(map(str, bits[:4])), 2)
    b = int("".join(map(str, bits[4:])), 2)
    return format(a + b, "05b")  # Ausgaenge C, S3, S2, S1, S0


def probe(a, b):
    return [a * 16 + b, f"{a} + {b} = {a + b}"]


def schritt(ein, soll, de="", en=""):
    """Ein Schritt im Ablauf (Schaltungen mit Speicher); ohne Text wird er in der Tabelle nicht gezeigt."""
    st = {"ein": ein, "soll": soll}
    if de:
        st["text"] = L(de, en)
    return st


def vorlauf(steps, taster):
    """Wie bei echten Speichern: Schalter vor dem Takt einstellen. Aendern sich Schalter im selben Schritt, in dem ein
    Taster gedrueckt wird, kommt ein nicht gezeigter Schritt davor (Schalter neu, Taster wie vorher)."""
    out, ein, soll = [], None, None
    for st in steps:
        if ein is not None:
            presses = any(ein[i] == "0" and st["ein"][i] == "1" for i in taster)
            others = any(ein[i] != st["ein"][i] for i in range(len(ein)) if i not in taster)
            if presses and others:
                out.append(schritt("".join(ein[i] if i in taster else c for i, c in enumerate(st["ein"])), soll))
        else:
            ein = "0" * len(st["ein"])
            soll = "0" * len(st["soll"])
            if any(st["ein"][i] == "1" for i in taster) and "1" in "".join(c for i, c in enumerate(st["ein"]) if i not in taster):
                out.append(schritt("".join("0" if i in taster else c for i, c in enumerate(st["ein"])), soll))
        out.append(st)
        ein, soll = st["ein"], st["soll"]
    return out


def bits(v, n):
    return format(v, f"0{n}b")


def zaehl_ablauf():
    """Zaehler mit Loeschen: 16 Takte und einer mehr (Ueberlauf), dann Loeschen. Eingaenge T L, Loslassen nicht gezeigt."""
    steps = []
    for k in range(1, 18):
        v = bits(k % 16, 4)
        if k == 16:
            steps.append(schritt("10", v, "Takt – Überlauf", "Clock – overflow"))
        else:
            steps.append(schritt("10", v, "Takt", "Clock"))
        steps.append(schritt("00", v))
    steps.append(schritt("01", "0000", "Löschen", "Clear"))
    steps.append(schritt("00", "0000"))
    steps.append(schritt("10", "0001", "Takt nach dem Löschen", "Clock after clearing"))
    return steps


def takt_ablauf():
    """Taktgeber mit Teiler: Eingaenge C L, Ausgaenge Takt, halb, viertel (Ticks gezeigt, Pausen nicht)."""
    steps = []
    for k in range(1, 9):
        steps.append(schritt("10", f"1{k & 1}{(k >> 1) & 1}", "Tick", "Tick"))
        steps.append(schritt("00", f"0{k & 1}{(k >> 1) & 1}"))
    steps.append(schritt("01", "000", "Löschen", "Clear"))
    steps.append(schritt("00", "000"))
    steps.append(schritt("10", "110", "Tick nach dem Löschen", "Tick after clearing"))
    return steps


def lauf_ablauf():
    """Lauflicht: Eingaenge E T L, Ausgaenge Y1..Y4."""
    rows = [("110", "1000", "Eingang an, Takt: das erste Licht geht an", "Input on, clock: the first light goes on"),
            ("100", "1000", "", ""),
            ("010", "0100", "Eingang aus, Takt: das Licht wandert", "Input off, clock: the light moves"),
            ("000", "0100", "", ""),
            ("010", "0010", "Takt", "Clock"), ("000", "0010", "", ""),
            ("010", "0001", "Takt", "Clock"), ("000", "0001", "", ""),
            ("010", "0000", "Takt: es fällt hinten heraus", "Clock: it drops out at the end"), ("000", "0000", "", ""),
            ("110", "1000", "Eingang an, Takt", "Input on, clock"), ("100", "1000", "", ""),
            ("110", "1100", "Takt mit Eingang an", "Clock with input on"), ("100", "1100", "", ""),
            ("101", "0000", "Löschen", "Clear")]
    return [schritt(*r) for r in rows]


def gleich(bits_):
    return "1" if bits_[:4] == bits_[4:] else "0"


def plusminus(bits_):
    a = int("".join(map(str, bits_[:4])), 2)
    b = int("".join(map(str, bits_[4:8])), 2)
    r = a + ((~b) & 15) + 1 if bits_[8] else a + b
    return bits(r & 31, 5)  # Ausgaenge C, S3, S2, S1, S0


def probe_gleich(a, b):
    return [a * 16 + b, f"{a} und {b}"]


def probe_rw(a, b, minus):
    return [a * 32 + b * 2 + minus, f"{a} {'−' if minus else '+'} {b}"]


def steuer_schritt(zahl, minus, druck, akku):
    return schritt(f"{bits(zahl, 4)}{minus}{druck}0", bits(akku, 4))


stufen = [
    {"kuerzel": "BIT", "art": "bauen", "chip": False, "symbol": "",
     "titel": L("Licht an!", "Lights on!"),
     "geschichte": L("Ein Rechner kennt nur zwei Zustände: an und aus – genau wie ein Lichtschalter.",
                     "A computer only knows two states: on and off – just like a light switch."),
     "frage": L("Wie sagt man einem Rechner »Ja« oder »Nein«?", "How do you tell a computer »yes« or »no«?"),
     "hilfe": L("Zieh eine Leitung vom Lichtschalter zur Lampe und vom Klingelknopf zur Klingel. Der Kippschalter "
                "bleibt stehen, der Taster nur, solange du drückst.",
                "Draw a wire from the light switch to the lamp and from the bell button to the bell. The toggle "
                "switch stays put, the push button only while you press it."),
     "merksatz": L("An ist 1, aus ist 0 – das ist ein Bit.", "On is 1, off is 0 – that is one bit."),
     "eingaenge": [inp("A", "Lichtschalter", "Light switch",
                       zustand=Z("Lichtschalter aus", "Lichtschalter an", "Light switch off", "Light switch on")),
                   inp("B", "Klingelknopf", "Bell button", "taster",
                       Z("Klingelknopf losgelassen", "Klingelknopf gedrückt", "Bell button released",
                         "Bell button pressed"))],
     "ausgaenge": [out("X", "Lampe", "Lamp", Z("Lampe aus", "Lampe leuchtet", "Lamp off", "Lamp glows")),
                   out("Y", "Klingel", "Bell", Z("Klingel still", "Klingel läutet", "Bell silent", "Bell rings"))],
     "bausteine": [], "wahrheitstabelle": ["00", "01", "10", "11"],
     "loesung": sol([], [("A", "X"), ("B", "Y")])},
    {"kuerzel": "NICHT", "art": "entdecken", "werkzeug": "NOT", "symbol": "1", "invertiert": True,
     "titel": L("Das Nachtlicht", "The night light"),
     "geschichte": L("Lenas Nachtlicht hat einen Lichtsensor. Es soll leuchten, wenn es im Zimmer dunkel ist – und "
                     "ausgehen, sobald es hell wird.",
                     "Lena's night light has a light sensor. It should glow when the room is dark – and switch off "
                     "as soon as it gets bright."),
     "frage": L("Welche Kiste schaltet das Nachtlicht richtig?", "Which box switches the night light correctly?"),
     "hilfe": L("Stell den Sensor auf hell und auf dunkel. Wann soll das Nachtlicht leuchten?",
                "Set the sensor to bright and to dark. When should the night light glow?"),
     "merksatz": L("NICHT dreht um: Aus 1 wird 0, aus 0 wird 1.", "NOT flips it: 1 becomes 0, 0 becomes 1."),
     "eingaenge": [inp("A", "Lichtfühler", "Light sensor",
                       zustand=Z("Im Zimmer ist es dunkel", "Im Zimmer ist es hell", "The room is dark",
                                 "The room is bright"))],
     "ausgaenge": [out("Y", "Nachtlicht", "Night light", Z("Nachtlicht aus", "Nachtlicht leuchtet", "Night light off", "Night light glows"))],
     "kisten": [{"tabelle": ["0", "1"]}, {"tabelle": ["1", "0"]}, {"tabelle": ["1", "1"]}], "richtig": 1,
     "wahrheitstabelle": ["1", "0"]},
    {"kuerzel": "UND", "art": "entdecken", "werkzeug": "AND", "symbol": "&",
     "titel": L("Der Tresor", "The safe"),
     "geschichte": L("Der Tresor der Bank hat zwei Schlösser. Er darf nur aufgehen, wenn beide Schlüssel gedreht "
                     "sind – sonst käme ein Dieb mit einem Schlüssel allein hinein.",
                     "The bank's safe has two locks. It may only open when both keys are turned – otherwise a "
                     "thief with just one key could get in."),
     "frage": L("Welche Kiste schützt den Tresor?", "Which box protects the safe?"),
     "hilfe": L("Probier alle vier Möglichkeiten: kein Schlüssel, nur einer, nur der andere, beide.",
                "Try all four possibilities: no key, one, only the other, both."),
     "merksatz": L("UND: Beide müssen an sein.", "AND: Both must be on."),
     "eingaenge": [inp("A", "Schlüssel 1", "Key 1", zustand=Z("Schlüssel 1 nicht gedreht", "Schlüssel 1 gedreht",
                                                              "Key 1 not turned", "Key 1 turned")),
                   inp("B", "Schlüssel 2", "Key 2", zustand=Z("Schlüssel 2 nicht gedreht", "Schlüssel 2 gedreht",
                                                              "Key 2 not turned", "Key 2 turned"))],
     "ausgaenge": [out("Y", "Tresor", "Safe", Z("Tresor bleibt zu", "Tresor geht auf", "Safe stays shut", "Safe opens"))],
     "kisten": [{"tabelle": ["0", "1", "1", "1"]}, {"tabelle": ["0", "0", "1", "1"]},
                {"tabelle": ["0", "0", "0", "1"]}], "richtig": 2,
     "wahrheitstabelle": ["0", "0", "0", "1"]},
    {"kuerzel": "ODER", "art": "entdecken", "werkzeug": "OR", "symbol": "≥1",
     "titel": L("Die Klingel", "The doorbell"),
     "geschichte": L("Familie Berger hat zwei Türen, aber nur eine Klingel. Sie soll läuten, wenn vorne an der "
                     "Haustür oder hinten am Gartentor jemand drückt.",
                     "The Berger family has two doors but only one bell. It should ring when someone presses at the "
                     "front door or at the garden gate."),
     "frage": L("Welche Kiste lässt die Klingel richtig läuten?", "Which box makes the bell ring correctly?"),
     "hilfe": L("Halte die Taster gedrückt – einzeln und beide zusammen.",
                "Hold the buttons down – one at a time and both together."),
     "merksatz": L("ODER: Eins reicht schon.", "OR: One is enough."),
     "eingaenge": [inp("A", "Haustür", "Front door", "taster",
                       Z("An der Haustür drückt niemand", "Jemand drückt an der Haustür",
                         "Nobody presses at the front door", "Someone presses at the front door")),
                   inp("B", "Gartentor", "Garden gate", "taster",
                       Z("Am Gartentor drückt niemand", "Jemand drückt am Gartentor",
                         "Nobody presses at the garden gate", "Someone presses at the garden gate"))],
     "ausgaenge": [out("Y", "Klingel", "Bell", Z("Klingel still", "Klingel läutet", "Bell silent", "Bell rings"))],
     "kisten": [{"tabelle": ["0", "0", "0", "1"]}, {"tabelle": ["0", "1", "1", "1"]},
                {"tabelle": ["0", "0", "1", "1"]}], "richtig": 1,
     "wahrheitstabelle": ["0", "1", "1", "1"]},
    {"kuerzel": "NAND", "art": "entdecken", "werkzeug": "NAND", "symbol": "&", "invertiert": True,
     "titel": L("Die Fensterwarnung", "The window warning"),
     "geschichte": L("Im Klassenzimmer hat jedes der beiden Fenster einen Fühler. Im Winter soll keine Wärme "
                     "verloren gehen: Die Warnlampe leuchtet, solange noch ein Fenster offen ist. Erst wenn beide "
                     "Fenster zu sind, geht sie aus.",
                     "Each of the two classroom windows has a sensor. In winter no heat should be lost: the warning "
                     "lamp glows as long as a window is still open. Only when both windows are shut does it go off."),
     "frage": L("Welche Kiste warnt richtig?", "Which box warns correctly?"),
     "hilfe": L("Probier alle vier Stellungen. Wann darf die Warnlampe aus sein?",
                "Try all four settings. When may the warning lamp be off?"),
     "merksatz": L("NAND heißt NICHT-UND: Nur wenn alle Eingänge 1 sind, ist der Ausgang 0.",
                   "NAND means NOT-AND: only when all inputs are 1 is the output 0."),
     "eingaenge": [inp("A", "Fenster 1", "Window 1",
                       zustand=Z("Fenster 1 offen", "Fenster 1 zu", "Window 1 open", "Window 1 shut")),
                   inp("B", "Fenster 2", "Window 2",
                       zustand=Z("Fenster 2 offen", "Fenster 2 zu", "Window 2 open", "Window 2 shut"))],
     "ausgaenge": [out("Y", "Warnlampe", "Warning lamp", Z("Warnlampe aus", "Warnlampe leuchtet", "Warning lamp off", "Warning lamp glows"))],
     "kisten": [{"tabelle": ["0", "0", "0", "1"]}, {"tabelle": ["1", "1", "1", "0"]},
                {"tabelle": ["1", "0", "0", "0"]}], "richtig": 1,
     "wahrheitstabelle": ["1", "1", "1", "0"]},
    {"kuerzel": "ALARM", "art": "bauen", "symbol": "ALARM", "name": L("Alarmanlage", "Alarm system"),
     "titel": L("Die Alarmanlage", "The alarm system"),
     "geschichte": L("Jetzt bist du der Erfinder! Die Sirene soll heulen, wenn die Alarmanlage eingeschaltet ist und "
                     "ein Fenster oder die Tür aufgeht. Tagsüber, wenn die Anlage aus ist, bleibt sie still.",
                     "Now you are the inventor! The siren should wail when the alarm is switched on and a window or "
                     "the door opens. During the day, when the alarm is off, it stays quiet."),
     "frage": L("Baue die Alarmanlage aus deinen Werkzeugen.", "Build the alarm system from your tools."),
     "hilfe": L("Fasse erst Fenster ODER Tür zusammen – und schicke das dann mit »Anlage an« durch ein UND.",
                "First combine window OR door – then pass it with »alarm on« through an AND."),
     "merksatz": L("Werkzeuge lassen sich kombinieren – so entstehen Erfindungen.",
                   "Tools can be combined – that's how inventions are made."),
     "eingaenge": [inp("A", "Anlage", "Alarm",
                       zustand=Z("Anlage aus", "Anlage an", "Alarm off", "Alarm on")),
                   inp("F", "Fenster", "Window",
                       zustand=Z("Fenster zu", "Fenster offen", "Window shut", "Window open")),
                   inp("T", "Tür", "Door", zustand=Z("Tür zu", "Tür offen", "Door shut", "Door open"))],
     "ausgaenge": [out("S", "Sirene", "Siren", Z("Sirene still", "Sirene heult", "Siren silent", "Siren wails"))],
     "bausteine": ["AND", "OR", "NOT"], "wahrheitstabelle": ["0", "0", "0", "0", "0", "1", "1", "1"],
     "loesung": sol([("o1", "OR", 260, 330), ("u1", "AND", 500, 220)],
                    [("F", "o1.a"), ("T", "o1.b"), ("A", "u1.a"), ("o1.y", "u1.b"), ("u1.y", "S")])},
    {"kuerzel": "XODER", "art": "bauen", "symbol": "=1", "name": L("Treppenlicht-Schaltung", "Stairway switch"),
     "titel": L("Das Treppenhauslicht", "The stairway light"),
     "geschichte": L("Im Treppenhaus gibt es unten und oben einen Schalter. Mit jedem kann man das Licht an- und "
                     "ausmachen. Es brennt, wenn die beiden Schalter verschieden stehen.",
                     "The staircase has a switch downstairs and one upstairs. Either one turns the light on and off. "
                     "It is on when the two switches are in different positions."),
     "frage": L("Wie merkt eine Schaltung, dass zwei Schalter verschieden stehen?",
                "How does a circuit notice that two switches are in different positions?"),
     "hilfe": L("Verschieden heißt: einer ODER der andere – aber NICHT beide (UND).",
                "Different means: one OR the other – but NOT both (AND)."),
     "merksatz": L("XODER: Verschieden ergibt 1.", "XOR: Different gives 1."),
     "eingaenge": [inp("A", "Schalter unten", "Switch downstairs",
                       zustand=Z("Schalter unten aus", "Schalter unten an", "Switch downstairs off",
                                 "Switch downstairs on")),
                   inp("B", "Schalter oben", "Switch upstairs",
                       zustand=Z("Schalter oben aus", "Schalter oben an", "Switch upstairs off", "Switch upstairs on"))],
     "ausgaenge": [out("Y", "Treppenlicht", "Stair light",
                       Z("Treppenlicht aus", "Treppenlicht brennt", "Stair light off", "Stair light on"))],
     "bausteine": ["AND", "OR", "NOT"], "wahrheitstabelle": ["0", "1", "1", "0"],
     "loesung": sol([("o1", "OR", 230, 90), ("u1", "AND", 230, 370), ("n1", "NOT", 390, 370), ("u2", "AND", 560, 230)],
                    [("A", "o1.a"), ("B", "o1.b"), ("A", "u1.a"), ("B", "u1.b"), ("u1.y", "n1.a"), ("o1.y", "u2.a"),
                     ("n1.y", "u2.b"), ("u2.y", "Y")])},
    {"kuerzel": "NANDUND", "art": "bauen", "chip": False, "symbol": "",
     "titel": L("Alles aus einem Baustein", "Everything from one part"),
     "geschichte": L("Chipfabriken mögen den NAND-Baustein besonders: Aus ihm allein lässt sich jede andere Schaltung "
                     "bauen. Probier es aus – baue ein UND nur aus NAND-Bausteinen.",
                     "Chip factories love the NAND part: every other circuit can be built from it alone. Try it – "
                     "build an AND from NAND parts only."),
     "frage": L("Geht alles mit einem einzigen Baustein?", "Can everything be done with a single part?"),
     "hilfe": L("Verbindest du beide Eingänge eines NAND, dreht er um wie NICHT. Und NICHT hinter NICHT-UND ergibt …?",
                "If you connect both inputs of a NAND, it flips like NOT. And NOT after NOT-AND gives …?"),
     "merksatz": L("Ein NAND mit verbundenen Eingängen ist ein NICHT. Zwei NAND ergeben ein UND – aus NAND lässt sich "
                   "alles bauen.",
                   "A NAND with joined inputs is a NOT. Two NANDs make an AND – everything can be built from NAND."),
     "eingaenge": [inp("A", "Schalter A", "Switch A", zustand=Z("Schalter A aus", "Schalter A an", "Switch A off",
                                                                 "Switch A on")),
                   inp("B", "Schalter B", "Switch B", zustand=Z("Schalter B aus", "Schalter B an", "Switch B off",
                                                                 "Switch B on"))],
     "ausgaenge": [out("Y", "Lampe", "Lamp", Z("Lampe aus", "Lampe leuchtet", "Lamp off", "Lamp glows"))],
     "bausteine": ["NAND"], "wahrheitstabelle": ["0", "0", "0", "1"],
     "loesung": sol([("n1", "NAND", 280, 196), ("n2", "NAND", 476, 196)],
                    [("A", "n1.a"), ("B", "n1.b"), ("n1.y", "n2.a"), ("n1.y", "n2.b"), ("n2.y", "Y")])},
    {"kuerzel": "ZAEHLEN", "art": "zahlen", "chip": False, "symbol": "",
     "titel": L("Vier Lampen", "Four lamps"),
     "geschichte": L("Mit einer Lampe kann man nur 0 und 1 zeigen. Mit vier Lampen nebeneinander geht viel mehr: Jede "
                     "Lampe hat einen Wert – 8, 4, 2 und 1. Die Werte der leuchtenden Lampen werden zusammengezählt. "
                     "So haben die ersten Heimcomputer Zahlen gezeigt: Schalter einstellen, Lampen ablesen.",
                     "One lamp can only show 0 and 1. With four lamps side by side you can do much more: each lamp "
                     "has a value – 8, 4, 2 and 1. The values of the lit lamps are added up. That is how the first "
                     "home computers showed numbers: set the switches, read the lamps."),
     "frage": L("Wie zählt ein Rechner mit vier Lampen?", "How does a computer count with four lamps?"),
     "hilfe": L("Fang bei der größten Lampe an: Passt die 8 hinein? Dann Achter an, den Rest bildest du mit 4, 2 und 1. "
                "Beispiel: 5 = 4 + 1.",
                "Start with the biggest lamp: does 8 fit? Then switch on the eight and make the rest with 4, 2 and 1. "
                "Example: 5 = 4 + 1."),
     "merksatz": L("Jede Stelle ist doppelt so viel wert wie die rechts daneben: 8, 4, 2, 1. Mit 4 Bit zählt man von "
                   "0 bis 15.",
                   "Each place is worth twice the one to its right: 8, 4, 2, 1. With 4 bits you count from 0 to 15."),
     "eingaenge": [inp("D3", "Achter", "Eights"), inp("D2", "Vierer", "Fours"), inp("D1", "Zweier", "Twos"),
                   inp("D0", "Einer", "Ones")],
     "ausgaenge": [out("L3", "Lampe 8", "Lamp 8"), out("L2", "Lampe 4", "Lamp 4"), out("L1", "Lampe 2", "Lamp 2"),
                   out("L0", "Lampe 1", "Lamp 1")],
     "aufgaben": [2, 5, 8, 11, 15],
     "bausteine": [], "wahrheitstabelle": table(4, lambda b: "".join(map(str, b))),
     "loesung": sol([], [("D3", "L3"), ("D2", "L2"), ("D1", "L1"), ("D0", "L0")])},
    {"kuerzel": "HA", "art": "bauen", "symbol": "HA", "name": L("Halbaddierer", "Half adder"),
     "titel": L("1 + 1 = 10", "1 + 1 = 10"),
     "geschichte": L("Rechner zählen nur mit 0 und 1. Kommen zwei Einsen zusammen, wird es eng: 1 + 1 ergibt »10« – "
                     "eine 0 und ein Übertrag in die nächste Stelle, wie beim Zehnerübergang.",
                     "Computers count with just 0 and 1. When two ones meet, there's no room: 1 + 1 gives »10« – a 0 "
                     "and a carry to the next place, just like carrying a ten."),
     "frage": L("Baue eine Schaltung, die zwei Bits addiert.", "Build a circuit that adds two bits."),
     "hilfe": L("Die Summe ist 1, wenn die Bits verschieden sind. Der Übertrag ist 1, wenn beide 1 sind.",
                "The sum is 1 when the bits differ. The carry is 1 when both are 1."),
     "merksatz": L("Summe und Übertrag – so rechnet ein Computer.", "Sum and carry – that's how a computer adds."),
     "eingaenge": [inp("A", "Bit A", "Bit A", zustand=Z("A ist 0", "A ist 1", "A is 0", "A is 1")),
                   inp("B", "Bit B", "Bit B", zustand=Z("B ist 0", "B ist 1", "B is 0", "B is 1"))],
     "ausgaenge": [out("S", "Summe", "Sum", Z("Summe 0", "Summe 1", "Sum 0", "Sum 1")),
                   out("C", "Übertrag", "Carry", Z("kein Übertrag", "Übertrag 1", "no carry", "carry 1"))],
     "bausteine": ["AND", "OR", "NOT", "XODER"], "wahrheitstabelle": ["00", "10", "10", "01"],
     "loesung": sol([("x1", "XODER", 400, 100), ("u1", "AND", 400, 380)],
                    [("A", "x1.A"), ("B", "x1.B"), ("A", "u1.a"), ("B", "u1.b"), ("x1.Y", "S"), ("u1.y", "C")])},
    {"kuerzel": "VA", "art": "bauen", "symbol": "VA", "name": L("Volladdierer", "Full adder"),
     "titel": L("Drei Bits auf einmal", "Three bits at once"),
     "geschichte": L("Beim schriftlichen Addieren kommt von rechts manchmal ein Übertrag dazu. Dann müssen drei Bits "
                     "zusammengezählt werden: A, B und der Übertrag. Das schafft ein Halbaddierer allein nicht.",
                     "When adding on paper, sometimes a carry comes in from the right. Then three bits have to be "
                     "added: A, B and the carry. A half adder alone cannot do that."),
     "frage": L("Baue eine Schaltung, die drei Bits addiert.", "Build a circuit that adds three bits."),
     "hilfe": L("Addiere erst A und B mit einem Halbaddierer, dann dessen Summe und den Übertrag mit einem zweiten. "
                "Liefert einer der beiden einen Übertrag, gibt es einen Übertrag raus – das ist ein ODER.",
                "First add A and B with a half adder, then its sum and the carry with a second one. If either of "
                "them gives a carry, there is a carry out – that is an OR."),
     "merksatz": L("Ein Volladdierer addiert drei Bits: A, B und den Übertrag von rechts.",
                   "A full adder adds three bits: A, B and the carry from the right."),
     "eingaenge": [inp("A", "Bit A", "Bit A", zustand=Z("A ist 0", "A ist 1", "A is 0", "A is 1")),
                   inp("B", "Bit B", "Bit B", zustand=Z("B ist 0", "B ist 1", "B is 0", "B is 1")),
                   inp("Ci", "Übertrag rein", "Carry in",
                       zustand=Z("kein Übertrag rein", "Übertrag rein 1", "no carry in", "carry in 1"))],
     "ausgaenge": [out("S", "Summe", "Sum", Z("Summe 0", "Summe 1", "Sum 0", "Sum 1")),
                   out("C", "Übertrag raus", "Carry out",
                       Z("kein Übertrag raus", "Übertrag raus 1", "no carry out", "carry out 1"))],
     "bausteine": ["HA", "OR", "AND", "XODER"], "wahrheitstabelle": table(3, full_add),
     "loesung": sol([("h1", "HA", 224, 140), ("h2", "HA", 392, 280), ("o1", "OR", 560, 112)],
                    [("A", "h1.A"), ("B", "h1.B"), ("h1.S", "h2.A"), ("Ci", "h2.B"), ("h2.S", "S"),
                     ("h1.C", "o1.a"), ("h2.C", "o1.b"), ("o1.y", "C")])},
    {"kuerzel": "ADD4", "art": "bauen", "symbol": "Σ4", "name": L("4-Bit-Addierer", "4-bit adder"),
     "titel": L("Rechnen bis 15", "Adding up to 15"),
     "geschichte": L("Jetzt rechnest du wie ein echtes Rechenwerk: Mit den oberen vier Schaltern stellst du die Zahl A "
                     "ein, mit den unteren die Zahl B. Deine Schaltung zeigt A + B an den Lampen. Und wenn das Ergebnis "
                     "größer als 15 wird? Dann muss die Übertrags-Lampe »Ü 16« helfen.",
                     "Now you calculate like a real arithmetic unit: the upper four switches set number A, the lower "
                     "four set number B. Your circuit shows A + B on the lamps. And if the result is bigger than 15? "
                     "Then the carry lamp »C 16« has to help."),
     "frage": L("Was passiert, wenn das Ergebnis nicht mehr in vier Lampen passt?",
                "What happens when the result no longer fits into four lamps?"),
     "hilfe": L("Rechne Stelle für Stelle wie beim schriftlichen Addieren: Für die Einer reicht ein Halbaddierer. Sein "
                "Übertrag geht in den Volladdierer der Zweier, dessen Übertrag zu den Vierern und so weiter. Der letzte "
                "Übertrag leuchtet an »Ü 16«.",
                "Work place by place as when adding on paper: a half adder is enough for the ones. Its carry goes "
                "into the full adder for the twos, its carry on to the fours, and so on. The last carry lights »C 16«."),
     "merksatz": L("Ein 4-Bit-Addierer rechnet bis 15 + 15 = 30. Passt das Ergebnis nicht in vier Bit, zeigt es der "
                   "Übertrag an – Rechner nennen das Überlauf.",
                   "A 4-bit adder calculates up to 15 + 15 = 30. If the result does not fit into four bits, the carry "
                   "shows it – computers call this overflow."),
     "eingaenge": [inp("A3", "A 8", "A 8"), inp("A2", "A 4", "A 4"), inp("A1", "A 2", "A 2"), inp("A0", "A 1", "A 1"),
                   inp("B3", "B 8", "B 8"), inp("B2", "B 4", "B 4"), inp("B1", "B 2", "B 2"), inp("B0", "B 1", "B 1")],
     "ausgaenge": [out("C", "Ü 16", "C 16"), out("S3", "8", "8"), out("S2", "4", "4"), out("S1", "2", "2"),
                   out("S0", "1", "1")],
     "eingabegruppen": [{"name": "A", "bits": 4}, {"name": "B", "bits": 4}],
     "proben": [probe(0, 0), probe(1, 1), probe(3, 5), probe(7, 8), probe(6, 9), probe(9, 9), probe(15, 1),
                probe(15, 15)],
     "bausteine": ["VA", "HA"], "wahrheitstabelle": table(8, add4),
     "loesung": sol([("h1", "HA", 224, 448), ("v1", "VA", 224, 224), ("v2", "VA", 448, 336), ("v3", "VA", 448, 56)],
                    [("A0", "h1.A"), ("B0", "h1.B"), ("h1.S", "S0"),
                     ("A1", "v1.A"), ("B1", "v1.B"), ("h1.C", "v1.Ci"), ("v1.S", "S1"),
                     ("A2", "v2.A"), ("B2", "v2.B"), ("v1.C", "v2.Ci"), ("v2.S", "S2"),
                     ("A3", "v3.A"), ("B3", "v3.B"), ("v2.C", "v3.Ci"), ("v3.S", "S3"), ("v3.C", "C")])},
    {"kuerzel": "MERKER", "art": "bauen", "symbol": "M", "name": L("Merker", "Latch"),
     "titel": L("Hat jemand geklingelt?", "Did someone ring?"),
     "geschichte": L("Du warst kurz im Garten. Hat in der Zeit jemand geklingelt? Eine normale Klingellampe leuchtet "
                     "nur, solange jemand drückt. Deine Merklampe soll anbleiben, bis du auf »Löschen« drückst.",
                     "You were out in the garden for a moment. Did anyone ring meanwhile? A normal bell lamp only glows "
                     "while someone presses. Your memory lamp should stay on until you press »clear«."),
     "frage": L("Wie kann sich eine Schaltung etwas merken?", "How can a circuit remember something?"),
     "hilfe": L("Die Lampe soll an bleiben, wenn sie schon an ist. Führe ihren Ausgang zurück: Lampe = (Klingel ODER "
                "Lampe) UND NICHT Löschen.",
                "The lamp should stay on once it is on. Feed its output back: lamp = (bell OR lamp) AND NOT clear."),
     "merksatz": L("Führt ein Ausgang zurück an den eigenen Eingang, kann sich die Schaltung etwas merken. Das ist ein "
                   "Speicher für ein Bit.",
                   "When an output leads back to its own input, the circuit can remember something. That is a memory "
                   "for one bit."),
     "eingaenge": [inp("S", "Klingelknopf", "Bell button", "taster"), inp("R", "Löschen", "Clear", "taster")],
     "ausgaenge": [out("Q", "Merklampe", "Memory lamp")],
     "bausteine": ["OR", "AND", "NOT"],
     "ablauf": [schritt("00", "0", "Einschalten: alles aus", "Switch on: all off"),
                schritt("10", "1", "Jemand klingelt", "Someone rings"),
                schritt("00", "1", "Klingelknopf loslassen", "Release the bell button"),
                schritt("01", "0", "Löschen drücken", "Press clear"),
                schritt("00", "0", "Löschen loslassen", "Release clear"),
                schritt("10", "1", "Noch einmal klingeln", "Ring again"),
                schritt("00", "1", "Loslassen", "Release"),
                schritt("11", "0", "Klingeln und Löschen zugleich", "Ring and clear together")],
     "loesung": sol([("o1", "OR", 252, 168), ("n1", "NOT", 252, 392), ("a1", "AND", 476, 280)],
                    [("S", "o1.a"), ("a1.y", "o1.b"), ("R", "n1.a"), ("o1.y", "a1.a"), ("n1.y", "a1.b"),
                     ("a1.y", "Q")])},
    {"kuerzel": "SBIT", "art": "bauen", "symbol": "D", "name": L("Speicherbit", "Memory bit"),
     "titel": L("Der Schnappschuss", "The snapshot"),
     "geschichte": L("Ein Rechner muss sich Zahlen merken – aber nur dann, wenn er es soll. Wie bei einem Foto: Erst "
                     "beim Drücken auf »Merken« wird festgehalten, wie der Schalter gerade steht. Danach darfst du ihn "
                     "umlegen, die Lampe bleibt, wie sie war.",
                     "A computer has to remember numbers – but only when it is told to. Like a photo: only when you "
                     "press »store« is the switch position captured. Afterwards you may flip it, the lamp stays as it "
                     "was."),
     "frage": L("Wie merkt sich ein Rechner ein Bit genau dann, wenn er soll?",
                "How does a computer store a bit exactly when it should?"),
     "hilfe": L("Nimm deinen Merker. Beim Merken mit Schalter an soll er gesetzt werden (Schalter UND Merken), beim "
                "Merken mit Schalter aus gelöscht (NICHT Schalter UND Merken).",
                "Use your latch. Storing with the switch on should set it (switch AND store), storing with the switch "
                "off should clear it (NOT switch AND store)."),
     "merksatz": L("Ein Speicherbit übernimmt den Wert nur beim Merken. Sonst behält es, was es hat.",
                   "A memory bit only takes the value when storing. Otherwise it keeps what it has."),
     "eingaenge": [inp("D", "Schalter", "Switch"), inp("M", "Merken", "Store", "taster")],
     "ausgaenge": [out("Q", "Lampe", "Lamp")],
     "bausteine": ["MERKER", "AND", "NOT"],
     "ablauf": [schritt("10", "0", "Schalter an – noch nicht gemerkt", "Switch on – not stored yet"),
                schritt("11", "1", "Merken drücken", "Press store"),
                schritt("10", "1", "Merken loslassen", "Release store"),
                schritt("00", "1", "Schalter aus – die Lampe bleibt", "Switch off – the lamp stays"),
                schritt("01", "0", "Merken drücken", "Press store"),
                schritt("00", "0", "Loslassen", "Release")],
     "loesung": sol([("a1", "AND", 252, 112), ("n1", "NOT", 252, 336), ("a2", "AND", 420, 392),
                     ("m1", "MERKER", 560, 224)],
                    [("D", "a1.a"), ("M", "a1.b"), ("a1.y", "m1.S"), ("D", "n1.a"), ("n1.y", "a2.a"), ("M", "a2.b"),
                     ("a2.y", "m1.R"), ("m1.Q", "Q")])},
    {"kuerzel": "TBIT", "art": "bauen", "symbol": "DT", "name": L("Taktbit", "Clock bit"),
     "titel": L("Genau im Moment des Drückens", "Exactly at the moment of pressing"),
     "geschichte": L("Dein Schnappschuss hat einen Haken: Solange »Merken« gedrückt ist, folgt die Lampe dem Schalter. "
                     "Ein Rechner braucht Speicher, die nur im Moment des Drückens übernehmen – wie der Auslöser einer "
                     "Kamera. Und einen Knopf zum Löschen.",
                     "Your snapshot has a catch: as long as »store« is held, the lamp follows the switch. A computer "
                     "needs memory that takes over only at the moment of pressing – like a camera shutter. And a "
                     "clear button."),
     "frage": L("Wie übernimmt ein Speicher genau im Moment des Takts?",
                "How does a memory take over exactly at the moment of the clock?"),
     "hilfe": L("Zuerst ohne Löschen: zwei Speicherbits hintereinander. Das erste merkt, solange der Takt NICHT "
                "gedrückt ist, das zweite beim Drücken. Dann das Löschen: Es öffnet beide Speicherbits (ODER) und gibt "
                "beiden eine 0 (UND NICHT Löschen).",
                "First without clearing: two memory bits in a row. The first stores while the clock is NOT pressed, "
                "the second on pressing. Then clearing: it opens both memory bits (OR) and gives both a 0 (AND NOT "
                "clear)."),
     "merksatz": L("Ein Taktbit übernimmt nur im Moment des Takts. Danach kann sich der Schalter ändern, die Lampe bleibt. "
                   "Echte Speicher starten beim Einschalten zufällig – deshalb gibt es »Löschen«.",
                   "A clock bit takes over only at the moment of the clock. Afterwards the switch may change, the lamp "
                   "stays. Real memories start randomly when switched on – that is why there is »clear«."),
     "eingaenge": [inp("D", "Schalter", "Switch"), inp("T", "Takt", "Clock", "taster"),
                   inp("L", "Löschen", "Clear", "taster")],
     "ausgaenge": [out("Q", "Lampe", "Lamp")],
     "bausteine": ["SBIT", "AND", "OR", "NOT"],
     "ablauf": vorlauf([schritt("100", "0", "Schalter an – noch kein Takt", "Switch on – no clock yet"),
                schritt("110", "1", "Takt drücken: übernommen", "Press clock: taken over"),
                schritt("010", "1", "Schalter aus, Takt noch gedrückt – die Lampe bleibt",
                        "Switch off, clock still held – the lamp stays"),
                schritt("000", "1", "Takt loslassen", "Release clock"),
                schritt("001", "0", "Löschen drücken", "Press clear"),
                schritt("000", "0", "Löschen loslassen", "Release clear"),
                schritt("110", "1", "Schalter an, Takt", "Switch on, clock"),
                schritt("100", "1", "", ""),
                schritt("101", "0", "Löschen, obwohl der Schalter an ist", "Clear although the switch is on")],
                       [1, 2]),
     "loesung": sol([("n1", "NOT", 196, 168), ("o1", "OR", 196, 336), ("n2", "NOT", 196, 476),
                     ("a1", "AND", 364, 28), ("o2", "OR", 364, 196), ("a2", "AND", 364, 420),
                     ("s1", "SBIT", 560, 56), ("s2", "SBIT", 560, 308)],
                    [("T", "n1.a"), ("n1.y", "o2.a"), ("L", "o2.b"), ("o2.y", "s1.M"),
                     ("T", "o1.a"), ("L", "o1.b"), ("o1.y", "s2.M"),
                     ("L", "n2.a"), ("D", "a1.a"), ("n2.y", "a1.b"), ("a1.y", "s1.D"),
                     ("s1.Q", "a2.a"), ("n2.y", "a2.b"), ("a2.y", "s2.D"), ("s2.Q", "Q")])},
    {"kuerzel": "REG4", "art": "bauen", "symbol": "REG", "name": L("Register", "Register"),
     "titel": L("Vier Bits merken", "Storing four bits"),
     "geschichte": L("Mit vier Taktbits nebeneinander merkt sich ein Rechner eine ganze Zahl von 0 bis 15. So etwas heißt "
                     "Register. Alle vier hören auf denselben Takt und denselben Löschknopf.",
                     "With four clock bits side by side, a computer stores a whole number from 0 to 15. This is called "
                     "a register. All four listen to the same clock and the same clear button."),
     "frage": L("Wie merkt sich ein Rechner eine ganze Zahl?", "How does a computer store a whole number?"),
     "hilfe": L("Jeder Schalter bekommt sein eigenes Taktbit. Takt und Löschen verbindest du mit allen vier.",
                "Every switch gets its own clock bit. Connect clock and clear to all four."),
     "merksatz": L("Ein Register besteht aus mehreren Taktbits, die gemeinsam übernehmen. Eins mit 4 Bit hält eine "
                   "Zahl von 0 bis 15.",
                   "A register consists of several clock bits that take over together. One with 4 bits holds a number "
                   "from 0 to 15."),
     "eingaenge": [inp("D3", "8", "8"), inp("D2", "4", "4"), inp("D1", "2", "2"), inp("D0", "1", "1"),
                   inp("T", "Merken", "Store", "taster"), inp("L", "Löschen", "Clear", "taster")],
     "ausgaenge": [out("Q3", "8", "8"), out("Q2", "4", "4"), out("Q1", "2", "2"), out("Q0", "1", "1")],
     "eingabegruppen": [{"name": "Zahl", "bits": 4}, {"name": "T", "bits": 1}, {"name": "L", "bits": 1}],
     "bausteine": ["TBIT"],
     "ablauf": vorlauf([schritt("010110", "0101", "5 einstellen und merken", "Set 5 and store"),
                schritt("010100", "0101", "", ""),
                schritt("110000", "0101", "12 einstellen – es bleibt 5", "Set 12 – it stays 5"),
                schritt("110010", "1100", "Merken drücken", "Press store"),
                schritt("000000", "1100", "Alle Schalter aus – es bleibt 12", "All switches off – it stays 12"),
                schritt("000001", "0000", "Löschen", "Clear"),
                schritt("000000", "0000", "", ""),
                schritt("100110", "1001", "9 einstellen und merken", "Set 9 and store")], [4, 5]),
     "loesung": sol([("t3", "TBIT", 392, 28), ("t2", "TBIT", 392, 168), ("t1", "TBIT", 392, 308),
                     ("t0", "TBIT", 392, 448)],
                    [("D3", "t3.D"), ("D2", "t2.D"), ("D1", "t1.D"), ("D0", "t0.D")]
                    + [("T", f"t{i}.T") for i in range(4)] + [("L", f"t{i}.L") for i in range(4)]
                    + [("t3.Q", "Q3"), ("t2.Q", "Q2"), ("t1.Q", "Q1"), ("t0.Q", "Q0")])},
    {"kuerzel": "LAUF", "art": "bauen", "symbol": "SR", "name": L("Schieberegister", "Shift register"),
     "titel": L("Das Lauflicht", "The running light"),
     "geschichte": L("Bei einem Lauflicht wandert ein Licht von Lampe zu Lampe. Dahinter steckt ein Register, in dem jedes "
                     "Bit bei jedem Takt einen Platz weiterrückt – ein Schieberegister.",
                     "In a running light, a light moves from lamp to lamp. Behind it is a register in which every bit "
                     "moves one place on with every clock – a shift register."),
     "frage": L("Wie wandert ein Bit von Lampe zu Lampe?", "How does a bit move from lamp to lamp?"),
     "hilfe": L("Vier Taktbits in einer Reihe: Das erste bekommt den Eingang, jedes weitere den Ausgang seines "
                "Vordermanns. Takt und Löschen gehen an alle.",
                "Four clock bits in a row: the first gets the input, every other one the output of the one before. "
                "Clock and clear go to all of them."),
     "merksatz": L("In einem Schieberegister rückt jedes Bit bei jedem Takt einen Platz weiter. Schiebt man eine Zahl um "
                   "eine Stelle, wird sie doppelt oder halb so groß.",
                   "In a shift register every bit moves one place on with each clock. Shifting a number by one place "
                   "doubles or halves it."),
     "eingaenge": [inp("E", "Eingang", "Input"), inp("T", "Takt", "Clock", "taster"),
                   inp("L", "Löschen", "Clear", "taster")],
     "ausgaenge": [out("Y1", "1", "1"), out("Y2", "2", "2"), out("Y3", "3", "3"), out("Y4", "4", "4")],
     "bausteine": ["TBIT"],
     "ablauf": vorlauf(lauf_ablauf(), [1, 2]),
     "loesung": sol([("t1", "TBIT", 224, 56), ("t2", "TBIT", 336, 196), ("t3", "TBIT", 448, 336),
                     ("t4", "TBIT", 560, 476)],
                    [("E", "t1.D"), ("t1.Q", "t2.D"), ("t2.Q", "t3.D"), ("t3.Q", "t4.D")]
                    + [("T", f"t{i}.T") for i in range(1, 5)] + [("L", f"t{i}.L") for i in range(1, 5)]
                    + [(f"t{i}.Q", f"Y{i}") for i in range(1, 5)])},
    {"kuerzel": "TFF", "art": "bauen", "symbol": "T", "name": L("Umschalter", "Toggle"),
     "titel": L("Ein Knopf für An und Aus", "One button for on and off"),
     "geschichte": L("Viele Geräte haben nur einen Knopf: Einmal drücken – an. Noch einmal drücken – aus. Dafür muss "
                     "sich die Schaltung merken, wie sie gerade steht, und beim nächsten Takt das Gegenteil nehmen.",
                     "Many devices have just one button: press once – on. Press again – off. For that the circuit has "
                     "to remember its state and take the opposite on the next clock."),
     "frage": L("Wie wechselt eine Lampe mit jedem Druck?", "How does a lamp change with every press?"),
     "hilfe": L("Ein Taktbit genügt: Gib ihm als Schalter das Gegenteil seiner eigenen Lampe (NICHT). Dann übernimmt es "
                "bei jedem Takt das Gegenteil.",
                "One clock bit is enough: give it the opposite of its own lamp as the switch (NOT). Then it takes the "
                "opposite on every clock."),
     "merksatz": L("Ein Taktbit, das bei jedem Takt sein eigenes Gegenteil übernimmt, ist ein Umschalter: Er wechselt "
                   "bei jedem Druck genau einmal.",
                   "A clock bit that takes its own opposite on every clock is a toggle: it changes exactly once per "
                   "press."),
     "eingaenge": [inp("T", "Knopf", "Button", "taster"), inp("L", "Ganz aus", "All off", "taster")],
     "ausgaenge": [out("Q", "Lampe", "Lamp")],
     "bausteine": ["TBIT", "NOT"],
     "ablauf": [schritt("00", "0", "Einschalten: Lampe aus", "Switch on: lamp off"),
                schritt("10", "1", "Drücken – an", "Press – on"),
                schritt("00", "1", "Loslassen – bleibt an", "Release – stays on"),
                schritt("10", "0", "Drücken – aus", "Press – off"),
                schritt("00", "0", "Loslassen – bleibt aus", "Release – stays off"),
                schritt("10", "1", "Drücken – an", "Press – on"),
                schritt("00", "1", "", ""),
                schritt("01", "0", "Ganz aus", "All off")],
     "loesung": sol([("b1", "TBIT", 420, 252), ("n1", "NOT", 252, 112)],
                    [("b1.Q", "n1.a"), ("n1.y", "b1.D"), ("T", "b1.T"), ("L", "b1.L"), ("b1.Q", "Q")])},
    {"kuerzel": "ZAEHLER", "art": "bauen", "symbol": "CTR", "name": L("Zähler", "Counter"),
     "titel": L("Der Zähler", "The counter"),
     "geschichte": L("Ein Rechner arbeitet im Takt: Bei jedem Takt geschieht ein Schritt. Damit er weiß, wo er gerade "
                     "ist, zählt er die Takte mit. Hier bist du der Taktgeber – jeder Druck ist ein Takt.",
                     "A computer works to a clock: on every tick, one step happens. To know where it is, it counts the "
                     "ticks. Here you are the clock – every press is a tick."),
     "frage": L("Wie zählt ein Rechner mit jedem Takt weiter?", "How does a computer count on with every tick?"),
     "hilfe": L("Die Einerstelle wechselt bei jedem Takt. Die nächste Stelle wechselt immer dann, wenn die vorige von "
                "1 auf 0 fällt – so wie beim Zählen die Zehner weiterspringen. Ein NICHT dazwischen macht aus dem "
                "Fallen ein Drücken. Löschen geht an alle.",
                "The ones place changes on every tick. The next place changes whenever the one before falls from 1 "
                "to 0 – just like the tens move on when you count. A NOT in between turns falling into pressing. "
                "Clear goes to all."),
     "merksatz": L("Vier Umschalter hintereinander zählen von 0 bis 15. Nach 15 kommt wieder 0 – der Zähler läuft "
                   "über.",
                   "Four toggles in a row count from 0 to 15. After 15 comes 0 again – the counter overflows."),
     "eingaenge": [inp("T", "Takt", "Clock", "taster"), inp("L", "Löschen", "Clear", "taster")],
     "ausgaenge": [out("Q3", "8", "8"), out("Q2", "4", "4"), out("Q1", "2", "2"), out("Q0", "1", "1")],
     "bausteine": ["TFF", "NOT"],
     "ablauf": zaehl_ablauf(),
     "loesung": sol([("t0", "TFF", 224, 448), ("n0", "NOT", 392, 448), ("t1", "TFF", 392, 308),
                     ("n1", "NOT", 560, 308), ("t2", "TFF", 560, 168), ("n2", "NOT", 560, 28), ("t3", "TFF", 392, 28)],
                    [("T", "t0.T"), ("t0.Q", "n0.a"), ("n0.y", "t1.T"), ("t1.Q", "n1.a"), ("n1.y", "t2.T"),
                     ("t2.Q", "n2.a"), ("n2.y", "t3.T"), ("t0.Q", "Q0"), ("t1.Q", "Q1"), ("t2.Q", "Q2"),
                     ("t3.Q", "Q3")] + [("L", f"t{i}.L") for i in range(4)])},
    {"kuerzel": "TAKT", "art": "bauen", "symbol": "÷4", "name": L("Teiler", "Divider"),
     "titel": L("Der Taktgeber", "The clock generator"),
     "geschichte": L("Jetzt drückt niemand mehr: Links tickt ein Taktgeber von allein. Für eine Uhr ist er aber viel zu "
                     "schnell. Bau einen Teiler: Die zweite Lampe soll halb so schnell blinken, die dritte noch einmal "
                     "halb so schnell.",
                     "Now nobody presses any more: on the left a clock generator ticks by itself. For a clock it is far "
                     "too fast, though. Build a divider: the second lamp should blink half as fast, the third half as "
                     "fast again."),
     "frage": L("Wie wird aus einem schnellen Takt ein langsamer?", "How does a fast clock become a slow one?"),
     "hilfe": L("Ein Umschalter am Taktgeber wechselt bei jedem Tick – er blinkt also halb so schnell. Ein zweiter "
                "Umschalter dahinter (mit NICHT wie beim Zähler) halbiert noch einmal.",
                "A toggle on the clock generator changes on every tick – so it blinks half as fast. A second toggle "
                "behind it (with NOT as in the counter) halves it again."),
     "merksatz": L("Jeder Umschalter teilt den Takt durch 2. So entstehen aus einem schnellen Takt Sekunden, Minuten "
                   "und Stunden.",
                   "Every toggle divides the clock by 2. That is how seconds, minutes and hours come from a fast "
                   "clock."),
     "eingaenge": [inp("C", "Taktgeber", "Clock generator", "takt"), inp("L", "Löschen", "Clear", "taster")],
     "ausgaenge": [out("C1", "Takt", "Clock"), out("C2", "halb", "half"), out("C4", "Viertel", "quarter")],
     "bausteine": ["TFF", "NOT"],
     "ablauf": takt_ablauf(),
     "loesung": sol([("t1", "TFF", 280, 336), ("n1", "NOT", 448, 336), ("t2", "TFF", 588, 476)],
                    [("C", "C1"), ("C", "t1.T"), ("L", "t1.L"), ("t1.Q", "C2"), ("t1.Q", "n1.a"), ("n1.y", "t2.T"),
                     ("L", "t2.L"), ("t2.Q", "C4")])},
    {"kuerzel": "GLEICH", "art": "bauen", "symbol": "=", "name": L("Vergleicher", "Comparator"),
     "titel": L("Das Zahlenschloss", "The combination lock"),
     "geschichte": L("Ein Zahlenschloss geht nur auf, wenn die eingestellte Zahl genau die Geheimzahl ist. Oben stellst du "
                     "die Zahl A ein, unten die Geheimzahl B. Die Lampe »offen« soll nur leuchten, wenn beide gleich "
                     "sind.",
                     "A combination lock only opens if the number set is exactly the secret number. At the top you set "
                     "number A, below the secret number B. The lamp »open« should only glow when both are equal."),
     "frage": L("Woran erkennt ein Rechner, dass zwei Zahlen gleich sind?",
                "How does a computer recognise that two numbers are equal?"),
     "hilfe": L("Vergleiche Stelle für Stelle: Deine Treppenlicht-Schaltung (XODER) meldet 1, wenn zwei Bits verschieden "
                "sind. Fasse die vier Meldungen mit ODER zusammen – und dreh das Ergebnis mit NICHT um.",
                "Compare place by place: your stairway circuit (XOR) reports 1 when two bits differ. Combine the four "
                "reports with OR – and turn the result round with NOT."),
     "merksatz": L("Zwei Zahlen sind gleich, wenn sich keine Stelle unterscheidet. XODER findet Unterschiede, NICHT-ODER "
                   "meldet »keiner«.",
                   "Two numbers are equal when no place differs. XOR finds differences, NOT-OR reports »none«."),
     "eingaenge": [inp("A3", "A 8", "A 8"), inp("A2", "A 4", "A 4"), inp("A1", "A 2", "A 2"), inp("A0", "A 1", "A 1"),
                   inp("B3", "B 8", "B 8"), inp("B2", "B 4", "B 4"), inp("B1", "B 2", "B 2"), inp("B0", "B 1", "B 1")],
     "ausgaenge": [out("G", "offen", "open")],
     "eingabegruppen": [{"name": "A", "bits": 4}, {"name": "B", "bits": 4}],
     "proben": [probe_gleich(5, 5), probe_gleich(5, 4), probe_gleich(12, 12), probe_gleich(9, 6), probe_gleich(0, 0),
                probe_gleich(15, 14), probe_gleich(8, 1), probe_gleich(3, 3)],
     "bausteine": ["XODER", "OR", "NOT"], "wahrheitstabelle": table(8, gleich),
     "loesung": sol([("x3", "XODER", 224, 28), ("x2", "XODER", 224, 168), ("x1", "XODER", 224, 308),
                     ("x0", "XODER", 224, 448), ("o1", "OR", 420, 98), ("o2", "OR", 420, 378), ("o3", "OR", 560, 238),
                     ("n1", "NOT", 588, 448)],
                    [("A3", "x3.A"), ("B3", "x3.B"), ("A2", "x2.A"), ("B2", "x2.B"), ("A1", "x1.A"), ("B1", "x1.B"),
                     ("A0", "x0.A"), ("B0", "x0.B"), ("x3.Y", "o1.a"), ("x2.Y", "o1.b"), ("x1.Y", "o2.a"),
                     ("x0.Y", "o2.b"), ("o1.y", "o3.a"), ("o2.y", "o3.b"), ("o3.y", "n1.a"), ("n1.y", "G")])},
    {"kuerzel": "RW", "art": "bauen", "symbol": "±", "name": L("Rechenwerk", "Arithmetic unit"),
     "titel": L("Plus und Minus", "Plus and minus"),
     "geschichte": L("Ein Rechner kann eigentlich nur addieren – und zieht trotzdem ab. Der Trick: Alle Bits von B "
                     "umdrehen und noch 1 dazuzählen. Mit dem Schalter »Minus« soll dein Rechenwerk wahlweise A + B oder "
                     "A − B rechnen.",
                     "A computer can really only add – and still subtracts. The trick: flip all the bits of B and add 1. "
                     "With the »minus« switch your arithmetic unit should compute either A + B or A − B."),
     "frage": L("Wie zieht ein Rechner ab, wenn er nur addieren kann?",
                "How does a computer subtract if it can only add?"),
     "hilfe": L("Nimm vier Volladdierer wie beim Rechnen bis 15. Jedes Bit von B geht durch ein XODER mit »Minus« – das "
                "dreht es um, wenn Minus an ist. Und »Minus« ist zugleich der Übertrag in die Einerstelle: das »+ 1«.",
                "Use four full adders as for adding up to 15. Every bit of B goes through an XOR with »minus« – that "
                "flips it when minus is on. And »minus« is also the carry into the ones place: the »+ 1«."),
     "merksatz": L("A − B = A + (B umgedreht) + 1. Ist dabei Ü = 1, ging die Rechnung auf; ist Ü = 0, war B größer "
                   "als A.",
                   "A − B = A + (B flipped) + 1. If C = 1, the result is fine; if C = 0, B was larger than A."),
     "eingaenge": [inp("A3", "A 8", "A 8"), inp("A2", "A 4", "A 4"), inp("A1", "A 2", "A 2"), inp("A0", "A 1", "A 1"),
                   inp("B3", "B 8", "B 8"), inp("B2", "B 4", "B 4"), inp("B1", "B 2", "B 2"), inp("B0", "B 1", "B 1"),
                   inp("M", "Minus", "Minus")],
     "ausgaenge": [out("C", "Ü", "C"), out("S3", "8", "8"), out("S2", "4", "4"), out("S1", "2", "2"),
                   out("S0", "1", "1")],
     "eingabegruppen": [{"name": "A", "bits": 4}, {"name": "B", "bits": 4}, {"name": "−", "bits": 1}],
     "proben": [probe_rw(3, 5, 0), probe_rw(9, 8, 0), probe_rw(9, 4, 1), probe_rw(7, 7, 1), probe_rw(12, 5, 1),
                probe_rw(15, 1, 0), probe_rw(2, 5, 1), probe_rw(10, 3, 1)],
     "bausteine": ["VA", "XODER"], "wahrheitstabelle": table(9, plusminus),
     "loesung": sol([("x0", "XODER", 196, 448), ("x1", "XODER", 196, 308), ("x2", "XODER", 196, 168),
                     ("x3", "XODER", 196, 28), ("v0", "VA", 588, 420), ("v1", "VA", 392, 308), ("v2", "VA", 588, 168),
                     ("v3", "VA", 392, 28)],
                    [(f"B{i}", f"x{i}.A") for i in range(4)] + [("M", f"x{i}.B") for i in range(4)]
                    + [(f"A{i}", f"v{i}.A") for i in range(4)] + [(f"x{i}.Y", f"v{i}.B") for i in range(4)]
                    + [("M", "v0.Ci"), ("v0.C", "v1.Ci"), ("v1.C", "v2.Ci"), ("v2.C", "v3.Ci"), ("v3.C", "C")]
                    + [(f"v{i}.S", f"S{i}") for i in range(4)])},
    {"kuerzel": "STEUER", "art": "zahlen", "chip": False, "symbol": "",
     "titel": L("Du bist das Steuerwerk", "You are the control unit"),
     "geschichte": L("Hier ist eine fertige Rechenmaschine aus deinen Erfindungen: dein Rechenwerk und ein Register, der "
                     "Akku, der sich das Ergebnis merkt. Was sie rechnen soll, sagst du ihr Schritt für Schritt: Zahl "
                     "einstellen, Plus oder Minus wählen, »Rechnen« drücken. Du bist das Steuerwerk!",
                     "Here is a finished calculating machine made of your inventions: your arithmetic unit and a "
                     "register, the accumulator, which remembers the result. You tell it step by step what to compute: "
                     "set a number, choose plus or minus, press »compute«. You are the control unit!"),
     "frage": L("Wer sagt der Rechenmaschine, was sie tun soll?", "Who tells the calculating machine what to do?"),
     "hilfe": L("Der Akku zeigt immer das Zwischenergebnis. Jeder Druck auf »Rechnen« rechnet Akku + Zahl (oder "
                "Akku − Zahl, wenn Minus an ist) und merkt sich das Ergebnis.",
                "The accumulator always shows the intermediate result. Every press on »compute« computes "
                "accumulator + number (or accumulator − number with minus on) and stores the result."),
     "merksatz": L("Eine Rechenmaschine braucht jemanden, der ihr die Schritte nacheinander gibt – das Steuerwerk. Konrad "
                   "Zuses große Idee: Die Schritte selbst in einem Speicher ablegen, dann rechnet die Maschine von "
                   "allein. Das ist ein Programm.",
                   "A calculating machine needs someone to give it the steps one after another – the control unit. "
                   "Konrad Zuse's great idea: store the steps themselves in a memory, then the machine calculates by "
                   "itself. That is a program."),
     "eingaenge": [inp("D3", "8", "8"), inp("D2", "4", "4"), inp("D1", "2", "2"), inp("D0", "1", "1"),
                   inp("M", "Minus", "Minus"), inp("T", "Rechnen", "Compute", "taster"),
                   inp("L", "Löschen", "Clear", "taster")],
     "ausgaenge": [out("Q3", "Akku 8", "Acc 8"), out("Q2", "Akku 4", "Acc 4"), out("Q1", "Akku 2", "Acc 2"),
                   out("Q0", "Akku 1", "Acc 1")],
     "eingabegruppen": [{"name": "Zahl", "bits": 4}, {"name": "−", "bits": 1}, {"name": "T", "bits": 1},
                        {"name": "L", "bits": 1}],
     "aufgaben": [3, 8, 6, 13],
     "aufgabentexte": [L("0 + 3: Lade 3 in den Akku", "0 + 3: load 3 into the accumulator"),
                       L("3 + 5", "3 + 5"), L("8 − 2", "8 − 2"), L("6 + 7", "6 + 7")],
     "erfolg": L("Geschafft: 3 + 5 − 2 + 7 = 13! Du hast die Maschine Schritt für Schritt gesteuert. Genau das macht in "
                 "einem Rechner das Steuerwerk – von allein, nach einem Programm.",
                 "Done: 3 + 5 − 2 + 7 = 13! You controlled the machine step by step. That is exactly what the control "
                 "unit does in a computer – by itself, following a program."),
     "ablauf": vorlauf([steuer_schritt(3, 0, 1, 3), steuer_schritt(3, 0, 0, 3), steuer_schritt(5, 0, 1, 8),
                steuer_schritt(5, 0, 0, 8), steuer_schritt(2, 1, 1, 6), steuer_schritt(2, 1, 0, 6),
                steuer_schritt(7, 0, 1, 13), steuer_schritt(7, 0, 0, 13)], [5, 6]),
     "loesung": sol([("rw", "RW", 252, 28), ("ak", "REG4", 532, 140)],
                    [(f"D{i}", f"rw.B{i}") for i in range(4)] + [("M", "rw.M")]
                    + [(f"rw.S{i}", f"ak.D{i}") for i in range(4)] + [("T", "ak.T"), ("L", "ak.L")]
                    + [(f"ak.Q{i}", f"Q{i}") for i in range(4)] + [(f"ak.Q{i}", f"rw.A{i}") for i in range(4)])},
    # Freies Labor wie bei Volta: keine Aufgabe, alle bisher entdeckten und erfundenen Werkzeuge; gehoert nicht zum
    # Lernpfad und bleibt immer offen
    {"kuerzel": "FREI", "art": "bauen", "frei": True, "chip": False, "symbol": "",
     "titel": L("Freies Labor", "Free lab"),
     "frage": L("Was baust du heute?", "What will you build today?"),
     "geschichte": L("Hier gibt es keine Aufgabe. Zieh deine Werkzeuge auf die Werkbank, verbinde sie mit den "
                     "Schaltern und Lampen und probier aus, was passiert. Dein Aufbau bleibt gespeichert.",
                     "There is no task here. Drag your tools onto the workbench, connect them to the switches and "
                     "lamps and try out what happens. Your setup is saved."),
     "hilfe": L("Im Werkzeugfach liegen alle Werkzeuge, die du schon entdeckt oder erfunden hast. Je weiter du im "
                "Lernpfad kommst, desto mehr werden es.",
                "The tool tray holds every tool you have discovered or invented so far. The further you get on the "
                "learning path, the more there are."),
     "merksatz": L("", ""),
     "eingaenge": [inp("A", "", ""), inp("B", "", ""), inp("C", "", ""), inp("D", "", ""),
                   inp("T", "Taster", "Button", "taster"), inp("TG", "Taktgeber", "Clock", "takt")],
     "ausgaenge": [out("W", "", ""), out("X", "", ""), out("Y", "", ""), out("Z", "", "")],
     "eingabegruppen": [{"name": "", "bits": 4}, {"name": "T", "bits": 1}, {"name": "TG", "bits": 1}],
     "bausteine": ["NOT", "AND", "OR", "NAND", "XODER", "HA", "VA", "ADD4", "ALARM", "MERKER", "SBIT", "TBIT",
                   "REG4", "LAUF", "TFF", "ZAEHLER", "TAKT", "GLEICH", "RW"],
     "loesung": sol([], [])},
]

# Lernstufen wie bei Volta: einfach beginnen, pro Stufe anspruchsvoller. Ausgangspunkt ist ein Kind, das noch nie etwas
# von Digitaltechnik gehoert hat. Die Experimente bleiben der Reihe nach freigeschaltet (Werkzeuge bauen aufeinander auf).
lernstufen = [
    {"nr": 0, "titel": L("Auftakt", "Warm-up"),
     "frage": L("Was versteht ein Rechner überhaupt?", "What does a computer understand at all?"),
     "experimente": ["BIT"]},
    {"nr": 1, "titel": L("Logik entdecken", "Discovering logic"),
     "frage": L("Wie entscheidet ein Rechner?", "How does a computer decide?"),
     "experimente": ["NICHT", "UND", "ODER", "NAND"]},
    {"nr": 2, "titel": L("Erfinden mit Bausteinen", "Inventing with parts"),
     "frage": L("Was kann ich aus meinen Bausteinen bauen?", "What can I build from my parts?"),
     "experimente": ["ALARM", "XODER", "NANDUND"]},
    {"nr": 3, "titel": L("Zahlen und Rechnen", "Numbers and adding"),
     "frage": L("Wie rechnet ein Rechner mit 0 und 1?", "How does a computer calculate with 0 and 1?"),
     "experimente": ["ZAEHLEN", "HA", "VA", "ADD4"]},
    {"nr": 4, "titel": L("Speichern und Zählen", "Memory and counting"),
     "frage": L("Wie merkt sich ein Rechner etwas – und wie zählt er?",
                "How does a computer remember something – and how does it count?"),
     "experimente": ["MERKER", "SBIT", "TBIT", "REG4", "LAUF", "TFF", "ZAEHLER", "TAKT"]},
    {"nr": 5, "titel": L("Die Rechenmaschine", "The calculating machine"),
     "frage": L("Wie rechnet eine Maschine – und wer sagt ihr, was sie tun soll?",
                "How does a machine calculate – and who tells it what to do?"),
     "experimente": ["GLEICH", "RW", "STEUER"],
     # Abgrenzung (Konzept 4.1): Speicher mit Adressen, Programm und Steuerwerk sind Zuse 2
     "ausblick": L("Weiter in Zuse 2: Der Rechner bekommt einen Speicher für sein Programm und steuert sich selbst.",
                   "Continued in Zuse 2: the computer gets a memory for its program and controls itself.")},
]
assert [k for l in lernstufen for k in l["experimente"]] == [st["kuerzel"] for st in stufen if not st.get("frei")]
d = {"schema_version": 2,
     "hinweis": "Zuse 1 Prototyp: jede Stufe beginnt mit einer Geschichte; »entdecken« mit Raetselkisten, »bauen« auf "
                "der Werkbank. Wahrheitstabelle: Ausgaenge je Zeile, Eingaenge binaer aufsteigend (erster Eingang "
                "hoechstes Bit).",
     "lernstufen": lernstufen, "stufen": stufen}
with open(Path(__file__).resolve().parent.parent / "zuse" / "content" / "stufen.json", "w", encoding="utf-8",
          newline="\n") as f:
    json.dump(d, f, ensure_ascii=False, indent=1)
print(len(stufen))
