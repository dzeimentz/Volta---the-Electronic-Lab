import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Aufgabe fuer die Klasse anzeigen (Lehrkraft, aus dem Elternbereich): grosser QR-Code, daneben der Code zum
// Eintippen (falls die Aufgabe hineinpasst) und was die Aufgabe enthaelt. Fuer Beamer und Tafel: ganzes Fenster.
Popup {
    id: dlg
    property string link: ""
    property string code: ""
    property var info: ({})

    function showTask(newLink, newCode) {
        link = newLink
        code = newCode
        info = workbench.readTask(newLink)
        open()
    }

    objectName: "taskQrDialog"
    anchors.centerIn: Overlay.overlay
    width: Overlay.overlay ? Overlay.overlay.width : 800
    height: Overlay.overlay ? Overlay.overlay.height : 600
    modal: true
    focus: true
    padding: 32
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { color: "white" }

    contentItem: RowLayout {
        spacing: 40
        QrCanvas {
            id: qr
            objectName: "taskQrCanvas"
            Layout.preferredWidth: Math.min(dlg.availableHeight, dlg.availableWidth * 0.55)
            Layout.preferredHeight: Layout.preferredWidth
            Layout.alignment: Qt.AlignVCenter
            code: dlg.opened && dlg.link !== "" ? workbench.qrCode(dlg.link) : ({})
            Accessible.role: Accessible.Graphic
            Accessible.name: qsTr("QR-Code der Aufgabe")
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 16
            RowLayout {
                spacing: 12
                Image {
                    Layout.preferredWidth: 84; Layout.preferredHeight: 104
                    source: "qrc:/zuse/zetti/zetti_zeigend.png"
                    fillMode: Image.PreserveAspectFit
                    mipmap: true
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Aufgabe für die Klasse")
                    color: ZTheme.navy
                    font.pixelSize: 30; font.bold: true
                    wrapMode: Text.WordWrap
                }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: dlg.code !== ""
                      ? qsTr("Öffnet in Zuse 1 die Karte »Aufgabe« und tippt den Code ein – oder scannt den QR-Code mit dem Tablet.")
                      : qsTr("Scannt den QR-Code mit dem Tablet. Zuse 1 öffnet sich und zeigt die Aufgabe.")
                color: ZTheme.text
                font.pixelSize: 20
            }
            Text {
                objectName: "taskCodeText"
                visible: dlg.code !== ""
                text: dlg.code
                color: ZTheme.navy
                font.pixelSize: 56; font.bold: true
                font.family: ZTheme.mono
                font.letterSpacing: 4
            }
            Text {
                visible: (dlg.info.text || "") !== ""
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: "»" + (dlg.info.text || "") + "«"
                color: ZTheme.text
                font.pixelSize: 20; font.italic: true
            }
            Repeater {
                model: dlg.info.experiments || []
                delegate: Text {
                    required property var modelData
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    text: "•  " + modelData.label + "  " + modelData.title
                    color: ZTheme.text
                    font.pixelSize: 19
                }
            }
            Text {
                visible: dlg.info.hasBoard === true
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: "•  " + qsTr("Aufbau im freien Labor: %1").arg(dlg.info.title || qsTr("ohne Titel"))
                color: ZTheme.text
                font.pixelSize: 19
            }
            ZButton {
                objectName: "taskQrClose"
                text: qsTr("Schließen")
                onClicked: dlg.close()
            }
        }
    }
}
