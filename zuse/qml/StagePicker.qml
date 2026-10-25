import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Stufenauswahl in der Kopfzeile wie bei Volta: Knopf »Stufe n: Titel« oeffnet die Liste aller Stufen mit Frage;
// geschafft mit Haken, gesperrt mit Schloss und blass (nie nur ueber die Farbe).
HeaderButton {
    id: root
    readonly property var current: workbench.stages[workbench.stageIndex]
    text: current ? qsTr("Stufe %1: %2").arg(current.index).arg(current.title) + "  ▾" : ""
    implicitWidth: Math.min(360, contentItem.implicitWidth + 28)
    onClicked: list.open()
    Accessible.name: qsTr("Stufe wählen")

    Popup {
        id: list
        objectName: "stagePicker"
        y: root.height + 6
        width: 440
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
                Repeater {
                    model: workbench.stages
                    delegate: Rectangle {
                        id: item
                        required property var modelData
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
                                Layout.preferredWidth: 36; Layout.preferredHeight: 36
                                radius: 18
                                color: item.modelData.done ? ZTheme.okBg : ZTheme.surfaceHigh
                                border.color: item.modelData.done ? ZTheme.ok : ZTheme.line
                                Text {
                                    anchors.centerIn: parent
                                    text: String(item.modelData.index)
                                    color: ZTheme.text
                                    font.pixelSize: 16; font.bold: true
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
            }
        }
    }
}
