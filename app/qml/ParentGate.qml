// Elternschranke (docs/ui-spec.md, docs/privacy-safety.md): PIN fuer Erwachsene (Start-PIN 1234, im Elternbereich
// aenderbar) schuetzt Elternbereich und externe Links. Eingabe ueber einen Ziffernblock (>= 48 dp), verdeckt; nach
// fuenf Fehlversuchen 30 s Sperre (AppSettings). ask(action) ruft action() erst nach der richtigen PIN auf.
// Am Rechner auch per Tastatur: Ziffern (auch Ziffernblock), Ruecktaste/Entf loescht, Eingabe bestaetigt.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: gate
    property var settings: null
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
        gate.lockSeconds = gate.settings ? gate.settings.parentPinLockSeconds() : 0
        gate.open()
    }
    function typeDigit(d) {
        if (gate.lockSeconds > 0 || gate.pin.length >= 8) return
        gate.pin += d
        gate.wrong = false
    }
    function submit() {
        if (gate.pin === "" || !gate.settings) return
        if (gate.settings.checkParentPin(gate.pin)) {
            const action = gate.pending
            gate.pending = null
            gate.close()
            if (action) action()
            return
        }
        gate.pin = ""
        gate.wrong = true
        gate.lockSeconds = gate.settings.parentPinLockSeconds()
    }
    Timer {
        interval: 1000
        repeat: true
        running: gate.opened && gate.lockSeconds > 0
        onTriggered: gate.lockSeconds = gate.settings ? gate.settings.parentPinLockSeconds() : 0
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
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    contentItem: Column {
        spacing: 12
        // Tastatur: haelt den Fokus (die Knoepfe nehmen ihn nicht), Escape bleibt beim Popup (schliesst)
        Item {
            id: keyCatcher
            objectName: "gateKeys"
            width: 0; height: 0
            focus: true
            Keys.onPressed: (event) => {
                if (event.text.length === 1 && event.text >= "0" && event.text <= "9") { gate.typeDigit(event.text); event.accepted = true }
                else if (event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete) { gate.pin = gate.pin.slice(0, -1); event.accepted = true }
                else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) { gate.submit(); event.accepted = true }
            }
        }
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
            text: qsTr("Bitte geben Sie die PIN für den Elternbereich ein.")
            font.pixelSize: Math.round(14 * Theme.fontScale)
            color: "#1E2B38"
        }
        Rectangle {
            width: parent.width
            height: Theme.control
            radius: 6
            color: "white"
            border.color: Theme.fieldOutline
            Label {
                objectName: "gatePin"
                anchors.centerIn: parent
                text: gate.pin === "" ? " " : "●".repeat(gate.pin.length) // verdeckt (Punkte)
                font.pixelSize: Math.round(22 * Theme.fontScale)
                font.letterSpacing: 6
                color: Theme.navy
            }
        }
        Label {
            objectName: "gateMessage"
            visible: text !== ""
            width: parent.width
            wrapMode: Text.Wrap
            text: gate.lockSeconds > 0 ? qsTr("Zu viele Versuche. Bitte warten Sie %1 Sekunden.").arg(gate.lockSeconds)
                : gate.wrong ? qsTr("Die PIN stimmt nicht.") : ""
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
                    focusPolicy: Qt.NoFocus // Fokus bleibt bei der Tastatureingabe
                    enabled: gate.lockSeconds === 0
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
                        else if (modelData === "del") gate.pin = gate.pin.slice(0, -1)
                        else gate.typeDigit(modelData)
                    }
                }
            }
        }
        Label {
            objectName: "gateForgot"
            width: parent.width
            wrapMode: Text.Wrap
            visible: gate.forgotShown
            text: qsTr("PIN vergessen? Dann lässt sich Volta nur zurücksetzen, indem Sie die App-Daten löschen (Android: Einstellungen, Apps, Volta 1, Speicher, Daten löschen). Dabei gehen auch Fortschritt und Forscherheft verloren.")
            font.pixelSize: Math.round(12 * Theme.fontScale)
            color: "#4A5866"
        }
        Row {
            anchors.right: parent.right
            spacing: 12
            Button {
                objectName: "gateForgotButton"
                focusPolicy: Qt.NoFocus
                visible: !gate.forgotShown
                height: Theme.control
                flat: true
                text: qsTr("PIN vergessen?")
                onClicked: gate.forgotShown = true
            }
            Button {
                objectName: "gateCancel"
                height: Theme.control
                text: qsTr("Abbrechen")
                onClicked: gate.close()
            }
        }
    }
}
