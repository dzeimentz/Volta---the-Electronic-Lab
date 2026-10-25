import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Bibliothek links wie Volta (Bauteilfach): beim Bauen die Werkzeuge der Stufe (antippen setzt, ziehen legt ab),
// beim Entdecken die Raetselkisten zum Einsetzen. Vor der Vermutung bleiben die Kisten zu (mit Hinweis, nicht nur blass).
Rectangle {
    id: root
    required property Item bench   // Werkbank (placePart, dropPart, Auswahl)
    required property Item ghost   // Vorschau beim Ziehen
    readonly property bool building: workbench.stageKind === "bauen"
    readonly property bool discovering: workbench.stageKind === "entdecken"
    color: ZTheme.library

    signal removeSelection()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Text {
            Layout.fillWidth: true
            text: root.discovering ? qsTr("Rätselkisten") : qsTr("Werkzeuge")
            color: ZTheme.textOnShell
            font.pixelSize: 18; font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }
        Rectangle { Layout.fillWidth: true; height: 3; radius: 1.5; color: ZTheme.accent }

        // ---- Bauen: Werkzeuge der Stufe
        Repeater {
            model: root.building ? workbench.palette : []
            delegate: Rectangle {
                id: tile
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredHeight: 92
                radius: 12
                color: modelData.own ? "#FFF7E6" : "#ECFAFD"
                border.color: modelData.own ? ZTheme.pad : ZTheme.accentDim
                border.width: 2
                opacity: workbench.inside ? 0.5 : 1
                GateShape {
                    anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 6 }
                    width: 70; height: 54
                    symbol: tile.modelData.symbol
                    inverted: tile.modelData.inverted
                    us: workbench.symbolStyle === "us"
                    stroke: ZTheme.navy
                    fontSize: tile.modelData.symbol.length > 2 ? 16 : 22
                }
                Text {
                    anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 6 }
                    text: (tile.modelData.own ? "★ " : "") + tile.modelData.name
                    color: ZTheme.navy
                    font.pixelSize: 14; font.bold: true
                }
                TapHandler { enabled: !workbench.inside; onTapped: root.bench.placePart(tile.modelData.type) }
                DragHandler {
                    target: null
                    enabled: !workbench.inside
                    onActiveChanged: {
                        if (active) {
                            root.ghost.symbol = tile.modelData.symbol
                            root.ghost.visible = true
                        } else {
                            root.ghost.visible = false
                            root.bench.dropPart(tile.modelData.type, centroid.scenePosition.x, centroid.scenePosition.y)
                        }
                    }
                    onCentroidChanged: if (active) {
                        root.ghost.x = centroid.scenePosition.x - root.ghost.width / 2
                        root.ghost.y = centroid.scenePosition.y - root.ghost.height / 2
                    }
                }
            }
        }

        Text { // Bedienfeld fest verdrahtet: keine Werkzeuge noetig
            visible: workbench.benchLocked
            Layout.fillWidth: true
            text: qsTr("Hier brauchst du keine Werkzeuge – nur die Schalter am Bedienfeld.")
            color: ZTheme.textOnShellMuted
            font.pixelSize: 14
            wrapMode: Text.WordWrap
        }

        // ---- Entdecken: Raetselkisten
        Text {
            visible: root.discovering && workbench.guessing
            Layout.fillWidth: true
            text: "🔒 " + qsTr("Erst vermuten – dann gehen die Kisten auf.")
            color: ZTheme.textOnShellMuted
            font.pixelSize: 14
            wrapMode: Text.WordWrap
        }
        Repeater {
            model: root.discovering ? workbench.boxes : []
            delegate: Rectangle {
                id: card
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredHeight: 104
                radius: 14
                color: modelData.selected ? "#FFF7E6" : ZTheme.surface
                border.width: modelData.selected ? 4 : 2
                border.color: modelData.selected ? ZTheme.pad : ZTheme.line
                opacity: workbench.guessing ? 0.55 : 1
                Column {
                    anchors.centerIn: parent
                    spacing: 2
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: card.modelData.revealed ? card.modelData.name : "?"
                        color: card.modelData.revealed ? ZTheme.accentText : ZTheme.padDark
                        font.pixelSize: card.modelData.revealed ? 26 : 44; font.bold: true
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: qsTr("Kiste %1").arg(card.modelData.index + 1)
                        color: ZTheme.muted
                        font.pixelSize: 15
                    }
                }
                Text { // eingesetzt: nicht nur ueber die Farbe
                    visible: card.modelData.selected
                    anchors { top: parent.top; right: parent.right; margins: 8 }
                    text: "✓"
                    color: ZTheme.padDark
                    font.pixelSize: 20; font.bold: true
                }
                TapHandler { onTapped: workbench.selectBox(card.modelData.index) }
            }
        }

        Item { Layout.fillHeight: true }

        HeaderButton {
            visible: root.building
            Layout.fillWidth: true
            text: qsTr("Entfernen")
            enabled: !workbench.inside && (root.bench.selectedPart !== "" || root.bench.selectedWire >= 0)
            onClicked: root.removeSelection()
        }
        HeaderButton {
            visible: root.building
            Layout.fillWidth: true
            text: qsTr("Leeren")
            enabled: !workbench.inside && workbench.parts.length > 0
            onClicked: workbench.clearBench()
        }
    }
}
