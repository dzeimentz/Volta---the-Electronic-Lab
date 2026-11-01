// Kennlinienschreiber ab Volta 2 (Konzept 5.2): zeichnet den Strom durch das Bauteil zwischen KS+ und KS- ueber der
// Spannung daran. Der Schreiber faehrt die Pruefspannung in 2 s hin und zurueck; der Vorwiderstand begrenzt den Strom.
// Bedienung: hoechste Spannung, nur positiv oder beide Richtungen, Strombegrenzung; »Merken« behaelt die Kurve
// gestrichelt zum Vergleich. Spannung und Strom stehen zusaetzlich als Zahl da. Nicht modal: die Grundplatte bleibt
// bedienbar. Das Bild rechnet der LabController.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Popup {
    id: panel
    property var controller: null
    property var ts: ({points: []})
    property var kept: []          // gemerkte Kurve (flach x, y)
    property bool frozen: false    // Standbild

    function refresh() { if (controller) ts = controller.tracerState() }
    function polyline(flat, w, h) {
        const pts = []
        for (let i = 0; i + 1 < flat.length; i += 2) pts.push(Qt.point(flat[i] * w, h - flat[i + 1] * h))
        return pts
    }
    onOpened: refresh()

    Timer {
        interval: 66
        running: panel.opened && !panel.frozen
        repeat: true
        onTriggered: panel.refresh()
    }

    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape
    x: parent ? parent.width - width - 16 : 0
    y: parent ? Math.max(16, parent.height - height - 16) : 0
    width: Math.min(parent ? parent.width - 32 : 480, 480)
    padding: 12
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    contentItem: Column {
        spacing: 8
        Row {
            width: parent.width
            Label {
                width: parent.width - closeButton.width
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Kennlinienschreiber")
                color: Theme.navy
                font.pixelSize: Math.round(18 * Theme.fontScale)
                font.bold: true
            }
            ToolButton {
                id: closeButton
                width: Theme.control; height: Theme.control
                contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 18 } }
                onClicked: panel.close()
                Accessible.name: qsTr("Kennlinienschreiber schließen")
            }
        }

        // ------------------------------------------------------------ Schirm: I (senkrecht) ueber U (waagrecht)
        Rectangle {
            id: screen
            objectName: "tracerScreen"
            width: parent.width
            height: width * 0.62
            color: "#0E1A16"
            radius: 4
            clip: true
            Accessible.role: Accessible.Graphic
            Accessible.name: qsTr("Kennlinie: Strom über Spannung")

            Repeater {
                model: 11
                Rectangle {
                    required property int index
                    x: index * screen.width / 10; y: 0; width: 1; height: screen.height
                    color: "#26392F"
                }
            }
            Repeater {
                model: 9
                Rectangle {
                    required property int index
                    x: 0; y: index * screen.height / 8; width: screen.width; height: 1
                    color: "#26392F"
                }
            }
            // Achsen durch 0 V und 0 A, kraeftiger
            Rectangle { x: (panel.ts.xZero || 0) * screen.width; y: 0; width: 2; height: screen.height; color: "#6F9584" }
            Rectangle { x: 0; y: screen.height - (panel.ts.yZero || 0) * screen.height; width: screen.width; height: 2; color: "#6F9584" }
            Text { // Achsenbeschriftung: U rechts, I oben
                x: screen.width - width - 4
                y: screen.height - (panel.ts.yZero || 0) * screen.height - height - 2
                text: "U"; color: "#9FC2B2"; font.pixelSize: 14; font.bold: true
            }
            Text {
                x: (panel.ts.xZero || 0) * screen.width + 6; y: 2
                text: "I"; color: "#9FC2B2"; font.pixelSize: 14; font.bold: true
            }
            Text { x: 4; y: screen.height - height - 2; text: panel.ts.xMinText || ""; color: "#9FC2B2"; font.pixelSize: 11 }
            Text { x: screen.width - width - 4; y: screen.height - height - 2; text: panel.ts.xMaxText || ""; color: "#9FC2B2"; font.pixelSize: 11 }
            Text { x: (panel.ts.xZero || 0) * screen.width + 18; y: 2; text: panel.ts.yMaxText || ""; color: "#9FC2B2"; font.pixelSize: 11 }

            Shape {
                anchors.fill: parent
                preferredRendererType: Shape.CurveRenderer
                ShapePath { // gemerkte Kurve: grau gestrichelt
                    strokeColor: "#B8C4BE"
                    strokeWidth: 2
                    strokeStyle: ShapePath.DashLine
                    dashPattern: [3, 3]
                    fillColor: "transparent"
                    PathPolyline { path: panel.polyline(panel.kept, screen.width, screen.height) }
                }
                ShapePath { // aktuelle Kurve
                    strokeColor: "#7CFC9A"
                    strokeWidth: 2.5
                    fillColor: "transparent"
                    joinStyle: ShapePath.RoundJoin
                    PathPolyline { path: panel.polyline(panel.ts.points || [], screen.width, screen.height) }
                }
            }
            Rectangle { // wo der Schreiber gerade steht
                visible: panel.ts.has === true
                width: 10; height: 10; radius: 5
                color: "white"
                x: (panel.ts.dotX || 0) * screen.width - 5
                y: screen.height - (panel.ts.dotY || 0) * screen.height - 5
            }
        }

        Label {
            width: parent.width
            text: panel.ts.has ? qsTr("U = %1    I = %2").arg(panel.ts.uText).arg(panel.ts.iText) : ""
            font.bold: true
            color: Theme.navy
            font.pixelSize: Math.round(15 * Theme.fontScale)
        }

        // ------------------------------------------------------------ Bedienung
        Flow {
            width: parent.width
            spacing: 6
            Label { height: Theme.control; verticalAlignment: Text.AlignVCenter; text: qsTr("Spannung"); color: Theme.navy; font.bold: true }
            Button {
                objectName: "tracerVoltsDown"
                width: Theme.control; height: Theme.control; text: "−"
                enabled: (panel.ts.voltsIndex || 0) > 0
                onClicked: { panel.controller.setTracer("volts", panel.ts.voltsIndex - 1); panel.kept = []; panel.refresh() }
                Accessible.name: qsTr("Prüfspannung kleiner")
            }
            Label { width: 72; height: Theme.control; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; text: panel.ts.rangeText || ""; color: Theme.navy }
            Button {
                objectName: "tracerVoltsUp"
                width: Theme.control; height: Theme.control; text: "+"
                enabled: (panel.ts.voltsIndex || 0) < (panel.ts.voltsCount || 1) - 1
                onClicked: { panel.controller.setTracer("volts", panel.ts.voltsIndex + 1); panel.kept = []; panel.refresh() }
                Accessible.name: qsTr("Prüfspannung größer")
            }
            Button {
                objectName: "tracerBipolar"
                height: Theme.control
                checkable: true
                checked: panel.ts.bipolar === true
                text: qsTr("beide Richtungen")
                onClicked: { panel.controller.setTracer("bipolar", checked); panel.kept = []; panel.refresh() }
                Accessible.name: qsTr("Prüfspannung in beide Richtungen")
            }
            Label { height: Theme.control; verticalAlignment: Text.AlignVCenter; text: qsTr("Strom höchstens"); color: Theme.navy; font.bold: true }
            Button {
                objectName: "tracerLimitDown"
                width: Theme.control; height: Theme.control; text: "−"
                enabled: (panel.ts.limitIndex || 0) > 0
                onClicked: { panel.controller.setTracer("limit", panel.ts.limitIndex - 1); panel.kept = []; panel.refresh() }
                Accessible.name: qsTr("Strombegrenzung kleiner")
            }
            Label { width: 64; height: Theme.control; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; text: panel.ts.limitText || ""; color: Theme.navy }
            Button {
                objectName: "tracerLimitUp"
                width: Theme.control; height: Theme.control; text: "+"
                enabled: (panel.ts.limitIndex || 0) < (panel.ts.limitCount || 1) - 1
                onClicked: { panel.controller.setTracer("limit", panel.ts.limitIndex + 1); panel.kept = []; panel.refresh() }
                Accessible.name: qsTr("Strombegrenzung größer")
            }
            Button {
                objectName: "tracerKeep"
                height: Theme.control
                text: panel.kept.length > 0 ? qsTr("Vergessen") : qsTr("Merken")
                onClicked: panel.kept = panel.kept.length > 0 ? [] : (panel.ts.points || []).slice()
                Accessible.name: panel.kept.length > 0 ? qsTr("Gemerkte Kurve vergessen") : qsTr("Kurve zum Vergleich merken")
            }
            Button {
                objectName: "tracerFreeze"
                height: Theme.control
                checkable: true
                checked: panel.frozen
                text: panel.frozen ? qsTr("Weiterlaufen") : qsTr("Standbild")
                onClicked: panel.frozen = checked
            }
        }
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            color: "#4A5560"
            text: qsTr("Schalte das Bauteil zwischen KS+ und KS−. Der Schreiber fährt die Spannung in 2 s hin und zurück und zeichnet den Strom darüber. Vorwiderstand %1.").arg(panel.ts.ohmText || "")
        }
    }
}
