// Drehregler mit Lichtring (docs/ui-spec.md, Bedienpult): 16 Segmente ueber 270 Grad, Luecke unten; die Segmente
// leuchten vom linken Anschlag bis zur Stellung, die uebrigen bleiben als blasse Umrisse sichtbar.
// Bedienung: Kreisbewegung um den Knopf oder Wischen nach oben/unten, Mausrad, Doppeltippen = Mitte.
// Die Stellung ist intern stufenlos; die Segmente runden nur die Anzeige.
import QtQuick
import QtQuick.Shapes

Item {
    id: knob
    property real value: 0.5 // 0 = linker Anschlag, 1 = rechter Anschlag
    property string label
    signal moved(real value)

    readonly property int segments: 16
    readonly property int lit: Math.round(value * segments)

    width: 96
    height: 96

    function setValue(v) {
        const c = Math.max(0, Math.min(1, v))
        if (c !== value) knob.moved(c)
    }

    // Lichtring
    Repeater {
        model: knob.segments
        Item {
            id: seg
            required property int index
            anchors.fill: parent
            // Winkel: -135 Grad (links unten) bis +135 Grad (rechts unten), 0 = oben
            readonly property real angle: -135 + (index + 0.5) * 270 / knob.segments
            rotation: angle
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: 2
                width: 8
                height: 14
                radius: 3
                color: seg.index < knob.lit ? Theme.signalOrange : "transparent"
                border.color: seg.index < knob.lit ? Theme.solderAmber : "#7F93A8"
                border.width: 1.5
            }
        }
    }

    // Drehknopf mit Zeiger
    Rectangle {
        id: cap
        anchors.centerIn: parent
        width: 52; height: 52; radius: 26
        color: "#2A4A6E"
        border.color: "#0E2238"
        border.width: 2
        rotation: -135 + knob.value * 270
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 5
            width: 4; height: 14; radius: 2
            color: "white"
        }
    }

    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 40
        text: knob.label
        color: "white"
        font.pixelSize: 13
        font.bold: true
    }

    // Kreisbewegung (aussen) bzw. Wischen nach oben/unten (innen)
    property real startValue
    property point startPos
    DragHandler {
        id: drag
        target: null
        onActiveChanged: if (active) { knob.startValue = knob.value; knob.startPos = centroid.position }
        onCentroidChanged: {
            if (!active) return
            const cx = knob.width / 2, cy = knob.height / 2
            const p = centroid.position
            const r = Math.hypot(p.x - cx, p.y - cy)
            if (r > cap.width / 2) {
                // Winkel ab oben im Uhrzeigersinn, Luecke unten
                let deg = Math.atan2(p.x - cx, -(p.y - cy)) * 180 / Math.PI
                if (deg > 135 || deg < -135) return // in der Luecke: Stellung halten
                knob.setValue((deg + 135) / 270)
            } else {
                knob.setValue(knob.startValue - (p.y - knob.startPos.y) / 150)
            }
        }
    }
    WheelHandler {
        onWheel: (event) => knob.setValue(knob.value + (event.angleDelta.y > 0 ? 1 : -1) / 32)
    }
    TapHandler {
        onDoubleTapped: knob.setValue(0.5)
    }

    Accessible.role: Accessible.Dial
    Accessible.name: knob.label
}
