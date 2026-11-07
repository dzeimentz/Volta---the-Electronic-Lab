// Stellfenster des Trimmers (ab Volta 2): stufenlose Stellung des Schleifers wie beim Pult-Drehregler, dazu die
// beiden Teilwiderstaende als Zahl. Ab Volta 3 auch fuer den Drehkondensator (Kapazitaet 10 ... 500 pF als Zahl).
// Bedienhandlung, kein Befehl.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: panel
    property var controller: null
    property string partId: ""
    property string partType: ""
    property real position: 0.5
    readonly property real ohm: partType === "trimmer_100k" ? 100000 : 10000
    readonly property bool variableCap: partType === "drehkondensator"
    readonly property int picofarad: Math.round(10 + position * 490) // wie der Katalog (c_min_F, c_max_F)

    function openFor(id, type, pos) {
        partId = id
        partType = type
        position = pos
        open()
    }

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - 32 : 400, 400)
    padding: 16
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    contentItem: Column {
        spacing: 10
        Row {
            width: parent.width
            Label {
                width: parent.width - closeButton.width
                anchors.verticalCenter: parent.verticalCenter
                text: Texts.partName(panel.partType) + " " + panel.partId
                color: Theme.navy
                font.pixelSize: Math.round(18 * Theme.fontScale)
                font.bold: true
            }
            ToolButton {
                id: closeButton
                width: Theme.control; height: Theme.control
                contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 18 } }
                onClicked: panel.close()
                Accessible.name: qsTr("Schließen")
            }
        }
        Slider {
            objectName: "trimmerSlider"
            width: parent.width
            height: Theme.control
            from: 0; to: 1; stepSize: 0.01
            value: panel.position
            onMoved: {
                panel.position = value
                panel.controller.setPartPosition(panel.partId, value)
            }
            Accessible.name: panel.variableCap ? qsTr("Stellung des Drehkondensators") : qsTr("Stellung des Trimmers")
        }
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            color: Theme.navy
            text: panel.variableCap ? qsTr("Kapazität: %1 pF").arg(panel.picofarad) : qsTr("a bis s: %1 · s bis b: %2").arg(Texts.ohm(Math.round(panel.ohm * panel.position)))
                                                   .arg(Texts.ohm(Math.round(panel.ohm * (1 - panel.position))))
        }
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            color: "#4A5560"
            text: panel.variableCap ? qsTr("Dreh die Platten weiter ineinander: Die Kapazität wird größer. Dein Schwingkreis schwingt dann langsamer.")
                                    : qsTr("Ein Trimmer wird einmal eingestellt und bleibt dann so – in echt mit einem kleinen Schraubendreher.")
        }
    }
}
