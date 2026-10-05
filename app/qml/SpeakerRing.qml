// Lautsprecher mit Leuchtring (Klanganzeige, docs/ui-spec.md):
// - Tonhoehe -> Farbton, logarithmisch: bis 100 Hz Gelb (#FFD400) ueber Orange bis ab 5 kHz Rot (#E8221B)
// - Signalstaerke -> Helligkeit; Stille -> dunkel
// - Impulse unter 20 Hz -> kurzes Aufblinken je Impuls
// - Skalenmarke wandert mit der Tonhoehe im Uhrzeigersinn (lesbar auch bei Farbfehlsichtigkeit)
// - langer Druck zeigt die Frequenz in Hz (ab Stufe 3 und im freien Labor)
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: speaker
    required property var controller
    property var state: ({})
    property real lastPulses: -1
    readonly property real freq: state.frequencyHz || 0
    readonly property real level: state.level || 0
    readonly property bool sounding: freq >= 20 && level > 0.02
    readonly property bool destroyed: state.destroyed === true

    width: 72
    height: 72

    // Farbton zur Tonhoehe (log): 100 Hz -> 0, 5 kHz -> 1
    function toneColor(f) {
        const t = Math.max(0, Math.min(1, Math.log(Math.max(f, 1) / 100) / Math.log(50)))
        const yellow = Qt.color("#FFD400"), orange = Qt.color("#FF8C00"), red = Qt.color("#E8221B")
        const mix = (a, b, u) => Qt.rgba(a.r + (b.r - a.r) * u, a.g + (b.g - a.g) * u, a.b + (b.b - a.b) * u, 1)
        return t < 0.5 ? mix(yellow, orange, t * 2) : mix(orange, red, (t - 0.5) * 2)
    }
    // Lage der Skalenmarke: 20 Hz oben (0 Grad) bis 5 kHz (300 Grad), im Uhrzeigersinn
    function markAngle(f) { return 300 * Math.max(0, Math.min(1, Math.log(Math.max(f, 20) / 20) / Math.log(250))) }

    Connections {
        target: speaker.controller
        function onStateChanged() {
            const s = speaker.controller.speakerState()
            if (s.pulses === undefined) return // keine gueltige Messung (z. B. direkt nach einem Topologiewechsel)
            // neuer Impuls unter 20 Hz: kurz aufblinken
            if (speaker.lastPulses >= 0 && s.pulses > speaker.lastPulses && (s.frequencyHz || 0) < 20) flash.restart()
            speaker.lastPulses = s.pulses
            speaker.state = s
        }
    }

    // Leuchtring
    Rectangle {
        id: ring
        objectName: "speakerRing"
        anchors.centerIn: parent
        width: 64; height: 64; radius: 32
        color: "#203650"
        border.width: 6
        border.color: speaker.destroyed ? "#4A4A4A"
                    : speaker.sounding ? speaker.toneColor(speaker.freq) : "#34485E"
        opacity: 1
        // Helligkeit ~ Signalstaerke
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "transparent"
            border.width: 6
            border.color: "#000000"
            opacity: speaker.sounding ? 0.6 * (1 - Math.min(1, speaker.level * 1.5)) : 0
        }
        // Aufblinken je Impuls
        Rectangle {
            id: flashRing
            anchors.fill: parent
            radius: width / 2
            color: "transparent"
            border.width: 6
            border.color: "#FFD400"
            opacity: 0
            SequentialAnimation {
                id: flash
                NumberAnimation { target: flashRing; property: "opacity"; to: 1; duration: 40 }
                NumberAnimation { target: flashRing; property: "opacity"; to: 0; duration: 220 }
            }
        }
        Text {
            anchors.centerIn: parent
            text: "🔈"
            font.pixelSize: 24
            opacity: speaker.destroyed ? 0.4 : 1
        }
    }
    // Skalenmarke (wandert im Uhrzeigersinn mit der Tonhoehe)
    Item {
        anchors.fill: ring
        visible: speaker.sounding
        rotation: speaker.markAngle(speaker.freq)
        Rectangle {
            objectName: "speakerMark"
            anchors.horizontalCenter: parent.horizontalCenter
            y: -4
            width: 6; height: 12; radius: 2
            color: "white"
            border.color: "#0A2540"
        }
    }

    // Frequenz bei langem Druck (ab Stufe 3 / freies Labor)
    Rectangle {
        id: hzLabel
        visible: false
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.top
        width: hzText.implicitWidth + 12
        height: hzText.implicitHeight + 6
        radius: 4
        color: "#FFFDF7"
        Label {
            id: hzText
            anchors.centerIn: parent
            text: speaker.freq > 0 ? qsTr("%1 Hz").arg(Math.round(speaker.freq).toLocaleString(Qt.locale(), "f", 0))
                                   : qsTr("Stille")
            color: Theme.navy
            font.bold: true
        }
    }
    TapHandler {
        onLongPressed: if (speaker.controller.showNumbers) hzLabel.visible = true
        onPressedChanged: if (!pressed) hzLabel.visible = false
        onTapped: if (speaker.destroyed) speaker.controller.repairSpeaker()
    }

    Accessible.role: Accessible.Indicator
    Accessible.name: speaker.destroyed ? qsTr("Lautsprecher kaputt – antippen zum Reparieren")
                   : speaker.sounding ? qsTr("Lautsprecher: Ton") : qsTr("Lautsprecher: still")
}
