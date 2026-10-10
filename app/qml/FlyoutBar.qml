// Ausklappende Werkzeuggruppe am unteren Rand (Bedienmuster wie in Visual Technic Studio): eingeklappt eine dunkle
// Pille mit Titel, Pfeil nach oben und Anzahl der Werkzeuge (»Licht und Wärme ▴ 3«); bei Mausberuehrung oder Antippen klappt
// darueber die Werkzeugleiste aus. Wird der Zeiger entfernt, schliesst sie von selbst (Touch: einige Sekunden nach
// der letzten Beruehrung). Solange busy (ein Werkzeug wird herausgezogen), bleibt sie offen. Mehrere Gruppen lassen
// sich nebeneinander setzen; der Inhalt kommt als Kinder (Default-Eigenschaft).
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: bar
    default property alias content: row.data
    property string title: ""
    property int count: row.children.length
    property bool busy: false
    property bool expanded: false
    readonly property bool hovered: handleHover.hovered || panelHover.hovered
    property int collapseDelay: hovered ? 600 : 4000

    // Groesse = Griff; die Leiste liegt darueber und verschiebt nichts
    width: handle.width
    height: Theme.control

    function poke() {
        expanded = true
        collapseTimer.restart()
    }
    onHoveredChanged: if (hovered) poke(); else collapseTimer.restart()
    onBusyChanged: if (busy) poke(); else collapseTimer.restart()

    Timer {
        id: collapseTimer
        interval: bar.collapseDelay
        onTriggered: {
            if (bar.busy || bar.hovered) restart()
            else bar.expanded = false
        }
    }

    // Griff: Pille mit Titel, Pfeil und Anzahl (Fangbereich 48 dp hoch)
    Item {
        id: handle
        objectName: "flyoutHandle"
        width: pill.width
        height: Theme.control
        Rectangle {
            id: pill
            anchors.verticalCenter: parent.verticalCenter
            width: handleLabel.implicitWidth + 32
            height: 38
            radius: height / 2
            color: bar.expanded ? "#1F4670" : "#14304D"
            border.color: "#6F9CC8"
            border.width: 2
            Row {
                id: handleLabel
                anchors.centerIn: parent
                spacing: 8
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: bar.title
                    color: "white"
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    font.bold: true
                }
                DrawnIcon { anchors.verticalCenter: parent.verticalCenter; kind: "up"; size: 16 }
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: bar.count
                    color: "white"
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    font.bold: true
                }
            }
        }
        HoverHandler { id: handleHover }
        TapHandler { onTapped: bar.poke() }
        Accessible.name: bar.title
        Accessible.role: Accessible.Button
    }

    // Werkzeugleiste (ausgeklappt), mittig ueber dem Griff
    Rectangle {
        id: panel
        visible: bar.expanded
        x: (handle.width - width) / 2
        y: -height - 6
        width: row.implicitWidth + 16
        height: row.implicitHeight + 12
        radius: 10
        color: "#14304D"
        border.color: "#6F9CC8"
        border.width: 2
        Row {
            id: row
            x: 8; y: 6
            spacing: 8
        }
        HoverHandler { id: panelHover }
        TapHandler { // jede Beruehrung haelt die Leiste offen
            gesturePolicy: TapHandler.DragThreshold
            grabPermissions: PointerHandler.ApprovesTakeOverByAnything
            onPressedChanged: if (pressed) bar.poke()
        }
    }
}
