// Kontaktfeld (Grundplatte, Pultleiste, Messgeraet, Modulleiste): ein Knoten mit fuenf Steckplaetzen.
// Von einem Feld aus wird eine Leitung gezogen; langer Druck/Rechtsklick oeffnet das Feldmenue.
import QtQuick

Rectangle {
    id: root
    property string anchorName: ""
    property string label: anchorName
    property bool showVoltage: false
    property int bars: 0
    property int sockets: 5 // Steckplaetze (Modulleiste: schmale Felder mit einem Steckplatz)
    property int pitch: Theme.socketPitch // Lochabstand (Lab.socketPoint rechnet mit demselben Wert)
    property bool highlighted: false
    property bool target: false // Zielfeld der Schritt-fuer-Schritt-Aufbauhilfe
    property int recorderChannel: -1 // Zeitschreiber: Kanal 0/1 haengt an diesem Feld

    signal wireDragUpdated(point scenePos)
    signal wireDragFinished(point scenePos)
    signal menuRequested(point scenePos)

    radius: 6
    color: Theme.field
    border.color: highlighted ? Theme.signalOrange : Theme.fieldOutline
    border.width: highlighted ? 4 : 1

    // Steckloecher mittig im Feld (waagrecht und senkrecht); Bauteilbeine und Leitungen stecken genau hier
    Repeater {
        model: root.sockets
        Rectangle {
            required property int index
            width: Theme.socketSize
            height: width
            radius: 1 // quadratische Loecher der Steckfedern
            x: root.width / 2 + (index - (root.sockets - 1) / 2) * root.pitch - width / 2
            y: root.height / 2 - height / 2
            color: "#6E6553"
            border.color: "#4F4839"
            border.width: 1
        }
    }

    Text {
        anchors.top: parent.top
        anchors.topMargin: 3
        anchors.horizontalCenter: parent.horizontalCenter
        text: root.label
        font.pixelSize: Math.round((13) * Theme.fontScale)
        font.bold: true
        color: Theme.navy
    }

    // Zielfeld: blauer, pulsierender Rahmen und Pfeil (bei reduzierter Bewegung ruhig)
    Rectangle {
        visible: root.target
        anchors.fill: parent
        anchors.margins: -5
        radius: root.radius + 4
        color: "transparent"
        border.color: "#1E7BE0"
        border.width: 4
        SequentialAnimation on opacity {
            running: root.target && !Theme.reduceMotion
            loops: Animation.Infinite
            NumberAnimation { from: 1; to: 0.25; duration: 550 }
            NumberAnimation { from: 0.25; to: 1; duration: 550 }
        }
    }
    Text {
        visible: root.target
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.top
        anchors.bottomMargin: 2
        text: "▼"
        color: "#1E7BE0"
        font.pixelSize: 14
    }

    // Zeitschreiber-Schild (Text und Farbe)
    Rectangle {
        visible: root.recorderChannel >= 0
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        width: 22; height: 14; radius: 4
        color: root.recorderChannel === 0 ? "#D32F2F" : "#1E5FC2"
        Text { anchors.centerIn: parent; text: "K" + (root.recorderChannel + 1); color: "white"; font.pixelSize: 9; font.bold: true }
    }

    VoltageBars {
        visible: root.showVoltage
        bars: root.bars
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2 // unter der Lochreihe
    }

    property point lastScenePos
    DragHandler {
        id: wireDrag
        target: null
        onCentroidChanged: if (active) { root.lastScenePos = centroid.scenePosition; root.wireDragUpdated(root.lastScenePos) }
        onActiveChanged: if (!active) root.wireDragFinished(root.lastScenePos)
    }
    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onLongPressed: root.menuRequested(point.scenePosition)
        onTapped: (eventPoint, button) => { if (button === Qt.RightButton) root.menuRequested(eventPoint.scenePosition) }
    }

    // Bildschirmleser: Feldname, bei eingeschalteter Anzeige auch die Spannungsstufe
    Accessible.role: Accessible.Button
    Accessible.name: qsTr("Feld %1").arg(root.label) + (root.showVoltage ? ", " + qsTr("Spannungsanzeige %1 von 4").arg(root.bars) : "")
}
