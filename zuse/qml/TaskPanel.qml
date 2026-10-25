import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Aufgabe rechts wie Volta (Experiment-Leiste): oben fest Titel, Frage und Zetti mit Sprechblase (Geschichte,
// Rueckmeldung oder Tipp); in der Mitte Hilfen, Tabelle und Merksatz (rollt bei wenig Hoehe); unten fest die Aktion.
// Lernzyklus Entdecken: Frage -> Vermutung (Tabelle ausfuellen) -> Ausprobieren -> Benennen.
Rectangle {
    id: root
    property bool hintVisible: false
    readonly property bool building: workbench.stageKind === "bauen"
    readonly property bool counting: workbench.stageKind === "zahlen"
    readonly property bool hasNext: workbench.stageIndex + 1 < workbench.stages.length
    readonly property bool free: workbench.mode === "frei"
    readonly property bool demo: workbench.mode === "vorfuehrung"

    signal demoPickerRequested()
    signal homeRequested()
    color: ZTheme.surface
    Rectangle { anchors { left: parent.left; top: parent.top; bottom: parent.bottom } width: 1; color: ZTheme.line }

    Connections {
        target: workbench
        function onStageChanged() { root.hintVisible = false }
        function onMessageChanged() { root.hintVisible = false }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        // ---- fest oben: Stufe und Zetti
        Text {
            Layout.fillWidth: true
            text: workbench.stageTitle
            color: ZTheme.navy
            font.pixelSize: 22; font.bold: true
            wrapMode: Text.WordWrap
        }
        Text {
            Layout.fillWidth: true
            text: workbench.stageQuestion
            color: ZTheme.accentText
            font.pixelSize: 16; font.bold: true
            wrapMode: Text.WordWrap
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: 4
            Image { // Zetti klein neben der Sprechblase, begleitet im Hintergrund
                Layout.preferredWidth: 64
                Layout.preferredHeight: 80
                Layout.alignment: Qt.AlignBottom
                source: "qrc:/zuse/zetti/zetti_" + workbench.zettiPose + ".png"
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
            }
            Rectangle { // Sprechblase
                Layout.fillWidth: true
                Layout.preferredHeight: bubbleText.implicitHeight + 24
                radius: 14
                color: ZTheme.bubble
                border.width: 2
                border.color: ZTheme.accentDim
                Text {
                    id: bubbleText
                    anchors { left: parent.left; right: parent.right; top: parent.top; margins: 12 }
                    wrapMode: Text.WordWrap
                    color: ZTheme.text
                    font.pixelSize: 15
                    lineHeight: 1.1
                    text: root.hintVisible ? workbench.stageHint
                                           : (workbench.message !== "" ? workbench.message : workbench.stageStory)
                }
                Canvas { // Spitze der Blase zu Zetti
                    width: 14; height: 20
                    x: -12; y: parent.height - 34
                    onPaint: {
                        const ctx = getContext("2d")
                        ctx.fillStyle = ZTheme.bubble
                        ctx.strokeStyle = ZTheme.accentDim
                        ctx.lineWidth = 2
                        ctx.beginPath(); ctx.moveTo(width, 0); ctx.lineTo(0, height * 0.7); ctx.lineTo(width, height)
                        ctx.fill(); ctx.stroke()
                        ctx.fillRect(width - 2, 1, 3, height - 2)
                    }
                }
            }
        }
        RowLayout {
            spacing: 8
            ZButton {
                text: root.hintVisible ? qsTr("Tipp aus") : qsTr("Tipp")
                onClicked: root.hintVisible = !root.hintVisible
            }
            ZButton {
                visible: root.building && !root.free && !root.demo
                text: qsTr("Lösung zeigen")
                enabled: !workbench.inside
                onClicked: workbench.showSolution()
            }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: ZTheme.line }

        // ---- Mitte: Tabelle (rollt, wenn sie nicht passt)
        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: middle.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            ColumnLayout {
                id: middle
                width: parent.width
                spacing: 8
                Text {
                    visible: !root.free
                    Layout.fillWidth: true
                    text: root.counting ? qsTr("Aufgaben")
                        : root.building ? (workbench.stageSequence ? qsTr("Ablauf") : qsTr("Wahrheitstabelle"))
                        : workbench.guessing ? qsTr("Was vermutest du?") : qsTr("Was passiert?")
                    color: ZTheme.navy
                    font.pixelSize: 18; font.bold: true
                }
                Text {
                    visible: workbench.guessing
                    Layout.fillWidth: true
                    text: qsTr("Tippe in der Spalte »Vermutung« für jede Schalterstellung: 1 = an, 0 = aus.")
                    color: ZTheme.muted
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                }
                TruthPanel { visible: !root.counting && !root.free; Layout.fillWidth: true }
                Repeater { // Art »zahlen«: Zahlen, die an den Lampen erscheinen sollen
                    model: workbench.tasks
                    delegate: Rectangle {
                        id: task
                        required property var modelData
                        Layout.fillWidth: true
                        implicitHeight: 44
                        radius: 8
                        color: modelData.done ? ZTheme.okBg : ZTheme.surfaceHigh
                        border.width: modelData.current ? 2 : 0
                        border.color: ZTheme.accent
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            Text {
                                Layout.fillWidth: true
                                text: task.modelData.text ? task.modelData.text
                                                          : qsTr("Zeige die %1").arg(task.modelData.value)
                                wrapMode: Text.WordWrap
                                color: ZTheme.text
                                font.pixelSize: 16; font.bold: task.modelData.current
                            }
                            Text { // binaer erst, wenn geschafft – sonst waere die Loesung verraten
                                text: !task.modelData.done ? ""
                                    : task.modelData.text ? "= " + task.modelData.value + "  ✓"
                                    : (task.modelData.value + 16).toString(2).slice(1) + "  ✓"
                                color: ZTheme.ok
                                font.family: ZTheme.mono; font.pixelSize: 16; font.bold: true
                            }
                        }
                    }
                }
                Rectangle { // Merksatz nach dem Schaffen der Stufe
                    visible: (workbench.stageDone || root.demo) && workbench.stageMerksatz !== ""
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    Layout.preferredHeight: merk.implicitHeight + 20
                    radius: 12
                    color: "#FFF7E6"
                    border.color: ZTheme.pad
                    Text {
                        id: merk
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 12 }
                        wrapMode: Text.WordWrap
                        text: qsTr("Merke: ") + workbench.stageMerksatz
                        color: ZTheme.text
                        font.pixelSize: 15; font.bold: true
                    }
                }
            }
        }

        // ---- fest unten: Aktion
        ZButton { // ohne Vermutung weiter
            visible: workbench.guessing
            Layout.fillWidth: true
            text: qsTr("Weiß ich nicht – ausprobieren")
            onClicked: workbench.skipGuess()
        }
        ZButton { // entdecken
            visible: workbench.stageKind === "entdecken" && !workbench.stageDone && !workbench.guessing
            Layout.fillWidth: true
            text: qsTr("Diese Kiste passt!")
            primary: workbench.boxIndex >= 0
            onClicked: workbench.confirmBox()
        }
        ZButton { // bauen
            visible: root.building && !workbench.stageDone && !root.free && !root.demo
            Layout.fillWidth: true
            text: qsTr("Prüfen")
            primary: !workbench.chipReady
            enabled: !workbench.inside
            onClicked: workbench.check()
        }
        ZButton {
            visible: root.building && !workbench.stageDone && !root.free && !root.demo
            Layout.fillWidth: true
            text: workbench.stageMakesChip ? qsTr("Erfindung einpacken") : qsTr("Geschafft!")
            primary: workbench.chipReady
            enabled: workbench.chipReady
            onClicked: workbench.createChip()
        }
        ZButton { // Vorfuehrung wie Volta: andere zeigen oder beenden (zurueck zur Startseite)
            visible: root.demo
            Layout.fillWidth: true
            text: qsTr("Andere Vorführung")
            onClicked: root.demoPickerRequested()
        }
        ZButton {
            visible: root.demo || root.free
            Layout.fillWidth: true
            text: root.demo ? qsTr("Vorführung beenden") : qsTr("Zur Startseite")
            primary: root.demo
            onClicked: root.homeRequested()
        }
        ZButton {
            visible: workbench.stageDone && root.hasNext && !root.demo
            Layout.fillWidth: true
            text: qsTr("Weiter") + " →"
            primary: true
            onClicked: workbench.selectStage(workbench.stageIndex + 1)
        }
    }
}
