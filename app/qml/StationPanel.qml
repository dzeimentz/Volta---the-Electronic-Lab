// Senderfenster (ab Volta 3, Konzept 5.3): drei Mittelwellensender der Senderanlage an der Antenne ANT, je Sender
// Schalter, Frequenz und Inhalt (eigene Inhalte: Morsezeichen, Melodie, Zeitzeichen). Alles Bedienhandlungen (kein
// Befehl); der Zustand steht als Text neben dem Schalter (Farbe nie allein). Nicht modal und verschiebbar wie das
// Generatorfenster.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: panel
    property var controller: null
    property bool showHelp: false

    readonly property var stations: [
        {khz: 600, content: qsTr("Morsezeichen")},
        {khz: 1000, content: qsTr("Melodie")},
        {khz: 1400, content: qsTr("Zeitzeichen")}
    ]

    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape
    property bool moved: false
    property point movedTo: Qt.point(0, 0)
    readonly property real homeX: parent ? (parent.width - width) / 2 : 0
    readonly property real homeY: parent ? Math.max(0, (parent.height - height) / 2) : 0
    function clampX(v) { return parent ? Math.max(0, Math.min(parent.width - width, v)) : v }
    function clampY(v) { return parent ? Math.max(0, Math.min(parent.height - Theme.control - 2 * padding, v)) : v }
    x: moved ? clampX(movedTo.x) : homeX
    y: moved ? clampY(movedTo.y) : homeY
    width: Math.min(parent ? parent.width - 32 : 440, 440)
    padding: 16
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    contentItem: Column {
        spacing: 10
        Row {
            width: parent.width
            spacing: 4
            Label {
                objectName: "stationTitle"
                width: parent.width - helpButton.width - closeButton.width - 2 * parent.spacing
                height: Theme.control
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Senderanlage")
                color: Theme.navy
                font.pixelSize: Math.round(18 * Theme.fontScale)
                font.bold: true
                HoverHandler { cursorShape: Qt.SizeAllCursor }
                DragHandler {
                    target: null
                    property point start
                    onActiveChanged: if (active) { start = Qt.point(panel.x, panel.y); panel.movedTo = start; panel.moved = true }
                    onCentroidChanged: if (active) panel.movedTo = Qt.point(
                        panel.clampX(start.x + centroid.scenePosition.x - centroid.scenePressPosition.x),
                        panel.clampY(start.y + centroid.scenePosition.y - centroid.scenePressPosition.y))
                }
            }
            HelpToggle {
                id: helpButton
                objectName: "stationHelpButton"
                checked: panel.showHelp
                onClicked: panel.showHelp = !panel.showHelp
            }
            PanelButton {
                id: closeButton
                width: Theme.control; height: Theme.control
                contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 18 } }
                onClicked: panel.close()
                Accessible.name: qsTr("Schließen")
            }
        }

        HelpBox {
            objectName: "stationHelp"
            visible: panel.showHelp
            width: parent.width
            lines: [qsTr("Die Senderanlage schickt drei Sender auf Mittelwelle an das Feld ANT. Verbinde ANT mit deiner Antenne."),
                    qsTr("Jeder Sender hat seine eigene Frequenz. Dein Schwingkreis sucht sich einen davon aus."),
                    qsTr("Schalte Sender aus, wenn du hören willst, ob dein Empfänger einen einzelnen Sender findet.")]
        }

        Repeater {
            model: panel.stations
            Rectangle {
                id: row
                required property var modelData
                required property int index
                readonly property bool on: { panel.controller ? panel.controller.cells : 0; return panel.controller ? panel.controller.stationOn(index) : false }
                width: parent.width
                height: Theme.control + 12
                radius: 8
                color: on ? "#EEF4FB" : "#F3F1EA"
                border.color: on ? "#2873B0" : "#B8B4A8"
                border.width: 1
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 10
                    spacing: 10
                    Switch {
                        objectName: "stationSwitch" + row.index
                        anchors.verticalCenter: parent.verticalCenter
                        checked: row.on
                        onToggled: panel.controller.setStationOn(row.index, checked)
                        Accessible.name: qsTr("Sender %1 ein oder aus").arg(row.index + 1)
                    }
                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        Label {
                            text: qsTr("Sender %1 · %2 kHz").arg(row.index + 1).arg(row.modelData.khz)
                            color: Theme.navy
                            font.bold: true
                            font.pixelSize: Math.round(15 * Theme.fontScale)
                        }
                        Label {
                            text: row.modelData.content + " · " + (row.on ? qsTr("sendet") : qsTr("aus"))
                            color: "#3A4652"
                            font.pixelSize: Math.round(13 * Theme.fontScale)
                        }
                    }
                }
            }
        }
    }
}
