// Knopf in Geraetefenstern (Generator, Oszilloskop): rechteckig mit abgerundeten Ecken und Rahmen wie die Knoepfe der
// Menueleiste. Eingerastet dunkelblau mit weisser Schrift und kraeftigerem Rahmen (nicht nur die Farbe).
import QtQuick
import QtQuick.Controls.Basic

ToolButton {
    id: btn
    readonly property color ink: checked ? "white" : Theme.navy
    font.pixelSize: Math.round(13 * Theme.fontScale)
    font.bold: true
    leftPadding: 10; rightPadding: 10
    ToolTip.visible: hovered && ToolTip.text !== ""
    contentItem: Text {
        text: btn.text
        font: btn.font
        color: btn.ink
        opacity: btn.enabled ? 1 : 0.35
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    background: Rectangle {
        radius: 6
        color: btn.checked ? Theme.navy : btn.down ? "#D7DEE6" : (btn.hovered && btn.enabled ? "#E6ECF2" : "#F4F7FA")
        border.color: btn.checked ? Theme.navy : (btn.enabled ? "#7F93A8" : "#C3CDD7")
        border.width: btn.checked ? 2.5 : 1.5
    }
}
