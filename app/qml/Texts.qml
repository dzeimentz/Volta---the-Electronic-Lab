// Sichtbare Bezeichnungen der Oberflaeche (alle ueber qsTr, docs/i18n.md).
pragma Singleton
import QtQuick

QtObject {
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

    readonly property var wireColors: ["rot", "schwarz", "blau", "gelb", "gruen", "weiss"]
    // E12-Auswahl fuer Volta 1 (Werkzeugkasten zeigt spaeter alle Werte)
    readonly property var resistorValues: [100, 220, 330, 470, 680, 1000, 2200, 4700, 10000, 47000, 100000]
}
