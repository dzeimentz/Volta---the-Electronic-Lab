import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Vorfuehrung wie Volta: fertige Aufbauten aus dem Lernpfad zum Zeigen (z. B. am Beamer). Alle Musterloesungen sind
// verfuegbar, auch noch nicht geschaffte; gespeichert wird nichts.
Popup {
    id: root
    signal chosen(int index)
    anchors.centerIn: Overlay.overlay
    width: Math.min(560, parent ? parent.width - 48 : 560)
    height: Math.min(content.implicitHeight + 32, parent ? parent.height - 80 : 600)
    padding: 16
    modal: true
    background: Rectangle { radius: 16; color: ZTheme.surface; border.color: ZTheme.line }

    Flickable {
        anchors.fill: parent
        contentHeight: content.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ColumnLayout {
            id: content
            width: parent.width
            spacing: 8
            Text {
                text: qsTr("Vorführung")
                color: ZTheme.navy
                font.pixelSize: 22; font.bold: true
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Fertige Aufbauten zum Zeigen. Schalter und Taster lassen sich bedienen, gespeichert wird nichts.")
                color: ZTheme.muted
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }
            Repeater {
                model: workbench.demos
                delegate: Rectangle {
                    id: item
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: Math.max(56, col.implicitHeight + 16)
                    radius: 10
                    color: tap.pressed ? ZTheme.surfaceHigh : "#F3FAFC"
                    border.color: ZTheme.accentDim
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        spacing: 12
                        Text {
                            Layout.preferredWidth: 34
                            text: item.modelData.label
                            color: ZTheme.muted
                            font.pixelSize: 14; font.bold: true
                        }
                        ColumnLayout {
                            id: col
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                Layout.fillWidth: true
                                text: item.modelData.name !== "" ? item.modelData.name + " – " + item.modelData.title
                                                                  : item.modelData.title
                                color: ZTheme.text
                                font.pixelSize: 16; font.bold: true
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: item.modelData.question
                                color: ZTheme.muted
                                font.pixelSize: 13
                                elide: Text.ElideRight
                            }
                        }
                        Text { text: "▶"; color: ZTheme.navy; font.pixelSize: 18 }
                    }
                    TapHandler {
                        id: tap
                        onTapped: { root.chosen(item.modelData.index); root.close() }
                    }
                    Accessible.role: Accessible.Button
                    Accessible.name: item.modelData.title
                }
            }
        }
    }
}
