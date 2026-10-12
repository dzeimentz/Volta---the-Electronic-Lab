// Forscherheft (Konzept 5.8): je Experiment Vermutung, Beobachtung, Vergleich und eigene Notizen, neueste zuerst.
// Nur lokal gespeichert. Fotos ueber die Systemauswahl (verwaltete Konfiguration »fotoimport« kann es abschalten);
// Zeichnungen und PDF-Export folgen.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs

Rectangle {
    id: journal
    required property var controller
    property var languageService: null
    property var settings: null
    signal backRequested()
    signal infoRequested()
    signal helpRequested()
    signal quickGuideRequested()
    signal parentAreaRequested()
    signal experimentRequested(string id)

    color: "#FBF6EA"
    readonly property bool photosAllowed: typeof volta_managed === "undefined" || volta_managed.photoImport
    property string photoTarget: "" // Experiment, zu dem das naechste Foto gehoert
    property string photoMessage: ""

    FileDialog {
        id: photoDialog
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Bilder (*.jpg *.jpeg *.png)")]
        onAccepted: journal.photoMessage = journal.controller.addJournalPhoto(journal.photoTarget, selectedFile)
                    ? "" : qsTr("Dieses Bild konnte Volta nicht lesen.")
    }
    // Foto gross ansehen, auf Wunsch entfernen (mit Rueckfrage)
    Popup {
        id: photoView
        objectName: "photoView"
        property string experimentId: ""
        property string file: ""
        property url url
        property bool confirming: false
        anchors.centerIn: Overlay.overlay
        width: Math.min(900, journal.width - 32)
        height: Math.min(700, journal.height - 32)
        modal: true
        padding: 12
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onClosed: confirming = false
        background: Rectangle { radius: 12; color: "white"; border.color: Theme.navy; border.width: 2 }
        contentItem: Column {
            spacing: 10
            Image {
                width: parent.width
                height: parent.height - Theme.control - 10
                source: photoView.url
                fillMode: Image.PreserveAspectFit
                asynchronous: true
            }
            Row {
                anchors.right: parent.right
                spacing: 10
                Label {
                    visible: photoView.confirming
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Foto wirklich entfernen?")
                    font.pixelSize: Math.round(15 * Theme.fontScale)
                }
                Button {
                    objectName: "photoRemove"
                    height: Theme.control
                    text: photoView.confirming ? qsTr("Ja, entfernen") : qsTr("Foto entfernen")
                    onClicked: {
                        if (!photoView.confirming) { photoView.confirming = true; return }
                        journal.controller.removeJournalPhoto(photoView.experimentId, photoView.file)
                        photoView.close()
                    }
                }
                Button {
                    height: Theme.control
                    text: qsTr("Schließen")
                    onClicked: photoView.close()
                }
            }
        }
    }

    Rectangle {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Theme.control + 8
        color: Theme.desk
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            ToolButton {
                objectName: "journalBack"
                width: Theme.control; height: Theme.control
                contentItem: Item {
                    Image {
                        anchors.centerIn: parent
                        width: 24; height: 24
                        sourceSize: Qt.size(48, 48)
                        source: "qrc:/icons/home_light.svg"
                    }
                }
                palette.buttonText: "white"
                onClicked: journal.backRequested()
                Accessible.name: qsTr("Startseite")
                background: Rectangle { radius: 6; color: parent.down ? "#33FFFFFF" : (parent.hovered ? "#1FFFFFFF" : "transparent") }
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: "📓 " + qsTr("Forscherheft")
                color: "white"
                font.pixelSize: Math.round(20 * Theme.fontScale)
                font.bold: true
            }
        }
        TopBarActions {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            controller: journal.controller
            languageService: journal.languageService
            settings: journal.settings
            onInfoRequested: journal.infoRequested()
            onHelpRequested: journal.helpRequested()
            onQuickGuideRequested: journal.quickGuideRequested()
            onParentAreaRequested: journal.parentAreaRequested()
        }
    }

    ScrollView {
        id: scroll
        anchors { left: parent.left; right: parent.right; top: header.bottom; bottom: parent.bottom }
        contentWidth: availableWidth
        clip: true

        Column {
            x: Theme.margin
            width: Math.min(scroll.availableWidth - 2 * Theme.margin, 900)
            topPadding: Theme.margin
            bottomPadding: Theme.margin
            spacing: 16

            Label {
                visible: journal.controller.journal.length === 0
                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("Noch leer. Sobald du bei einem Experiment eine Vermutung wählst, schreibt Volta sie hier hinein.")
                font.pixelSize: Math.round(16 * Theme.fontScale)
                color: "#4A5866"
            }

            Repeater {
                model: journal.controller.journal
                Rectangle {
                    id: page
                    required property var modelData
                    objectName: "journal_" + modelData.id
                    width: parent.width
                    height: col.implicitHeight + 28
                    radius: 12
                    color: "white"
                    border.color: "#D8CDB4"
                    // linierte Heftseite angedeutet: Rand links
                    Rectangle { x: 40; width: 2; height: parent.height; color: "#F2B8B8" }

                    Column {
                        id: col
                        x: 56
                        y: 14
                        width: parent.width - 72
                        spacing: 8

                        Row {
                            width: parent.width
                            Label {
                                width: parent.width - dateLabel.width
                                text: page.modelData.title
                                font.pixelSize: Math.round(17 * Theme.fontScale)
                                font.bold: true
                                color: Theme.navy
                                wrapMode: Text.Wrap
                            }
                            Label {
                                id: dateLabel
                                text: page.modelData.date
                                font.pixelSize: Math.round(12 * Theme.fontScale)
                                color: "#6E6A60"
                            }
                        }
                        component Field: Column {
                            property string caption
                            property string value
                            visible: value !== ""
                            width: parent.width
                            Label { text: caption; font.pixelSize: Math.round(12 * Theme.fontScale); font.bold: true; color: "#6E6A60" }
                            Label {
                                width: parent.width
                                text: value
                                wrapMode: Text.Wrap
                                font.pixelSize: Math.round(15 * Theme.fontScale)
                                color: "#1E2B38"
                            }
                        }
                        Field { caption: qsTr("Meine Vermutung"); value: page.modelData.guess }
                        Field { caption: qsTr("Meine Beobachtung"); value: page.modelData.observation }
                        Label {
                            visible: page.modelData.comparison > 0
                            text: page.modelData.comparison === 1 ? "✓ " + qsTr("Die Vermutung hat gestimmt.")
                                : page.modelData.comparison === 2 ? "★ " + qsTr("Es kam anders als vermutet.")
                                : "★ " + qsTr("Jetzt weiß ich es.")
                            font.pixelSize: Math.round(14 * Theme.fontScale)
                            font.bold: true
                            color: page.modelData.comparison === 1 ? "#2E7D32" : "#8A5A00"
                        }
                        Repeater {
                            model: page.modelData.notes
                            Label {
                                required property string modelData
                                width: col.width
                                text: "📝 " + modelData // Notiz (Bildzeichen; der Stift U+270E fehlt auf Android)
                                wrapMode: Text.Wrap
                                font.pixelSize: Math.round(15 * Theme.fontScale)
                                color: "#1E2B38"
                            }
                        }
                        // Fotos: Vorschaubilder, Antippen vergroessert
                        Flow {
                            width: parent.width
                            spacing: 8
                            visible: (page.modelData.photos || []).length > 0
                            Repeater {
                                model: page.modelData.photos || []
                                Rectangle {
                                    required property var modelData
                                    required property int index
                                    objectName: "photo_" + page.modelData.id + "_" + index
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
                        Row {
                            spacing: 10
                            Button {
                                text: qsTr("Experiment öffnen")
                                height: Theme.control
                                onClicked: journal.experimentRequested(page.modelData.id)
                            }
                            Button {
                                objectName: "photoAdd_" + page.modelData.id
                                visible: journal.photosAllowed && (page.modelData.photos || []).length < 12
                                height: Theme.control
                                text: "📷 " + qsTr("Foto hinzufügen")
                                onClicked: { journal.photoTarget = page.modelData.id; journal.photoMessage = ""; photoDialog.open() }
                            }
                        }
                        Label {
                            visible: journal.photoMessage !== "" && journal.photoTarget === page.modelData.id
                            width: parent.width
                            wrapMode: Text.Wrap
                            text: journal.photoMessage
                            color: "#B23B1E"
                            font.pixelSize: Math.round(13 * Theme.fontScale)
                        }
                    }
                }
            }
        }
    }
}
