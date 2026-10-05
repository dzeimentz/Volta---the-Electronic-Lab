// Gefuehrtes Experiment (Phase 1): Frage mit Vermutung -> Aufbau mit gestuften Hilfen -> Erfolg mit Erklaerung.
// Reine Anzeige; Ablauf und Erfolgspruefung liegen im Modell (ExperimentFlow, LabSession).
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: panel
    required property var controller
    readonly property int phase: controller.phase // 1 Frage, 2 Aufbau, 3 Erfolg
    property bool collapsed: false // eingeklappt: nur der Knopf zum Aufklappen (mehr Platz fuer die Grundplatte)

    color: "#FFFDF7"
    border.color: Theme.fieldOutline

    component Heading: Label {
        width: parent.width
        wrapMode: Text.Wrap
        font.pixelSize: Math.round((15) * Theme.fontScale)
        font.bold: true
        color: Theme.navy
    }
    component Body: Label {
        width: parent.width
        wrapMode: Text.Wrap
        font.pixelSize: Math.round((15) * Theme.fontScale)
        lineHeight: 1.15
        color: "#1E2B38"
    }
    component WideButton: Button {
        id: wide
        width: parent.width
        implicitHeight: Math.max(Theme.control, contentItem.implicitHeight + 16)
        background: Rectangle {
            radius: 8
            color: wide.down ? "#CFE2F3" : (wide.hovered ? "#E3EEF8" : "#EEF4FA")
            border.color: Theme.accent
            border.width: wide.activeFocus ? 3 : 1.5
        }
        contentItem: Label {
            text: parent.text
            wrapMode: Text.Wrap
            font.pixelSize: Math.round((15) * Theme.fontScale)
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
        }
    }

    // Neue Phase: nach oben; neuer Tipp: ans Ende, damit Tipp und naechster Knopf sichtbar sind
    property int lastPhase: 0
    Connections {
        target: panel.controller
        function onFlowChanged() { follow.restart() }
    }
    // erst nach dem Neuanordnen der Spalte (Hoehe stimmt dann)
    Timer { id: follow; interval: 30; onTriggered: panel.followFlow() }
    function followFlow() {
        const f = scroll.contentItem // Flickable der ScrollView
        if (panel.phase !== panel.lastPhase) f.contentY = 0
        else f.contentY = Math.max(0, f.contentHeight - f.height)
        panel.lastPhase = panel.phase
    }

    // Ein-/Ausklappen (oben links im Feld)
    ToolButton {
        id: toggle
        objectName: "panelToggle"
        x: 4
        y: 4
        width: Theme.control; height: Theme.control
        text: panel.collapsed ? "«" : "»"
        font.pixelSize: Math.round((24) * Theme.fontScale)
        onClicked: panel.collapsed = !panel.collapsed
        ToolTip.visible: hovered
        ToolTip.text: panel.collapsed ? qsTr("Experiment aufklappen") : qsTr("Experiment einklappen")
        Accessible.name: ToolTip.text
    }
    Label {
        visible: panel.collapsed
        anchors.horizontalCenter: parent.horizontalCenter
        y: toggle.y + toggle.height + 16 + width / 2
        rotation: 90
        transformOrigin: Item.Center
        text: qsTr("Experiment")
        font.pixelSize: Math.round((15) * Theme.fontScale)
        font.bold: true
        color: Theme.navy
    }

    ScrollView {
        id: scroll
        visible: !panel.collapsed
        anchors.fill: parent
        anchors.margins: 16
        anchors.topMargin: Theme.control + 8
        contentWidth: availableWidth
        clip: true

        Column {
            width: scroll.availableWidth
            spacing: 12

            Label {
                text: panel.phase === 1 ? qsTr("Frage") : panel.phase === 2 ? qsTr("Aufbau") : qsTr("Geschafft!")
                font.pixelSize: Math.round((22) * Theme.fontScale)
                font.bold: true
                color: panel.phase === 3 ? "#2E7D32" : Theme.navy
                // Haken zusaetzlich zur Farbe (Farbe nie allein)
                leftPadding: panel.phase === 3 ? 30 : 0
                Label {
                    visible: panel.phase === 3
                    text: "✓"
                    font.pixelSize: Math.round((24) * Theme.fontScale)
                    font.bold: true
                    color: "#2E7D32"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Rectangle {
                visible: panel.controller.destructive
                width: parent.width
                height: onlyApp.implicitHeight + 12
                radius: 6
                color: "#FFE9DC"
                border.color: Theme.signalOrange
                Label {
                    id: onlyApp
                    anchors.centerIn: parent
                    text: "⚠ " + qsTr("Nur in der App!")
                    font.bold: true
                    color: "#8A3200"
                }
            }

            // ---- 1 Frage
            Body { visible: panel.phase === 1; text: panel.controller.question }
            Heading { visible: panel.phase === 1; text: qsTr("Was vermutest du?") }
            Repeater {
                model: panel.phase === 1 ? panel.controller.guesses : []
                WideButton {
                    required property int index
                    required property string modelData
                    objectName: "guess_" + index
                    text: modelData
                    onClicked: panel.controller.chooseGuess(index)
                }
            }

            // ---- 2 Aufbau
            Body {
                visible: panel.phase >= 2 && panel.controller.guess >= 0
                text: qsTr("Deine Vermutung: %1").arg(panel.controller.guesses[panel.controller.guess] ?? "")
                font.italic: true
                color: "#4A5866"
            }
            Repeater {
                model: panel.phase === 2 ? panel.controller.steps : []
                Row {
                    required property int index
                    required property string modelData
                    width: parent.width
                    spacing: 8
                    Rectangle {
                        width: 28; height: 28; radius: 14
                        color: Theme.accent
                        Label { anchors.centerIn: parent; text: index + 1; color: "white"; font.bold: true }
                    }
                    Body { width: parent.width - 36; text: modelData }
                }
            }
            Repeater {
                model: panel.phase === 2 ? panel.controller.hints : []
                Rectangle {
                    required property int index
                    required property string modelData
                    width: parent.width
                    height: hintText.implicitHeight + 16
                    radius: 6
                    color: "#FFF4D6"
                    border.color: Theme.solderAmber
                    Body {
                        id: hintText
                        x: 8; y: 8
                        width: parent.width - 16
                        text: "💡 " + modelData
                    }
                }
            }
            WideButton {
                objectName: "hintButton"
                visible: panel.phase === 2 && panel.controller.moreHints
                text: panel.controller.hints.length === 0 ? qsTr("Ich brauche einen Tipp")
                                                          : qsTr("Noch ein Tipp")
                onClicked: panel.controller.nextHint()
            }
            WideButton {
                objectName: "templateButton"
                visible: panel.phase === 2 && panel.controller.solutionShown
                text: qsTr("Musteraufbau einsetzen")
                onClicked: panel.controller.showTemplate()
            }

            // ---- 3 Erfolg
            Body { visible: panel.phase === 3; text: panel.controller.explanation }
            Heading { visible: panel.phase === 3 && panel.controller.moreKnowledge !== ""; text: qsTr("Mehr wissen") }
            Body { visible: panel.phase === 3 && panel.controller.moreKnowledge !== ""; text: panel.controller.moreKnowledge }
            Heading { visible: panel.phase === 3 && panel.controller.further.length > 0; text: qsTr("Weiterforschen") }
            Repeater {
                model: panel.phase === 3 ? panel.controller.further : []
                Body {
                    required property string modelData
                    text: "• " + modelData
                }
            }
            Heading { visible: panel.phase === 3; text: qsTr("Jetzt in echt") }
            Body { visible: panel.phase === 3; text: panel.controller.realLife }
            WideButton {
                objectName: "nextButton"
                visible: panel.phase === 3
                text: panel.controller.nextExperimentId !== "" ? qsTr("Nächstes Experiment") : qsTr("Freies Labor")
                onClicked: panel.controller.nextExperimentId !== ""
                           ? panel.controller.startExperiment(panel.controller.nextExperimentId)
                           : panel.controller.startFreeLab()
            }
        }
    }
}
