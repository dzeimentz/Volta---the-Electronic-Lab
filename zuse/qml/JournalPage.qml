import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

// Forscherheft wie bei Volta: je Experiment eine Heftseite mit Vermutung, Beobachtung und Vergleich (schreibt Zetti),
// dazu eigene Notizen und Fotos. Neueste Seite zuerst. Nur lokal gespeichert, nichts verlaesst das Geraet.
Rectangle {
    id: root
    signal backRequested()
    signal experimentRequested(int index)
    color: "#FBF6EA"

    property string photoTarget: "" // Experiment, zu dem das naechste Foto gehoert
    property string photoMessage: ""
    property string noteTarget: ""  // Experiment, dessen Notizfeld offen ist

    function kindIcon(kind) { return kind === "entdecken" ? "🔍" : kind === "zahlen" ? "🔢" : "🔧" }

    FileDialog {
        id: photoDialog
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Bilder (*.jpg *.jpeg *.png)")]
        onAccepted: root.photoMessage = workbench.addJournalPhoto(root.photoTarget, selectedFile)
                    ? "" : qsTr("Dieses Bild konnte Zuse nicht lesen.")
    }

    Popup { // Foto gross ansehen, auf Wunsch entfernen (mit Rueckfrage)
        id: photoView
        objectName: "photoView"
        property string experimentId: ""
        property string file: ""
        property url url
        property bool confirming: false
        anchors.centerIn: Overlay.overlay
        width: Math.min(900, root.width - 32)
        height: Math.min(700, root.height - 32)
        modal: true
        padding: 12
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onClosed: confirming = false
        background: Rectangle { radius: 12; color: ZTheme.surface; border.color: ZTheme.navy; border.width: 2 }
        contentItem: ColumnLayout {
            spacing: 10
            Image {
                Layout.fillWidth: true
                Layout.fillHeight: true
                source: photoView.url
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 10
                Text {
                    visible: photoView.confirming
                    text: qsTr("Foto wirklich entfernen?")
                    color: ZTheme.text
                    font.pixelSize: Math.round(15 * ZTheme.fontScale)
                }
                ZButton {
                    objectName: "photoRemove"
                    text: photoView.confirming ? qsTr("Ja, entfernen") : qsTr("Foto entfernen")
                    onClicked: {
                        if (!photoView.confirming) { photoView.confirming = true; return }
                        workbench.removeJournalPhoto(photoView.experimentId, photoView.file)
                        photoView.close()
                    }
                }
                ZButton { text: qsTr("Schließen"); onClicked: photoView.close() }
            }
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: column.implicitHeight + 48
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: column
            x: Math.max(24, (flick.width - width) / 2)
            y: 24
            width: Math.min(flick.width - 48, 920)
            spacing: 16

            RowLayout {
                Layout.fillWidth: true
                spacing: 14
                ZButton { objectName: "journalBack"; text: "← " + qsTr("Zurück"); onClicked: root.backRequested() }
                Text {
                    Layout.fillWidth: true
                    text: "📓 " + qsTr("Forscherheft")
                    color: ZTheme.navy
                    font.pixelSize: Math.round(26 * ZTheme.fontScale); font.bold: true
                }
            }

            RowLayout { // Zetti erklaert das Heft
                Layout.fillWidth: true
                spacing: 10
                Image {
                    Layout.preferredWidth: 70; Layout.preferredHeight: 88
                    source: "qrc:/zuse/zetti/zetti_erklaerend.png"
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: intro.implicitHeight + 20
                    radius: 14
                    color: ZTheme.bubble
                    border.width: 2
                    border.color: ZTheme.accentDim
                    Text {
                        id: intro
                        anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter; margins: 10 }
                        wrapMode: Text.WordWrap
                        color: ZTheme.text
                        font.pixelSize: Math.round(15 * ZTheme.fontScale)
                        text: workbench.journal.length === 0
                              ? qsTr("Noch leer. Sobald du bei einem Experiment eine Vermutung abgibst oder etwas baust, "
                                     + "schreibe ich es hier hinein. Deine eigenen Notizen und Fotos kommen dazu.")
                              : qsTr("Hier steht, was du vermutet und herausgefunden hast. Schreib dazu, was dir "
                                     + "aufgefallen ist – oder füge ein Foto von deinem echten Aufbau hinzu.")
                    }
                }
            }

            Repeater {
                model: workbench.journal
                delegate: Rectangle { // eine Heftseite
                    id: page
                    required property var modelData
                    objectName: "journal_" + modelData.id
                    Layout.fillWidth: true
                    Layout.preferredHeight: col.implicitHeight + 28
                    radius: 12
                    color: ZTheme.surface
                    border.color: "#D8CDB4"
                    Rectangle { x: 40; width: 2; height: parent.height; color: "#F2B8B8" } // Heftrand

                    component Field: ColumnLayout {
                        property string caption
                        property string value
                        visible: value !== ""
                        Layout.fillWidth: true
                        spacing: 2
                        Text {
                            text: parent.caption
                            color: "#6E6A60"
                            font.pixelSize: Math.round(13 * ZTheme.fontScale); font.bold: true
                        }
                        Text {
                            Layout.fillWidth: true
                            text: parent.value
                            wrapMode: Text.WordWrap
                            color: ZTheme.text
                            font.pixelSize: Math.round(15 * ZTheme.fontScale)
                            lineHeight: 1.15
                        }
                    }

                    ColumnLayout {
                        id: col
                        x: 56
                        y: 14
                        width: parent.width - 72
                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: root.kindIcon(page.modelData.kind) + "  " + page.modelData.title
                                color: ZTheme.navy
                                font.pixelSize: Math.round(18 * ZTheme.fontScale); font.bold: true
                                wrapMode: Text.WordWrap
                            }
                            Text {
                                text: page.modelData.date
                                color: "#6E6A60"
                                font.pixelSize: Math.round(12 * ZTheme.fontScale)
                            }
                        }
                        Field { caption: qsTr("Meine Vermutung"); value: page.modelData.guess }
                        Field { caption: qsTr("Was ich herausgefunden habe"); value: page.modelData.observation }
                        Text { // Zeichen und Wort, nicht nur Farbe
                            visible: page.modelData.comparison > 0
                            text: page.modelData.comparison === 1 ? "✓ " + qsTr("Die Vermutung hat gestimmt.")
                                : page.modelData.comparison === 2 ? "★ " + qsTr("Es kam anders als vermutet.")
                                : "★ " + qsTr("Jetzt weiß ich es.")
                            color: page.modelData.comparison === 1 ? ZTheme.ok : "#8A5A00"
                            font.pixelSize: Math.round(15 * ZTheme.fontScale); font.bold: true
                        }
                        Repeater {
                            model: page.modelData.notes
                            delegate: Text {
                                required property string modelData
                                Layout.fillWidth: true
                                text: "📝 " + modelData
                                wrapMode: Text.WordWrap
                                color: ZTheme.text
                                font.pixelSize: Math.round(15 * ZTheme.fontScale)
                            }
                        }
                        Flow { // Fotos: Vorschaubilder, Antippen vergroessert
                            Layout.fillWidth: true
                            spacing: 8
                            visible: page.modelData.photos.length > 0
                            Repeater {
                                model: page.modelData.photos
                                delegate: Rectangle {
                                    required property var modelData
                                    required property int index
                                    width: 132; height: 99
                                    radius: 6
                                    color: "#EEE8DA"
                                    border.color: "#D8CDB4"
                                    Image {
                                        anchors.fill: parent
                                        anchors.margins: 3
                                        source: parent.modelData.url
                                        sourceSize: Qt.size(264, 198)
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                        clip: true
                                    }
                                    TapHandler {
                                        onTapped: {
                                            photoView.experimentId = page.modelData.id
                                            photoView.file = parent.modelData.file
                                            photoView.url = parent.modelData.url
                                            photoView.open()
                                        }
                                    }
                                    Accessible.role: Accessible.Button
                                    Accessible.name: qsTr("Foto %1 ansehen").arg(index + 1)
                                }
                            }
                        }
                        ColumnLayout { // eigene Notiz schreiben
                            visible: root.noteTarget === page.modelData.id
                            Layout.fillWidth: true
                            spacing: 6
                            TextArea {
                                id: noteField
                                objectName: "noteField_" + page.modelData.id
                                Layout.fillWidth: true
                                Layout.preferredHeight: Math.max(96, implicitHeight)
                                wrapMode: TextEdit.Wrap
                                placeholderText: qsTr("Was ist dir aufgefallen?")
                                font.pixelSize: Math.round(15 * ZTheme.fontScale)
                                background: Rectangle { radius: 8; color: "#FFFDF7"; border.color: noteField.activeFocus ? ZTheme.accent : ZTheme.line }
                            }
                            RowLayout {
                                spacing: 8
                                ZButton {
                                    text: qsTr("Eintragen")
                                    primary: true
                                    enabled: noteField.text.trim() !== ""
                                    onClicked: {
                                        if (workbench.addJournalNote(page.modelData.id, noteField.text)) {
                                            noteField.text = ""
                                            root.noteTarget = ""
                                        }
                                    }
                                }
                                ZButton { text: qsTr("Abbrechen"); onClicked: { noteField.text = ""; root.noteTarget = "" } }
                            }
                        }
                        Flow {
                            Layout.fillWidth: true
                            spacing: 8
                            ZButton {
                                visible: root.noteTarget !== page.modelData.id
                                objectName: "noteAdd_" + page.modelData.id
                                text: "📝 " + qsTr("Notiz schreiben")
                                onClicked: { root.noteTarget = page.modelData.id; noteField.forceActiveFocus() }
                            }
                            ZButton {
                                visible: page.modelData.photos.length < 12
                                text: "📷 " + qsTr("Foto hinzufügen")
                                onClicked: { root.photoTarget = page.modelData.id; root.photoMessage = ""; photoDialog.open() }
                            }
                            ZButton {
                                visible: page.modelData.index >= 0
                                text: qsTr("Experiment öffnen")
                                onClicked: root.experimentRequested(page.modelData.index)
                            }
                        }
                        Text {
                            visible: root.photoMessage !== "" && root.photoTarget === page.modelData.id
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            text: root.photoMessage
                            color: ZTheme.bad
                            font.pixelSize: Math.round(13 * ZTheme.fontScale)
                        }
                    }
                }
            }
        }
    }
}
