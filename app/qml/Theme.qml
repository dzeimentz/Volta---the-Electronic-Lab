// Farben und Masse zentral (app/CLAUDE.md). Werte aus docs/ui-spec.md (Markenwerte DC Software Engineering).
pragma Singleton
import QtQuick

QtObject {
    // Farben
    readonly property color navy: "#0A2540"
    readonly property color desk: "#1A3A5C"
    readonly property color accent: "#2E7DBE"
    readonly property color signalOrange: "#FF6B1A"
    readonly property color solderAmber: "#FFB07A"
    readonly property color voltageBar: "#8EE68A"
    readonly property color voltageBarOutline: "#3E8E3A"
    readonly property color board: "#F7F3EA"
    readonly property color field: "#CFC5AE"
    readonly property color fieldOutline: "#9C9079"
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
    property bool showCurrentFlow: true // Stromflusspunkte auf den Leitungen
    property real fontScale: 1.0        // Schriftgroesse (Einstellung A-/A+): 0,8 / 0,9 / 1,0
    // Touch-Oberflaeche (Android, iOS; auf dem Desktop mit VOLTA_DEMO_TOUCH=1): Arbeitsflaeche startet in
    // Originalgroesse, damit Kontaktfelder 56 dp hoch bleiben; Zoom/Verschieben mit zwei Fingern
    property bool touchUi: Qt.platform.os === "android" || Qt.platform.os === "ios"

    // Masse in dp (Qt Quick: geraeteunabhaengige Pixel)
    readonly property int fieldWidth: 72
    readonly property int fieldHeight: 56   // Kontaktfelder >= 56 dp
    readonly property int fieldGap: 24
    readonly property int deskFieldWidth: 56
    readonly property int deskFieldGap: 6 // kompakt: Pultleiste passt ueber 8 Spalten
    readonly property int slotWidth: 36
    // Steckloecher: fuenf je Feld, mittig; Abstand von Lochmitte zu Lochmitte (Pultfelder sind schmaler)
    readonly property int meterAreaWidth: 190 // Messgeraet rechts neben der Grundplatte (einblendbar)
    readonly property int socketSize: 8
    readonly property int socketPitch: 13
    readonly property int deskSocketPitch: 10
    readonly property int slotGap: 8
    readonly property int control: 48       // Bedienelemente >= 48 dp
    readonly property int margin: 24
    readonly property real maxZoom: 2.0     // groesste Vergroesserung der Arbeitsflaeche
    readonly property int compactWidth: 1280 // darunter (Hochformat, iPad, kleine Tablets): schmale Werkzeugleiste
}
