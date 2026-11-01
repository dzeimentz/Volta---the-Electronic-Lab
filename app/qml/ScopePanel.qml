// Oszilloskop ab Volta 2 (Konzept 5.2, 9.3), an der Stelle des Zeitschreibers unter der Arbeitsflaeche: zwei Kanaele
// an den Tastkoepfen K1 (gelb, durchgezogen) und K2 (tuerkis, gestrichelt), Bezug 0V - Farbe, Linienart und
// Beschriftung unterscheiden die Kanaele. Schirm 10 x 8 Teilungen; Zeitbasis, Volt je Teilung und Lage je Kanal,
// Trigger (Kanal, Flanke, Pegel), Rollbetrieb (der Zeitschreiber, ohne Trigger von rechts nach links), XY-Betrieb
// und Standbild. Spitze-Spitze, Frequenz und Periode stehen als Zahl in der Kanalfarbe daneben.
// Alles Bedienhandlungen; das Bild rechnet der LabController (model/oscilloscope).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Rectangle {
    id: panel
    required property var controller
    signal closeRequested()

    color: "white"
    border.color: Theme.fieldOutline
    property var sc: ({channels: []})
    property bool frozen: false // Standbild: das Bild bleibt stehen, die Simulation laeuft weiter
    readonly property var chans: sc.channels || []

    function refresh() { panel.sc = panel.controller.scopeState() }
    Timer { // ca. 15 Bilder je Sekunde
        interval: 66
        running: panel.visible && !panel.frozen
        repeat: true
        triggeredOnStart: true
        onTriggered: panel.refresh()
    }
    Connections {
        target: panel.controller
        function onDeskChanged() { if (!panel.frozen) panel.refresh() }
    }
    function polyline(flat, w, h, xy) {
        const pts = []
        for (let i = 0; i + 1 < flat.length; i += 2)
            pts.push(Qt.point(flat[i] / 10 * w, h / 2 - flat[i + 1] / 8 * h))
        return pts
    }

    // ---------------------------------------------------------------- Schirm
    Rectangle {
        id: screen
        objectName: "scopeScreen"
        x: 6; y: 6
        height: panel.height - 12
        width: height * 1.25
        color: "#0E1A16"
        radius: 4
        clip: true

        // Raster 10 x 8, Mittelachsen kraeftiger
        Repeater {
            model: 11
            Rectangle {
                required property int index
                x: index * screen.width / 10; y: 0; width: 1; height: screen.height
                color: index === 5 ? "#4C6B5E" : "#26392F"
            }
        }
        Repeater {
            model: 9
            Rectangle {
                required property int index
                x: 0; y: index * screen.height / 8; height: 1; width: screen.width
                color: index === 4 ? "#4C6B5E" : "#26392F"
            }
        }
        // Nullinie je Kanal: Marke am linken Rand mit Kanalnamen
        Repeater {
            model: panel.chans.length
            Label {
                required property int index
                readonly property var ch: panel.chans[index]
                visible: ch.has && !panel.sc.xy
                x: 2
                y: screen.height / 2 - ch.position / 8 * screen.height - height / 2
                text: "▸" + ch.label
                color: Theme.scopeTraceColors[index]
                font.pixelSize: Math.round(11 * Theme.fontScale); font.bold: true
            }
        }
        // Triggerpegel: Marke am rechten Rand in der Farbe des Triggerkanals
        Label {
            readonly property var ch: panel.chans[panel.sc.triggerChannel || 0]
            visible: !!ch && ch.has && !panel.sc.roll && !panel.sc.xy
            x: screen.width - width - 2
            y: screen.height / 2 - (panel.sc.triggerDiv || 0) / 8 * screen.height - height / 2
            text: "T◂"
            color: Theme.scopeTraceColors[panel.sc.triggerChannel || 0]
            font.pixelSize: Math.round(11 * Theme.fontScale); font.bold: true
        }
        // Kurven
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeColor: Theme.scopeTraceColors[0]; strokeWidth: 2; fillColor: "transparent"
                PathPolyline { path: panel.chans.length > 0 && !panel.sc.xy ? panel.polyline(panel.chans[0].points, screen.width, screen.height) : [] }
            }
            ShapePath {
                strokeColor: Theme.scopeTraceColors[1]; strokeWidth: 2; fillColor: "transparent"
                strokeStyle: ShapePath.DashLine; dashPattern: [4, 2]
                PathPolyline { path: panel.chans.length > 1 && !panel.sc.xy ? panel.polyline(panel.chans[1].points, screen.width, screen.height) : [] }
            }
            ShapePath {
                strokeColor: "#A6F07A"; strokeWidth: 2; fillColor: "transparent"
                PathPolyline { path: panel.sc.xy ? panel.polyline(panel.sc.xyPoints || [], screen.width, screen.height) : [] }
            }
        }
        // Kennwerte je Kanal in der Kanalfarbe
        Column {
            x: 22; y: 2
            spacing: 0
            Repeater {
                model: panel.chans.length
                Label {
                    required property int index
                    readonly property var ch: panel.chans[index]
                    visible: ch.has
                    text: ch.label + "  " + ch.voltsText + "/Div"
                          + (ch.any ? "   " + qsTr("Spitze-Spitze %1").arg(ch.ppText)
                                      + (ch.frequencyText ? "   " + ch.frequencyText + "   T = " + ch.periodText
                                                          : "   " + qsTr("Mittel %1").arg(ch.meanText)) : "")
                    color: Theme.scopeTraceColors[index]
                    font.pixelSize: Math.round(11 * Theme.fontScale)
                }
            }
            Label {
                text: panel.sc.xy ? qsTr("XY: K1 waagrecht, K2 senkrecht")
                    : (panel.sc.timebaseText || "") + "/Div" + (panel.sc.roll ? "   " + qsTr("Rollbetrieb")
                       : panel.sc.triggered ? "   " + qsTr("Trigger") : "   " + qsTr("frei laufend"))
                       + (panel.frozen ? "   " + qsTr("Standbild") : "")
                color: "#C9D8D0"
                font.pixelSize: Math.round(11 * Theme.fontScale)
            }
        }
        Label { // ohne Tastkopf: was zu tun ist
            visible: panel.chans.length > 0 && !panel.chans[0].has && !panel.chans[1].has
            anchors.centerIn: parent
            width: parent.width - 24
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("Stecke den Tastkopf K1 (Messen) in ein Feld. Bezug ist immer 0V.")
            color: "#E0E8E4"
            font.pixelSize: Math.round(13 * Theme.fontScale)
        }
        Accessible.role: Accessible.Graphic
        Accessible.name: qsTr("Oszilloskop-Schirm")
    }

    // ---------------------------------------------------------------- Bedienung
    component Btn: ToolButton {
        height: Theme.control
        width: Math.max(Theme.control, implicitWidth)
        font.pixelSize: Math.round(13 * Theme.fontScale)
        font.bold: true
        ToolTip.visible: hovered && ToolTip.text !== ""
    }
    component Caption: Label {
        height: Theme.control
        verticalAlignment: Text.AlignVCenter
        color: Theme.navy
        font.pixelSize: Math.round(12 * Theme.fontScale)
    }

    Flickable {
        anchors { left: screen.right; leftMargin: 8; right: closeButton.left; top: parent.top; bottom: parent.bottom; topMargin: 4 }
        contentHeight: controls.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        Flow {
            id: controls
            width: parent.width
            spacing: 4
            Label {
                height: Theme.control
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Oszilloskop")
                font.bold: true
                font.pixelSize: Math.round(14 * Theme.fontScale)
                color: Theme.navy
            }
            // Zeitbasis
            Row {
                Btn {
                    objectName: "scopeFaster"
                    text: "−"; enabled: panel.sc.canFaster === true && !panel.sc.xy
                    onClicked: panel.controller.setScope("timebase", panel.sc.timebase - 1)
                    ToolTip.text: qsTr("Zeitbasis kürzer"); Accessible.name: ToolTip.text
                }
                Caption { text: (panel.sc.timebaseText || "") + "/Div"; width: 76; horizontalAlignment: Text.AlignHCenter }
                Btn {
                    objectName: "scopeSlower"
                    text: "+"; enabled: panel.sc.canSlower === true && !panel.sc.xy
                    onClicked: panel.controller.setScope("timebase", panel.sc.timebase + 1)
                    ToolTip.text: qsTr("Zeitbasis länger"); Accessible.name: ToolTip.text
                }
            }
            Btn {
                objectName: "scopeRoll"
                text: qsTr("Rollen"); checkable: true; checked: panel.sc.roll === true
                onClicked: panel.controller.setScope("roll", checked)
                ToolTip.text: qsTr("Rollbetrieb: aufzeichnen wie ein Zeitschreiber"); Accessible.name: ToolTip.text
            }
            Btn {
                objectName: "scopeXy"
                text: "XY"; checkable: true; checked: panel.sc.xy === true
                onClicked: panel.controller.setScope("xy", checked)
                ToolTip.text: qsTr("XY-Betrieb: K1 waagrecht, K2 senkrecht"); Accessible.name: ToolTip.text
            }
            Btn {
                objectName: "scopeFreeze"
                text: panel.frozen ? "▶" : "⏸"; checkable: true; checked: panel.frozen
                onClicked: panel.frozen = checked
                ToolTip.text: panel.frozen ? qsTr("Weiterlaufen") : qsTr("Standbild"); Accessible.name: ToolTip.text
            }
            // Kanaele: Volt je Teilung und Lage
            Repeater {
                model: 2
                Row {
                    id: chRow
                    required property int index
                    readonly property var ch: panel.chans[index] || ({})
                    Rectangle { // Kanalfarbe mit Namen (Farbe nie allein)
                        width: 34; height: Theme.control - 12; y: 6; radius: 4
                        color: Theme.probeColors[chRow.index]
                        Label { anchors.centerIn: parent; text: "K" + (chRow.index + 1); font.bold: true; color: "#1E2B38" }
                    }
                    Btn {
                        text: "−"; enabled: (chRow.ch.volts || 0) > 0
                        onClicked: panel.controller.setScope("volts" + chRow.index, chRow.ch.volts - 1)
                        ToolTip.text: qsTr("K%1 empfindlicher").arg(chRow.index + 1); Accessible.name: ToolTip.text
                    }
                    Caption { text: (chRow.ch.voltsText || "") + "/Div"; width: 70; horizontalAlignment: Text.AlignHCenter }
                    Btn {
                        text: "+"; enabled: (chRow.ch.volts || 0) < 8
                        onClicked: panel.controller.setScope("volts" + chRow.index, chRow.ch.volts + 1)
                        ToolTip.text: qsTr("K%1 unempfindlicher").arg(chRow.index + 1); Accessible.name: ToolTip.text
                    }
                    Btn {
                        text: "▲"; enabled: (chRow.ch.position || 0) < 4
                        onClicked: panel.controller.setScope("position" + chRow.index, chRow.ch.position + 1)
                        ToolTip.text: qsTr("K%1 nach oben").arg(chRow.index + 1); Accessible.name: ToolTip.text
                    }
                    Btn {
                        text: "▼"; enabled: (chRow.ch.position || 0) > -4
                        onClicked: panel.controller.setScope("position" + chRow.index, chRow.ch.position - 1)
                        ToolTip.text: qsTr("K%1 nach unten").arg(chRow.index + 1); Accessible.name: ToolTip.text
                    }
                }
            }
            // Trigger: Kanal, Flanke, Pegel
            Row {
                visible: !panel.sc.roll && !panel.sc.xy
                Caption { text: qsTr("Trigger"); rightPadding: 4 }
                Btn {
                    objectName: "scopeTriggerChannel"
                    text: "K" + ((panel.sc.triggerChannel || 0) + 1)
                    onClicked: panel.controller.setScope("triggerChannel", 1 - (panel.sc.triggerChannel || 0))
                    ToolTip.text: qsTr("Triggerkanal wechseln"); Accessible.name: ToolTip.text
                }
                Btn {
                    objectName: "scopeTriggerEdge"
                    text: panel.sc.triggerRising ? "↗" : "↘"
                    onClicked: panel.controller.setScope("triggerRising", !panel.sc.triggerRising)
                    ToolTip.text: panel.sc.triggerRising ? qsTr("steigende Flanke") : qsTr("fallende Flanke")
                    Accessible.name: ToolTip.text
                }
                Btn {
                    text: "−"
                    onClicked: panel.controller.setScope("triggerLevel", Math.round(((panel.sc.triggerLevel || 0) - 0.5) * 10) / 10)
                    ToolTip.text: qsTr("Triggerpegel tiefer"); Accessible.name: ToolTip.text
                }
                Caption { text: panel.sc.triggerText || ""; width: 54; horizontalAlignment: Text.AlignHCenter }
                Btn {
                    text: "+"
                    onClicked: panel.controller.setScope("triggerLevel", Math.round(((panel.sc.triggerLevel || 0) + 0.5) * 10) / 10)
                    ToolTip.text: qsTr("Triggerpegel höher"); Accessible.name: ToolTip.text
                }
            }
        }
    }

    ToolButton {
        id: closeButton
        anchors.right: parent.right; anchors.rightMargin: 4
        y: 4
        width: Theme.control; height: Theme.control
        contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 20 } }
        onClicked: panel.closeRequested()
        Accessible.name: qsTr("Oszilloskop schließen")
    }
}
