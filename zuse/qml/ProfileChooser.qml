import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Forscherprofile wie Volta: »Wer forscht heute?« – Kacheln mit Symbol und Spitzname, dazu »Neues Profil« (Symbol
// waehlen, Spitzname eingeben; ausdruecklich kein richtiger Name, kein Passwort). Loeschen nur im Elternbereich.
Rectangle {
    id: chooser
    objectName: "profileChooser"
    property bool creating: false
    property string newSymbol: ""
    signal chosen(string id)
    color: ZTheme.graphite

    readonly property var symbols: ["🦊", "🐼", "🦉", "🐢", "🐙", "🦄", "🐝", "🐬", "🦁", "🐸", "🐧", "🦖",
                                    "🚀", "🌟", "⚡", "🔭", "🧲", "💡", "🌈", "🍀", "🎈", "🐞", "🦋", "🐳"]
    readonly property bool full: workbench.profiles.length >= 30

    // Eingaben nicht an die Seiten darunter durchlassen
    MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onWheel: (wheel) => wheel.accepted = true }

    component ProfileTile: Rectangle {
        id: tile
        property string symbol
        property string name
        property bool add: false
        signal activated()
        width: 160; height: 160
        radius: 16
        color: tap.pressed ? "#D9F3F8" : add ? "#F3FAFC" : ZTheme.surface
        border.width: 2
        border.color: add ? ZTheme.accent : ZTheme.navy
        ColumnLayout {
            anchors.centerIn: parent
            width: parent.width - 16
            spacing: 6
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: tile.symbol
                color: ZTheme.accentText
                font.pixelSize: 56; font.family: "Segoe UI Emoji"
            }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
                text: tile.name
                color: ZTheme.navy
                font.pixelSize: Math.round(17 * ZTheme.fontScale); font.bold: true
            }
        }
        TapHandler { id: tap; onTapped: tile.activated() }
        Accessible.role: Accessible.Button
        Accessible.name: name
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: col.implicitHeight + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: col
            x: Math.max(24, (flick.width - width) / 2)
            y: 24
            width: Math.min(flick.width - 48, 920)
            spacing: 18

            RowLayout { // Zetti fragt
                Layout.fillWidth: true
                spacing: 14
                Image {
                    Layout.preferredWidth: 96; Layout.preferredHeight: 120
                    source: "qrc:/zuse/zetti/zetti_freundlich.png"
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                }
                Text {
                    Layout.fillWidth: true
                    text: chooser.creating ? qsTr("Neues Forscherprofil") : qsTr("Wer forscht heute?")
                    color: ZTheme.navy
                    font.pixelSize: Math.round(28 * ZTheme.fontScale); font.bold: true
                    wrapMode: Text.WordWrap
                }
            }

            // ---- Auswahl
            Flow {
                visible: !chooser.creating
                Layout.fillWidth: true
                spacing: 14
                Repeater {
                    model: workbench.profiles
                    delegate: ProfileTile {
                        required property var modelData
                        objectName: "profile_" + modelData.id
                        symbol: modelData.symbol
                        name: modelData.name
                        onActivated: chooser.chosen(modelData.id)
                    }
                }
                ProfileTile {
                    objectName: "profileNew"
                    visible: !chooser.full
                    add: true
                    symbol: "+"
                    name: qsTr("Neues Profil")
                    onActivated: { chooser.newSymbol = ""; nameField.text = ""; chooser.creating = true }
                }
            }

            // ---- Neues Profil
            Text {
                visible: chooser.creating
                text: qsTr("1. Such dir ein Symbol aus:")
                color: ZTheme.text
                font.pixelSize: Math.round(17 * ZTheme.fontScale)
            }
            Flow {
                visible: chooser.creating
                Layout.fillWidth: true
                spacing: 8
                Repeater {
                    model: chooser.symbols
                    delegate: Rectangle {
                        required property string modelData
                        required property int index
                        readonly property bool picked: chooser.newSymbol === modelData
                        objectName: "profileSymbol_" + index
                        width: 64; height: 64
                        radius: 12
                        color: picked ? ZTheme.accentDim : ZTheme.surface
                        border.color: picked ? ZTheme.navy : ZTheme.line
                        border.width: picked ? 4 : 1 // Rahmen, nicht nur Farbe
                        Text { anchors.centerIn: parent; text: parent.modelData; font.pixelSize: 36; font.family: "Segoe UI Emoji" }
                        TapHandler { onTapped: chooser.newSymbol = parent.modelData }
                        Accessible.role: Accessible.RadioButton
                        Accessible.name: qsTr("Symbol %1").arg(modelData) + (picked ? ", " + qsTr("ausgewählt") : "")
                    }
                }
            }
            Text {
                visible: chooser.creating
                text: qsTr("2. Gib dir einen Spitznamen:")
                color: ZTheme.text
                font.pixelSize: Math.round(17 * ZTheme.fontScale)
            }
            TextField {
                id: nameField
                objectName: "profileName"
                visible: chooser.creating
                Layout.preferredWidth: Math.min(400, col.width)
                implicitHeight: ZTheme.control
                maximumLength: 16
                placeholderText: qsTr("Spitzname")
                font.pixelSize: Math.round(18 * ZTheme.fontScale)
            }
            Text {
                visible: chooser.creating
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("Bitte nicht deinen richtigen Namen – ein Spitzname reicht, zum Beispiel »Funkenfuchs«.")
                color: ZTheme.muted
                font.pixelSize: Math.round(14 * ZTheme.fontScale)
            }
            RowLayout {
                visible: chooser.creating
                spacing: 12
                ZButton {
                    objectName: "profileCreate"
                    primary: true
                    enabled: chooser.newSymbol !== "" && nameField.text.trim() !== ""
                    text: qsTr("Los geht’s")
                    onClicked: {
                        const id = workbench.addProfile(chooser.newSymbol, nameField.text)
                        chooser.creating = false
                        if (id !== "") chooser.chosen(id)
                    }
                }
                ZButton { objectName: "profileCreateCancel"; text: qsTr("Abbrechen"); onClicked: chooser.creating = false }
            }
        }
    }
}
