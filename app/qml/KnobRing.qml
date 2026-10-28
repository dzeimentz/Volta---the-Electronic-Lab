// Drehregler mit Lichtring (docs/ui-spec.md, Bedienpult): 16 Segmente ueber 270 Grad, Luecke unten; die Segmente
// leuchten vom linken Anschlag bis zur Stellung, die uebrigen bleiben als blasse Umrisse sichtbar.
// Bedienung: Kreisbewegung um den Knopf oder Wischen nach oben/unten, Mausrad, Doppeltippen = Mitte.
// Die Stellung ist intern stufenlos; die Segmente runden nur die Anzeige. Durchgebrannt: grau, Rauch steigt auf,
// »kaputt« unter dem Knopf; Antippen repariert (repairRequested), Drehen ist dann gesperrt.
import QtQuick
import QtQuick.Shapes

Item {
    id: knob
    property real value: 0.5 // 0 = linker Anschlag, 1 = rechter Anschlag
    property string label
    property bool broken: false
    property bool overloaded: false
    signal moved(real value)
    signal repairRequested()

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
                color: knob.broken ? "transparent" : (seg.index < knob.lit ? Theme.signalOrange : "transparent")
                border.color: knob.broken ? Theme.destroyed : (seg.index < knob.lit ? Theme.solderAmber : "#7F93A8")
                border.width: 1.5
            }
        }
    }

    // Drehknopf mit Zeiger
    Rectangle {
        id: cap
        anchors.centerIn: parent
        width: 52; height: 52; radius: 26
        color: knob.broken ? Theme.destroyed : "#2A4A6E"
        border.color: knob.overloaded && !knob.broken ? Theme.signalOrange : "#0E2238"
        border.width: 2
        rotation: -135 + knob.value * 270
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 5
            width: 4; height: 14; radius: 2
            color: "white"
        }
    }

    // Rauch ueber dem durchgebrannten Drehregler (wie bei Bauteilen)
    Repeater {
        model: knob.broken ? 3 : 0
        Rectangle {
            id: puff
            required property int index
            width: 12 + index * 4; height: width; radius: width / 2
            color: "#8C8C8C"
            x: knob.width / 2 - width / 2 + (index - 1) * 8
            y: cap.y
            SequentialAnimation on y {
                loops: Animation.Infinite
                PauseAnimation { duration: puff.index * 400 }
                NumberAnimation { from: cap.y; to: cap.y - 40; duration: 1600 }
            }
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                PauseAnimation { duration: puff.index * 400 }
                NumberAnimation { from: 0.8; to: 0; duration: 1600 }
            }
        }
    }

    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 40
        // kaputt: Hinweis zusaetzlich als Text (Farbe nie allein)
        text: knob.broken ? qsTr("kaputt – antippen") : knob.label
        color: "white"
        font.pixelSize: Math.round((13) * Theme.fontScale)
        font.bold: true
    }

    // Kreisbewegung (aussen) bzw. Wischen nach oben/unten (innen)
    property real startValue
    property point startPos
    DragHandler {
        id: drag
        target: null
        enabled: !knob.broken
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
        enabled: !knob.broken
        onWheel: (event) => knob.setValue(knob.value + (event.angleDelta.y > 0 ? 1 : -1) / 32)
    }
    TapHandler {
        onTapped: if (knob.broken) knob.repairRequested()
        onDoubleTapped: if (!knob.broken) knob.setValue(0.5)
    }

    Accessible.role: Accessible.Dial
    Accessible.name: knob.broken ? qsTr("%1 kaputt – antippen zum Reparieren").arg(knob.label) : knob.label
}
