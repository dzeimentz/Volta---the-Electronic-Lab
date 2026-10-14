pragma Singleton
import QtQuick

// Gestaltung Zuse 1: hell und freundlich (wie Volta 1), Akzent Cyan, goldene Loetpads, rote LEDs;
// Farben der Figur Zetti (Muetze Blau) als zweite Farbe.
QtObject {
    readonly property color graphite: "#EEF3F7"   // Seitenhintergrund (Name aus der dunklen Fassung beibehalten)
    readonly property color surface: "#FFFFFF"
    readonly property color surfaceHigh: "#E6EDF3"
    readonly property color board: "#FFFFFF"
    readonly property color dot: "#D5DEE8"
    readonly property color pad: "#C9A45C"
    readonly property color padDark: "#8A6D35"
    readonly property color accent: "#06B6D4"     // Leitung mit 1, Hervorhebung
    readonly property color accentText: "#0E7490" // Akzent fuer Schrift (Kontrast auf Weiss)
    readonly property color accentDim: "#A5E3EF"
    readonly property color navy: "#2E4A7D"       // Muetze von Zetti
    readonly property color text: "#14213D"
    readonly property color textOnAccent: "#FFFFFF"
    readonly property color muted: "#5B6B7C"
    readonly property color zero: "#9AA7B4"
    readonly property color ok: "#15803D"
    readonly property color okBg: "#DCFCE7"
    readonly property color bad: "#DC2626"
    readonly property color badBg: "#FEE2E2"
    readonly property color line: "#CBD5E1"
    readonly property color bubble: "#FFFFFF"

    readonly property string mono: Qt.platform.os === "windows" ? "Consolas" : "monospace"
    readonly property int control: 48   // Bedienelemente >= 48 dp
    readonly property int pin: 56       // Anschluesse >= 56 dp
    readonly property int radius: 12

    function levelColor(level) { return level === "1" ? accent : zero }
}
