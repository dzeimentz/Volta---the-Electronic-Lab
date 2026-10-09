// Runder Loeschen-Knopf neben dem ausgewaehlten Bauteil bzw. auf der ausgewaehlten Leitung (sichtbare Alternative
// zu langem Druck, Rechtsklick und Entf-Taste). Bleibt trotz Verkleinerung der Arbeitsflaeche 48 dp gross.
import QtQuick
import QtQuick.Controls.Basic

ToolButton {
    id: badge
    property point at            // Mitte des Knopfs (Koordinaten der Arbeitsflaeche)
    property real canvasScale: 1 // Massstab der Arbeitsflaeche (Lab.canvas.scale)
    width: Theme.control
    height: Theme.control
    x: at.x - width / 2
    y: at.y - height / 2
    scale: 1 / Math.max(0.3, canvasScale)
    z: 50
    text: "🗑"
    font.pixelSize: 20
    ToolTip.visible: hovered
    ToolTip.text: Accessible.name
    background: Rectangle {
        radius: width / 2
        color: badge.down ? "#B71C1C" : (badge.hovered ? "#D32F2F" : "#E53935")
        border.color: "white"
        border.width: 3
    }
}
