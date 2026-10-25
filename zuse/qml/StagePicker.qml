import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Auswahl in der Kopfzeile wie bei Volta: Knopf »Stufe n: Experiment« oeffnet die Lernstufen (Titel und Leitfrage)
// mit ihren Experimenten; geschafft mit Haken, gesperrt mit Schloss und blass (nie nur ueber die Farbe).
HeaderButton {
    id: root
    readonly property var current: workbench.stages[workbench.stageIndex]
    text: current ? qsTr("Stufe %1: %2").arg(current.level >= 0 ? current.level : current.index).arg(current.title)
                    + "  ▾" : ""
    implicitWidth: Math.min(360, contentItem.implicitWidth + 28)
    onClicked: list.open()
    Accessible.name: qsTr("Experiment wählen")

    Popup {
        id: list
        objectName: "stagePicker"
        y: root.height + 6
        width: 460
        height: Math.min(contentCol.implicitHeight + 24, win.height - 100)
        padding: 12
        modal: true
        background: Rectangle { radius: 14; color: ZTheme.surface; border.color: ZTheme.line }
        Flickable {
            anchors.fill: parent
            contentHeight: contentCol.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ColumnLayout {
                id: contentCol
                width: parent.width
                spacing: 6
                Repeater { // Lernstufen
                    model: workbench.levels
                    delegate: ColumnLayout {
                        id: level
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        Layout.topMargin: index > 0 ? 10 : 0
                        spacing: 6
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("Stufe %1: %2").arg(level.modelData.number).arg(level.modelData.title)
                                    color: level.modelData.unlocked ? ZTheme.text : ZTheme.muted
                                    font.pixelSize: 17; font.bold: true
                                    wrapMode: Text.WordWrap
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: level.modelData.planned ? level.modelData.question + " " + qsTr("Experimente folgen.")
                                                                  : level.modelData.question
                                    color: ZTheme.muted
                                    font.pixelSize: 13; font.italic: true
                                    wrapMode: Text.WordWrap
                                }
                            }
                            Text {
                                text: level.modelData.done ? "✓" : level.modelData.unlocked ? "" : "🔒"
                                color: ZTheme.ok
                                font.pixelSize: 18; font.bold: true
                                font.family: level.modelData.done ? "" : "Segoe UI Emoji"
                                Accessible.name: level.modelData.done ? qsTr("Stufe geschafft")
                                               : level.modelData.unlocked ? "" : qsTr("Stufe noch gesperrt")
                            }
                        }
                        Repeater { // Experimente der Stufe
                            model: level.modelData.experiments
                            delegate: Rectangle {
                                id: item
                                required property var modelData
                                required property int index
                                readonly property bool isCurrent: modelData.index === workbench.stageIndex
                                Layout.fillWidth: true
                                implicitHeight: Math.max(56, textCol.implicitHeight + 16)
                                radius: 10
                                color: isCurrent ? "#ECFAFD" : tap.pressed ? ZTheme.surfaceHigh : ZTheme.surface
                                border.width: isCurrent ? 2 : 1
                                border.color: isCurrent ? ZTheme.accent : ZTheme.line
                                opacity: modelData.unlocked ? 1 : 0.5
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 10
                                    spacing: 12
                                    Rectangle {
                                        Layout.preferredWidth: 40; Layout.preferredHeight: 40
                                        radius: 20
                                        color: item.modelData.done ? ZTheme.okBg : ZTheme.surfaceHigh
                                        border.color: item.modelData.done ? ZTheme.ok : ZTheme.line
                                        Text {
                                            anchors.centerIn: parent
                                            text: level.modelData.number + "." + (item.index + 1)
                                            color: ZTheme.text
                                            font.pixelSize: 13; font.bold: true
                                        }
                                    }
                                    ColumnLayout {
                                        id: textCol
                                        Layout.fillWidth: true
                                        spacing: 2
                                        Text {
                                            Layout.fillWidth: true
                                            text: item.modelData.title
                                            color: ZTheme.text
                                            font.pixelSize: 16; font.bold: true
                                            elide: Text.ElideRight
                                        }
                                        Text {
                                            Layout.fillWidth: true
                                            text: item.modelData.question
                                            color: ZTheme.muted
                                            font.pixelSize: 13
                                            wrapMode: Text.WordWrap
                                            maximumLineCount: 2
                                            elide: Text.ElideRight
                                        }
                                    }
                                    Text {
                                        text: item.modelData.done ? "✓" : item.modelData.unlocked ? "" : "🔒"
                                        color: ZTheme.ok
                                        font.pixelSize: 20; font.bold: true
                                        font.family: item.modelData.done ? "" : "Segoe UI Emoji"
                                    }
                                }
                                TapHandler {
                                    id: tap
                                    enabled: item.modelData.unlocked
                                    onTapped: { workbench.selectStage(item.modelData.index); list.close() }
                                }
                            }
                        }
                        Repeater { // geplante Experimente: sichtbar, aber noch nicht zu haben
                            model: level.modelData.upcoming
                            delegate: Rectangle {
                                required property var modelData
                                required property int index
                                Layout.fillWidth: true
                                implicitHeight: 48
                                radius: 10
                                color: ZTheme.surface
                                border.color: ZTheme.line
                                opacity: 0.5
                                Text {
                                    anchors.left: parent.left
                                    anchors.leftMargin: 62
                                    anchors.right: parent.right
                                    anchors.rightMargin: 10
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: parent.modelData + " – " + qsTr("folgt")
                                    color: ZTheme.text
                                    font.pixelSize: 15
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
