import QtQuick
import QtQuick.Controls

// Knopf der Zuse-Linie: mindestens 48 dp, Text in Monospace
Button {
    id: root
    property bool primary: false
    implicitHeight: ZTheme.control
    implicitWidth: Math.max(ZTheme.control, contentItem.implicitWidth + 32)
    font.family: ZTheme.mono
    font.pixelSize: 16
    contentItem: Text {
        text: root.text
        font: root.font
        color: !root.enabled ? ZTheme.muted : root.primary ? ZTheme.graphite : ZTheme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: ZTheme.radius
        color: root.primary && root.enabled ? (root.down ? Qt.darker(ZTheme.accent, 1.2) : ZTheme.accent)
                                            : (root.down ? ZTheme.surfaceHigh : ZTheme.surface)
        border.width: 1
        border.color: root.primary && root.enabled ? ZTheme.accent : root.activeFocus ? ZTheme.accent : ZTheme.line
        opacity: root.enabled ? 1.0 : 0.6
    }
}
