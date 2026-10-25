import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Elternschranke wie bei Volta: PIN fuer Erwachsene (Start-PIN 1234, im Elternbereich aenderbar) schuetzt den
// Elternbereich und Links nach aussen. Ziffernblock (>= 48 dp), Eingabe verdeckt; nach fuenf Fehlversuchen 30 s
// Sperre. Am Rechner auch per Tastatur. ask(action) ruft action() erst nach der richtigen PIN auf.
Popup {
    id: gate
    property string pin: ""
    property bool wrong: false
    property int lockSeconds: 0
    property bool forgotShown: false
    property var pending: null // Aktion nach richtiger PIN

    function ask(action) {
        gate.pending = action
        gate.pin = ""
        gate.wrong = false
        gate.forgotShown = false
        gate.lockSeconds = settings.parentPinLockSeconds()
        gate.open()
    }
    function typeDigit(d) {
        if (gate.lockSeconds > 0 || gate.pin.length >= 8) return
        gate.pin += d
        gate.wrong = false
    }
    function submit() {
        if (gate.pin === "") return
        if (settings.checkParentPin(gate.pin)) {
            const action = gate.pending
            gate.pending = null
            gate.close()
            if (action) action()
            return
        }
        gate.pin = ""
        gate.wrong = true
        gate.lockSeconds = settings.parentPinLockSeconds()
    }
    Timer {
        interval: 1000
        repeat: true
        running: gate.opened && gate.lockSeconds > 0
        onTriggered: gate.lockSeconds = settings.parentPinLockSeconds()
    }

    objectName: "parentGate"
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(420, parent ? parent.width - 32 : 420)
    padding: 20
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onClosed: gate.pending = null
    onOpened: keyCatcher.forceActiveFocus()
    onActiveFocusChanged: if (opened && !activeFocus) Qt.callLater(() => {
        if (gate.opened && !gate.activeFocus) keyCatcher.forceActiveFocus()
    })
    background: Rectangle { radius: 16; color: "#FFFDF7"; border.color: ZTheme.navy; border.width: 2 }

    contentItem: ColumnLayout {
        spacing: 12
        Item { // Tastatur: haelt den Fokus, die Knoepfe nehmen ihn nicht
            id: keyCatcher
            objectName: "gateKeys"
            Layout.preferredWidth: 0; Layout.preferredHeight: 0
            focus: true
            Keys.onPressed: (event) => {
                if (event.text.length === 1 && event.text >= "0" && event.text <= "9") { gate.typeDigit(event.text); event.accepted = true }
                else if (event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete) { gate.pin = gate.pin.slice(0, -1); event.accepted = true }
                else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) { gate.submit(); event.accepted = true }
            }
        }
        Text {
            Layout.fillWidth: true
            text: "🔒 " + qsTr("Für Erwachsene")
            color: ZTheme.navy
            font.pixelSize: Math.round(20 * ZTheme.fontScale); font.bold: true
        }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("Bitte geben Sie die PIN für den Elternbereich ein.")
            color: ZTheme.text
            font.pixelSize: Math.round(15 * ZTheme.fontScale)
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: ZTheme.control
            radius: 8
            color: ZTheme.surface
            border.color: ZTheme.line
            Text {
                objectName: "gatePin"
                anchors.centerIn: parent
                text: gate.pin === "" ? " " : "●".repeat(gate.pin.length) // verdeckt
                color: ZTheme.navy
                font.pixelSize: 22; font.letterSpacing: 6
            }
        }
        Text {
            objectName: "gateMessage"
            visible: text !== ""
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: gate.lockSeconds > 0 ? qsTr("Zu viele Versuche. Bitte warten Sie %1 Sekunden.").arg(gate.lockSeconds)
                : gate.wrong ? qsTr("Die PIN stimmt nicht.") : ""
            color: ZTheme.bad
            font.pixelSize: Math.round(14 * ZTheme.fontScale)
        }
        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            columns: 3
            columnSpacing: 8
            rowSpacing: 8
            Repeater {
                model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "del", "0", "ok"]
                delegate: ZButton {
                    required property string modelData
                    objectName: "gateKey_" + modelData
                    Layout.preferredWidth: 76
                    focusPolicy: Qt.NoFocus
                    enabled: gate.lockSeconds === 0
                    primary: modelData === "ok"
                    text: modelData === "ok" ? qsTr("OK") : modelData === "del" ? "⌫" : modelData
                    font.pixelSize: 20
                    Accessible.name: modelData === "del" ? qsTr("Ziffer löschen") : text
                    onClicked: {
                        if (modelData === "ok") gate.submit()
                        else if (modelData === "del") gate.pin = gate.pin.slice(0, -1)
                        else gate.typeDigit(modelData)
                    }
                }
            }
        }
        Text {
            objectName: "gateForgot"
            visible: gate.forgotShown
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: qsTr("PIN vergessen? Dann lässt sich Zuse nur zurücksetzen, indem Sie die App-Daten löschen. Dabei gehen auch Fortschritt, Erfindungen und Forscherheft verloren.")
            color: ZTheme.muted
            font.pixelSize: Math.round(13 * ZTheme.fontScale)
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10
            ZButton {
                objectName: "gateForgotButton"
                visible: !gate.forgotShown
                focusPolicy: Qt.NoFocus
                text: qsTr("PIN vergessen?")
                onClicked: gate.forgotShown = true
            }
            ZButton { objectName: "gateCancel"; focusPolicy: Qt.NoFocus; text: qsTr("Abbrechen"); onClicked: gate.close() }
        }
    }
}
