// Zeitschreiber (Werkzeug ab Stufe 3): zeichnet die Spannung an bis zu zwei Kontaktfeldern oder die Anzeige des
// Messgeraets ueber die Zeit auf, etwa das Laden eines Kondensators oder das Blinken einer Kippstufe. Kanaele
// schliesst man ueber das Menue eines Felds an (langer Druck bzw. Rechtsklick). Kanal 1 rot durchgezogen, Kanal 2
// blau gestrichelt (Farbe nie allein). Die Skala passt sich an: Spannung 0 bis Batteriespannung (bzw. hoeher).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Rectangle {
    id: panel
    required property var controller
    signal closeRequested()

    color: "white"
    border.color: Theme.fieldOutline
    property var rec: ({channels: [], window: 10})
    readonly property var chans: rec.channels || []
    readonly property real window: rec.window || 10
    readonly property color c1: "#D32F2F"
    readonly property color c2: "#1E5FC2"

    Timer { // ca. 15 Bilder je Sekunde genuegen fuer eine ruhige Kurve
        interval: 66
        running: panel.visible && !panel.controller.recorderPaused
        repeat: true
        triggeredOnStart: true
        onTriggered: panel.rec = panel.controller.recorderData(Math.max(60, Math.round(plot.width / 2)))
    }
    Connections {
        target: panel.controller
        function onRecorderChanged() { panel.rec = panel.controller.recorderData(Math.max(60, Math.round(plot.width / 2))) }
    }

    // Skala: gemeinsame Einheit je Kanal; Spannung mindestens 0 .. Batteriespannung
    function range(ch) {
        const d = chans[ch]
        if (!d || !d.any) return {lo: 0, hi: panel.controller.batteryVolt}
        if (d.unit === "V") return {lo: Math.min(0, d.min), hi: Math.max(panel.controller.batteryVolt, d.max)}
        const span = Math.max(1e-6, d.max - Math.min(0, d.min))
        return {lo: Math.min(0, d.min), hi: Math.min(0, d.min) + span * 1.15}
    }
    function fmt(v, unit) {
        const a = Math.abs(v)
        const digits = a >= 100 ? 0 : a >= 10 ? 1 : 2
        return Number(v).toLocaleString(Qt.locale(), "f", digits) + " " + unit
    }
    function label(src) {
        if (src === "MESS") return qsTr("Messgerät")
        return src.startsWith("P:") ? src.substring(2) : src.startsWith("M:") ? "M" + src.substring(2) : src
    }

    // ---- Kopfzeile: Titel, Kanaele, Zeitfenster, Pause, Loeschen, Schliessen
    Row {
        id: head
        x: 8; y: 4
        spacing: 8
        height: Theme.control
        Label {
            anchors.verticalCenter: parent.verticalCenter
            text: "📈 " + qsTr("Zeitschreiber")
            font.bold: true
            font.pixelSize: Math.round(14 * Theme.fontScale)
            color: Theme.navy
        }
        Repeater {
            model: 2
            Rectangle {
                required property int index
                readonly property string src: panel.controller.recorderChannels[index] || ""
                anchors.verticalCenter: parent.verticalCenter
                height: 32
                width: chipText.implicitWidth + (src !== "" ? 44 : 16)
                radius: 16
                color: src !== "" ? (index === 0 ? "#FDE3E3" : "#E0EAFB") : "#F0F2F4"
                border.color: index === 0 ? panel.c1 : panel.c2
                border.width: src !== "" ? 2 : 1
                Label {
                    id: chipText
                    x: 8
                    anchors.verticalCenter: parent.verticalCenter
                    readonly property var d: panel.chans[index]
                    text: (index === 0 ? "K1 ━ " : "K2 ┅ ")
                          + (src === "" ? qsTr("frei – Feld-Menü") : panel.label(src)
                             + (d && d.any ? ": " + panel.fmt(d.last, d.unit) : ""))
                    font.pixelSize: Math.round(12 * Theme.fontScale)
                    color: Theme.navy
                }
                ToolButton {
                    visible: src !== ""
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 32; height: 32
                    contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 14 } }
                    onClicked: panel.controller.setRecorderChannel(index, "")
                    Accessible.name: qsTr("Kanal %1 trennen").arg(index + 1)
                }
            }
        }
    }
    Row {
        anchors.right: parent.right
        anchors.rightMargin: 4
        y: 4
        spacing: 2
        Repeater {
            model: [2, 10, 30]
            ToolButton {
                required property int modelData
                height: Theme.control
                checkable: true
                checked: Math.abs(panel.window - modelData) < 0.01
                text: qsTr("%1 s").arg(modelData)
                onClicked: panel.controller.recorderWindow = modelData
                font.pixelSize: Math.round(12 * Theme.fontScale)
            }
        }
        ToolButton {
            objectName: "recorderPause"
            width: Theme.control; height: Theme.control
            text: panel.controller.recorderPaused ? "▶" : "⏸"
            onClicked: panel.controller.recorderPaused = !panel.controller.recorderPaused
            ToolTip.visible: hovered
            ToolTip.text: Accessible.name
            Accessible.name: panel.controller.recorderPaused ? qsTr("Weiter aufzeichnen") : qsTr("Anhalten")
        }
        ToolButton {
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "backspace"; color: Theme.navy; size: 22 } }
            onClicked: panel.controller.clearRecorder()
            ToolTip.visible: hovered
            ToolTip.text: Accessible.name
            Accessible.name: qsTr("Aufzeichnung löschen")
        }
        ToolButton {
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 20 } }
            onClicked: panel.closeRequested()
            Accessible.name: qsTr("Zeitschreiber schließen")
        }
    }

    // ---- Diagramm
    Item {
        id: plot
        anchors { left: parent.left; right: parent.right; top: head.bottom; bottom: parent.bottom }
        anchors.leftMargin: 56; anchors.rightMargin: 56; anchors.topMargin: 6; anchors.bottomMargin: 22

        Rectangle { anchors.fill: parent; color: "#FAFBFC"; border.color: "#C9D1D9" }
        // Raster: 4 waagrechte, Zeitstriche je nach Fenster
        Repeater {
            model: 5
            Rectangle { required property int index; width: plot.width; height: 1; y: plot.height * index / 4; color: "#E3E8ED" }
        }
        Repeater {
            model: panel.window <= 2 ? 5 : panel.window <= 10 ? 6 : 7
            Item {
                required property int index
                readonly property real step: panel.window <= 2 ? 0.5 : panel.window <= 10 ? 2 : 5
                x: plot.width * index * step / panel.window
                height: plot.height
                Rectangle { width: 1; height: parent.height; color: "#E3E8ED" }
                Label {
                    y: plot.height + 2; x: -width / 2
                    text: qsTr("%1 s").arg(Number(index * parent.step).toLocaleString(Qt.locale(), "f", parent.step < 1 ? 1 : 0))
                    font.pixelSize: 10; color: "#5D6B7A"
                }
            }
        }
        // Achsenbeschriftung links (Kanal 1) und rechts (Kanal 2)
        Repeater {
            model: 2
            Column {
                required property int index
                readonly property var r: panel.range(index)
                readonly property var d: panel.chans[index]
                visible: d !== undefined && d.source !== ""
                x: index === 0 ? -width - 6 : plot.width + 6
                height: plot.height
                Label { text: panel.fmt(parent.r.hi, parent.d ? parent.d.unit : "V"); font.pixelSize: 10; color: index === 0 ? panel.c1 : panel.c2 }
                Item { width: 1; height: plot.height - 28 }
                Label { text: panel.fmt(parent.r.lo, parent.d ? parent.d.unit : "V"); font.pixelSize: 10; color: index === 0 ? panel.c1 : panel.c2 }
            }
        }
        // Kurven
        Repeater {
            model: 2
            Shape {
                id: curve
                required property int index
                anchors.fill: parent
                readonly property var d: panel.chans[index]
                readonly property var r: panel.range(index)
                visible: d !== undefined && d.any
                ShapePath {
                    strokeColor: curve.index === 0 ? panel.c1 : panel.c2
                    strokeWidth: 2.5
                    strokeStyle: curve.index === 0 ? ShapePath.SolidLine : ShapePath.DashLine
                    dashPattern: [4, 3]
                    fillColor: "transparent"
                    PathPolyline {
                        path: {
                            const out = []
                            const d = curve.d
                            if (!d || !d.any) return out
                            const r = curve.r
                            const span = Math.max(1e-9, r.hi - r.lo)
                            for (let i = 0; i + 1 < d.points.length; i += 2)
                                out.push(Qt.point(plot.width * d.points[i] / panel.window,
                                                  plot.height * (1 - (d.points[i + 1] - r.lo) / span)))
                            return out
                        }
                    }
                }
            }
        }
        Label {
            anchors.centerIn: parent
            visible: !(panel.chans.length > 0 && (panel.chans[0].source !== "" || panel.chans[1].source !== ""))
            width: parent.width - 20
            wrapMode: Text.Wrap
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Langer Druck oder Rechtsklick auf ein Feld: »Zeitschreiber Kanal 1« wählen. Dann wird die Spannung dort aufgezeichnet.")
            color: "#5D6B7A"
            font.pixelSize: Math.round(13 * Theme.fontScale)
        }
    }
}
