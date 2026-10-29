// Lizenzen (docs/ui-spec.md): alle Fremdbestandteile dieser Plattform mit Version, Lizenz und Copyright; Antippen
// zeigt den Lizenztext im Original (Englisch). Erzeugt von tools/make_licenses.py aus den Qt-Stuecklisten.
// Hinweise zur LGPL: Qt unveraendert und dynamisch eingebunden, austauschbar, Quelltext auf Anfrage.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: dlg
    required property var controller
    property var entries: []
    property var current: null // gewaehlter Bestandteil (Lizenztext) oder null (Liste)

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(640, parent ? parent.width - 48 : 640)
    height: parent ? parent.height - 48 : 700
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onAboutToShow: { entries = controller.licenses(); current = null }

    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    component Line: Label {
        width: parent.width
        wrapMode: Text.Wrap
        font.pixelSize: Math.round(14 * Theme.fontScale)
        color: "#1E2B38"
    }

    function textOf(entry) {
        let parts = []
        if (entry.copyright) parts.push(entry.copyright)
        for (const id of entry.texts) parts.push("――― " + id + " ―――\n\n" + controller.licenseText(id))
        return parts.join("\n\n")
    }

    // Kopf
    Rectangle {
        id: head
        width: parent.width
        height: 64
        radius: 12
        color: Theme.navy
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 12; color: Theme.navy }
        ToolButton {
            id: back
            objectName: "licensesBack"
            visible: dlg.current !== null
            x: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            contentItem: Label {
                text: "‹"; color: "white"; font.pixelSize: 30
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
            onClicked: dlg.current = null
            Accessible.name: qsTr("Zurück zur Liste")
        }
        Label {
            objectName: "licensesTitle"
            anchors.left: back.visible ? back.right : parent.left
            anchors.leftMargin: back.visible ? 4 : 20
            anchors.right: closeBtn.left
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight
            text: dlg.current ? dlg.current.name : qsTr("Lizenzen")
            color: "white"
            font.pixelSize: Math.round(18 * Theme.fontScale)
            font.bold: true
        }
        ToolButton {
            id: closeBtn
            objectName: "licensesClose"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: "white"; size: 20 } }
            onClicked: dlg.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    // Liste der Bestandteile
    ListView {
        id: list
        objectName: "licensesList"
        visible: dlg.current === null
        anchors.top: head.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        clip: true
        spacing: 4
        model: dlg.entries
        ScrollBar.vertical: ScrollBar { }

        header: Column {
            width: list.width
            spacing: 6
            bottomPadding: 10
            Line {
                text: qsTr("Volta wurde mit freier Software gebaut. Danke an alle, die daran mitgearbeitet haben! "
                           + "Tippe auf einen Eintrag, um die Lizenz zu lesen. Die Lizenztexte stehen im englischen "
                           + "Original.")
            }
            Line {
                text: qsTr("Qt steht unter der GNU LGPL v3. Volta bindet Qt unverändert als eigene Bibliotheken ein "
                           + "(Windows: DLL-Dateien im Programmordner, Android: Bibliotheken in der App); sie können "
                           + "durch kompatible Qt-Bibliotheken ersetzt werden. Den Quelltext der verwendeten "
                           + "Qt-Version gibt es auf download.qt.io oder auf Anfrage an "
                           + "info@dc-software-engineering.com.")
                font.pixelSize: Math.round(13 * Theme.fontScale)
                color: "#5A6672"
            }
        }

        delegate: AbstractButton {
            id: row
            required property var modelData
            required property int index
            objectName: "license_" + index
            width: ListView.view.width
            height: Math.max(Theme.control, rowText.implicitHeight + 16)
            onClicked: dlg.current = modelData
            Accessible.name: modelData.name + ", " + modelData.license
            background: Rectangle {
                radius: 8
                color: row.pressed ? "#E8EEF5" : (row.index % 2 ? "#F6F2E8" : "transparent")
                border.color: row.visualFocus ? Theme.navy : "transparent"
            }
            contentItem: Column {
                id: rowText
                leftPadding: 10
                rightPadding: 10
                topPadding: 8
                Label {
                    width: row.width - 20
                    wrapMode: Text.Wrap
                    text: row.modelData.name + (row.modelData.version && row.modelData.version !== "unknown"
                                                && row.modelData.version.length < 24 ? "  " + row.modelData.version : "")
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    font.bold: true
                    color: Theme.navy
                }
                Label {
                    width: row.width - 20
                    wrapMode: Text.Wrap
                    text: row.modelData.license.replace("LicenseRef-Qt-Commercial OR ", "")
                    font.pixelSize: Math.round(12 * Theme.fontScale)
                    color: "#5A6672"
                }
            }
        }
    }

    // Lizenztext
    ScrollView {
        id: textView
        visible: dlg.current !== null
        anchors.top: head.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        contentWidth: availableWidth
        clip: true
        Column {
            width: textView.availableWidth
            spacing: 8
            Line {
                visible: dlg.current !== null && dlg.current.modules !== undefined
                text: dlg.current && dlg.current.modules ? qsTr("Enthaltene Qt-Teile: %1").arg(dlg.current.modules.join(", ")) : ""
                font.pixelSize: Math.round(12 * Theme.fontScale)
                color: "#5A6672"
            }
            Label {
                objectName: "licenseText"
                width: parent.width
                wrapMode: Text.Wrap
                textFormat: Text.PlainText
                text: dlg.current ? dlg.textOf(dlg.current) : ""
                font.family: "monospace"
                font.pixelSize: Math.round(12 * Theme.fontScale)
                color: "#1E2B38"
            }
        }
    }
}
