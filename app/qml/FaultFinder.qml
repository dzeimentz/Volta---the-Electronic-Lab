// Fehlersuche im Werkzeugkasten (docs/ui-spec.md, ab Stufe 2): prueft auf Wunsch den Aufbau und nennt
// Auffaelligkeiten als Fragen, nie als Loesung (Modell: model/circuit_check). Jede Frage hat eine Nummer, die auch
// als Marke an der Stelle auf der Grundplatte erscheint; Antippen hebt die Stelle hervor.
import QtQuick
import QtQuick.Controls.Basic

Flickable {
    id: finder
    required property var controller
    readonly property bool unlocked: controller.unlockedStage >= 2
    property var findings: []
    property bool checked: false // mindestens einmal geprueft (seit der letzten Aenderung)
    signal findingSelected(var anchors, string partId)
    signal checkResults(var list)

    contentHeight: col.implicitHeight + 16
    clip: true

    function check() {
        finder.findings = finder.controller.checkCircuit()
        finder.checked = true
        finder.checkResults(finder.findings)
    }
    function reset() {
        if (!finder.checked) return
        finder.findings = []
        finder.checked = false
        finder.checkResults([])
    }
    // Aenderung am Aufbau: alte Befunde gelten nicht mehr
    Connections {
        target: finder.controller
        function onBoardChanged() { finder.reset() }
        function onExperimentChanged() { finder.reset() }
    }

    Column {
        id: col
        width: parent.width
        spacing: 10

        TickSays {
            width: parent.width
            figure: 76
            visible: !finder.unlocked
            mood: "nachdenklich"
            text: "🔒 " + qsTr("Die Fehlersuche wird mit Stufe 2 freigeschaltet.")
            textColor: "#4A5866"
        }
        TickSays {
            objectName: "checkIntro"
            width: parent.width
            figure: 76
            visible: finder.unlocked && !finder.checked
            mood: "neugierig"
            text: qsTr("Funktioniert etwas nicht? Ich schaue mir deinen Aufbau an und stelle dir Fragen dazu.")
        }
        Button {
            objectName: "checkCircuitButton"
            visible: finder.unlocked
            width: parent.width
            height: Theme.control
            text: finder.checked ? qsTr("Nochmal prüfen") : qsTr("Schaltung prüfen")
            font.bold: true
            onClicked: finder.check()
        }
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            visible: finder.unlocked && !finder.controller.mainSwitch
            text: qsTr("Tipp: Schalte den Hauptschalter ein, dann schaue ich auch nach, was gerade fließt.")
            font.pixelSize: Math.round(12 * Theme.fontScale)
            color: "#4A5866"
        }

        // Ergebnis
        TickSays {
            objectName: "checkNothingFound"
            width: parent.width
            figure: 76
            visible: finder.checked && finder.findings.length === 0
            mood: "gut"
            text: "✓ " + qsTr("Ich habe nichts Auffälliges gefunden. Macht die Schaltung, was du erwartest? Vergleiche mit dem Schaltplan.")
            textColor: "#2E7D32"
        }
        TickSays {
            objectName: "checkSomethingFound"
            width: parent.width
            figure: 76
            visible: finder.checked && finder.findings.length > 0
            mood: "fragend"
            text: qsTr("Mir ist etwas aufgefallen. Tippe auf eine Frage, dann zeige ich dir die Stelle.")
            bubbleColor: "#FFF4D6"
            borderColor: Theme.solderAmber
        }
        Repeater {
            model: finder.findings
            Rectangle {
                id: item
                required property int index
                required property var modelData
                objectName: "finding_" + index
                width: col.width
                height: Math.max(Theme.control, text.implicitHeight + 16)
                radius: 8
                color: tap.pressed ? "#FFE7C2" : "#FFF4D6"
                border.color: Theme.solderAmber
                border.width: 1.5
                Rectangle {
                    id: badge
                    x: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 28; height: 28; radius: 14
                    color: Theme.signalOrange
                    Label { anchors.centerIn: parent; text: item.index + 1; color: "white"; font.bold: true; font.pixelSize: 15 }
                }
                Label {
                    id: text
                    x: badge.x + badge.width + 10
                    width: parent.width - x - 8
                    anchors.verticalCenter: parent.verticalCenter
                    wrapMode: Text.Wrap
                    text: item.modelData.text
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    color: "#1E2B38"
                }
                TapHandler {
                    id: tap
                    onTapped: finder.findingSelected(item.modelData.anchors, item.modelData.part)
                }
                Accessible.role: Accessible.Button
                Accessible.name: (item.index + 1) + ": " + item.modelData.text
            }
        }
    }
}
