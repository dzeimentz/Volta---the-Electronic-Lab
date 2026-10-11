// Gefuehrtes Experiment (Phase 1): Frage mit Vermutung -> Aufbau mit gestuften Hilfen -> Erfolg mit Erklaerung.
// Reine Anzeige; Ablauf und Erfolgspruefung liegen im Modell (ExperimentFlow, LabSession).
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: panel
    required property var controller
    property var speech: null // SpeechService; Knopf nur, wenn eine Stimme fuer die Sprache da ist
    readonly property int phase: controller.phase // 1 Frage, 2 Aufbau, 3 Beobachtung, 4 Erklaerung

    // Text eines Schritts der Aufbauhilfe (LabController::guideStep)
    function guideText(s) { return Texts.stepText(s) }
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
        if (panel.phase !== panel.lastPhase) { f.contentY = 0; noteSaved.visible = false }
        else f.contentY = Math.max(0, f.contentHeight - f.height)
        panel.lastPhase = panel.phase
    }

    // Text der aktuellen Phase zum Vorlesen
    function speechText() {
        const c = controller
        const list = (items) => items.map((t, i) => (i + 1) + ". " + t).join(". ")
        if (phase === 1) return c.question + " " + list(c.guesses)
        if (phase === 2) {
            let t = qsTr("Aufbau") + ". " + list(c.steps)
            if (c.guideActive) t += ". " + guideText(c.guideStep)
            if (c.hints.length > 0) t += ". " + c.hints[c.hints.length - 1]
            return t
        }
        if (phase === 3) return qsTr("Geschafft! Was hast du beobachtet?") + " " + list(c.observations)
        if (phase === 4) return c.explanation + (c.realLife !== "" ? " " + c.realLife : "")
        return ""
    }
    onPhaseChanged: if (speech) speech.stop()

    // Vorlesen (oben rechts im Feld)
    ToolButton {
        objectName: "speakButton"
        visible: !panel.collapsed && panel.speech !== null && panel.speech.available && panel.phase > 0
        x: parent.width - width - 4
        y: 4
        width: Theme.control; height: Theme.control
        text: panel.speech && panel.speech.speaking ? "⏹" : "🔊"
        font.pixelSize: 20
        onClicked: panel.speech.speaking ? panel.speech.stop() : panel.speech.say(panel.speechText())
        ToolTip.visible: hovered
        ToolTip.text: panel.speech && panel.speech.speaking ? qsTr("Vorlesen beenden") : qsTr("Vorlesen")
        Accessible.name: ToolTip.text
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

    // »Fertig« (Aufbau): immer sichtbar unten im Feld; grau und nicht klickbar, bis alle Bauteile stecken und die
    // Schaltung funktioniert. Der Zustand steht zusaetzlich als Text darunter (Farbe nie allein).
    Rectangle {
        id: finishBar
        visible: !panel.collapsed && panel.phase === 2
        anchors { left: parent.left; right: parent.right; bottom: parent.bottom; margins: 1 }
        height: finishColumn.implicitHeight + 24
        color: "#F4EFE2"
        Rectangle { anchors { left: parent.left; right: parent.right; top: parent.top } height: 1; color: Theme.fieldOutline }
        Column {
            id: finishColumn
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
            spacing: 6
            Button {
                id: finishButton
                objectName: "finishButton"
                width: parent.width
                implicitHeight: Theme.control + 4
                enabled: panel.controller.canFinish
                text: "✓ " + qsTr("Fertig")
                onClicked: panel.controller.finishExperiment()
                background: Rectangle {
                    radius: 8
                    color: !finishButton.enabled ? "#C9CED4"
                         : finishButton.down ? "#1F5E23" : (finishButton.hovered ? "#276D2B" : "#2E7D32")
                    border.color: finishButton.enabled ? "#1B4D1E" : "#9AA3AD"
                    border.width: finishButton.activeFocus ? 3 : 1
                }
                contentItem: Label {
                    text: finishButton.text
                    color: finishButton.enabled ? "white" : "#6B7580"
                    font.pixelSize: Math.round(17 * Theme.fontScale)
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                Accessible.name: text + (enabled ? "" : ", " + finishHint.text)
            }
            Label {
                id: finishHint
                objectName: "finishHint"
                width: parent.width
                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Math.round(13 * Theme.fontScale)
                color: "#3C4752"
                text: !panel.controller.buildComplete ? qsTr("Es fehlen noch Bauteile oder Module.")
                    : !panel.controller.canFinish ? qsTr("Schalte ein und probiere aus – die Schaltung muss funktionieren.")
                    : qsTr("Es funktioniert! Tippe auf »Fertig«.")
            }
        }
    }

    ScrollView {
        id: scroll
        visible: !panel.collapsed
        anchors { left: parent.left; right: parent.right; top: parent.top }
        anchors.bottom: finishBar.visible ? finishBar.top : parent.bottom
        anchors.margins: 16
        anchors.topMargin: Theme.control + 8
        contentWidth: availableWidth
        clip: true

        Column {
            width: scroll.availableWidth
            spacing: 12

            Label {
                text: panel.phase === 1 ? qsTr("Frage") : panel.phase === 2 ? qsTr("Aufbau")
                    : panel.phase === 3 ? qsTr("Geschafft! Was hast du beobachtet?") : qsTr("Erklärung")
                width: parent.width
                wrapMode: Text.Wrap
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
            TickSays {
                visible: panel.controller.destructive
                width: parent.width
                figure: 80
                mood: "warnend"
                text: "⚠ " + qsTr("Nur in der App!")
                bold: true
                textColor: "#8A3200"
                bubbleColor: "#FFE9DC"
                borderColor: Theme.signalOrange
            }

            // ---- 1 Frage
            TickSays {
                objectName: "questionTick"
                visible: panel.phase === 1
                width: parent.width
                figure: 80
                mood: "neugierig"
                text: panel.controller.question
            }
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
            // Schritt-fuer-Schritt-Aufbauhilfe: das naechste Teil schemenhaft auf der Grundplatte, Zielfelder leuchten
            WideButton {
                objectName: "guideButton"
                visible: panel.phase === 2
                text: panel.controller.guideActive ? qsTr("Schritt-für-Schritt-Hilfe ausblenden")
                                                   : qsTr("Schritt für Schritt bauen")
                onClicked: panel.controller.guideActive = !panel.controller.guideActive
            }
            Rectangle {
                id: guideCard
                objectName: "guideCard"
                readonly property var step: panel.controller.guideStep
                visible: panel.phase === 2 && panel.controller.guideActive && step.kind !== undefined
                width: parent.width
                height: guideColumn.implicitHeight + 20
                radius: 8
                color: "#E3F0FF"
                border.color: Theme.accent
                border.width: 2
                Column {
                    id: guideColumn
                    x: 10; y: 10
                    width: parent.width - 20
                    spacing: 6
                    Label {
                        text: guideCard.step.kind === "fertig" ? "✓ " + qsTr("Alles gesteckt")
                              : qsTr("Schritt %1 von %2").arg((guideCard.step.index || 0) + 1).arg(guideCard.step.count || 0)
                        font.bold: true
                        color: Theme.navy
                        font.pixelSize: Math.round(14 * Theme.fontScale)
                    }
                    Body {
                        objectName: "guideText"
                        width: parent.width
                        text: panel.guideText(guideCard.step)
                    }
                    Body {
                        visible: guideCard.step.kind !== "fertig" && guideCard.step.kind !== "hauptschalter"
                        width: parent.width
                        text: qsTr("Die blinkenden Felder zeigen, wohin es gehört.")
                        color: "#4A5866"
                        font.pixelSize: Math.round(13 * Theme.fontScale)
                    }
                }
            }
            Repeater {
                model: panel.phase === 2 ? panel.controller.hints : []
                TickSays {
                    required property int index
                    required property string modelData
                    width: parent.width
                    figure: 80
                    figureShown: index === panel.controller.hints.length - 1
                    mood: index === 0 ? "nachdenklich" : "erklaerend"
                    text: "💡 " + modelData
                    bubbleColor: "#FFF4D6"
                    borderColor: Theme.solderAmber
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

            // ---- 3 Beobachtung (Auswahl; danach Vergleich mit der Vermutung, ohne zu werten)
            TickSays {
                visible: panel.phase === 3
                width: parent.width
                figure: 80
                mood: "gut"
                text: qsTr("Super, deine Schaltung funktioniert! Schau genau hin: Was passiert?")
            }
            Repeater {
                model: panel.phase === 3 ? panel.controller.observations : []
                WideButton {
                    required property int index
                    required property string modelData
                    objectName: "observation_" + index
                    text: modelData
                    onClicked: panel.controller.chooseObservation(index)
                }
            }

            // ---- 4 Erklaerung und Weiterforschen
            TickSays {
                objectName: "comparisonBox"
                visible: panel.phase === 4 && panel.controller.comparison > 0
                width: parent.width
                figure: 80
                mood: panel.controller.comparison === 1 ? "froehlich" : panel.controller.comparison === 2 ? "ueberrascht" : "motivierend"
                bubbleColor: panel.controller.comparison === 1 ? "#E6F4E7" : "#FFF4D6"
                borderColor: panel.controller.comparison === 1 ? "#2E7D32" : Theme.solderAmber
                bold: true
                // Symbol zusaetzlich zur Farbe
                text: panel.controller.comparison === 1 ? "✓ " + qsTr("Deine Vermutung hat gestimmt!")
                    : panel.controller.comparison === 2 ? "★ " + qsTr("Überraschung! Es kam anders, als du vermutet hast – genau das macht Forschen spannend.")
                    : "★ " + qsTr("Jetzt weißt du es!")
            }
            Body {
                visible: panel.phase === 4 && panel.controller.observation >= 0
                text: qsTr("Deine Beobachtung: %1").arg(panel.controller.observations[panel.controller.observation] ?? "")
                font.italic: true
                color: "#4A5866"
            }
            Body { visible: panel.phase === 4; text: panel.controller.explanation }
            Heading { visible: panel.phase === 4 && panel.controller.moreKnowledge !== ""; text: qsTr("Mehr wissen") }
            Body { visible: panel.phase === 4 && panel.controller.moreKnowledge !== ""; text: panel.controller.moreKnowledge }
            Heading { visible: panel.phase === 4 && panel.controller.further.length > 0; text: qsTr("Weiterforschen") }
            Repeater {
                model: panel.phase === 4 ? panel.controller.further : []
                Body {
                    required property string modelData
                    text: "• " + modelData
                }
            }
            // eigene Notiz ins Forscherheft
            TextArea {
                id: noteField
                objectName: "noteField"
                visible: panel.phase === 4
                width: parent.width
                height: Math.max(Theme.control + 24, implicitHeight)
                wrapMode: TextEdit.Wrap
                font.pixelSize: Math.round(15 * Theme.fontScale)
                placeholderText: qsTr("Was hast du beim Weiterforschen herausgefunden?")
                background: Rectangle { radius: 6; color: "white"; border.color: Theme.fieldOutline }
                Accessible.name: qsTr("Notiz für das Forscherheft")
            }
            WideButton {
                objectName: "noteButton"
                visible: panel.phase === 4
                enabled: noteField.text.trim().length > 0
                text: qsTr("Ins Forscherheft schreiben")
                onClicked: if (panel.controller.addNote(noteField.text)) { noteField.text = ""; noteSaved.visible = true }
            }
            Body {
                id: noteSaved
                visible: false
                text: "✓ " + qsTr("Im Forscherheft gespeichert.")
                color: "#2E7D32"
            }
            Heading { visible: panel.phase === 4; text: qsTr("Jetzt in echt") }
            Body { visible: panel.phase === 4; text: panel.controller.realLife }
            WideButton {
                objectName: "nextButton"
                visible: panel.phase === 4
                text: panel.controller.nextExperimentId !== "" ? qsTr("Nächstes Experiment") : qsTr("Freies Labor")
                onClicked: panel.controller.nextExperimentId !== ""
                           ? panel.controller.startExperiment(panel.controller.nextExperimentId)
                           : panel.controller.startFreeLab()
            }
        }
    }
}
