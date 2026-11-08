// Senderfenster (ab Volta 3, Konzept 5.3): drei Mittelwellen- und drei UKW-Sender der Senderanlage an der Antenne
// ANT, je Sender Schalter, Frequenz und Inhalt (Morsezeichen, Zeitzeichen, Volta-Melodie und das gemeinfreie Lied
// »O Holy Night«). Alles Bedienhandlungen (kein Befehl); der Zustand steht als Text neben dem Schalter (Farbe nie allein). Nicht modal und verschiebbar wie das
// Generatorfenster.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: panel
    property var controller: null
    property bool showHelp: false

    // index = Schaltzustand im Pult (0-2 Mittelwelle, 3-5 UKW, wie desk.stationsOn)
    readonly property var mwStations: [
        {index: 0, khz: 600, content: qsTr("Morsezeichen")},
        {index: 1, khz: 1000, content: qsTr("»O Holy Night« wie 1906")},
        {index: 2, khz: 1400, content: qsTr("Zeitzeichen")}
    ]
    readonly property var fmStations: [
        {index: 3, mhz: 89.3, content: qsTr("Zeitzeichen")},
        {index: 4, mhz: 96.8, content: qsTr("Volta-Melodie")},
        {index: 5, mhz: 104.5, content: qsTr("Morsezeichen")}
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
            lines: [qsTr("Die Senderanlage schickt drei Sender auf Mittelwelle und drei auf UKW an das Feld ANT. Verbinde ANT mit deiner Antenne."),
                    qsTr("Jeder Sender hat seine eigene Frequenz. Dein Schwingkreis sucht sich einen davon aus."),
                    qsTr("Schalte Sender aus, wenn du hören willst, ob dein Empfänger einen einzelnen Sender findet.")]
        }

        Flickable {
            objectName: "stationList"
            width: parent.width
            height: Math.min(contentHeight, panel.parent ? panel.parent.height - 2 * Theme.control - 64 : contentHeight)
            contentHeight: list.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            Column {
                id: list
                width: parent.width
                spacing: 8
                BandLabel { text: qsTr("Mittelwelle (MW)") }
                Repeater {
                    model: panel.mwStations
                    StationRow { band: qsTr("Sender %1").arg(modelData.index + 1); freq: qsTr("%1 kHz").arg(modelData.khz) }
                }
                BandLabel { text: qsTr("UKW") }
                Repeater {
                    model: panel.fmStations
                    StationRow {
                        band: qsTr("UKW %1").arg(modelData.index - 2)
                        freq: qsTr("%1 MHz").arg(Number(modelData.mhz).toLocaleString(Qt.locale(), "f", 1))
                    }
                }
            }
        }
    }

    component BandLabel: Label {
        color: Theme.navy
        font.bold: true
        font.pixelSize: Math.round(14 * Theme.fontScale)
        topPadding: 2
    }

    component StationRow: Rectangle {
        id: row
        required property var modelData
        property string band
        property string freq
        readonly property int station: modelData.index
        readonly property bool on: { panel.controller ? panel.controller.cells : 0; return panel.controller ? panel.controller.stationOn(station) : false }
        width: list.width
        height: Theme.control + 8
        radius: 8
        color: on ? "#EEF4FB" : "#F3F1EA"
        border.color: on ? "#2873B0" : "#B8B4A8"
        border.width: 1
        Row {
            anchors.verticalCenter: parent.verticalCenter
            x: 10
            spacing: 10
            Switch {
                objectName: "stationSwitch" + row.station
                anchors.verticalCenter: parent.verticalCenter
                checked: row.on
                onToggled: panel.controller.setStationOn(row.station, checked)
                Accessible.name: qsTr("%1 ein oder aus").arg(row.band)
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                Label {
                    text: row.band + " · " + row.freq
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
