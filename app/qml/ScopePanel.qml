// Oszilloskop ab Volta 2 (Konzept 5.2, 9.3), an der Stelle des Zeitschreibers unter der Arbeitsflaeche: zwei Kanaele
// an den Tastkoepfen K1 (gelb, durchgezogen) und K2 (tuerkis, gestrichelt), Bezug 0V - Farbe, Linienart und
// Beschriftung unterscheiden die Kanaele. Schirm 10 x 8 Teilungen; Zeitbasis, Volt je Teilung und Lage je Kanal,
// Trigger (Kanal, Flanke, Pegel), Rollbetrieb (der Zeitschreiber, ohne Trigger von rechts nach links), XY-Betrieb
// und Standbild. Spitze-Spitze, Frequenz und Periode stehen als Zahl in der Kanalfarbe daneben. Die Bedienung steht
// in Gruppen mit Rahmen (Zeit, K1, K2, Trigger), jede mit »Auto«, das das Signal einfaengt; »?« blendet eine kurze
// Hilfe ein. Es laeuft mit dem Hauptschalter; die Betriebslampe im Kopf leuchtet gruen, im Standbild gelb, aus dunkel.
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
    property bool showHelp: false // »?«: kurze Hilfe ueber den Bedienelementen
    readonly property var chans: sc.channels || []
    // Das Oszilloskop haengt am Hauptschalter (wie Generator und Wechselquelle): aus ist der Schirm dunkel
    readonly property bool powered: controller !== null && controller.mainSwitch

    function refresh() { panel.sc = panel.controller.scopeState() }
    Timer { // ca. 15 Bilder je Sekunde
        interval: 66
        running: panel.visible && panel.powered && !panel.frozen
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
        // 10 x 8 Teilungen; bei wenig Platz kleiner, damit neben ihm zwei Gruppen der Bedienung nebeneinander passen
        width: Math.max(160, Math.min((panel.height - 12) * 1.25, panel.width - 440))
        height: width / 1.25
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
    // Schirm aus (Hauptschalter aus): dunkel, ohne Raster und Kurven, mit Hinweis
    Rectangle {
        objectName: "scopeOff"
        visible: !panel.powered
        anchors.fill: screen
        radius: screen.radius
        color: "#070D0B"
        Label {
            anchors.fill: parent
            anchors.margins: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text: qsTr("Aus. Schalte den Hauptschalter ein.")
            color: "#8FA39A"
            font.pixelSize: Math.round(14 * Theme.fontScale)
            font.bold: true
        }
    }

    component Btn: PanelButton {
        height: Theme.control
        width: Math.max(Theme.control, implicitWidth)
    }
    // »Auto« einer Gruppe: faengt das Signal ein (Modell: scopeAutoChannel/-Timebase/-Trigger)
    component AutoBtn: Btn {
        required property string what
        text: qsTr("Auto")
        onClicked: panel.controller.setScope("auto", what)
    }
    readonly property bool anyProbe: powered && chans.some(c => c && c.has)
    component Caption: Label {
        height: Theme.control
        verticalAlignment: Text.AlignVCenter
        color: Theme.navy
        font.pixelSize: Math.round(12 * Theme.fontScale)
    }

    // Gruppe mit Rahmen und Ueberschrift (Zeit, K1, K2, Trigger): was zusammengehoert, steht zusammen
    component Group: Rectangle {
        id: grp
        property string title
        property color tag: "transparent" // Kanalfarbe als Streifen links (Name steht in der Ueberschrift)
        default property alias content: groupBody.data
        width: Math.max(groupBody.implicitWidth, groupTitle.implicitWidth) + 18
        height: groupBody.implicitHeight + groupTitle.height + 10
        radius: 8
        color: "#F7F9FB"
        border.color: "#B5C2CF"
        border.width: 1.5
        Rectangle { visible: grp.tag.a > 0; x: 2; y: 6; width: 6; height: parent.height - 12; radius: 3; color: grp.tag }
        Label {
            id: groupTitle
            x: 12; y: 3
            text: grp.title
            color: Theme.navy
            font.pixelSize: Math.round(12 * Theme.fontScale)
            font.bold: true
        }
        Column { id: groupBody; x: 10; y: groupTitle.y + groupTitle.height + 2; spacing: 2 }
    }

    // Kopf: Name, »?«, Schliessen
    Row {
        id: head
        anchors { left: screen.right; leftMargin: 10; right: parent.right; rightMargin: 4 }
        y: 4
        spacing: 4
        Item {
            width: 22; height: Theme.control
            PowerLed {
                objectName: "scopeLed"
                anchors.centerIn: parent
                on: panel.visible && panel.powered
                paused: panel.frozen
                accessibleName: !panel.powered ? qsTr("Oszilloskop aus")
                              : panel.frozen ? qsTr("Oszilloskop: Standbild") : qsTr("Oszilloskop zeichnet")
            }
        }
        Label {
            width: parent.width - 22 - helpButton.width - closeButton.width - 3 * parent.spacing
            height: Theme.control
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            text: !panel.powered ? qsTr("Oszilloskop – aus") : panel.frozen ? qsTr("Oszilloskop – Standbild") : qsTr("Oszilloskop")
            font.bold: true
            font.pixelSize: Math.round(15 * Theme.fontScale)
            color: Theme.navy
        }
        HelpToggle {
            id: helpButton
            objectName: "scopeHelpButton"
            checked: panel.showHelp
            onClicked: panel.showHelp = !panel.showHelp
        }
        PanelButton {
            id: closeButton
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 20 } }
            onClicked: panel.closeRequested()
            Accessible.name: qsTr("Oszilloskop schließen")
        }
    }

    Flickable {
        id: controlsFlick
        anchors { left: screen.right; leftMargin: 10; right: parent.right; rightMargin: 4; top: head.bottom; bottom: parent.bottom; bottomMargin: 4 }
        contentHeight: controlsColumn.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        Column {
            id: controlsColumn
            width: controlsFlick.width - 10
            spacing: 8
            HelpBox {
                objectName: "scopeHelp"
                visible: panel.showHelp
                width: parent.width
                lines: [qsTr("Das Oszilloskop zeichnet die Spannung an den Tastköpfen K1 und K2 über der Zeit. Bezug ist immer 0V. Es läuft mit dem Hauptschalter."),
                        qsTr("»Auto« fängt das Signal ein: bei K1 und K2 die Volt je Kästchen, bei Zeit die Dauer, beim Trigger den Pegel."),
                        qsTr("Zeit: »−« und »+« ändern, wie lange ein Kästchen waagrecht dauert. »Rollen« schreibt langsam von rechts nach links wie ein Zeitschreiber."),
                        qsTr("K1 und K2: »−« und »+« ändern, wie viel Volt ein Kästchen senkrecht ist. ▲ und ▼ schieben die Kurve."),
                        qsTr("Trigger: Er hält die Kurve still. Das Bild startet, wenn der Kanal den Pegel steigend ↗ oder fallend ↘ kreuzt."),
                        qsTr("XY zeichnet K2 über K1. Der Knopf mit den zwei Balken hält das Bild an.")]
            }
            Flow {
                id: controls
                width: parent.width
                spacing: 8
                // Zeit: Zeitbasis, Rollbetrieb, XY, Standbild
                Group {
                    objectName: "scopeGroupTime"
                    title: qsTr("Zeit")
                    Row {
                        spacing: 4
                        Btn {
                            objectName: "scopeFaster"
                            text: "−"; enabled: panel.sc.canFaster === true && !panel.sc.xy
                            onClicked: panel.controller.setScope("timebase", panel.sc.timebase - 1)
                            ToolTip.text: qsTr("Zeitbasis kürzer"); Accessible.name: ToolTip.text
                        }
                        PanelValue { objectName: "scopeTimebaseValue"; text: (panel.sc.timebaseText || "") + "/Div"; width: 90 }
                        Btn {
                            objectName: "scopeSlower"
                            text: "+"; enabled: panel.sc.canSlower === true && !panel.sc.xy
                            onClicked: panel.controller.setScope("timebase", panel.sc.timebase + 1)
                            ToolTip.text: qsTr("Zeitbasis länger"); Accessible.name: ToolTip.text
                        }
                        AutoBtn {
                            objectName: "scopeAutoTime"
                            what: "time"
                            enabled: panel.anyProbe && !panel.sc.xy && panel.sc.autoBusy !== true
                            text: panel.sc.autoBusy === true ? "…" : qsTr("Auto")
                            ToolTip.text: qsTr("Zeit einstellen, dass zwei bis fünf Schwingungen zu sehen sind")
                            Accessible.name: qsTr("Zeit automatisch")
                        }
                    }
                    Row {
                        spacing: 4
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
                            checkable: true; checked: panel.frozen
                            onClicked: panel.frozen = checked
                            // gezeichnet statt Schriftzeichen (das Pausezeichen erscheint sonst als buntes Emoji):
                            // laufend zwei Balken (anhalten), im Standbild ein Dreieck (weiterlaufen)
                            contentItem: Item {
                                Row {
                                    visible: !panel.frozen
                                    anchors.centerIn: parent
                                    spacing: 4
                                    Rectangle { width: 4; height: 14; radius: 1; color: Theme.navy }
                                    Rectangle { width: 4; height: 14; radius: 1; color: Theme.navy }
                                }
                                DrawnIcon { visible: panel.frozen; anchors.centerIn: parent; kind: "right"; color: "white"; size: 18 }
                            }
                            ToolTip.text: panel.frozen ? qsTr("Weiterlaufen") : qsTr("Standbild"); Accessible.name: ToolTip.text
                        }
                    }
                }
                // Kanaele: Volt je Teilung und Lage; Kanalfarbe als Streifen, Name und Linienart in der Ueberschrift
                Repeater {
                    model: 2
                    Group {
                        id: chGroup
                        required property int index
                        readonly property var ch: panel.chans[index] || ({})
                        objectName: "scopeGroupK" + (index + 1)
                        title: index === 0 ? qsTr("K1 (durchgezogen)") : qsTr("K2 (gestrichelt)")
                        tag: Theme.probeColors[index]
                        Row {
                            spacing: 4
                            Btn {
                                text: "−"; enabled: (chGroup.ch.volts || 0) > 0
                                onClicked: panel.controller.setScope("volts" + chGroup.index, chGroup.ch.volts - 1)
                                ToolTip.text: qsTr("K%1 empfindlicher").arg(chGroup.index + 1); Accessible.name: ToolTip.text
                            }
                            PanelValue { text: (chGroup.ch.voltsText || "") + "/Div"; width: 84 }
                            Btn {
                                text: "+"; enabled: (chGroup.ch.volts || 0) < 8
                                onClicked: panel.controller.setScope("volts" + chGroup.index, chGroup.ch.volts + 1)
                                ToolTip.text: qsTr("K%1 unempfindlicher").arg(chGroup.index + 1); Accessible.name: ToolTip.text
                            }
                        }
                        Row {
                            spacing: 4
                            Caption { text: qsTr("Lage"); width: 40; font.bold: true }
                            Btn {
                                text: "▲"; enabled: (chGroup.ch.position || 0) < 4
                                onClicked: panel.controller.setScope("position" + chGroup.index, chGroup.ch.position + 1)
                                ToolTip.text: qsTr("K%1 nach oben").arg(chGroup.index + 1); Accessible.name: ToolTip.text
                            }
                            Btn {
                                text: "▼"; enabled: (chGroup.ch.position || 0) > -4
                                onClicked: panel.controller.setScope("position" + chGroup.index, chGroup.ch.position - 1)
                                ToolTip.text: qsTr("K%1 nach unten").arg(chGroup.index + 1); Accessible.name: ToolTip.text
                            }
                            AutoBtn {
                                objectName: "scopeAutoK" + (chGroup.index + 1)
                                what: "k" + chGroup.index
                                enabled: panel.powered && chGroup.ch.has === true
                                ToolTip.text: qsTr("K%1 so einstellen, dass die ganze Kurve auf den Schirm passt").arg(chGroup.index + 1)
                                Accessible.name: qsTr("K%1 automatisch").arg(chGroup.index + 1)
                            }
                        }
                    }
                }
                // Trigger: Kanal, Flanke, Pegel
                Group {
                    objectName: "scopeGroupTrigger"
                    visible: !panel.sc.roll && !panel.sc.xy
                    title: qsTr("Trigger")
                    Row {
                        spacing: 4
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
                        AutoBtn {
                            objectName: "scopeAutoTrigger"
                            what: "trigger"
                            enabled: panel.anyProbe
                            ToolTip.text: qsTr("Trigger auf die Mitte des Signals stellen, steigende Flanke")
                            Accessible.name: qsTr("Trigger automatisch")
                        }
                    }
                    Row {
                        spacing: 4
                        Btn {
                            text: "−"
                            onClicked: panel.controller.setScope("triggerLevel", Math.round(((panel.sc.triggerLevel || 0) - 0.5) * 10) / 10)
                            ToolTip.text: qsTr("Triggerpegel tiefer"); Accessible.name: ToolTip.text
                        }
                        PanelValue { text: panel.sc.triggerText || ""; width: 66 }
                        Btn {
                            text: "+"
                            onClicked: panel.controller.setScope("triggerLevel", Math.round(((panel.sc.triggerLevel || 0) + 0.5) * 10) / 10)
                            ToolTip.text: qsTr("Triggerpegel höher"); Accessible.name: ToolTip.text
                        }
                    }
                }
            }
        }
    }

}
