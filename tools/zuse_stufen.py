"""Erzeugt zuse/content/stufen.json (Stufen von Zuse 1). Texte DE/EN; Wahrheitstabellen werden, wo moeglich,
aus Python-Funktionen berechnet. Aufruf: python tools/zuse_stufen.py"""
import json
from pathlib import Path


def L(de, en):
    return {"de": de, "en": en}


def inp(n, de, en, art="kipp"):
    return {"name": n, "bezeichnung": L(de, en), "art": art}


def out(n, de, en):
    return {"name": n, "bezeichnung": L(de, en)}


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


def zaehl_ablauf():
    """16 Takte und einer mehr: Druecken zeigt die neue Zahl, Loslassen haelt sie (nicht gezeigt)."""
    steps = []
    for k in range(1, 18):
        v = format(k % 16, "04b")
        if k == 16:
            steps.append(schritt("1", v, "Takt – Überlauf", "Clock – overflow"))
        else:
            steps.append(schritt("1", v, "Takt", "Clock"))
        steps.append(schritt("0", v))
    return steps


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
     "eingaenge": [inp("A", "Lichtschalter", "Light switch"), inp("B", "Klingelknopf", "Bell button", "taster")],
     "ausgaenge": [out("X", "Lampe", "Lamp"), out("Y", "Klingel", "Bell")],
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
     "eingaenge": [inp("A", "Es ist hell", "It is bright")], "ausgaenge": [out("Y", "Nachtlicht", "Night light")],
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
     "eingaenge": [inp("A", "Schlüssel 1", "Key 1"), inp("B", "Schlüssel 2", "Key 2")],
     "ausgaenge": [out("Y", "Tresor offen", "Safe open")],
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
     "eingaenge": [inp("A", "Haustür", "Front door", "taster"), inp("B", "Gartentor", "Garden gate", "taster")],
     "ausgaenge": [out("Y", "Klingel", "Bell")],
     "kisten": [{"tabelle": ["0", "0", "0", "1"]}, {"tabelle": ["0", "1", "1", "1"]},
                {"tabelle": ["0", "0", "1", "1"]}], "richtig": 1,
     "wahrheitstabelle": ["0", "1", "1", "1"]},
    {"kuerzel": "NAND", "art": "entdecken", "werkzeug": "NAND", "symbol": "&", "invertiert": True,
     "titel": L("Die Fensterwarnung", "The window warning"),
     "geschichte": L("Im Klassenraum haben die beiden Fenster Sensoren: 1 heißt »Fenster ist zu«. Damit keine Wärme "
                     "verloren geht, soll eine Warnlampe leuchten, solange nicht beide Fenster geschlossen sind.",
                     "The two classroom windows have sensors: 1 means »window is closed«. So that no heat is lost, a "
                     "warning lamp should glow as long as not both windows are closed."),
     "frage": L("Welche Kiste warnt richtig?", "Which box warns correctly?"),
     "hilfe": L("Probier alle vier Stellungen. Wann darf die Warnlampe aus sein?",
                "Try all four settings. When may the warning lamp be off?"),
     "merksatz": L("NAND heißt NICHT-UND: Nur wenn alle Eingänge 1 sind, ist der Ausgang 0.",
                   "NAND means NOT-AND: only when all inputs are 1 is the output 0."),
     "eingaenge": [inp("A", "Fenster 1 zu", "Window 1 closed"), inp("B", "Fenster 2 zu", "Window 2 closed")],
     "ausgaenge": [out("Y", "Warnlampe", "Warning lamp")],
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
     "eingaenge": [inp("A", "Anlage an", "Alarm on"), inp("F", "Fenster offen", "Window open"),
                   inp("T", "Tür offen", "Door open")],
     "ausgaenge": [out("S", "Sirene", "Siren")],
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
     "eingaenge": [inp("A", "Schalter unten", "Switch downstairs"), inp("B", "Schalter oben", "Switch upstairs")],
     "ausgaenge": [out("Y", "Treppenlicht", "Stair light")],
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
     "eingaenge": [inp("A", "Schalter A", "Switch A"), inp("B", "Schalter B", "Switch B")],
     "ausgaenge": [out("Y", "Lampe", "Lamp")],
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
     "eingaenge": [inp("A", "Bit A", "Bit A"), inp("B", "Bit B", "Bit B")],
     "ausgaenge": [out("S", "Summe", "Sum"), out("C", "Übertrag", "Carry")],
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
     "eingaenge": [inp("A", "Bit A", "Bit A"), inp("B", "Bit B", "Bit B"), inp("Ci", "Übertrag rein", "Carry in")],
     "ausgaenge": [out("S", "Summe", "Sum"), out("C", "Übertrag raus", "Carry out")],
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
    {"kuerzel": "REG4", "art": "bauen", "symbol": "REG", "name": L("Register", "Register"),
     "titel": L("Vier Bits merken", "Storing four bits"),
     "geschichte": L("Mit vier Speicherbits nebeneinander merkt sich ein Rechner eine ganze Zahl von 0 bis 15. So "
                     "etwas heißt Register. Alle vier hören auf denselben Knopf »Merken«.",
                     "With four memory bits side by side, a computer stores a whole number from 0 to 15. This is "
                     "called a register. All four listen to the same »store« button."),
     "frage": L("Wie merkt sich ein Rechner eine ganze Zahl?", "How does a computer store a whole number?"),
     "hilfe": L("Jeder Schalter bekommt sein eigenes Speicherbit. Den Knopf »Merken« verbindest du mit allen vier.",
                "Every switch gets its own memory bit. Connect the »store« button to all four."),
     "merksatz": L("Ein Register besteht aus mehreren Speicherbits, die gemeinsam merken. Eins mit 4 Bit hält eine "
                   "Zahl von 0 bis 15.",
                   "A register consists of several memory bits that store together. One with 4 bits holds a number "
                   "from 0 to 15."),
     "eingaenge": [inp("D3", "8", "8"), inp("D2", "4", "4"), inp("D1", "2", "2"), inp("D0", "1", "1"),
                   inp("M", "Merken", "Store", "taster")],
     "ausgaenge": [out("Q3", "8", "8"), out("Q2", "4", "4"), out("Q1", "2", "2"), out("Q0", "1", "1")],
     "eingabegruppen": [{"name": "Zahl", "bits": 4}, {"name": "M", "bits": 1}],
     "bausteine": ["SBIT"],
     "ablauf": [schritt("01011", "0101", "5 einstellen und merken", "Set 5 and store"),
                schritt("01010", "0101", "Loslassen", "Release"),
                schritt("11000", "0101", "12 einstellen – es bleibt 5", "Set 12 – it stays 5"),
                schritt("11001", "1100", "Merken drücken", "Press store"),
                schritt("00000", "1100", "Alle Schalter aus – es bleibt 12", "All switches off – it stays 12"),
                schritt("10011", "1001", "9 einstellen und merken", "Set 9 and store"),
                schritt("10010", "1001", "Loslassen", "Release")],
     "loesung": sol([("s3", "SBIT", 392, 28), ("s2", "SBIT", 392, 168), ("s1", "SBIT", 392, 308),
                     ("s0", "SBIT", 392, 448)],
                    [("D3", "s3.D"), ("D2", "s2.D"), ("D1", "s1.D"), ("D0", "s0.D"), ("M", "s3.M"), ("M", "s2.M"),
                     ("M", "s1.M"), ("M", "s0.M"), ("s3.Q", "Q3"), ("s2.Q", "Q2"), ("s1.Q", "Q1"), ("s0.Q", "Q0")])},
    {"kuerzel": "TFF", "art": "bauen", "symbol": "T", "name": L("Umschalter", "Toggle"),
     "titel": L("Ein Knopf für An und Aus", "One button for on and off"),
     "geschichte": L("Viele Geräte haben nur einen Knopf: Einmal drücken – an. Noch einmal drücken – aus. Dafür muss "
                     "sich die Schaltung merken, wie sie gerade steht, und beim nächsten Druck das Gegenteil nehmen.",
                     "Many devices have just one button: press once – on. Press again – off. For that the circuit has "
                     "to remember its state and take the opposite on the next press."),
     "frage": L("Wie wechselt eine Lampe mit jedem Druck?", "How does a lamp change with every press?"),
     "hilfe": L("Nimm zwei Speicherbits. Das erste merkt sich, solange der Knopf oben ist, das Gegenteil der Lampe "
                "(Merken = NICHT Knopf). Beim Drücken übernimmt das zweite diesen Wert (Merken = Knopf) und schaltet "
                "die Lampe.",
                "Use two memory bits. While the button is up, the first one stores the opposite of the lamp "
                "(store = NOT button). On pressing, the second one takes that value (store = button) and drives the "
                "lamp."),
     "merksatz": L("Zwei Speicherbits hintereinander, die abwechselnd merken, ergeben einen Umschalter: Er wechselt "
                   "bei jedem Druck genau einmal.",
                   "Two memory bits in a row that store in turn make a toggle: it changes exactly once per press."),
     "eingaenge": [inp("T", "Knopf", "Button", "taster")],
     "ausgaenge": [out("Q", "Lampe", "Lamp")],
     "bausteine": ["SBIT", "NOT"],
     "ablauf": [schritt("0", "0", "Einschalten: Lampe aus", "Switch on: lamp off"),
                schritt("1", "1", "Drücken – an", "Press – on"),
                schritt("0", "1", "Loslassen – bleibt an", "Release – stays on"),
                schritt("1", "0", "Drücken – aus", "Press – off"),
                schritt("0", "0", "Loslassen – bleibt aus", "Release – stays off"),
                schritt("1", "1", "Drücken – an", "Press – on")],
     "loesung": sol([("n2", "NOT", 224, 112), ("s1", "SBIT", 392, 112), ("s2", "SBIT", 588, 252),
                     ("n1", "NOT", 392, 392)],
                    [("s2.Q", "n1.a"), ("n1.y", "s1.D"), ("T", "n2.a"), ("n2.y", "s1.M"), ("s1.Q", "s2.D"),
                     ("T", "s2.M"), ("s2.Q", "Q")])},
    {"kuerzel": "ZAEHLER", "art": "bauen", "symbol": "CTR", "name": L("Zähler", "Counter"),
     "titel": L("Der Zähler", "The counter"),
     "geschichte": L("Ein Rechner arbeitet im Takt: Bei jedem Takt geschieht ein Schritt. Damit er weiß, wo er gerade "
                     "ist, zählt er die Takte mit. Hier bist du der Taktgeber – jeder Druck ist ein Takt.",
                     "A computer works to a clock: on every tick, one step happens. To know where it is, it counts the "
                     "ticks. Here you are the clock – every press is a tick."),
     "frage": L("Wie zählt ein Rechner mit jedem Takt weiter?", "How does a computer count on with every tick?"),
     "hilfe": L("Die Einerstelle wechselt bei jedem Takt. Die nächste Stelle wechselt immer dann, wenn die vorige von "
                "1 auf 0 fällt – so wie beim Zählen die Zehner weiterspringen. Ein NICHT dazwischen macht aus dem "
                "Fallen ein Drücken.",
                "The ones place changes on every tick. The next place changes whenever the one before falls from 1 "
                "to 0 – just like the tens move on when you count. A NOT in between turns falling into pressing."),
     "merksatz": L("Vier Umschalter hintereinander zählen von 0 bis 15. Nach 15 kommt wieder 0 – der Zähler läuft "
                   "über.",
                   "Four toggles in a row count from 0 to 15. After 15 comes 0 again – the counter overflows."),
     "eingaenge": [inp("T", "Takt", "Clock", "taster")],
     "ausgaenge": [out("Q3", "8", "8"), out("Q2", "4", "4"), out("Q1", "2", "2"), out("Q0", "1", "1")],
     "bausteine": ["TFF", "NOT"],
     "ablauf": zaehl_ablauf(),
     "loesung": sol([("t0", "TFF", 224, 448), ("n0", "NOT", 392, 448), ("t1", "TFF", 392, 308),
                     ("n1", "NOT", 560, 308), ("t2", "TFF", 560, 168), ("n2", "NOT", 560, 28), ("t3", "TFF", 392, 28)],
                    [("T", "t0.T"), ("t0.Q", "n0.a"), ("n0.y", "t1.T"), ("t1.Q", "n1.a"), ("n1.y", "t2.T"),
                     ("t2.Q", "n2.a"), ("n2.y", "t3.T"), ("t0.Q", "Q0"), ("t1.Q", "Q1"), ("t2.Q", "Q2"),
                     ("t3.Q", "Q3")])},
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
                   inp("T", "Taster", "Button", "taster")],
     "ausgaenge": [out("W", "", ""), out("X", "", ""), out("Y", "", ""), out("Z", "", "")],
     "eingabegruppen": [{"name": "", "bits": 4}, {"name": "T", "bits": 1}],
     "bausteine": ["NOT", "AND", "OR", "NAND", "XODER", "HA", "VA", "ADD4", "ALARM", "MERKER", "SBIT", "REG4",
                   "TFF", "ZAEHLER"],
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
    {"nr": 4, "titel": L("Speichern und Programmieren", "Memory and programs"),
     "frage": L("Wie merkt sich ein Rechner etwas – und wie läuft ein Programm?",
                "How does a computer remember – and how does a program run?"),
     "experimente": ["MERKER", "SBIT", "REG4", "TFF", "ZAEHLER"],
     # folgen: Speicher mit Adressen am Bedienfeld, Zuse-4 mit Programm
     "geplant": [L("Wo wohnt ein Programm?", "Where does a program live?"), L("Läuft er?", "Does it run?")]},
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
