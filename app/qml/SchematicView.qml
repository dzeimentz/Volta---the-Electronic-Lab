// Schaltplanansicht (Konzept 5.7, ab Stufe 2 parallel zum Aufbau): automatisch aus dem Aufbau (model/schematic_layout),
// Normsymbole (SchematicSymbol), Knoten mit Feldnamen. Wechselseitige Hervorhebung: das ausgewaehlte Bauteil ist hier
// markiert; Antippen eines Bauteils waehlt es auf der Grundplatte, Antippen eines Knotens hebt seine Felder hervor.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Rectangle {
    id: view
    required property var controller
    property string selectedPart: ""
    property var highlightedNode: -1
    signal partSelected(string id)
    signal nodeSelected(var anchors)

    readonly property var plan: controller.schematic
    readonly property real margin: 56
    color: "white"
    border.color: Theme.fieldOutline

    function nodePoint(i) {
        const n = plan.nodes[i]
        return Qt.point(margin + n.x * (width - 2 * margin), margin + n.y * (height - 2 * margin))
    }

    Label {
        x: 10; y: 6
        text: qsTr("Schaltplan")
        font.pixelSize: Math.round(14 * Theme.fontScale)
        font.bold: true
        color: Theme.navy
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

    // Elemente
    Repeater {
        model: view.plan.elements
        Item {
            id: el
            required property var modelData
            anchors.fill: parent
            readonly property bool selected: modelData.id === view.selectedPart
            readonly property color ink: selected ? Theme.signalOrange : Theme.navy
            readonly property var pts: modelData.nodes.map(i => view.nodePoint(i))
            // Zweipol: Symbol auf der Verbindung (parallele versetzt); Mehrpol: Symbol im Schwerpunkt
            readonly property bool twoPole: pts.length === 2 || modelData.type === "potentiometer"
            readonly property point a: pts[0]
            readonly property point b: modelData.type === "potentiometer" ? pts[2] : pts[pts.length - 1]
            readonly property real len: Math.max(1, Math.hypot(b.x - a.x, b.y - a.y))
            readonly property point dir: Qt.point((b.x - a.x) / len, (b.y - a.y) / len)
            readonly property real offset: (modelData.par - (modelData.parCount - 1) / 2) * 56
            // Batterie wie im Lehrbuch als eigener Zweig links aussen, rechtwinklig angeschlossen
            readonly property bool sideBranch: modelData.id === "PULT.B"
            readonly property real branchX: view.margin * 0.6
            readonly property point center: {
                if (sideBranch) return Qt.point(branchX, (a.y + b.y) / 2)
                if (twoPole) return Qt.point((a.x + b.x) / 2 - dir.y * offset, (a.y + b.y) / 2 + dir.x * offset)
                let x = 0, y = 0
                for (let i = 0; i < pts.length; ++i) { x += pts[i].x; y += pts[i].y }
                return Qt.point(x / pts.length + 40, y / pts.length)
            }
            readonly property point leadA: sideBranch ? Qt.point(branchX, center.y - 32)
                                         : twoPole ? Qt.point(center.x - dir.x * 32, center.y - dir.y * 32) : center
            readonly property point leadB: sideBranch ? Qt.point(branchX, center.y + 32)
                                         : twoPole ? Qt.point(center.x + dir.x * 32, center.y + dir.y * 32) : center

            Shape {
                anchors.fill: parent
                preferredRendererType: Shape.CurveRenderer
                ShapePath {
                    strokeColor: el.ink
                    strokeWidth: 2
                    fillColor: "transparent"
                    PathSvg {
                        path: {
                            let p = el.sideBranch
                                ? "M" + el.a.x + " " + el.a.y + " H" + el.branchX + " V" + el.leadA.y
                                  + " M" + el.leadB.x + " " + el.leadB.y + " V" + el.b.y + " H" + el.b.x
                                : "M" + el.a.x + " " + el.a.y + " L" + el.leadA.x + " " + el.leadA.y
                                  + " M" + el.leadB.x + " " + el.leadB.y + " L" + el.b.x + " " + el.b.y
                            // Mehrpol bzw. Schleifer des Potentiometers: Linie vom Symbol zu jedem weiteren Knoten
                            for (let i = 0; i < el.pts.length; ++i) {
                                if (el.twoPole && el.modelData.type !== "potentiometer") break
                                if (el.modelData.type === "potentiometer" && i !== 1) continue
                                p += " M" + el.center.x + " " + (el.center.y + (el.modelData.type === "potentiometer" ? 16 : 0))
                                   + " L" + el.pts[i].x + " " + el.pts[i].y
                            }
                            return p
                        }
                    }
                }
            }
            SchematicSymbol {
                type: el.modelData.type
                stroke: el.ink
                width: 64; height: 32
                x: el.center.x - 32
                y: el.center.y - 16
                rotation: el.sideBranch ? 90 : el.twoPole ? Math.atan2(el.dir.y, el.dir.x) * 180 / Math.PI : 0
            }
            Label {
                x: el.center.x + 20
                y: el.center.y - 26
                text: el.modelData.desk ? "" : el.modelData.id
                font.pixelSize: Math.round(11 * Theme.fontScale)
                color: el.ink
            }
            // Antippen waehlt das Bauteil auf der Grundplatte (Fangbereich 48 dp)
            Item {
                objectName: "schem_" + el.modelData.id
                x: el.center.x - 24; y: el.center.y - 24
                width: Theme.control; height: Theme.control
                TapHandler { enabled: !el.modelData.desk; onTapped: view.partSelected(el.modelData.id) }
            }
        }
    }

    // Knoten mit Feldnamen
    Repeater {
        model: view.plan.nodes
        Item {
            id: nd
            required property var modelData
            required property int index
            readonly property point p: view.nodePoint(index)
            Rectangle {
                x: nd.p.x - 5; y: nd.p.y - 5
                width: 10; height: 10; radius: 5
                color: view.highlightedNode === nd.index ? Theme.signalOrange : Theme.navy
            }
            Label {
                x: nd.p.x + 8; y: nd.p.y - height - 2
                text: nd.modelData.label
                font.pixelSize: Math.round(11 * Theme.fontScale)
                font.bold: view.highlightedNode === nd.index
                color: view.highlightedNode === nd.index ? "#B04A00" : "#3E5266"
            }
            Item {
                objectName: "node_" + nd.index
                x: nd.p.x - 24; y: nd.p.y - 24
                width: Theme.control; height: Theme.control
                TapHandler {
                    onTapped: {
                        view.highlightedNode = view.highlightedNode === nd.index ? -1 : nd.index
                        view.nodeSelected(view.highlightedNode >= 0 ? nd.modelData.anchors : [])
                    }
                }
            }
        }
    }
}
