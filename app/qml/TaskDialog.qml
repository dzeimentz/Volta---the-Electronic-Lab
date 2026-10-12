// Aufgabe der Lehrkraft fuer das Kind (Main.qml): ueber den QR-Code (Kamera-App oeffnet Volta) oder den Code zum
// Eintippen. Tick zeigt, was zu tun ist; Antippen startet ein Experiment bzw. oeffnet den eigenen Aufbau.
// Die Aufgabe bleibt gespeichert (je Profil), bis eine neue kommt oder sie entfernt wird.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: dlg
    required property var controller
    signal experimentRequested(string id)
    signal boardRequested()

    readonly property var task: controller.task
    readonly property bool hasTask: task.ok === true
    property bool entering: false // Code eingeben statt Aufgabe zeigen
    property string message: ""

    // Link aus dem QR-Code: uebernehmen und zeigen
    function showLink(link) {
        if (controller.acceptTask(link)) { entering = false; message = "" }
        else { entering = true; message = qsTr("Mit diesem Code kann Volta nichts anfangen.") }
        open()
    }
    // von der Startseite: vorhandene Aufgabe zeigen, sonst Code eingeben
    function openTask() {
        entering = !hasTask
        message = ""
        codeField.text = ""
        codeField2.text = ""
        open()
    }

    // beim Eingeben gleich mit dem Cursor im ersten Feld beginnen
    onOpened: if (entering) codeField.forceActiveFocus()
    onEnteringChanged: if (entering && opened) Qt.callLater(() => codeField.forceActiveFocus())
    objectName: "taskDialog"
    anchors.centerIn: Overlay.overlay
    width: Math.min(620, parent ? parent.width - 32 : 620)
    modal: true
    focus: true
    padding: 18
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    contentItem: Column {
        spacing: 12

        // ---- Code eingeben
        TickSays {
            visible: dlg.entering
            width: parent.width
            figure: 84
            mood: dlg.message !== "" ? "fragend" : "neugierig"
            text: dlg.message !== "" ? dlg.message : qsTr("Hat dir deine Lehrkraft einen Code gegeben? Tippe ihn hier ein.")
        }
        Row {
            visible: dlg.entering
            spacing: 10
            // Code in zwei Haelften mit festem Trennstrich: ist die erste voll, geht es in der zweiten weiter; ein
            // eingefuegter ganzer Code (mit oder ohne Strich) wird aufgeteilt; Ruecktaste in der leeren zweiten Haelfte
            // springt zurueck
            component CodePart: TextField {
                width: 112
                height: Theme.control
                horizontalAlignment: TextInput.AlignHCenter
                placeholderText: "····"
                font.pixelSize: 24
                font.family: "monospace"
                font.capitalization: Font.AllUppercase
                inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                validator: RegularExpressionValidator { regularExpression: /[0-9A-Za-z]{0,4}/ }
                onAccepted: if (openButton.enabled) openButton.clicked()
            }
            CodePart {
                id: codeField
                objectName: "taskCodeField"
                // ganzer Code eingefuegt: der Pruefer laesst ihn erst gar nicht durch, darum vorher abfangen
                validator: null
                maximumLength: 12
                onTextChanged: {
                    const clean = text.replace(/[^0-9A-Za-z]/g, "")
                    if (clean.length > 4 || clean !== text) {
                        codeField2.text = clean.substring(4, 8)
                        text = clean.substring(0, 4)
                    }
                    if (text.length === 4 && activeFocus) codeField2.forceActiveFocus()
                }
                Accessible.name: qsTr("Code der Lehrkraft, erster Teil")
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: "–"
                font.pixelSize: 28
                font.bold: true
                color: Theme.navy
                Accessible.ignored: true
            }
            CodePart {
                id: codeField2
                objectName: "taskCodeField2"
                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Backspace && text.length === 0) { codeField.forceActiveFocus(); event.accepted = true }
                }
                Accessible.name: qsTr("Code der Lehrkraft, zweiter Teil")
            }
            Button {
                id: openButton
                objectName: "taskCodeOpen"
                height: Theme.control
                text: qsTr("Aufgabe öffnen")
                font.bold: true
                enabled: codeField.text.length === 4 && codeField2.text.length === 4
                onClicked: {
                    if (dlg.controller.acceptTask(codeField.text + "-" + codeField2.text)) { dlg.entering = false; dlg.message = "" }
                    else dlg.message = qsTr("Der Code stimmt nicht. Prüfe jeden Buchstaben und jede Zahl.")
                }
            }
        }

        // ---- Aufgabe zeigen
        TickSays {
            visible: !dlg.entering && dlg.hasTask
            width: parent.width
            figure: 84
            mood: "motivierend"
            text: qsTr("Deine Lehrkraft hat eine Aufgabe für dich!")
                  + ((dlg.task.text || "") !== "" ? "\n»" + dlg.task.text + "«" : "")
        }
        Repeater {
            model: !dlg.entering && dlg.hasTask ? dlg.task.experiments : []
            Button {
                required property var modelData
                required property int index
                objectName: "taskExperiment_" + index
                width: parent.width
                height: Theme.control
                text: (modelData.done ? "✓  " : "▶  ") + qsTr("Stufe %1: %2").arg(modelData.stage).arg(modelData.title)
                font.pixelSize: Math.round(15 * Theme.fontScale)
                onClicked: { dlg.close(); dlg.experimentRequested(modelData.id) }
            }
        }
        Button {
            objectName: "taskBoard"
            visible: !dlg.entering && dlg.hasTask && dlg.task.hasBoard === true
            width: parent.width
            height: Theme.control
            text: "▶  " + qsTr("Aufbau öffnen: %1").arg(dlg.task.title || qsTr("Aufbau der Lehrkraft"))
            font.pixelSize: Math.round(15 * Theme.fontScale)
            onClicked: { dlg.close(); dlg.boardRequested() }
        }

        Row {
            anchors.right: parent.right
            spacing: 10
            Button {
                objectName: "taskNewCode"
                visible: !dlg.entering
                height: Theme.control
                text: qsTr("Anderen Code eingeben")
                onClicked: { dlg.entering = true; dlg.message = ""; codeField.text = ""; codeField2.text = "" }
            }
            Button {
                objectName: "taskRemove"
                visible: !dlg.entering && dlg.hasTask
                height: Theme.control
                text: qsTr("Aufgabe entfernen")
                onClicked: { dlg.controller.clearTask(); dlg.close() }
            }
            Button {
                objectName: "taskClose"
                height: Theme.control
                text: qsTr("Schließen")
                onClicked: dlg.close()
            }
        }
    }
}
