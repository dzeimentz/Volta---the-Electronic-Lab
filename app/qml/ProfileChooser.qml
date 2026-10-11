// Forscherprofile (Konzept 8.2): »Wer forscht heute?« - Kacheln mit Symbol und Spitzname, dazu »Neues Profil«
// (Symbol waehlen, Spitzname eingeben; ausdruecklich kein richtiger Name, kein Passwort). Loeschen nur im
// Elternbereich. Daten je Profil verwaltet der LabController.
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: chooser
    required property var controller
    property bool creating: false
    property string newSymbol: ""
    signal chosen(string id)
    signal parentAreaRequested()

    objectName: "profileChooser"
    color: Theme.board
    readonly property var symbols: ["🦊", "🐼", "🦉", "🐢", "🐙", "🦄", "🐝", "🐬", "🦁", "🐸", "🐧", "🦖",
                                    "🚀", "🌟", "⚡", "🔭", "🧲", "💡", "🌈", "🍀", "🎈", "🐞", "🦋", "🐳"]
    readonly property bool full: controller.profiles.length >= controller.profileLimit
    readonly property var managed: typeof volta_managed !== "undefined" ? volta_managed : null

    // Eingaben nicht an die Ansichten darunter durchlassen
    MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onWheel: (wheel) => wheel.accepted = true }

    Rectangle {
        id: header
        width: parent.width
        height: Theme.control + 8
        color: Theme.navy
        Label {
            x: Theme.margin
            anchors.verticalCenter: parent.verticalCenter
            text: chooser.controller.appInfo().name
            color: "white"
            font.pixelSize: Math.round(20 * Theme.fontScale)
            font.bold: true
        }
        ToolButton {
            objectName: "profileParentArea"
            visible: !chooser.managed || chooser.managed.parentArea
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            height: Theme.control
            text: qsTr("Elternbereich")
            palette.buttonText: "white"
            onClicked: chooser.parentAreaRequested()
        }
    }

    Flickable {
        anchors { left: parent.left; right: parent.right; top: header.bottom; bottom: parent.bottom }
        contentHeight: col.implicitHeight + 2 * Theme.margin
        clip: true

        Column {
            id: col
            x: Theme.margin
            y: Theme.margin
            width: parent.width - 2 * Theme.margin
            spacing: 18

            Label {
                text: chooser.creating ? qsTr("Neues Forscherprofil") : qsTr("Wer forscht heute?")
                font.pixelSize: Math.round(26 * Theme.fontScale)
                font.bold: true
                color: Theme.navy
            }

            // ---- Auswahl
            Flow {
                visible: !chooser.creating
                width: parent.width
                spacing: 14
                Repeater {
                    model: chooser.controller.profiles
                    Rectangle {
                        id: tile
                        required property var modelData
                        objectName: "profile_" + modelData.id
                        width: 150; height: 150
                        radius: 16
                        color: tap.pressed ? "#F2C894" : "#F7E3C2"
                        border.color: Theme.navy
                        border.width: 2
                        Column {
                            anchors.centerIn: parent
                            spacing: 6
                            Label { anchors.horizontalCenter: parent.horizontalCenter; text: tile.modelData.symbol; font.pixelSize: 56 }
                            Label {
                                width: tile.width - 16
                                horizontalAlignment: Text.AlignHCenter
                                elide: Text.ElideRight
                                text: tile.modelData.name
                                font.pixelSize: Math.round(17 * Theme.fontScale)
                                font.bold: true
                                color: Theme.navy
                            }
                        }
                        TapHandler { id: tap; onTapped: chooser.chosen(tile.modelData.id) }
                        Accessible.role: Accessible.Button
                        Accessible.name: tile.modelData.name
                    }
                }
                Rectangle {
                    objectName: "profileNew"
                    visible: !chooser.full
                    width: 150; height: 150
                    radius: 16
                    color: newTap.pressed ? "#D6E6F5" : "#EEF4FA"
                    border.color: Theme.accent
                    border.width: 2
                    Column {
                        anchors.centerIn: parent
                        spacing: 6
                        Label { anchors.horizontalCenter: parent.horizontalCenter; text: "+"; font.pixelSize: 56; color: Theme.accent }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Neues Profil")
                            font.pixelSize: Math.round(16 * Theme.fontScale)
                            font.bold: true
                            color: Theme.navy
                        }
                    }
                    TapHandler { id: newTap; onTapped: { chooser.newSymbol = ""; nameField.text = ""; chooser.creating = true } }
                    Accessible.role: Accessible.Button
                    Accessible.name: qsTr("Neues Profil")
                }
            }

            // ---- Neues Profil
            Label {
                visible: chooser.creating
                text: qsTr("1. Such dir ein Symbol aus:")
                font.pixelSize: Math.round(16 * Theme.fontScale)
                color: "#1E2B38"
            }
            Flow {
                visible: chooser.creating
                width: parent.width
                spacing: 8
                Repeater {
                    model: chooser.symbols
                    Rectangle {
                        required property string modelData
                        required property int index
                        objectName: "profileSymbol_" + index
                        width: 64; height: 64
                        radius: 12
                        color: chooser.newSymbol === modelData ? Theme.solderAmber : "white"
                        border.color: chooser.newSymbol === modelData ? Theme.signalOrange : Theme.fieldOutline
                        border.width: chooser.newSymbol === modelData ? 4 : 1
                        Label { anchors.centerIn: parent; text: parent.modelData; font.pixelSize: 36 }
                        TapHandler { onTapped: chooser.newSymbol = parent.modelData }
                    }
                }
            }
            Label {
                visible: chooser.creating
                text: qsTr("2. Gib dir einen Spitznamen:")
                font.pixelSize: Math.round(16 * Theme.fontScale)
                color: "#1E2B38"
            }
            TextField {
                id: nameField
                objectName: "profileName"
                visible: chooser.creating
                width: Math.min(400, parent.width)
                height: Theme.control
                maximumLength: 16
                placeholderText: qsTr("Spitzname")
                font.pixelSize: Math.round(18 * Theme.fontScale)
            }
            Label {
                visible: chooser.creating
                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("Bitte nicht deinen richtigen Namen – ein Spitzname reicht, zum Beispiel »Funkenfuchs«.")
                font.pixelSize: Math.round(13 * Theme.fontScale)
                color: "#4A5866"
            }
            Row {
                visible: chooser.creating
                spacing: 12
                Button {
                    objectName: "profileCreate"
                    height: Theme.control
                    enabled: chooser.newSymbol !== "" && nameField.text.trim() !== ""
                    text: qsTr("Los geht's")
                    font.bold: true
                    onClicked: {
                        const id = chooser.controller.addProfile(chooser.newSymbol, nameField.text)
                        chooser.creating = false
                        if (id !== "") chooser.chosen(id)
                    }
                }
                Button {
                    objectName: "profileCreateCancel"
                    height: Theme.control
                    text: qsTr("Abbrechen")
                    onClicked: chooser.creating = false
                }
            }
        }
    }
}
