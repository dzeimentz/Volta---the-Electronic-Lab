// Eigene Fensterleiste der Windows-Ausgabe (Variante C): Verlauf blau -> hellblau -> blau von oben nach unten,
// Schaltplanmuster in Dunkelgrau (tools/make_titlebar.py), Symbol und Titel, Knoepfe Minimieren, Maximieren,
// Schliessen. Verschieben und Andocken uebernimmt das System (startSystemMove), Doppelklick maximiert.
// Main.qml blendet sie im Vollbild (Praesentation) aus und legt die Raender zum Groessenaendern um das Fenster.
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: bar
    required property var window
    readonly property bool maximized: window.visibility === Window.Maximized

    objectName: "titleBar"
    height: 40
    gradient: Gradient {
        GradientStop { position: 0.0; color: "#1A3A5C" }
        GradientStop { position: 0.5; color: "#4A8CCC" }
        GradientStop { position: 1.0; color: "#1A3A5C" }
    }

    // Schaltplanmuster (drei Perioden: Kondensator, Diode, Widerstand), doppelte Aufloesung
    Image {
        anchors.fill: parent
        source: bar.visible ? "qrc:/brand/titlebar_pattern.png" : "" // nur Windows-Ausgabe (Ressource fehlt mobil)
        fillMode: Image.Tile
        sourceSize: Qt.size(345, 40)
        horizontalAlignment: Image.AlignLeft
        verticalAlignment: Image.AlignTop
        smooth: true
    }

    // Ziehen verschiebt das Fenster (System: Andocken an Raender, Win + Pfeil), Doppelklick maximiert
    DragHandler {
        target: null
        grabPermissions: PointerHandler.CanTakeOverFromAnything
        onActiveChanged: if (active) bar.window.startSystemMove()
    }
    TapHandler {
        onDoubleTapped: bar.maximized ? bar.window.showNormal() : bar.window.showMaximized()
    }

    Row {
        x: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10
        Image {
            anchors.verticalCenter: parent.verticalCenter
            width: 22; height: 22
            source: "qrc:/icons/volta_64.png"
            sourceSize: Qt.size(44, 44)
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            text: bar.window.title
            color: "white"
            font.pixelSize: 15
            font.bold: true
            style: Text.Raised
            styleColor: "#0E2238"
        }
    }

    // Fensterknoepfe, gezeichnet (Schriftzeichen fehlen auf manchen Systemen)
    component WindowButton: AbstractButton {
        id: wb
        property string kind: "min"
        property bool danger: false
        width: 46
        height: bar.height
        hoverEnabled: true
        background: Rectangle {
            color: wb.danger ? (wb.down ? "#9B1C12" : (wb.hovered ? "#C42B1C" : "transparent"))
                             : (wb.down ? "#40FFFFFF" : (wb.hovered ? "#26FFFFFF" : "transparent"))
        }
        contentItem: Item {
            // Minimieren: Strich
            Rectangle {
                visible: wb.kind === "min"
                anchors.centerIn: parent
                width: 11; height: 1.5
                color: "white"
            }
            // Maximieren: Quadrat; wiederherstellen: zwei versetzte Quadrate
            Rectangle {
                visible: wb.kind === "max" || wb.kind === "restore"
                x: (parent.width - width) / 2 + (wb.kind === "restore" ? -1.5 : 0)
                y: (parent.height - height) / 2 + (wb.kind === "restore" ? 1.5 : 0)
                width: wb.kind === "restore" ? 9 : 10
                height: width
                color: "transparent"
                border.color: "white"
                border.width: 1.2
            }
            Rectangle {
                visible: wb.kind === "restore"
                x: (parent.width - 9) / 2 + 1.5
                y: (parent.height - 9) / 2 - 1.5
                width: 9; height: 9
                color: "transparent"
                border.color: "white"
                border.width: 1.2
            }
            // Schliessen: Kreuz
            Repeater {
                model: wb.kind === "close" ? [45, -45] : []
                Rectangle {
                    required property int modelData
                    anchors.centerIn: parent
                    width: 14; height: 1.4
                    color: "white"
                    rotation: modelData
                    antialiasing: true
                }
            }
        }
        Accessible.role: Accessible.Button
    }

    Row {
        anchors.right: parent.right
        anchors.top: parent.top
        WindowButton {
            objectName: "titleMinimize"
            kind: "min"
            onClicked: bar.window.showMinimized()
            Accessible.name: qsTr("Minimieren")
        }
        WindowButton {
            objectName: "titleMaximize"
            kind: bar.maximized ? "restore" : "max"
            onClicked: bar.maximized ? bar.window.showNormal() : bar.window.showMaximized()
            Accessible.name: bar.maximized ? qsTr("Verkleinern") : qsTr("Maximieren")
        }
        WindowButton {
            objectName: "titleClose"
            kind: "close"
            danger: true
            onClicked: bar.window.close()
            Accessible.name: qsTr("Schließen")
        }
    }
}
