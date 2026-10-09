// Werkzeug auf der Grundplatte (Variante B): Taschenlampe oder Foehn, frei verschiebbar (Bedienhandlung, auch im
// Betrieb). Antippen schaltet ein/aus, langer Druck bzw. Rechtsklick oeffnet das Menue (Wegraeumen). Der Schein
// zeigt die Reichweite: Je naeher ein Fotowiderstand bzw. Heissleiter liegt, desto mehr Licht bzw. Waerme bekommt er
// (berechnet im Modell, model/environment.hpp).
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: tool
    property string toolType: "taschenlampe"
    property bool on: true
    property real reach: 96 // Radius des Scheins in Bildpunkten der Arbeitsflaeche
    signal moved(point delta)
    signal toggled()
    signal menuRequested(point scenePos)

    width: Theme.control + 8
    height: Theme.control + 8
    readonly property bool flash: toolType === "taschenlampe"
    readonly property bool water: toolType === "wasser"

    transform: Translate {
        x: drag.active ? drag.activeTranslation.x : 0
        y: drag.active ? drag.activeTranslation.y : 0
    }

    // Schein (Licht gelb, warme Luft orange), nach aussen schwaecher: drei Ringe
    Repeater {
        model: tool.on ? [1.0, 0.66, 0.36] : []
        Rectangle {
            required property real modelData
            anchors.centerIn: parent
            width: tool.reach * 2 * modelData; height: width; radius: width / 2
            color: tool.flash ? "#FFF3A0" : tool.water ? "#5AA9E6" : "#FF8C3A"
            opacity: 0.15
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: tool.on ? (tool.flash ? "#FFE58A" : tool.water ? "#BFE0F7" : "#FFB27A") : "#C9CED4"
        border.color: Theme.navy
        border.width: 2
        Text {
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -5
            text: tool.flash ? "🔦" : tool.water ? "💧" : "♨"
            font.pixelSize: 24
            color: tool.flash ? "#2E3B4A" : "#B23A10"
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 4
            text: tool.on ? qsTr("an") : qsTr("aus")
            font.pixelSize: 10
            font.bold: true
            color: Theme.navy
        }
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.bottom
        text: tool.flash ? qsTr("Taschenlampe") : tool.water ? qsTr("Wassertropfen") : qsTr("Föhn")
        font.pixelSize: Math.round(11 * Theme.fontScale)
        font.bold: true
        color: Theme.navy
    }

    // activeTranslation ist beim Deaktivieren schon zurueckgesetzt: letzte Verschiebung selbst merken
    property point lastDelta
    DragHandler {
        id: drag
        target: null
        onActiveTranslationChanged: if (active) tool.lastDelta = activeTranslation
        onActiveChanged: if (!active) tool.moved(tool.lastDelta)
    }
    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        gesturePolicy: TapHandler.ReleaseWithinBounds
        onLongPressed: tool.menuRequested(point.scenePosition)
        onTapped: (eventPoint, button) => {
            if (button === Qt.RightButton) tool.menuRequested(eventPoint.scenePosition)
            else tool.toggled()
        }
    }
    Accessible.name: (flash ? qsTr("Taschenlampe") : water ? qsTr("Wassertropfen") : qsTr("Föhn")) + ", " + (on ? qsTr("an") : qsTr("aus"))
}
