// Sichtbare Bezeichnungen der Oberflaeche (alle ueber qsTr, docs/i18n.md).
pragma Singleton
import QtQuick

QtObject {
    // Themen des Bauteilfachs (Katalog »thema«)
    function themeName(key) {
        switch (key) {
        case "licht": return qsTr("Licht")
        case "schalten": return qsTr("Schalten")
        case "widerstaende": return qsTr("Widerstände und Sensoren")
        case "halbleiter": return qsTr("Dioden und Transistoren")
        case "speicher": return qsTr("Kondensatoren")
        case "bewegung": return qsTr("Bewegung und Ton")
        case "": return qsTr("Alle Bauteile")
        }
        return key
    }

    function partName(type) {
        switch (type) {
        case "widerstand": return qsTr("Widerstand")
        case "gluehlampe": return qsTr("Glühlämpchen")
        case "taster": return qsTr("Taster")
        case "reedkontakt": return qsTr("Reedkontakt")
        case "wechselschalter": return qsTr("Wechselschalter")
        case "motor": return qsTr("Motor")
        case "summer": return qsTr("Summer")
        case "led_rot": return qsTr("LED rot")
        case "led_gruen": return qsTr("LED grün")
        case "led_gelb": return qsTr("LED gelb")
        case "led_blau": return qsTr("LED blau")
        case "diode_si": return qsTr("Diode")
        case "kondensator": return qsTr("Kondensator")
        case "elko": return qsTr("Elko")
        case "fotowiderstand": return qsTr("Fotowiderstand")
        case "transistor_npn": return qsTr("Transistor")
        case "heissleiter": return qsTr("Heißleiter")
        case "elektromagnet": return qsTr("Elektromagnet")
        case "relais": return qsTr("Relais")
        case "feuchtesonde": return qsTr("Feuchtesonde")
        case "piezo": return qsTr("Piezo-Schallgeber")
        }
        return type
    }

    // Aufbauschritt in Stichpunkte zerlegen (ein Handgriff je Zeile): getrennt wird an Satzende, Strichpunkt,
    // Gedankenstrich und Komma - nicht in Klammern und nicht vor einem Nebensatz (»…, bis das Messgeraet 3 V zeigt«)
    function stepBullets(text) {
        const keep = qsTr("bis,damit,dass,wenn,sodass,ob,sonst,aber,weil", "Woerter nach einem Komma, an dem der Aufbauschritt nicht in Stichpunkte geteilt wird")
                         .split(",").map(w => w.trim().toLowerCase())
        const out = []
        let depth = 0, start = 0
        const cut = (end, next) => {
            const piece = text.substring(start, end).trim().replace(/[.,;]+$/, "").trim()
            if (piece !== "") out.push(piece.charAt(0).toUpperCase() + piece.substring(1))
            start = next
        }
        for (let i = 0; i < text.length; ++i) {
            const c = text.charAt(i)
            if (c === "(") ++depth
            else if (c === ")") depth = Math.max(0, depth - 1)
            if (depth > 0) continue
            const after = text.charAt(i + 1)
            if ((c === ";" || c === "." || c === "!") && after === " ") {
                // Abkuerzung (»z. B.«): ein einzelner Buchstabe davor
                if (c === "." && /(^|[\s(])\S$/.test(text.substring(0, i))) continue
                cut(i, i + 2)
            } else if (c === "," && after === " ") {
                const word = text.substring(i + 2).split(/[\s']/)[0].toLowerCase()
                if (keep.indexOf(word) < 0) cut(i, i + 2)
            } else if ((c === "–" || c === "—") && text.charAt(i - 1) === " " && after === " ") {
                cut(i - 1, i + 2)
            }
        }
        cut(text.length, text.length)
        return out
    }

    // Wortanfaenge, an denen Aufbautexte ein Bauteil nennen (je Sprache, durch Komma getrennt); dahinter zeigt
    // StepText das Schaltzeichen. Reihenfolge: das erste passende Bauteil gewinnt.
    function partWords() {
        return [
            ["gluehlampe", qsTr("Glühlämpchen,Lämpchen", "Wortanfaenge im Aufbautext")],
            ["taster", qsTr("Taster", "Wortanfaenge im Aufbautext")],
            ["summer", qsTr("Summer", "Wortanfaenge im Aufbautext")],
            ["motor", qsTr("Motor", "Wortanfaenge im Aufbautext")],
            ["widerstand", qsTr("Widerstand,Widerstände", "Wortanfaenge im Aufbautext")],
            ["led_rot", qsTr("Leuchtdiode,LED", "Wortanfaenge im Aufbautext")],
            ["wechselschalter", qsTr("Wechselschalter", "Wortanfaenge im Aufbautext")],
            ["reedkontakt", qsTr("Reedkontakt", "Wortanfaenge im Aufbautext")],
            ["diode_si", qsTr("Diode", "Wortanfaenge im Aufbautext")],
            ["blinkmodul", qsTr("Blinkmodul", "Wortanfaenge im Aufbautext")],
            ["tonmodul", qsTr("Tonmodul", "Wortanfaenge im Aufbautext")],
            ["lautsprecher", qsTr("Lautsprecher", "Wortanfaenge im Aufbautext")],
            ["fotowiderstand", qsTr("Fotowiderstand", "Wortanfaenge im Aufbautext")],
            ["heissleiter", qsTr("Heißleiter", "Wortanfaenge im Aufbautext")],
            ["elko", qsTr("Elko", "Wortanfaenge im Aufbautext")],
            ["elektromagnet", qsTr("Elektromagnet", "Wortanfaenge im Aufbautext")],
            ["relais", qsTr("Relais", "Wortanfaenge im Aufbautext")],
            ["transistor_npn", qsTr("Transistor", "Wortanfaenge im Aufbautext")],
            ["feuchtesonde", qsTr("Feuchtesonde", "Wortanfaenge im Aufbautext")],
            ["piezo", qsTr("Piezo", "Wortanfaenge im Aufbautext")],
            ["zeitmodul", qsTr("Zeitmodul", "Wortanfaenge im Aufbautext")],
            ["verstaerkermodul", qsTr("Verstärkermodul", "Wortanfaenge im Aufbautext")]
        ]
    }

    function moduleName(type) {
        switch (type) {
        case "blinkmodul": return qsTr("Blinkmodul")
        case "tonmodul": return qsTr("Tonmodul")
        case "zeitmodul": return qsTr("Zeitmodul")
        case "verstaerkermodul": return qsTr("Verstärkermodul")
        }
        return type
    }

    // Anschlussname eines Moduls auf dem Feld der Modulleiste (kurz)
    function pinName(pin) {
        switch (pin) {
        case "plus": return "+"
        case "minus": return "−"
        case "aus": return qsTr("aus")
        case "ein": return qsTr("ein")
        case "takt": return qsTr("Takt")
        case "start": return qsTr("Start")
        }
        return pin
    }

    function colorName(c) {
        switch (c) {
        case "rot": return qsTr("Rot")
        case "schwarz": return qsTr("Schwarz")
        case "blau": return qsTr("Blau")
        case "gelb": return qsTr("Gelb")
        case "gruen": return qsTr("Grün")
        case "weiss": return qsTr("Weiß")
        }
        return c
    }

    // Widerstandswert lesbar: 470 Ω, 4,7 kΩ, 1 MΩ
    function ohm(v) {
        if (v >= 1e6) return Number(v / 1e6).toLocaleString(Qt.locale(), "f", v % 1e6 === 0 ? 0 : 1) + " MΩ"
        if (v >= 1e3) return Number(v / 1e3).toLocaleString(Qt.locale(), "f", v % 1e3 === 0 ? 0 : 1) + " kΩ"
        return Number(v).toLocaleString(Qt.locale(), "f", 0) + " Ω"
    }

    // Text eines Schritts der Aufbauhilfe (LabController::guideStep, buildSteps)
    function stepText(s) {
        if (!s || s.kind === undefined) return ""
        const pinField = (name) => {
            const list = s.pins || []
            for (let i = 0; i < list.length; ++i) if (list[i].name === name) return list[i].field
            return ""
        }
        // Feld der Modulleiste mit Anschlussname, z. B. "M3 (aus)": Anschlussname uebersetzen
        const field = (label) => String(label).replace(/\(([a-z]+)\)$/, (m, pin) => "(" + pinName(pin) + ")")
        switch (s.kind) {
        case "fertig":
            return qsTr("Jetzt ist alles an seinem Platz. Probiere aus, was passiert!")
        case "hauptschalter":
            return qsTr("Schalte den Hauptschalter der Batterie ein.")
        case "modul":
            return qsTr("Setze das %1 ab Feld %2 in die Modulleiste.").arg(moduleName(s.type)).arg(field(s.labels[0]))
        case "leitung":
            return qsTr("Ziehe eine Leitung von %1 nach %2.").arg(field(s.labels[0])).arg(field(s.labels[1]))
        case "bauteil": {
            const fields = (s.pins || []).map(p => p.field)
            // LED ohne festgelegte Farbe: jede Farbe passt
            const name = s.anyColour === true && s.type.startsWith("led_") ? qsTr("eine Leuchtdiode (Farbe egal)") : partName(s.type)
            let t = fields.length === 3
                ? qsTr("Stecke %1 auf %2, %3 und %4.").arg(name).arg(fields[0]).arg(fields[1]).arg(fields[2])
                : fields.length === 4
                ? qsTr("Stecke %1 auf %2, %3, %4 und %5.").arg(name).arg(fields[0]).arg(fields[1]).arg(fields[2]).arg(fields[3])
                : qsTr("Stecke %1 auf %2 und %3.").arg(name).arg(fields[0]).arg(fields[fields.length - 1])
            if (s.type.startsWith("led_")) t += " " + qsTr("Das lange Bein kommt an %1.").arg(pinField("anode"))
            else if (s.type.startsWith("diode")) t += " " + qsTr("Der Strich auf der Diode zeigt nach %1.").arg(pinField("kathode"))
            else if (s.type === "summer" || s.type === "elko" || s.type === "piezo") t += " " + qsTr("Plus kommt an %1.").arg(pinField("plus"))
            else if (s.type === "transistor_npn") t += " " + qsTr("B kommt an %1, C an %2.").arg(pinField("b")).arg(pinField("c"))
            else if (s.type === "wechselschalter") t += " " + qsTr("Die Mitte kommt an %1.").arg(pinField("mitte"))
            if ((s.type === "widerstand" || s.type === "kondensator" || s.type === "elko") && s.value > 0)
                t += " " + qsTr("Wert: %1.").arg(partValue(s.type, s.value))
            if (s.type === "gluehlampe" && s.value > 0)
                t += " " + qsTr("Nennspannung: %1 V.").arg(Number(s.value).toLocaleString(Qt.locale(), "f", s.value % 1 === 0 ? 0 : 1))
            return t
        }
        }
        return ""
    }

    // Kapazitaet lesbar: 100 nF, 4,7 µF, 470 µF
    function farad(v) {
        const fmt = (x) => Number(x).toLocaleString(Qt.locale(), "f", Math.abs(x - Math.round(x)) < 1e-9 ? 0 : 1)
        if (v >= 1e-6) return fmt(v * 1e6) + " µF"
        if (v >= 1e-9) return fmt(v * 1e9) + " nF"
        return fmt(v * 1e12) + " pF"
    }

    // Wert eines Bauteils fuer Teileliste und Aufbaukarte ("" ohne Wert)
    function partValue(type, value) {
        if (!(value > 0)) return ""
        if (type === "widerstand") return ohm(value)
        if (type === "kondensator" || type === "elko") return farad(value)
        if (type === "gluehlampe") return Number(value).toLocaleString(Qt.locale(), "f", value % 1 === 0 ? 0 : 1) + " V"
        return ""
    }

    function toolName(type) {
        switch (type) {
        case "taschenlampe": return qsTr("Taschenlampe")
        case "foehn": return qsTr("Föhn")
        case "wasser": return qsTr("Wassertropfen")
        }
        return type
    }

    // Zeile der Teileliste (LabController::partsList)
    function partsLine(l) {
        const ids = l.ids && l.ids.length > 0 ? " (" + l.ids.join(", ") + ")" : ""
        switch (l.kind) {
        case "modul": return qsTr("%1 × %2").arg(l.count).arg(moduleName(l.type)) + ids
        case "bauteil": {
            const v = partValue(l.type, l.value)
            return qsTr("%1 × %2").arg(l.count).arg(partName(l.type) + (v !== "" ? " " + v : "")) + ids
        }
        case "leitung": return qsTr("%1 × Leitung %2").arg(l.count).arg(colorName(l.type))
        case "werkzeug": return qsTr("%1 × %2").arg(l.count).arg(toolName(l.type))
        case "batterie": return qsTr("Batterie %1 V").arg(Number(l.value).toLocaleString(Qt.locale(), "f", l.value % 1 === 0 ? 0 : 1))
        }
        return ""
    }

    readonly property var wireColors: ["rot", "schwarz", "blau", "gelb", "gruen", "weiss"]
    // E12-Auswahl fuer Volta 1 (Werkzeugkasten zeigt spaeter alle Werte)
    // Elkos (gepolt): 1 µF bis 1000 µF in der Reihe E3/E6 (Musteraufbauten nutzen 10 bis 1000 µF)
    readonly property var elkoValues: [1e-6, 2.2e-6, 4.7e-6, 10e-6, 22e-6, 47e-6, 100e-6, 220e-6, 470e-6, 1e-3]
    readonly property var resistorValues: [100, 220, 330, 470, 680, 1000, 2200, 4700, 10000, 47000, 100000, 1000000]
}
