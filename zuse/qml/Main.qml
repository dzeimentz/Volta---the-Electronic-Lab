import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Zuse-Prototyp Z0: Werkbank mit Stufen (Bauteilfach links, Werkbank Mitte, Aufgabe und Pruefung rechts)
ApplicationWindow {
    id: win
    width: 1280
    height: 800
    minimumWidth: 960
    minimumHeight: 600
    visible: true
    color: ZTheme.graphite
    title: qsTr("Zuse 1 – Werkbank (Prototyp)")

    property bool hintVisible: false
    Connections {
        target: workbench
        function onStageChanged() { win.hintVisible = false; bench.clearSelection() }
    }

    function removeSelection() {
        if (bench.selectedPart !== "") workbench.removePart(bench.selectedPart)
        else if (bench.selectedWire >= 0) workbench.removeWire(bench.selectedWire)
        bench.clearSelection()
    }

    Shortcut { sequence: StandardKey.Undo; onActivated: workbench.undo() }
    Shortcut { sequences: [StandardKey.Redo, "Ctrl+Y"]; onActivated: workbench.redo() }
    Shortcut { sequences: [StandardKey.Delete, "Backspace"]; onActivated: win.removeSelection() }
    Shortcut { sequence: "Escape"; onActivated: workbench.inside ? workbench.leaveTo(workbench.path.length - 2) : bench.clearSelection() }

    header: Rectangle {
        height: 68
        color: ZTheme.surface
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 10
            Image { // Symbol Zuse 1 (zuse/resources/icons/zuse1.svg)
                source: "qrc:/zuse/zuse1_64.png"
                sourceSize: Qt.size(64, 64)
                Layout.preferredWidth: 48
                Layout.preferredHeight: 48
                smooth: true
            }
            Text {
                text: "// " + qsTr("Werkbank")
                color: ZTheme.text
                font.family: ZTheme.mono; font.pixelSize: 20
            }
            Item { Layout.preferredWidth: 12 }
            Repeater { // Stufen
                model: workbench.stages
                delegate: ZButton {
                    required property var modelData
                    text: (modelData.index + 1).toString().padStart(2, "0") + " · " + modelData.kuerzel
                          + (modelData.done ? " ✓" : "")
                    enabled: modelData.unlocked
                    primary: modelData.index === workbench.stageIndex
                    onClicked: workbench.selectStage(modelData.index)
                    ToolTip.visible: hovered
                    ToolTip.text: modelData.title + (modelData.unlocked ? "" : " – " + qsTr("noch gesperrt"))
                }
            }
            Item { Layout.fillWidth: true }
            ZButton {
                text: "↶"
                font.family: "Segoe UI Symbol"
                font.pixelSize: 24
                enabled: workbench.canUndo && !workbench.inside
                onClicked: workbench.undo()
                Accessible.name: qsTr("Rückgängig")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Rückgängig")
            }
            ZButton {
                text: "↷"
                font.family: "Segoe UI Symbol"
                font.pixelSize: 24
                enabled: workbench.canRedo && !workbench.inside
                onClicked: workbench.redo()
                Accessible.name: qsTr("Wiederholen")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Wiederholen")
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // Bauteilfach: antippen setzt den Baustein, ziehen legt ihn an die Stelle
        ColumnLayout {
            Layout.preferredWidth: 150
            Layout.fillWidth: false
            Layout.fillHeight: true
            spacing: 10
            Text {
                text: "// " + qsTr("Bausteine")
                color: ZTheme.muted; font.family: ZTheme.mono; font.pixelSize: 16
            }
            Repeater {
                model: workbench.palette
                delegate: Rectangle {
                    id: tile
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 84
                    radius: ZTheme.radius
                    color: ZTheme.surface
                    border.color: modelData.own ? ZTheme.accentDim : ZTheme.line
                    border.width: 2
                    opacity: workbench.inside ? 0.5 : 1
                    Text {
                        anchors { left: parent.left; leftMargin: 16; verticalCenter: parent.verticalCenter }
                        text: tile.modelData.symbol + (tile.modelData.inverted ? "○" : "")
                        color: ZTheme.text; font.family: ZTheme.mono; font.pixelSize: 30; font.bold: true
                    }
                    Text {
                        anchors { right: parent.right; rightMargin: 12; verticalCenter: parent.verticalCenter }
                        text: (tile.modelData.own ? "★ " : "") + tile.modelData.type
                        color: tile.modelData.own ? ZTheme.accent : ZTheme.muted
                        font.family: ZTheme.mono; font.pixelSize: 14
                    }
                    TapHandler { enabled: !workbench.inside; onTapped: bench.placePart(tile.modelData.type) }
                    DragHandler {
                        id: paletteDrag
                        target: null
                        enabled: !workbench.inside
                        onActiveChanged: {
                            if (active) {
                                ghost.symbol = tile.modelData.symbol
                                ghost.visible = true
                            } else {
                                ghost.visible = false
                                bench.dropPart(tile.modelData.type, centroid.scenePosition.x, centroid.scenePosition.y)
                            }
                        }
                        onCentroidChanged: if (active) {
                            ghost.x = centroid.scenePosition.x - ghost.width / 2
                            ghost.y = centroid.scenePosition.y - ghost.height / 2
                        }
                    }
                }
            }
            Item { Layout.fillHeight: true }
            ZButton {
                Layout.fillWidth: true
                text: qsTr("Entfernen")
                enabled: !workbench.inside && (bench.selectedPart !== "" || bench.selectedWire >= 0)
                onClicked: win.removeSelection()
            }
            ZButton {
                Layout.fillWidth: true
                text: qsTr("Leeren")
                enabled: !workbench.inside && workbench.parts.length > 0
                onClicked: workbench.clearBench()
            }
        }

        // Werkbank mit Pfadanzeige (Blick hinein)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            RowLayout {
                spacing: 6
                Repeater {
                    model: workbench.path
                    delegate: RowLayout {
                        required property string modelData
                        required property int index
                        spacing: 6
                        Text {
                            visible: index > 0
                            text: "›"; color: ZTheme.muted; font.pixelSize: 22
                        }
                        ZButton {
                            text: modelData
                            primary: index === workbench.path.length - 1
                            onClicked: workbench.leaveTo(index)
                        }
                    }
                }
                Text {
                    Layout.leftMargin: 8
                    text: workbench.inside ? qsTr("Zurück: auf den Pfad tippen") : qsTr("Doppelt tippen: Blick in einen ★-Chip")
                    color: ZTheme.muted; font.family: ZTheme.mono; font.pixelSize: 14
                }
            }
            Workbench {
                id: bench
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        // Aufgabe, Wahrheitstabelle, Pruefen, Chip erstellen
        Rectangle {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            radius: 14
            color: ZTheme.surface
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                Text {
                    Layout.fillWidth: true
                    text: workbench.stageTitle
                    color: ZTheme.text; font.pixelSize: 22; font.bold: true
                    wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    text: workbench.stageQuestion
                    color: ZTheme.accent; font.pixelSize: 17
                    wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    visible: win.hintVisible
                    text: workbench.stageHint
                    color: ZTheme.text; font.pixelSize: 15
                    wrapMode: Text.WordWrap
                }
                RowLayout {
                    ZButton { text: win.hintVisible ? qsTr("Tipp aus") : qsTr("Tipp"); onClicked: win.hintVisible = !win.hintVisible }
                    ZButton { text: qsTr("Lösung"); enabled: !workbench.inside; onClicked: workbench.showSolution() }
                }
                Text {
                    text: "// " + qsTr("Wahrheitstabelle")
                    color: ZTheme.muted; font.family: ZTheme.mono; font.pixelSize: 15
                }
                TruthPanel {
                    Layout.fillWidth: true
                    inputNames: workbench.stageInputs
                    outputNames: workbench.stageOutputs
                }
                Item { Layout.fillHeight: true }
                Text {
                    Layout.fillWidth: true
                    text: workbench.message
                    color: workbench.checkState === "failed" ? ZTheme.bad : ZTheme.text
                    font.pixelSize: 15
                    wrapMode: Text.WordWrap
                }
                ZButton {
                    Layout.fillWidth: true
                    text: qsTr("Prüfen")
                    primary: !workbench.chipReady
                    enabled: !workbench.inside
                    onClicked: workbench.check()
                }
                ZButton {
                    Layout.fillWidth: true
                    text: workbench.stageMakesChip ? "★ " + qsTr("Chip erstellen") : qsTr("Geschafft – weiter")
                    primary: workbench.chipReady
                    enabled: workbench.chipReady
                    onClicked: workbench.createChip()
                }
            }
        }
    }

    Rectangle { // Baustein, der gerade aus dem Fach gezogen wird
        id: ghost
        property string symbol
        visible: false
        z: 100
        width: 120; height: 112
        radius: ZTheme.radius
        color: ZTheme.surfaceHigh
        border.color: ZTheme.accent; border.width: 2
        opacity: 0.85
        Text {
            anchors.centerIn: parent
            text: ghost.symbol; color: ZTheme.text
            font.family: ZTheme.mono; font.pixelSize: 30; font.bold: true
        }
    }
}
