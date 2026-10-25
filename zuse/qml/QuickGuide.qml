import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Kurzanleitung wie bei Volta (neben der Hilfe): in sechs Schritten durch ein Experiment und die Bedienung auf einen
// Blick. Die ausfuehrliche Hilfe mit Zetti bleibt im HelpDialog.
Popup {
    id: guide
    objectName: "quickGuide"
    signal helpRequested()
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(900, parent ? parent.width - 32 : 900)
    height: parent ? parent.height - 32 : 700
    padding: 0
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: ZTheme.navy; border.width: 2 }

    readonly property var steps: [
        { title: qsTr("Experiment wählen"),
          text: qsTr("Tippe auf der Startseite auf ein Experiment im Lernpfad oder auf »Weitermachen«.") },
        { title: qsTr("Geschichte lesen"),
          text: qsTr("Rechts erzählt Zetti, worum es geht, und stellt die Forscherfrage.") },
        { title: qsTr("Vermuten"),
          text: qsTr("Bei Rätselkisten tippst du zuerst bei jeder Situation an, was du glaubst – dann gehen die Kisten auf.") },
        { title: qsTr("Bauen"),
          text: qsTr("Ziehe Werkzeuge auf die Werkbank und verbinde die goldenen Anschlüsse mit Leitungen.") },
        { title: qsTr("Ausprobieren und prüfen"),
          text: qsTr("Bediene die Schalter links und beobachte die Lampen rechts. Dann tippe auf »Prüfen«.") },
        { title: qsTr("Einpacken"),
          text: qsTr("Stimmt alles, packst du deine Erfindung ein. Ab jetzt ist sie ein Werkzeug für die nächsten Experimente.") }
    ]
    readonly property var gestures: [
        [qsTr("Werkzeug auf die Werkbank"), qsTr("ziehen oder antippen")],
        [qsTr("Leitung legen"), qsTr("von Anschluss zu Anschluss ziehen")],
        [qsTr("Baustein verschieben"), qsTr("ziehen")],
        [qsTr("Etwas entfernen"), qsTr("antippen, dann »Entfernen« (am Computer: Entf-Taste)")],
        [qsTr("In eine Erfindung hineinschauen"), qsTr("zweimal schnell tippen")],
        [qsTr("Rückgängig und Wiederholen"), qsTr("gebogene Pfeile oben (am Computer: Strg+Z, Strg+Y)")],
        [qsTr("Zurück zur Startseite"), qsTr("⌂ oben links")]
    ]

    onAboutToShow: flick.contentY = 0

    Rectangle {
        id: head
        width: parent.width
        height: 64
        radius: 12
        color: ZTheme.navy
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 12; color: ZTheme.navy }
        Row {
            x: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 12
            GuideIcon { size: 32; anchors.verticalCenter: parent.verticalCenter }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Kurzanleitung")
                color: "white"
                font.pixelSize: Math.round(20 * ZTheme.fontScale); font.bold: true
            }
        }
        ZButton {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: "✕"
            onClicked: guide.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    Flickable {
        id: flick
        anchors { top: head.bottom; left: parent.left; right: parent.right; bottom: parent.bottom; margins: 20 }
        contentHeight: column.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        ColumnLayout {
            id: column
            width: flick.width - 12
            spacing: 14
            Text {
                Layout.fillWidth: true
                text: qsTr("In sechs Schritten durch ein Experiment")
                color: ZTheme.navy
                font.pixelSize: Math.round(19 * ZTheme.fontScale); font.bold: true
            }
            GridLayout {
                Layout.fillWidth: true
                columns: guide.width > 700 ? 2 : 1
                columnSpacing: 14
                rowSpacing: 12
                Repeater {
                    model: guide.steps
                    RowLayout {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        spacing: 12
                        Rectangle {
                            Layout.alignment: Qt.AlignTop
                            Layout.preferredWidth: 40; Layout.preferredHeight: 40
                            radius: 20
                            color: "#D32F2F"
                            Text {
                                anchors.centerIn: parent
                                text: index + 1
                                color: "white"
                                font.pixelSize: 18; font.bold: true
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text {
                                Layout.fillWidth: true
                                text: modelData.title
                                color: ZTheme.text
                                font.pixelSize: Math.round(16 * ZTheme.fontScale); font.bold: true
                                wrapMode: Text.WordWrap
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.text
                                color: ZTheme.text
                                font.pixelSize: Math.round(14 * ZTheme.fontScale)
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.topMargin: 8
                text: qsTr("Bedienung auf einen Blick")
                color: ZTheme.navy
                font.pixelSize: Math.round(19 * ZTheme.fontScale); font.bold: true
            }
            Repeater {
                model: guide.gestures
                Rectangle {
                    required property var modelData
                    required property int index
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.max(44, row.implicitHeight + 16)
                    radius: 8
                    color: index % 2 ? "transparent" : ZTheme.surfaceHigh
                    RowLayout {
                        id: row
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 12
                        Text {
                            Layout.preferredWidth: parent.width * 0.45
                            text: modelData[0]
                            color: ZTheme.text
                            font.pixelSize: Math.round(15 * ZTheme.fontScale); font.bold: true
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData[1]
                            color: ZTheme.text
                            font.pixelSize: Math.round(15 * ZTheme.fontScale)
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
            ZButton {
                Layout.topMargin: 8
                text: qsTr("Ausführliche Hilfe mit Zetti")
                onClicked: { guide.close(); guide.helpRequested() }
            }
        }
    }
}
