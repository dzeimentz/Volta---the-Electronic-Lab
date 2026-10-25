import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Aufgabe der Lehrkraft fuer das Kind (wie Volta): Code eintippen (oder einen ganzen Link einfuegen), dann zeigt Zetti,
// was zu tun ist; Antippen startet ein Experiment bzw. bringt den Aufbau ins freie Labor. Die Aufgabe bleibt
// gespeichert (je Profil), bis eine neue kommt oder sie entfernt wird.
Popup {
    id: dlg
    signal experimentRequested(int index)
    signal boardRequested()

    readonly property var task: workbench.teacherTask
    readonly property bool hasTask: task.ok === true
    property bool entering: false // Code eingeben statt Aufgabe zeigen
    property string message: ""

    function tryCode(text) {
        const info = workbench.readTask(text)
        if (workbench.acceptTask(text)) { entering = false; message = ""; return }
        message = info.otherBand ? qsTr("Dieser Code gehört zu einer anderen App der Reihe, zum Beispiel zu Volta.")
                                 : qsTr("Der Code stimmt nicht. Prüfe jeden Buchstaben und jede Zahl.")
    }

    objectName: "taskDialog"
    anchors.centerIn: Overlay.overlay
    width: Math.min(640, parent ? parent.width - 32 : 640)
    modal: true
    focus: true
    padding: 20
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onAboutToShow: {
        entering = !hasTask
        message = ""
        codeField.text = ""
        codeField2.text = ""
    }
    onOpened: if (entering) codeField.forceActiveFocus()
    onEnteringChanged: if (entering && opened) Qt.callLater(() => codeField.forceActiveFocus())
    background: Rectangle { radius: 16; color: "#FFFDF7"; border.color: ZTheme.navy; border.width: 2 }

    contentItem: ColumnLayout {
        spacing: 14

        RowLayout { // Zetti spricht
            Layout.fillWidth: true
            spacing: 12
            Image {
                Layout.preferredWidth: 76; Layout.preferredHeight: 96
                source: dlg.message !== "" ? "qrc:/zuse/zetti/zetti_gruebelnd.png"
                      : dlg.entering ? "qrc:/zuse/zetti/zetti_freundlich.png" : "qrc:/zuse/zetti/zetti_losgehts.png"
                fillMode: Image.PreserveAspectFit
                mipmap: true
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: say.implicitHeight + 24
                radius: 14
                color: ZTheme.bubble
                border.width: 2
                border.color: dlg.message !== "" ? ZTheme.bad : ZTheme.accentDim
                Text {
                    id: say
                    objectName: "taskSay"
                    anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 12 }
                    wrapMode: Text.WordWrap
                    color: ZTheme.text
                    font.pixelSize: Math.round(16 * ZTheme.fontScale)
                    text: dlg.message !== "" ? dlg.message
                        : dlg.entering ? qsTr("Hat dir deine Lehrkraft einen Code gegeben? Tippe ihn hier ein.")
                        : qsTr("Deine Lehrkraft hat eine Aufgabe für dich!")
                          + ((dlg.task.text || "") !== "" ? "\n»" + dlg.task.text + "«" : "")
                }
            }
        }

        // ---- Code eingeben: zwei Haelften mit festem Strich; ein eingefuegter ganzer Code oder Link wird erkannt
        RowLayout {
            visible: dlg.entering
            spacing: 10
            component CodePart: TextField {
                Layout.preferredWidth: 120
                implicitHeight: ZTheme.control + 8
                horizontalAlignment: TextInput.AlignHCenter
                placeholderText: "····"
                font.pixelSize: 26
                font.family: ZTheme.mono
                font.capitalization: Font.AllUppercase
                inputMethodHints: Qt.ImhUppercaseOnly | Qt.ImhNoPredictiveText | Qt.ImhPreferLatin
                onAccepted: if (openButton.enabled) openButton.clicked()
            }
            CodePart {
                id: codeField
                objectName: "taskCodeField"
                maximumLength: 4096
                onTextChanged: {
                    if (text.indexOf("://") >= 0) { // ganzer Link eingefuegt
                        const link = text
                        text = ""
                        dlg.tryCode(link)
                        return
                    }
                    const clean = text.replace(/[^0-9A-Za-z]/g, "")
                    if (clean.length > 4 || clean !== text) {
                        codeField2.text = clean.substring(4, 8)
                        text = clean.substring(0, 4)
                    }
                    if (text.length === 4 && activeFocus) codeField2.forceActiveFocus()
                }
                Accessible.name: qsTr("Code der Lehrkraft, erster Teil")
            }
            Text {
                text: "–"
                color: ZTheme.navy
                font.pixelSize: 30; font.bold: true
                Accessible.ignored: true
            }
            CodePart {
                id: codeField2
                objectName: "taskCodeField2"
                maximumLength: 4
                validator: RegularExpressionValidator { regularExpression: /[0-9A-Za-z]{0,4}/ }
                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Backspace && text.length === 0) { codeField.forceActiveFocus(); event.accepted = true }
                }
                Accessible.name: qsTr("Code der Lehrkraft, zweiter Teil")
            }
            ZButton {
                id: openButton
                objectName: "taskCodeOpen"
                primary: true
                text: qsTr("Aufgabe öffnen")
                enabled: codeField.text.length === 4 && codeField2.text.length === 4
                onClicked: dlg.tryCode(codeField.text + "-" + codeField2.text)
            }
        }

        // ---- Aufgabe zeigen
        Repeater {
            model: !dlg.entering && dlg.hasTask ? dlg.task.experiments : []
            delegate: ZButton {
                required property var modelData
                required property int index
                objectName: "taskExperiment_" + index
                Layout.fillWidth: true
                text: (modelData.done ? "✓  " : "▶  ") + modelData.label + "  " + modelData.title
                      + (modelData.done ? "  (" + qsTr("geschafft") + ")" : "")
                onClicked: { dlg.close(); dlg.experimentRequested(modelData.index) }
            }
        }
        ZButton {
            objectName: "taskBoard"
            visible: !dlg.entering && dlg.hasTask && dlg.task.hasBoard === true
            Layout.fillWidth: true
            text: "▶  " + qsTr("Aufbau öffnen: %1").arg(dlg.task.title || qsTr("Aufbau der Lehrkraft"))
            onClicked: { dlg.close(); dlg.boardRequested() }
        }
        Text {
            visible: !dlg.entering && dlg.hasTask && dlg.task.hasBoard === true && workbench.freeLabParts > 0
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Der Aufbau kommt ins freie Labor und ersetzt, was dort gerade steht.")
            color: ZTheme.muted
            font.pixelSize: Math.round(14 * ZTheme.fontScale)
        }

        Flow {
            Layout.fillWidth: true
            Layout.topMargin: 4
            spacing: 10
            layoutDirection: Qt.RightToLeft
            ZButton { objectName: "taskClose"; text: qsTr("Schließen"); onClicked: dlg.close() }
            ZButton {
                objectName: "taskRemove"
                visible: !dlg.entering && dlg.hasTask
                text: qsTr("Aufgabe entfernen")
                onClicked: { workbench.clearTask(); dlg.close() }
            }
            ZButton {
                objectName: "taskNewCode"
                visible: !dlg.entering
                text: qsTr("Anderen Code eingeben")
                onClicked: { dlg.entering = true; dlg.message = ""; codeField.text = ""; codeField2.text = "" }
            }
        }
    }
}
