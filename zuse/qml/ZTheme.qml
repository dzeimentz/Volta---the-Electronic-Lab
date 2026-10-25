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
    // drei Schichten der Arbeitsflaeche: Bedienfeld-Schichten links/rechts, Rasterplatine in der Mitte
    readonly property color panelLayer: "#E3EAF1"
    readonly property color layerEdge: "#C3CFDB"
    readonly property color hole: "#D3DCE6"
    readonly property color holeStrong: "#AFBECD"
    readonly property color pad: "#C9A45C"
    readonly property color padDark: "#8A6D35"
    readonly property color accent: "#06B6D4"     // Leitung mit 1, Hervorhebung
    readonly property color accentText: "#0E7490" // Akzent fuer Schrift (Kontrast auf Weiss)
    readonly property color accentDim: "#A5E3EF"
    readonly property color navy: "#2E4A7D"       // Muetze von Zetti
    // Grundschale wie Volta (DC Learning Design Standard): dunkelblaue Kopfzeile, Bibliothek links
    readonly property color shell: "#0A2540"
    readonly property color library: "#1A3A5C"
    readonly property color textOnShell: "#FFFFFF"
    readonly property color textOnShellMuted: "#B8C7D9"
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
    property real fontScale: 1.0 // Schriftgroesse (Einstellung A−/A+): 0,8 / 0,9 / 1,0; Main.qml bindet sie

    function levelColor(level) { return level === "1" ? accent : zero }
}
