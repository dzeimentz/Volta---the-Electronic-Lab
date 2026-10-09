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
            }
            return ""
        }
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
    ToolButton {
        objectName: "schematicFieldNames"
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
            // mindestens 0,75-fach (lesbar); passt der Plan dann nicht, laesst er sich verschieben
            scale: width > 0 ? Math.max(0.75, Math.min(1.3, flick.width / width, flick.height / height)) : 1

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
                            text: el.modelData.module ? Texts.moduleName(el.modelData.type) : qsTr("Messgerät")
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
