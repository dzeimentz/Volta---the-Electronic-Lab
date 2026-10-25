import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Entdecken ohne Nullen und Einsen: jede Schalterstellung als Situationskarte in Alltagssprache (»Fenster 1 offen,
// Fenster 2 zu«). Erst tippt das Kind an, was die Lampe seiner Meinung nach macht (Vermutung); dann setzt es eine
// Kiste ein und tippt eine Karte an – die Schalter stellen sich von selbst, die Karte zeigt, was die Kiste getan hat
// und ob es zur Vermutung passt. Die Bits stehen klein daneben (Bruecke zu 0 und 1), Zustand nie nur ueber Farbe.
ColumnLayout {
    id: root
    spacing: 8
    readonly property bool open: !workbench.stageDone

    component LampIcon: Rectangle { // Lampe als Bild: leuchtet (gelb mit Strahlen) oder aus (grau)
        property bool on: false
        implicitWidth: 22; implicitHeight: 22
        radius: 11
        color: on ? "#FDE68A" : ZTheme.surfaceHigh
        border.width: 2
        border.color: on ? "#D97706" : ZTheme.zero
        Text {
            anchors.centerIn: parent
            visible: parent.on
            text: "✦"
            color: "#B45309"
            font.pixelSize: 12
        }
    }
    component SwitchIcon: Rectangle { // Schalter als Bild: Knopf rechts und Cyan = an, links und grau = aus
        property bool on: false
        implicitWidth: 30; implicitHeight: 18
        radius: 9
        color: on ? ZTheme.accent : ZTheme.surfaceHigh
        border.width: 1
        border.color: on ? ZTheme.accentText : ZTheme.zero
        Rectangle {
            width: 12; height: 12; radius: 6
            anchors.verticalCenter: parent.verticalCenter
            x: parent.on ? parent.width - width - 3 : 3
            color: ZTheme.surface
            border.color: ZTheme.zero
        }
    }
    component Choice: Button { // Vermutung: was macht die Lampe?
        id: choice
        property bool lampOn: false
        property bool selected: false
        Layout.fillWidth: true
        Layout.preferredHeight: Math.max(ZTheme.control, choiceRow.implicitHeight + 12)
        contentItem: RowLayout {
            id: choiceRow
            spacing: 6
            LampIcon { on: choice.lampOn }
            Text {
                Layout.fillWidth: true
                text: choice.text
                wrapMode: Text.WordWrap
                color: choice.selected ? ZTheme.textOnAccent : ZTheme.text
                font.pixelSize: Math.round(14 * ZTheme.fontScale)
                font.bold: choice.selected
            }
        }
        background: Rectangle {
            radius: 10
            color: choice.selected ? ZTheme.navy : (choice.down ? ZTheme.surfaceHigh : ZTheme.surface)
            border.width: choice.selected ? 3 : 1
            border.color: choice.selected ? ZTheme.navy : ZTheme.holeStrong
        }
        Accessible.name: text
    }

    Repeater {
        model: workbench.checkRows
        delegate: Rectangle {
            id: card
            required property var modelData
            required property int index
            readonly property bool seen: modelData.actual !== ""
            readonly property bool current: !!modelData.current
            readonly property bool asking: root.open && workbench.guessing
            Layout.fillWidth: true
            implicitHeight: cardCol.implicitHeight + 20
            radius: 12
            color: card.current && !card.asking ? "#ECFAFD" : ZTheme.surface
            border.width: card.current && !card.asking ? 3 : 1
            border.color: card.current && !card.asking ? ZTheme.accent : ZTheme.line

            TapHandler { // ausprobieren: Schalter stellen sich wie auf der Karte
                enabled: !card.asking
                onTapped: workbench.trySituation(card.index)
            }

            ColumnLayout {
                id: cardCol
                anchors { left: parent.left; right: parent.right; top: parent.top; margins: 10 }
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Situation %1").arg(card.index + 1)
                              + (card.current && !card.asking ? "  ◀ " + qsTr("jetzt eingestellt") : "")
                        color: ZTheme.muted
                        font.pixelSize: Math.round(12 * ZTheme.fontScale); font.bold: true
                    }
                    Text { // Bruecke zu den Bits
                        text: card.modelData.inputs.split("").join(" ")
                        color: ZTheme.zero
                        font.family: ZTheme.mono; font.pixelSize: Math.round(12 * ZTheme.fontScale)
                    }
                }
                Repeater {
                    model: card.modelData.situation
                    delegate: RowLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 8
                        SwitchIcon { on: modelData.on }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.text
                            wrapMode: Text.WordWrap
                            color: ZTheme.text
                            font.pixelSize: Math.round(15 * ZTheme.fontScale); font.bold: true
                        }
                    }
                }
                // Vermutung abgeben
                RowLayout {
                    visible: card.asking
                    Layout.fillWidth: true
                    spacing: 8
                    Choice {
                        text: card.modelData.outOff
                        lampOn: false
                        selected: card.modelData.guess === "0"
                        onClicked: workbench.setGuess(card.index, false)
                    }
                    Choice {
                        text: card.modelData.outOn
                        lampOn: true
                        selected: card.modelData.guess === "1"
                        onClicked: workbench.setGuess(card.index, true)
                    }
                }
                // nach dem Vermuten: Vermutung und was die Kiste getan hat
                RowLayout {
                    visible: !card.asking && card.modelData.guess !== ""
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: qsTr("Vermutet:")
                        color: ZTheme.muted
                        font.pixelSize: Math.round(13 * ZTheme.fontScale)
                    }
                    LampIcon { on: card.modelData.guess === "1"; implicitWidth: 16; implicitHeight: 16; radius: 8 }
                    Text {
                        Layout.fillWidth: true
                        text: card.modelData.guess === "1" ? card.modelData.outOn : card.modelData.outOff
                        wrapMode: Text.WordWrap
                        color: ZTheme.muted
                        font.pixelSize: Math.round(13 * ZTheme.fontScale)
                    }
                }
                RowLayout {
                    visible: !card.asking
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        visible: !card.seen
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: workbench.boxIndex < 0 ? qsTr("Setz links eine Kiste ein.")
                                                     : "👆 " + qsTr("Antippen und ausprobieren")
                        color: ZTheme.accentText
                        font.pixelSize: Math.round(13 * ZTheme.fontScale)
                    }
                    Text {
                        visible: card.seen
                        text: qsTr("Kiste:")
                        color: ZTheme.text
                        font.pixelSize: Math.round(14 * ZTheme.fontScale); font.bold: true
                    }
                    LampIcon { visible: card.seen; on: card.modelData.actual === "1" }
                    Text {
                        visible: card.seen
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: card.modelData.actual === "1" ? card.modelData.outOn : card.modelData.outOff
                        color: ZTheme.text
                        font.pixelSize: Math.round(14 * ZTheme.fontScale); font.bold: true
                    }
                    Text { // passt es zur Vermutung? Zeichen und Wort, nicht nur Farbe
                        visible: card.seen && card.modelData.guess !== ""
                        text: card.modelData.match === "=" ? "✓ " + qsTr("wie vermutet") : "✗ " + qsTr("anders")
                        color: card.modelData.match === "=" ? ZTheme.ok : ZTheme.bad
                        font.pixelSize: Math.round(13 * ZTheme.fontScale); font.bold: true
                    }
                }
            }
        }
    }
}
