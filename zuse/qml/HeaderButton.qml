import QtQuick
import QtQuick.Controls

// Knopf in der dunkelblauen Kopfzeile (wie Volta): weisse Schrift, heller Rahmen, mindestens 48 dp;
// »checked« zeigt einen eingeschalteten Zustand (Flaeche hell, nicht nur Farbe: zusaetzlich Unterstrich)
Button {
    id: root
    property bool on: false
    implicitHeight: ZTheme.control
    implicitWidth: Math.max(ZTheme.control, contentItem.implicitWidth + 28)
    font.bold: true
    font.pixelSize: 16
    contentItem: Text {
        text: root.text
        font: root.font
        color: root.on ? ZTheme.shell : ZTheme.textOnShell
        opacity: root.enabled ? 1 : 0.4
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 8
        color: root.on ? ZTheme.textOnShell : root.down ? "#24476E" : root.hovered ? "#16385C" : "transparent"
        border.width: 1
        border.color: root.activeFocus ? ZTheme.accent : Qt.rgba(1, 1, 1, root.enabled ? 0.45 : 0.15)
        Rectangle {
            visible: root.on
            anchors { bottom: parent.bottom; horizontalCenter: parent.horizontalCenter; bottomMargin: 5 }
            width: parent.width - 24; height: 3; radius: 1.5
            color: ZTheme.accentText
        }
    }
}
