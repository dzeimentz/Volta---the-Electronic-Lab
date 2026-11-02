// Kurze Hilfe in Geraetefenstern: Punkte mit je einem kurzen Satz auf hellem Grund
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: box
    property var lines: []
    implicitHeight: col.implicitHeight + 16
    height: implicitHeight
    radius: 8
    color: "#EEF4FA"
    border.color: "#9AB4CC"
    Column {
        id: col
        x: 10; y: 8
        width: box.width - 20
        spacing: 4
        Repeater {
            model: box.lines
            Label {
                required property string modelData
                width: col.width
                wrapMode: Text.WordWrap
                text: "• " + modelData
                color: "#1E2B38"
                font.pixelSize: Math.round(13 * Theme.fontScale)
            }
        }
    }
}
