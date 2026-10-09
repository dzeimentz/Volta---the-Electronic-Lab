// Automatisch ausklappende Werkzeugleiste (wie eine Taskleiste mit automatischem Ausblenden): eingeklappt nur ein
// kleiner Griff mit Kurzsymbolen, ausgeklappt die Werkzeuge. Ausklappen: Maus darueber oder antippen; Einklappen von
// selbst nach dem Verlassen (Maus) bzw. einige Sekunden nach der letzten Beruehrung. Solange busy (z. B. ein Werkzeug
// wird herausgezogen), bleibt sie offen. Erweiterbar: Inhalt als Kinder (Default-Eigenschaft).
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: bar
    default property alias content: row.data
    property string handleText: "☰" // Kurzsymbole auf dem Griff (z. B. Zustand der Werkzeuge)
    property string title: ""
    property bool busy: false
    property bool expanded: false
    property int collapseDelay: hover.hovered ? 900 : 4000

    width: expanded ? panel.width : handle.width
    height: expanded ? panel.height : handle.height
    Behavior on width { enabled: !Theme.reduceMotion; NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

    function poke() { // Interaktion: offen halten
        expanded = true
        collapseTimer.restart()
    }
    onBusyChanged: if (busy) poke(); else collapseTimer.restart()

    Timer {
        id: collapseTimer
        interval: bar.collapseDelay
        onTriggered: if (!bar.busy && !hover.hovered) bar.expanded = false
                     else if (!bar.busy) restart()
    }
    HoverHandler {
        id: hover
        onHoveredChanged: if (hovered) bar.poke(); else collapseTimer.restart()
    }

    // Griff (eingeklappt)
    Rectangle {
        id: handle
        objectName: "flyoutHandle"
        visible: !bar.expanded
        width: Math.max(Theme.control + 24, handleLabel.implicitWidth + 24)
        height: Theme.control
        radius: 10
        color: "#E6F0F6FB"
        border.color: Theme.fieldOutline
        Label {
            id: handleLabel
            anchors.centerIn: parent
            text: bar.handleText + "  ▴"
            font.pixelSize: 16
            color: Theme.navy
        }
        TapHandler { onTapped: bar.poke() }
        Accessible.name: bar.title
        Accessible.role: Accessible.Button
    }

    // Leiste (ausgeklappt)
    Rectangle {
        id: panel
        visible: bar.expanded
        width: row.implicitWidth + 16
        height: row.implicitHeight + 12
        radius: 10
        color: "#EEF6FBFF"
        border.color: Theme.fieldOutline
        Row {
            id: row
            x: 8; y: 6
            spacing: 8
        }
        TapHandler { // jede Beruehrung haelt die Leiste offen
            gesturePolicy: TapHandler.DragThreshold
            grabPermissions: PointerHandler.ApprovesTakeOverByAnything
            onTapped: bar.poke()
            onPressedChanged: if (pressed) bar.poke()
        }
    }
}
