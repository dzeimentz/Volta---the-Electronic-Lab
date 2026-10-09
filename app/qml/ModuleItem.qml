// Modul in der Modulleiste (docs/domain-model.md, Modulleiste): Traegerplatte unter den Feldern, die es belegt, mit
// Name, Zustandsanzeige und Knopf »Blick hinein«. Die Anschluesse sind die Felder der Leiste darueber (Leitungen
// stecken dort). Ziehen am Namensschild verschiebt das Modul; langer Druck oder Rechtsklick oeffnet das Modulmenue.
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: mod
    required property var controller
    property string moduleId
    property string moduleType
    property int slotCount: 4
    property bool selected: false
    property var moduleState: ({})
    readonly property int bodyHeight: Theme.control + 8
    readonly property bool active: moduleState.active === true
    readonly property color plate: moduleType === "tonmodul" ? "#6A4C8C" : "#2F6F8F"

    signal tapped()
    signal insideRequested()
    signal menuRequested(point scenePos)
    signal dragFinished(point delta)

    // Feldbreite der Leiste: slotCount Felder samt Abstaenden, plus Rand
    width: slotCount * (Theme.slotWidth + Theme.slotGap) - Theme.slotGap + 12
    height: Theme.fieldHeight + 6 + bodyHeight

    Connections {
        target: mod.controller
        function onStateChanged() { mod.moduleState = mod.controller.moduleState(mod.moduleId) }
    }

    // Verschieben: das Modul folgt dem Finger, das Modell entscheidet beim Loslassen
    transform: Translate { x: bodyDrag.active ? bodyDrag.translation.x : 0 }

    Rectangle {
        anchors.fill: parent
        radius: 8
        color: mod.plate
        border.color: mod.selected ? Theme.signalOrange : "#1E2B38"
        border.width: mod.selected ? 4 : 1
        opacity: bodyDrag.active ? 0.85 : 1
    }

    // Namensschild mit Zustand und »Blick hinein«
    Item {
        id: body
        objectName: "moduleBody_" + mod.moduleId
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom }
        height: mod.bodyHeight

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            // Zustand: Blinkmodul Ausgangslampe, Tonmodul kleiner Leuchtring in der Farbe der Tonhoehe
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 22; height: 22; radius: 11
                color: !mod.active ? "#3A3F46"
                     : mod.moduleType === "tonmodul" ? toneColor(mod.moduleState.frequencyHz || 0) : "#FFC83D"
                border.color: "white"
                border.width: 2
                function toneColor(f) { // wie SpeakerRing: logarithmisch, 100 Hz gelb ueber Orange bis 5 kHz rot
                    const t = Math.max(0, Math.min(1, Math.log(Math.max(f, 1) / 100) / Math.log(50)))
                    const yellow = Qt.color("#FFD400"), orange = Qt.color("#FF8C00"), red = Qt.color("#E8221B")
                    const mix = (a, b, u) => Qt.rgba(a.r + (b.r - a.r) * u, a.g + (b.g - a.g) * u, a.b + (b.b - a.b) * u, 1)
                    return t < 0.5 ? mix(yellow, orange, t * 2) : mix(orange, red, (t - 0.5) * 2)
                }
                Accessible.name: mod.active ? qsTr("Ausgang an") : qsTr("Ausgang aus")
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                Label {
                    text: Texts.moduleName(mod.moduleType)
                    color: "white"
                    font.pixelSize: Math.round(12 * Theme.fontScale)
                    font.bold: true
                }
                Label {
                    // Text zusaetzlich zur Lampe (Farbe nie allein)
                    text: mod.moduleId + "  " + (mod.moduleType === "tonmodul"
                                                 ? (mod.active ? "♪" : "–")
                                                 : (mod.active ? qsTr("an") : qsTr("aus")))
                    color: "#E6ECF2"
                    font.pixelSize: Math.round(11 * Theme.fontScale)
                }
            }
        }

        ToolButton {
            objectName: "moduleInside_" + mod.moduleId
            anchors.right: parent.right
            anchors.rightMargin: 2
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            text: "🔍"
            font.pixelSize: 18
            onClicked: mod.insideRequested()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Blick hinein")
            Accessible.name: qsTr("Blick hinein")
            background: Rectangle { radius: 6; color: parent.down ? "#40FFFFFF" : (parent.hovered ? "#26FFFFFF" : "transparent") }
        }

        DragHandler {
            id: bodyDrag
            target: null
            yAxis.enabled: false
            onActiveChanged: if (!active) mod.dragFinished(Qt.point(translation.x, 0))
        }
        TapHandler {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onLongPressed: mod.menuRequested(point.scenePosition)
            onTapped: (eventPoint, button) => {
                if (button === Qt.RightButton) mod.menuRequested(eventPoint.scenePosition)
                else mod.tapped()
            }
        }
    }
}
