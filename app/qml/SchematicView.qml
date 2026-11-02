// Schaltplanansicht (Konzept 5.7, ab Stufe 2 parallel zum Aufbau): automatisch aus dem Aufbau, angeordnet nach
// DIN EN 61082-1 (model/schematic_layout: Knoten als waagrechte Leitungen, Bauteile senkrecht in Spalten, nur rechte
// Winkel, Verbindungspunkte an Abzweigungen), Schaltzeichen nach DIN EN 60617 (SchematicSymbol).
// Wechselseitige Hervorhebung: das ausgewaehlte Bauteil ist hier markiert; Antippen eines Bauteils waehlt es auf der
// Grundplatte, Antippen einer Knotenleitung hebt ihre Felder hervor. Feldnamen an den Knoten sind abschaltbar.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Rectangle {
    id: view
    required property var controller
    property string selectedPart: ""
    property var highlightedNode: -1
    property bool showFieldNames: true
    // ab Volta 2: der Plan passt ganz in die Spalte und laesst sich mit +/- in Schritten von 10 % vergroessern
    // oder verkleinern (100 % = passend); Volta 1 behaelt seine feste Untergrenze 0,75
    readonly property bool zoomable: controller.band !== "" && controller.band !== "volta1"
    property int zoomPercent: 100
    // Breite der Spalte bei 100 % (Lab.qml): beim Verkleinern wird auch die Spalte schmaler, der Plan bleibt aber
    // im selben Massstab wie bei voller Breite (sonst schrumpfte er doppelt)
    property real fullWidth: width
    // schmaler als Titel und Knoepfe wird die Spalte nicht
    readonly property real headerWidth: title.width + zoomRow.width + fieldNamesButton.width + 30
    signal partSelected(string id)
    signal nodeSelected(var anchors)

    readonly property var plan: controller.schematic
    color: "white"
    border.color: Theme.fieldOutline

    // Kennzeichnung neben dem Schaltzeichen: Kennung und Wert (Pultelemente mit ihrem Pultnamen)
    function elementLabel(e) {
        if (e.desk) {
            switch (e.id) {
            case "PULT.B": return qsTr("%1 V").arg(Number(view.controller.batteryVolt).toLocaleString(Qt.locale(), "f", view.controller.batteryVolt % 1 === 0 ? 0 : 1))
            case "PULT.S": return "S"
            case "PULT.P1": return "P1"
            case "PULT.P2": return "P2"
            case "PULT.LS": return "LS"
            case "PULT.B2": return "B2\n" + qsTr("%1 V").arg(Number(view.controller.cells2 * 1.5).toLocaleString(Qt.locale(), "f", view.controller.cells2 % 2 === 0 ? 0 : 1))
            case "PULT.W": return "W\n" + qsTr("%1 V").arg(view.controller.acRms) + "~"
            case "PULT.G": return "G"
            case "PULT.KS": return "KS"
            }
            return ""
        }
        // Messstellen und Tastkoepfe heissen wie im Messgeraet: V1, A2, Ω3, K1 (intern UM1, IM2, RM3, TK1)
        const m = /^(UM|IM|RM|TK)(\d+)$/.exec(e.id)
        if (m) return ({UM: "V", IM: "A", RM: "Ω", TK: "K"})[m[1]] + m[2]
        if (e.type === "widerstand" && e.value > 0) return e.id + "\n" + Texts.ohm(e.value)
        if (e.type === "gluehlampe" && e.value > 0)
            return e.id + "\n" + qsTr("%1 V").arg(Number(e.value).toLocaleString(Qt.locale(), "f", e.value % 1 === 0 ? 0 : 1))
        return e.id
    }

    Label {
        id: title
        x: 10; y: 6
        text: qsTr("Schaltplan")
        font.pixelSize: Math.round(14 * Theme.fontScale)
        font.bold: true
        color: Theme.navy
    }
    // Feldnamen an den Knoten ein/aus (aufgeraeumter Plan)
    PanelButton {
        id: fieldNamesButton
        objectName: "schematicFieldNames"
        anchors.right: parent.right
        anchors.rightMargin: 4
        y: 4
        height: Theme.control
        checkable: true
        checked: view.showFieldNames
        onClicked: view.showFieldNames = !view.showFieldNames
        text: qsTr("Feldnamen")
        Accessible.name: view.showFieldNames ? qsTr("Feldnamen ausblenden") : qsTr("Feldnamen zeigen")
        visible: view.zoomable
    }
    // Volta 1 (eingefroren): Feldnamen-Knopf ohne Rahmen wie bisher
    ToolButton {
        objectName: "schematicFieldNamesV1"
        visible: !view.zoomable
        anchors.right: parent.right
        anchors.rightMargin: 4
        y: 0
        height: Theme.control
        checkable: true
        checked: view.showFieldNames
        onClicked: view.showFieldNames = !view.showFieldNames
        text: qsTr("Feldnamen")
        font.pixelSize: Math.round(12 * Theme.fontScale)
        Accessible.name: view.showFieldNames ? qsTr("Feldnamen ausblenden") : qsTr("Feldnamen zeigen")
    }
    // Groesse des Plans: - und + in Schritten von 10 %, dazwischen die Stufe (Antippen: wieder passend)
    Row {
        id: zoomRow
        visible: view.zoomable
        anchors.right: fieldNamesButton.left
        anchors.rightMargin: 6
        y: 4
        spacing: 4
        PanelButton {
            objectName: "schematicZoomOut"
            width: Theme.control; height: Theme.control
            text: "−"
            font.pixelSize: Math.round(18 * Theme.fontScale)
            font.bold: true
            enabled: view.zoomPercent > 50
            onClicked: view.zoomPercent -= 10
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Schaltplan verkleinern")
            Accessible.name: ToolTip.text
        }
        PanelButton {
            objectName: "schematicZoomFit"
            height: Theme.control
            width: Math.max(Theme.control, implicitWidth)
            text: qsTr("%1 %").arg(view.zoomPercent)
            onClicked: view.zoomPercent = 100
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Schaltplan einpassen")
            Accessible.name: ToolTip.text
        }
        PanelButton {
            objectName: "schematicZoomIn"
            width: Theme.control; height: Theme.control
            text: "+"
            font.pixelSize: Math.round(18 * Theme.fontScale)
            font.bold: true
            enabled: view.zoomPercent < 300
            onClicked: view.zoomPercent += 10
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Schaltplan vergrößern")
            Accessible.name: ToolTip.text
        }
    }
    Label {
        anchors.centerIn: parent
        visible: view.plan.nodes.length === 0
        width: parent.width - 40
        wrapMode: Text.Wrap
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Sobald du Bauteile steckst und Leitungen ziehst, entsteht hier der Schaltplan.")
        color: "#6E6A60"
        font.pixelSize: Math.round(14 * Theme.fontScale)
    }

    Flickable {
        id: flick
        anchors { left: parent.left; right: parent.right; top: title.bottom; bottom: parent.bottom; margins: 8 }
        clip: true
        contentWidth: sheet.width * sheet.scale
        contentHeight: sheet.height * sheet.scale
        boundsBehavior: Flickable.StopAtBounds

        // Zeichenblatt in Bildpunkten des Modells; passt sich der Breite an (nicht groesser als 1,3-fach)
        Item {
            id: sheet
            width: view.plan.width || 0
            height: view.plan.height || 0
            transformOrigin: Item.TopLeft
            // passend, hoechstens 1,3-fach; Volta 1 mindestens 0,75-fach (lesbar), ab Volta 2 ganz sichtbar und mit
            // +/- einstellbar; passt der Plan nicht, laesst er sich verschieben
            readonly property real fit: width > 0
                ? Math.min(1.3, (view.zoomable ? view.fullWidth - 16 : flick.width) / width, flick.height / height) : 1
            scale: view.zoomable ? fit * view.zoomPercent / 100 : Math.max(0.75, fit)

            // Knotenleitungen (je Knoten ein Pfad; hervorgehoben beim Antippen)
            Repeater {
                model: view.plan.nodes
                Item {
                    id: nd
                    required property var modelData
                    required property int index
                    readonly property bool hot: view.highlightedNode === index
                    Shape {
                        preferredRendererType: Shape.CurveRenderer
                        ShapePath {
                            strokeColor: nd.hot ? Theme.signalOrange : Theme.navy
                            strokeWidth: nd.hot ? 3.5 : 2
                            fillColor: "transparent"
                            capStyle: ShapePath.SquareCap
                            PathSvg { path: nd.modelData.path || "M0 0" }
                        }
                    }
                    Label {
                        visible: view.showFieldNames && nd.modelData.drawn
                        x: nd.modelData.x - 2
                        y: nd.modelData.y - height - 1
                        text: nd.modelData.label
                        font.pixelSize: 10
                        font.bold: nd.hot
                        color: nd.hot ? "#B04A00" : "#6A7B8C"
                    }
                    // Fangstreifen auf der Knotenleitung (mindestens 48 breit)
                    Item {
                        objectName: "node_" + nd.index
                        visible: nd.modelData.drawn
                        x: Math.min(nd.modelData.x, (nd.modelData.x + nd.modelData.x2) / 2 - 24)
                        y: nd.modelData.y - 10
                        width: Math.max(48, nd.modelData.x2 - nd.modelData.x)
                        height: 20
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Knoten %1 auf der Grundplatte zeigen").arg(nd.index + 1)
                        TapHandler {
                            onTapped: {
                                view.highlightedNode = view.highlightedNode === nd.index ? -1 : nd.index
                                view.nodeSelected(view.highlightedNode >= 0 ? nd.modelData.anchors : [])
                            }
                        }
                    }
                }
            }

            // Verbindungspunkte an Abzweigungen
            Repeater {
                model: view.plan.dots
                Rectangle {
                    required property var modelData
                    x: modelData.x - 4.5; y: modelData.y - 4.5
                    width: 9; height: 9; radius: 4.5
                    color: view.highlightedNode === modelData.node ? Theme.signalOrange : Theme.navy
                }
            }

            // Schaltzeichen und Bloecke
            Repeater {
                model: view.plan.elements
                Item {
                    id: el
                    required property var modelData
                    readonly property bool selected: modelData.id === view.selectedPart
                    readonly property color ink: selected ? Theme.signalOrange : Theme.navy

                    SchematicSymbol {
                        visible: !el.modelData.block
                        type: el.modelData.type
                        stroke: el.ink
                        lineWidth: el.selected ? 3 : 2
                        width: 64; height: 32
                        x: el.modelData.x - 32
                        y: el.modelData.y - 16
                        rotation: el.modelData.rotation
                    }
                    // Block (Modul, Messgeraet): Rechteck mit Namen und Anschlussnamen innen
                    Rectangle {
                        visible: el.modelData.block
                        x: el.modelData.x - el.modelData.w / 2
                        y: el.modelData.y - el.modelData.h / 2
                        width: el.modelData.w
                        height: el.modelData.h
                        color: "white"
                        border.color: el.ink
                        border.width: el.selected ? 3 : 2
                        Label {
                            anchors.centerIn: parent
                            text: el.modelData.module ? Texts.moduleName(el.modelData.type)
                                  : el.modelData.type === "messgeraet" ? qsTr("Messgerät") : Texts.partName(el.modelData.type)
                            font.pixelSize: 10
                            font.bold: true
                            color: el.ink
                        }
                    }
                    Repeater {
                        model: el.modelData.pinLabels
                        Label {
                            required property var modelData
                            x: modelData.x - width / 2
                            y: modelData.y - height / 2
                            text: modelData.text
                            font.pixelSize: 9
                            color: "#3E5266"
                        }
                    }
                    Label {
                        x: el.modelData.labelX
                        y: el.modelData.labelY - height / 2
                        text: view.elementLabel(el.modelData)
                        font.pixelSize: 11
                        lineHeight: 0.95
                        color: el.ink
                    }
                    // Antippen waehlt das Bauteil auf der Grundplatte (Fangbereich 48)
                    Item {
                        objectName: "schem_" + el.modelData.id
                        x: el.modelData.x - Math.max(24, (el.modelData.block ? el.modelData.w : 32) / 2)
                        y: el.modelData.y - Math.max(24, (el.modelData.block ? el.modelData.h : 32) / 2)
                        width: Math.max(48, el.modelData.block ? el.modelData.w : 32)
                        height: Math.max(48, el.modelData.block ? el.modelData.h : 32)
                        TapHandler { enabled: !el.modelData.desk; onTapped: view.partSelected(el.modelData.id) }
                    }
                }
            }
        }
    }
}
