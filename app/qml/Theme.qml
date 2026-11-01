// Farben und Masse zentral (app/CLAUDE.md). Werte aus docs/ui-spec.md (Markenwerte DC Software Engineering).
pragma Singleton
import QtQuick

QtObject {
    // Farben
    readonly property color navy: "#0A2540"
    readonly property color desk: "#1A3A5C"
    // Themen des Bauteilfachs (Katalog »thema«): Reihenfolge der Knopfleiste, Knopffarbe, Kachelfarbe und ein
    // typisches Schaltzeichen fuer den Knopf (Farbe nie allein; Name als Tooltip und Ueberschrift)
    readonly property var partThemes: [
        {key: "licht", color: "#E0A800", tint: "#F8E7A6", symbol: "led_rot"},
        {key: "schalten", color: "#2F74C8", tint: "#C7DCF4", symbol: "taster"},
        {key: "widerstaende", color: "#2E8B47", tint: "#CBE7C7", symbol: "widerstand"},
        {key: "halbleiter", color: "#7B52BF", tint: "#DDD0F0", symbol: "transistor_npn"},
        {key: "speicher", color: "#1F8F8F", tint: "#C2E6E4", symbol: "kondensator"},
        {key: "bewegung", color: "#D9692B", tint: "#F7CFB6", symbol: "motor"}
    ]
    function partTheme(key) {
        for (let i = 0; i < partThemes.length; ++i) if (partThemes[i].key === key) return partThemes[i]
        return {key: "", color: "#8E9AA8", tint: "#E9D7B0", symbol: ""}
    }
    // Hintergrund der Grundplatte: jede Reihe ein eigener Farbton (A rot ... E blau), nach rechts dunkler (Verlauf)
    readonly property var rowHues: [[0, 0.85], [30, 0.85], [50, 0.80], [130, 0.55], [210, 0.70]]
    function fieldTint(row, col, columns) {
        const t = columns > 1 ? (col - 1) / (columns - 1) : 0
        return Qt.hsla(rowHues[row][0] / 360, rowHues[row][1], 0.92 - 0.24 * t, 1)
    }
    // Traegerplatte je Modul (Modulleiste und Modulfach), Kachel im Fach in der aufgehellten Farbe
    function modulePlate(type) {
        return type === "tonmodul" ? "#6A4C8C" : type === "zeitmodul" ? "#7A5A1E"
             : type === "verstaerkermodul" ? "#2E6B4A" : type === "opmodul" ? "#7A1F2B" : type === "timer555" ? "#3A3F46" : "#2F6F8F"
    }
    readonly property color accent: "#2873B0" // Weiss darauf 5:1 (WCAG AA)
    readonly property color signalOrange: "#FF6B1A"
    readonly property color solderAmber: "#FFB07A"
    readonly property color voltageBar: "#8EE68A"
    readonly property color voltageBarOutline: "#3E8E3A"
    readonly property color board: "#F7F3EA"
    readonly property color field: "#CFC5AE"
    readonly property color fieldOutline: "#9C9079"
    // leicht spiegelnder Metallglanz der Kontaktfelder: heller Glanz oben, Schattenkante in der Mitte,
    // Rueckstrahlung unten. Einstellung: Silber (vernickeltes Blech, Standard) oder Messing (verzinnt, warm)
    property bool silverFields: true
    readonly property color fieldMetal: silverFields ? "#D2D7DD" : field
    readonly property color fieldShine: silverFields ? "#F8FAFC" : "#F2EEE4"
    readonly property color fieldShade: silverFields ? "#AEB5BE" : "#B8AD93"
    readonly property color fieldGlow: silverFields ? "#DCE1E7" : "#DDD5C2"
    readonly property color fieldEdge: silverFields ? "#858D98" : fieldOutline
    readonly property color socket: silverFields ? "#59606A" : "#6E6553"
    readonly property color socketEdge: silverFields ? "#3D434B" : "#4F4839"
    readonly property color destroyed: "#4A4A4A"

    // Leitungsfarben (docs/domain-model.md)
    function wireColor(name) {
        switch (name) {
        case "rot": return "#D7262E"
        case "schwarz": return "#202020"
        case "blau": return "#2C62C9"
        case "gelb": return "#E8C21B"
        case "gruen": return "#2E9A44"
        case "weiss": return "#F2F2F2"
        }
        return "#808080"
    }

    // Einstellungen der Darstellung (von Lab.qml aus AppSettings gebunden)
    property bool reduceMotion: false   // Animationen reduzieren (docs/ui-spec.md)
    property bool tick: true            // Maskottchen Tick zeigen (Elternbereich)
    property bool showCurrentFlow: true // Stromflusspunkte auf den Leitungen
    property real fontScale: 1.0        // Schriftgroesse (Einstellung A-/A+): 0,8 / 0,9 / 1,0

    // Masse in dp (Qt Quick: geraeteunabhaengige Pixel)
    readonly property int fieldWidth: 72
    readonly property int fieldHeight: 56   // Kontaktfelder >= 56 dp
    readonly property int fieldGap: 24
    readonly property int deskFieldWidth: 56
    readonly property int deskFieldGap: 6 // kompakt: Pultleiste passt ueber 8 Spalten
    readonly property int slotWidth: 36
    // Steckloecher: fuenf je Feld, mittig; Abstand von Lochmitte zu Lochmitte (Pultfelder sind schmaler)
    readonly property int meterAreaWidth: 190 // Messgeraet rechts neben der Grundplatte (einblendbar)
    readonly property int meterDockWidth: 384 // Messgeraet oben im Pult (rechts neben der Pultleiste)
    readonly property int socketSize: 8
    readonly property int socketPitch: 13
    readonly property int deskSocketPitch: 10
    readonly property int slotGap: 8
    readonly property int control: 48       // Bedienelemente >= 48 dp
    readonly property int margin: 24
    readonly property real maxZoom: 2.0     // groesste Vergroesserung der Arbeitsflaeche (zwei Finger, Strg + Mausrad)
    readonly property real maxFitZoom: 1.6  // groesste selbsttaetige Vergroesserung (ganze Arbeitsflaeche im Fenster)
}
