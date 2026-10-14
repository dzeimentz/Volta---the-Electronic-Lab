import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Zuse 1 (Prototyp Z0): Zetti erzaehlt links, in der Mitte Raetselkisten (entdecken) oder Werkbank (bauen),
// rechts die Tabelle mit Pruefen bzw. »Diese Kiste passt«. Oben: Stufen, Zahlen, Werkzeugkiste, Rueckgaengig.
// Unter der Mitte (zuschaltbar): Schalter und LEDs als Binaer-, Dezimal- und Hexadezimalzahl.
ApplicationWindow {
    id: win
    width: 1280
    height: 800
    minimumWidth: 1024
    minimumHeight: 640
    visible: true
    color: ZTheme.graphite
    title: qsTr("Zuse 1 – Computertechnik für Einsteiger (Prototyp)")

    readonly property bool building: workbench.stageKind === "bauen"
    readonly property bool hasNext: workbench.stageIndex + 1 < workbench.stages.length

    Connections {
        target: workbench
        function onStageChanged() { bench.clearSelection() }
    }

    function removeSelection() {
        if (bench.selectedPart !== "") workbench.removePart(bench.selectedPart)
        else if (bench.selectedWire >= 0) workbench.removeWire(bench.selectedWire)
        bench.clearSelection()
    }
    function next() { workbench.selectStage(workbench.stageIndex + 1) }

    Shortcut { sequence: StandardKey.Undo; onActivated: workbench.undo() }
    Shortcut { sequences: [StandardKey.Redo, "Ctrl+Y"]; onActivated: workbench.redo() }
    Shortcut { sequences: [StandardKey.Delete, "Backspace"]; onActivated: win.removeSelection() }
    Shortcut { sequence: "Escape"; onActivated: workbench.inside ? workbench.leaveTo(workbench.path.length - 2) : bench.clearSelection() }

    header: Rectangle {
        height: 72
        color: ZTheme.surface
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: ZTheme.line }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12
            Image {
                source: "qrc:/zuse/zuse1_64.png"
                sourceSize: Qt.size(64, 64)
                Layout.preferredWidth: 48; Layout.preferredHeight: 48
            }
            Text {
                text: "Zuse 1"
                color: ZTheme.navy
                font.pixelSize: 24; font.bold: true
            }
            Item { Layout.preferredWidth: 12 }
            Repeater { // Stufen als Nummern: geschafft mit Haken, gesperrt blass
                model: workbench.stages
                delegate: Rectangle {
                    id: dot
                    required property var modelData
                    readonly property bool current: modelData.index === workbench.stageIndex
                    width: 48; height: 48; radius: 24
                    color: current ? ZTheme.navy : modelData.done ? ZTheme.okBg : ZTheme.surface
                    border.width: 2
                    border.color: current ? ZTheme.navy : modelData.done ? ZTheme.ok : ZTheme.line
                    opacity: modelData.unlocked ? 1 : 0.45
                    Text {
                        anchors.centerIn: parent
                        text: dot.modelData.done && !dot.current ? "✓" : String(dot.modelData.index)
                        color: dot.current ? ZTheme.textOnAccent : dot.modelData.done ? ZTheme.ok : ZTheme.text
                        font.pixelSize: 20; font.bold: true
                    }
                    HoverHandler { id: dotHover }
                    ToolTip.visible: dotHover.hovered
                    ToolTip.text: modelData.title + (modelData.unlocked ? "" : " – " + qsTr("noch gesperrt"))
                    TapHandler { enabled: dot.modelData.unlocked; onTapped: workbench.selectStage(dot.modelData.index) }
                }
            }
            Item { Layout.fillWidth: true }
            ZButton { // Zahlenleiste ein/aus
                text: "0b 0x"
                primary: workbench.numbersVisible
                onClicked: workbench.numbersVisible = !workbench.numbersVisible
                Accessible.name: qsTr("Zahlensysteme anzeigen")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Zahlen binär, dezimal und hexadezimal anzeigen")
            }
            ZButton {
                text: qsTr("Werkzeugkiste") + " (" + workbench.tools.length + ")"
                onClicked: toolbox.open()
            }
            ZButton {
                visible: win.building
                text: "↶"
                font.family: "Segoe UI Symbol"
                font.pixelSize: 24
                enabled: workbench.canUndo && !workbench.inside
                onClicked: workbench.undo()
                Accessible.name: qsTr("Rückgängig")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Rückgängig")
            }
            ZButton {
                visible: win.building
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

        StoryPanel {
            Layout.preferredWidth: 300
            Layout.fillWidth: false
            Layout.fillHeight: true
        }

        // Mitte: Raetselkisten oder Werkbank mit Werkzeugen, darunter die Zahlenleiste
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            DiscoverStage {
                visible: !win.building
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            RowLayout { // Pfad beim Blick hinein
                visible: win.building
                spacing: 6
                Repeater {
                    model: workbench.path
                    delegate: RowLayout {
                        required property string modelData
                        required property int index
                        spacing: 6
                        Text { visible: index > 0; text: "›"; color: ZTheme.muted; font.pixelSize: 22 }
                        ZButton {
                            text: modelData
                            primary: index === workbench.path.length - 1
                            onClicked: workbench.leaveTo(index)
                        }
                    }
                }
                Text {
                    Layout.leftMargin: 8
                    text: workbench.inside ? qsTr("Zurück: auf den Pfad tippen")
                                           : qsTr("Doppelt tippen: in eine ★-Erfindung hineinschauen")
                    color: ZTheme.muted; font.pixelSize: 14
                }
            }
            RowLayout {
                visible: win.building
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10
                ColumnLayout { // Werkzeuge der Stufe: antippen setzt, ziehen legt ab
                    Layout.preferredWidth: 120
                    Layout.fillWidth: false
                    Layout.fillHeight: true
                    spacing: 8
                    Repeater {
                        model: workbench.palette
                        delegate: Rectangle {
                            id: tile
                            required property var modelData
                            Layout.fillWidth: true
                            Layout.preferredHeight: 96
                            radius: 14
                            color: modelData.own ? "#FFF7E6" : "#ECFAFD"
                            border.color: modelData.own ? ZTheme.pad : ZTheme.accentDim
                            border.width: 2
                            opacity: workbench.inside ? 0.5 : 1
                            GateShape {
                                anchors { horizontalCenter: parent.horizontalCenter; top: parent.top; topMargin: 6 }
                                width: 70; height: 56
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
                            TapHandler { enabled: !workbench.inside; onTapped: bench.placePart(tile.modelData.type) }
                            DragHandler {
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
                Workbench {
                    id: bench
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            NumberStrip {
                visible: workbench.numbersVisible
                Layout.fillWidth: true
                onExplain: numbers.open()
            }
        }

        // Rechts: Tabelle und Aktion
        Rectangle {
            Layout.preferredWidth: 280
            Layout.fillWidth: false
            Layout.fillHeight: true
            radius: 18
            color: ZTheme.surface
            border.color: ZTheme.line
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10
                Text {
                    text: win.building ? qsTr("Wahrheitstabelle") : qsTr("Was passiert?")
                    color: ZTheme.navy
                    font.pixelSize: 20; font.bold: true
                }
                TruthPanel { Layout.fillWidth: true }
                Item { Layout.fillHeight: true }
                ZButton { // entdecken
                    visible: !win.building && !workbench.stageDone
                    Layout.fillWidth: true
                    text: qsTr("Diese Kiste passt!")
                    primary: workbench.boxIndex >= 0
                    onClicked: workbench.confirmBox()
                }
                ZButton { // bauen
                    visible: win.building && !workbench.stageDone
                    Layout.fillWidth: true
                    text: qsTr("Prüfen")
                    primary: !workbench.chipReady
                    enabled: !workbench.inside
                    onClicked: workbench.check()
                }
                ZButton {
                    visible: win.building && !workbench.stageDone
                    Layout.fillWidth: true
                    text: workbench.stageMakesChip ? qsTr("Erfindung einpacken") : qsTr("Geschafft!")
                    primary: workbench.chipReady
                    enabled: workbench.chipReady
                    onClicked: workbench.createChip()
                }
                ZButton {
                    visible: workbench.stageDone && win.hasNext
                    Layout.fillWidth: true
                    text: qsTr("Weiter") + " →"
                    primary: true
                    onClicked: win.next()
                }
            }
        }
    }

    ToolboxPopup { id: toolbox; objectName: "toolbox" }
    NumbersPopup { id: numbers; objectName: "numbers" }

    Rectangle { // Werkzeug, das gerade auf die Werkbank gezogen wird
        id: ghost
        property string symbol
        visible: false
        z: 100
        width: 120; height: 112
        radius: ZTheme.radius
        color: ZTheme.surface
        border.color: ZTheme.accent; border.width: 2
        opacity: 0.85
        Text {
            anchors.centerIn: parent
            text: ghost.symbol; color: ZTheme.text
            font.family: ZTheme.mono; font.pixelSize: 30; font.bold: true
        }
    }
}
