// Elternschranke (docs/ui-spec.md, docs/privacy-safety.md): eine Rechenaufgabe mit Zahlen in Worten schuetzt
// Elternbereich und externe Links. Eingabe ueber einen Ziffernblock (>= 48 dp); nach drei Fehlversuchen kommt eine
// neue Aufgabe. ask(action) oeffnet die Schranke und ruft action() erst nach der richtigen Antwort auf.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: gate
    required property var controller
    property var task: ({ a: 0, b: 0 })
    property string answer: ""
    property int tries: 0
    property var pending: null // Aktion nach bestandener Aufgabe

    function ask(action) {
        gate.pending = action
        gate.newTask()
        gate.open()
    }
    function newTask() {
        gate.task = gate.controller.newParentGateTask()
        gate.answer = ""
        gate.tries = 0
        wrong.visible = false
    }
    function submit() {
        if (gate.answer === "") return
        if (gate.controller.checkParentGate(parseInt(gate.answer))) {
            const action = gate.pending
            gate.pending = null
            gate.close()
            if (action) action()
            return
        }
        gate.answer = ""
        gate.tries += 1
        wrong.visible = true
        if (gate.tries >= 3) { gate.task = gate.controller.newParentGateTask(); gate.tries = 0 }
    }

    objectName: "parentGate"
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(420, parent ? parent.width - 32 : 420)
    padding: 20
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onClosed: gate.pending = null
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    contentItem: Column {
        spacing: 12
        Label {
            width: parent.width
            text: qsTr("Für Erwachsene")
            font.pixelSize: Math.round(18 * Theme.fontScale)
            font.bold: true
            color: Theme.navy
        }
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            text: qsTr("Bitte lösen Sie diese Aufgabe, um fortzufahren.")
            font.pixelSize: Math.round(14 * Theme.fontScale)
            color: "#1E2B38"
        }
        Label {
            objectName: "gateQuestion"
            width: parent.width
            wrapMode: Text.Wrap
            text: qsTr("Wie viel ist %1 mal %2?").arg(Texts.numberWord(gate.task.a)).arg(Texts.numberWord(gate.task.b))
            font.pixelSize: Math.round(18 * Theme.fontScale)
            font.bold: true
            color: Theme.navy
        }
        Rectangle {
            width: parent.width
            height: Theme.control
            radius: 6
            color: "white"
            border.color: Theme.fieldOutline
            Label {
                objectName: "gateAnswer"
                anchors.centerIn: parent
                text: gate.answer === "" ? " " : gate.answer
                font.pixelSize: Math.round(22 * Theme.fontScale)
                font.bold: true
                color: Theme.navy
            }
        }
        Label {
            id: wrong
            visible: false
            width: parent.width
            wrapMode: Text.Wrap
            text: gate.tries === 0 ? qsTr("Nicht richtig. Hier ist eine neue Aufgabe.") : qsTr("Nicht richtig. Bitte versuchen Sie es noch einmal.")
            font.pixelSize: Math.round(13 * Theme.fontScale)
            color: "#B23B1E"
        }
        Grid {
            anchors.horizontalCenter: parent.horizontalCenter
            columns: 3
            spacing: 8
            Repeater {
                model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "del", "0", "ok"]
                Button {
                    required property string modelData
                    objectName: "gateKey_" + modelData
                    width: 72; height: Theme.control
                    text: modelData === "ok" ? qsTr("OK") : modelData === "del" ? "" : modelData
                    font.pixelSize: Math.round(18 * Theme.fontScale)
                    font.bold: modelData === "ok"
                    contentItem: Item {
                        Label {
                            anchors.centerIn: parent
                            visible: modelData !== "del"
                            text: parent.parent.text
                            font: parent.parent.font
                            color: Theme.navy
                        }
                        DrawnIcon { anchors.centerIn: parent; visible: modelData === "del"; kind: "backspace"; color: Theme.navy; size: 22 }
                    }
                    Accessible.name: modelData === "del" ? qsTr("Ziffer löschen") : text
                    onClicked: {
                        if (modelData === "ok") gate.submit()
                        else if (modelData === "del") gate.answer = gate.answer.slice(0, -1)
                        else if (gate.answer.length < 4) gate.answer += modelData
                    }
                }
            }
        }
        Button {
            objectName: "gateCancel"
            anchors.right: parent.right
            height: Theme.control
            text: qsTr("Abbrechen")
            onClicked: gate.close()
        }
    }
}
