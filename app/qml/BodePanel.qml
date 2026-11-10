// Frequenzgang-Schreiber (ab Volta 3, Konzept 5.2) unter der Arbeitsflaeche: Eingang am Generator, Ausgang am
// Tastkopf K1. Die Kurve zeigt die Verstaerkung in dB ueber der Frequenz (logarithmisch 1 Hz ... 10 MHz) in der Farbe
// von K1, wahlweise die Phase gestrichelt. Resonanz- und Grenzfrequenzen stehen als senkrechte Marke mit Zahl im
// Bild und rechts als Text - Lage, Linienart und Beschriftung tragen die Information, nicht die Farbe allein.
// Volta rechnet sie mit der Wechselstromanalyse am Arbeitspunkt; das Bild rechnet der LabController.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Rectangle {
    id: panel
    required property var controller
    signal closeRequested()
    color: "white"
    border.color: Theme.fieldOutline
    property var bs: ({status: "wait"})
    property bool showPhase: false
    readonly property bool ok: bs.status === "ok"
    // schmal (neben dem Oszilloskop): Kennwerte rechts und Untertitel entfallen, die Marken im Bild bleiben
    readonly property bool narrow: width < 600

    function refresh() { panel.bs = panel.controller.bodeState() }
    // neu rechnen, solange er offen ist (Drehregler, Bauteilwerte und Arbeitspunkt aendern die Kurve)
    Timer {
        interval: 500
        running: panel.visible
        repeat: true
        triggeredOnStart: true
        onTriggered: panel.controller.requestBode()
    }
    Timer {
        interval: 150
        running: panel.visible
        repeat: true
        triggeredOnStart: true
        onTriggered: panel.refresh()
    }

    function kindText(k) {
        switch (k) {
        case "lowpass": return qsTr("Tiefpass: tiefe Frequenzen kommen durch")
        case "highpass": return qsTr("Hochpass: hohe Frequenzen kommen durch")
        case "bandpass": return qsTr("Bandpass: ein Bereich um die Resonanz kommt durch")
        case "bandstop": return qsTr("Bandsperre: ein Bereich wird gesperrt")
        case "flat": return qsTr("gerade: alle Frequenzen kommen gleich durch")
        }
        return ""
    }
    function statusText() {
        switch (panel.bs.status) {
        case "nogen": return qsTr("Der Frequenzgang-Schreiber braucht den Generator im Pult.")
        case "noprobe": return qsTr("Stecke den Tastkopf K1 an den Ausgang der Schaltung. Der Eingang ist der Generator G.")
        case "off": return qsTr("Schalte den Hauptschalter ein.")
        case "wait": return qsTr("Volta rechnet …")
        case "error": return qsTr("Hier kommt vom Generator nichts an K1 an. Ist G mit der Schaltung verbunden?")
        case "switching": {
            const names = (panel.bs.switching || []).map(t => Texts.partName(t) || Texts.moduleName(t))
            return qsTr("Keine Kurve: %1 schaltet, statt gleichmäßig zu verstärken. Der Frequenzgang gilt nur für Schaltungen, die jedes Signal gleich behandeln – miss solche Schaltungen mit dem Oszilloskop.").arg(names.join(", "))
        }
        }
        return ""
    }

    Row {
        id: head
        x: 8; y: 4
        spacing: 8
        height: Theme.control
        Label {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Frequenzgang-Schreiber")
            font.bold: true
            font.pixelSize: Math.round(14 * Theme.fontScale)
            color: Theme.navy
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            visible: !panel.narrow
            text: qsTr("Eingang G · Ausgang K1")
            font.pixelSize: Math.round(12 * Theme.fontScale)
            color: "#4A5866"
        }
        PanelButton {
            objectName: "bodePhase"
            height: Theme.control
            width: Math.max(Theme.control, implicitWidth)
            checkable: true
            checked: panel.showPhase
            text: panel.showPhase ? qsTr("Phase: an") : qsTr("Phase: aus")
            onClicked: panel.showPhase = !panel.showPhase
            Accessible.name: qsTr("Phase zeigen")
        }
    }
    PanelButton {
        objectName: "bodeClose"
        anchors { right: parent.right; top: parent.top; margins: 4 }
        width: Theme.control; height: Theme.control
        contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 20 } }
        onClicked: panel.closeRequested()
        Accessible.name: qsTr("Frequenzgang-Schreiber schließen")
    }

    // ---------------------------------------------------------------- Schirm
    Item {
        id: plotArea
        x: 8
        anchors { top: head.bottom; topMargin: 4; bottom: parent.bottom; bottomMargin: 6 }
        width: Math.max(200, panel.width - 16 - (side.visible ? side.width + 12 : 0))

        readonly property real axisL: 52
        readonly property real axisB: 18
        Rectangle {
            id: screen
            objectName: "bodeScreen"
            x: plotArea.axisL
            width: plotArea.width - plotArea.axisL - 8
            height: plotArea.height - plotArea.axisB
            color: "#0E1A16"
            radius: 4
            clip: true
            // senkrecht je Dekade, waagerecht je 10 dB
            Repeater {
                model: 8
                Rectangle {
                    required property int index
                    x: index * screen.width / 7; y: 0; width: 1; height: screen.height
                    color: "#26392F"
                }
            }
            Repeater {
                model: 9
                Rectangle {
                    required property int index
                    x: 0; y: index * screen.height / 8; height: 1; width: screen.width
                    color: "#26392F"
                }
            }
            // 0 dB kraeftiger
            Rectangle {
                visible: panel.ok && panel.bs.topDb >= 0 && panel.bs.bottomDb <= 0
                x: 0; width: screen.width; height: 1
                y: screen.height * (panel.bs.topDb / 80)
                color: "#4C6B5E"
            }
            Shape {
                anchors.fill: parent
                visible: panel.ok
                preferredRendererType: Shape.CurveRenderer
                ShapePath {
                    strokeColor: Theme.scopeTraceColors[0]; strokeWidth: 2; fillColor: "transparent"
                    PathPolyline {
                        path: {
                            const c = panel.bs.curve || [], pts = []
                            for (let i = 0; i + 1 < c.length; i += 2)
                                pts.push(Qt.point(c[i] * screen.width, (1 - c[i + 1]) * screen.height))
                            return pts
                        }
                    }
                }
                ShapePath {
                    strokeColor: "#C9D6E3"; strokeWidth: panel.showPhase ? 1.5 : 0; fillColor: "transparent"
                    strokeStyle: ShapePath.DashLine; dashPattern: [3, 3]
                    PathPolyline {
                        path: {
                            if (!panel.showPhase) return []
                            const c = panel.bs.phase || [], pts = []
                            for (let i = 0; i + 1 < c.length; i += 2)
                                pts.push(Qt.point(c[i] * screen.width, (1 - c[i + 1]) * screen.height))
                            return pts
                        }
                    }
                }
            }
            // Marken: Resonanz durchgezogen, Grenzfrequenzen gestrichelt, je mit Zahl
            Repeater {
                model: panel.ok ? (panel.bs.markers || []) : []
                Item {
                    id: mk
                    required property var modelData
                    required property int index
                    x: modelData.x * screen.width
                    height: screen.height
                    Shape {
                        ShapePath {
                            strokeColor: "white"; strokeWidth: 1.5; fillColor: "transparent"
                            strokeStyle: mk.modelData.kind === "res" ? ShapePath.SolidLine : ShapePath.DashLine
                            dashPattern: [4, 3]
                            PathPolyline { path: [Qt.point(0, 0), Qt.point(0, screen.height)] }
                        }
                    }
                    Label {
                        objectName: "bodeMarker" + mk.index
                        x: mk.x + width + 8 > screen.width ? -width - 4 : 4
                        y: 2 + mk.index * 16
                        text: (mk.modelData.kind === "res" ? qsTr("f₀ ") : qsTr("fg ")) + mk.modelData.text
                        color: "white"
                        font.pixelSize: Math.round(11 * Theme.fontScale); font.bold: true
                    }
                }
            }
            Label {
                visible: !panel.ok
                anchors.fill: parent
                anchors.margins: 10
                wrapMode: Text.Wrap
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: panel.statusText()
                color: "#E6EEF5"
                font.pixelSize: Math.round(13 * Theme.fontScale)
            }
        }
        // dB-Achse links (je 20 dB beschriftet)
        Repeater {
            model: panel.ok ? 5 : 0
            Label {
                required property int index
                width: plotArea.axisL - 6
                horizontalAlignment: Text.AlignRight
                y: screen.y + index * screen.height / 4 - height / 2
                text: Math.round(panel.bs.topDb - index * 20) + " dB"
                font.pixelSize: Math.round(10 * Theme.fontScale)
                color: "#4A5866"
            }
        }
        // Frequenzachse unten: Dekaden
        Repeater {
            model: ["1 Hz", "10 Hz", "100 Hz", "1 kHz", "10 kHz", "100 kHz", "1 MHz", "10 MHz"]
            Label {
                required property string modelData
                required property int index
                visible: !panel.narrow || index % 2 === 0 // schmal: jede zweite Dekade
                x: screen.x + index * screen.width / 7 - (index === 7 ? width : index === 0 ? 0 : width / 2)
                y: screen.y + screen.height + 2
                text: modelData
                font.pixelSize: Math.round(10 * Theme.fontScale)
                color: "#4A5866"
            }
        }
    }

    // ---------------------------------------------------------------- Kennwerte
    Column {
        id: side
        anchors { right: parent.right; rightMargin: 8; top: head.bottom; topMargin: 4 }
        width: Math.min(230, panel.width * 0.3)
        spacing: 3
        visible: panel.ok && !panel.narrow
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            text: panel.kindText(panel.bs.kind)
            font.bold: true
            font.pixelSize: Math.round(12 * Theme.fontScale)
            color: Theme.navy
        }
        Repeater {
            model: [
                {t: qsTr("Resonanz f₀: %1"), v: panel.bs.resonanceText},
                {t: qsTr("untere Grenzfrequenz: %1"), v: panel.bs.cutLowText},
                {t: qsTr("obere Grenzfrequenz: %1"), v: panel.bs.cutHighText},
                {t: qsTr("Bandbreite: %1"), v: panel.bs.bandwidthText},
                {t: qsTr("Güte: %1"), v: panel.bs.qualityText},
                {t: qsTr("höchste Verstärkung: %1"), v: panel.bs.maxDbText}
            ]
            Label {
                required property var modelData
                visible: !!modelData.v
                width: side.width
                wrapMode: Text.Wrap
                text: modelData.t.arg(modelData.v || "")
                font.pixelSize: Math.round(12 * Theme.fontScale)
                font.family: "Consolas"
                color: "#1E2B38"
            }
        }
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            text: qsTr("Grenzfrequenz: dort ist die Spannung 3 dB unter dem Höchstwert, also etwa 71 %.")
            font.pixelSize: Math.round(11 * Theme.fontScale)
            color: "#4A5866"
        }
    }
}
