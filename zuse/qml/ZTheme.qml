pragma Singleton
import QtQuick

// Gestaltungslinie Zuse (Konzept Abschnitt 6): Graphit, goldene Loetpads, Akzent je Band (Zuse 1: Cyan)
QtObject {
    readonly property color graphite: "#0E1217"
    readonly property color surface: "#161C24"
    readonly property color surfaceHigh: "#1F2731"
    readonly property color board: "#10161D"
    readonly property color dot: "#26313D"
    readonly property color pad: "#C9A45C"
    readonly property color padDark: "#8A6D35"
    readonly property color accent: "#22D3EE"
    readonly property color accentDim: "#155E6B"
    readonly property color text: "#E6EDF3"
    readonly property color muted: "#8B98A5"
    readonly property color zero: "#6B7785"
    readonly property color ok: "#4ADE80"
    readonly property color bad: "#F87171"
    readonly property color line: "#2C3744"

    readonly property string mono: Qt.platform.os === "windows" ? "Consolas" : "monospace"
    readonly property int control: 48   // Bedienelemente >= 48 dp
    readonly property int pin: 56       // Anschluesse >= 56 dp
    readonly property int radius: 10

    function levelColor(level) { return level === "1" ? accent : zero }
}
