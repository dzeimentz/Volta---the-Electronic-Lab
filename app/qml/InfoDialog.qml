// Info (docs/ui-spec.md): Programm, Plattformen, verwendete Technik, Hersteller und Kontakt.
// Web-Adresse und E-Mail stehen als Text da; die Knoepfe »Website« und »E-Mail« melden linkRequested, Main.qml
// oeffnet sie erst nach der Elternschranke (CLAUDE.md, Kindersicherheit).
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: info
    required property var controller
    readonly property var app: controller.appInfo()
    signal linkRequested(string url)

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(600, parent ? parent.width - 48 : 600)
    height: Math.min(contentColumn.implicitHeight + 120, parent ? parent.height - 48 : 700)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    component Heading: Label {
        width: parent.width
        font.pixelSize: Math.round((15) * Theme.fontScale)
        font.bold: true
        color: Theme.navy
        topPadding: 6
    }
    component Line: Label {
        width: parent.width
        wrapMode: Text.Wrap
        font.pixelSize: Math.round((14) * Theme.fontScale)
        color: "#1E2B38"
    }

    // Kopf
    Rectangle {
        id: head
        width: parent.width
        height: 72
        radius: 12
        color: Theme.navy
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 12; color: Theme.navy }
        Image {
            id: appIcon
            x: 16
            anchors.verticalCenter: parent.verticalCenter
            source: "qrc:/icons/volta_64.png"
            width: 48; height: 48
        }
        Column {
            anchors.left: appIcon.right
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            Label { text: info.app.name; color: "white"; font.pixelSize: Math.round((18) * Theme.fontScale); font.bold: true }
            Label { text: qsTr("Version %1").arg(info.app.version); color: Theme.solderAmber; font.pixelSize: Math.round((13) * Theme.fontScale) }
        }
        ToolButton {
            objectName: "infoClose"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: "white"; size: 20 } }
            onClicked: info.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    ScrollView {
        id: scroll
        anchors.top: head.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 20
        contentWidth: availableWidth
        clip: true

        Column {
            id: contentColumn
            width: scroll.availableWidth
            spacing: 6

            Line {
                text: qsTr("Virtueller Elektronik-Experimentierkasten für Kinder von 8 bis 12 Jahren. Alles, was "
                           + "leuchtet, summt oder sich dreht, berechnet eine echte Schaltungssimulation.")
            }

            Heading { text: qsTr("Plattformen") }
            Line { text: qsTr("Diese Ausgabe: %1").arg(info.app.platform) }
            Line { text: qsTr("Vorgesehen: Android (arm64, armv7), Windows und Linux (x86-64), iPadOS") }

            Heading { text: qsTr("Verwendete Technik") }
            Line { text: qsTr("Qt %1 – Qt Quick, Qt Quick Controls, Qt Quick Shapes, Qt SVG (Lizenz LGPL v3)").arg(info.app.qtVersion) }
            Line { text: qsTr("C++17 mit eigenem Simulationskern: Knotenanalyse (MNA), Newton-Raphson, Zeitintegration, Wärmemodell") }
            Line { text: qsTr("Übersetzer: %1 · Build-System: CMake, Ninja").arg(info.app.compiler) }
            Line { text: qsTr("Prüfung: Catch2 und Qt Test") }

            Heading { text: qsTr("Hersteller") }
            // Firmenlogo (app/resources/brand/dc_logo.png, mit eigenem Hintergrund), sonst Schriftzug
            Image {
                visible: info.app.hasLogo
                source: info.app.hasLogo ? "qrc:/brand/dc_logo.png" : ""
                width: Math.min(parent.width, 300)
                height: sourceSize.width > 0 ? width * sourceSize.height / sourceSize.width : 0
                fillMode: Image.PreserveAspectFit
                smooth: true
                mipmap: true
            }
            Rectangle {
                visible: !info.app.hasLogo
                width: Math.min(parent.width, 300)
                height: 64
                radius: 6
                color: "#14243A"
                Column {
                    anchors.centerIn: parent
                    Label { text: "DC SOFTWARE ENGINEERING"; color: "#2E8BEA"; font.pixelSize: Math.round((17) * Theme.fontScale); font.bold: true; font.letterSpacing: 1 }
                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "TECHNOLOGY. INNOVATION. SOLUTIONS."
                        color: "#C9D3DE"; font.pixelSize: Math.round((9) * Theme.fontScale); font.letterSpacing: 1.5
                    }
                }
            }
            Line { text: "DC Software Engineering, Irrel (DE)"; font.bold: true }
            Line { text: "Daniel Zeimentz" }
            Line { text: qsTr("E-Mail: %1").arg("info@dc-software-engineering.com") }
            Line { text: qsTr("Web: %1").arg("www.dc-software-engineering.com") }
            Flow {
                width: parent.width
                spacing: 10
                Button {
                    objectName: "infoWebsite"
                    height: Theme.control
                    text: qsTr("Website öffnen")
                    onClicked: info.linkRequested("https://dc-software-engineering.com")
                }
                Button {
                    objectName: "infoMail"
                    height: Theme.control
                    text: qsTr("E-Mail schreiben")
                    onClicked: info.linkRequested("mailto:info@dc-software-engineering.com?subject=Volta%201")
                }
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("Für Erwachsene: Vor dem Öffnen kommt eine kleine Rechenaufgabe.")
                font.pixelSize: Math.round(12 * Theme.fontScale)
                color: "#5A6672"
            }

            Heading { text: qsTr("Fremdbestandteile") }
            Line { text: qsTr("QR-Code: qrcodegen von Project Nayuki, MIT-Lizenz (Copyright Project Nayuki).") }

            Heading { text: qsTr("Datenschutz") }
            Line { text: qsTr("Volta erhebt keine Daten, zeigt keine Werbung und braucht kein Internet.") }
            Line { text: "© 2026 DC Software Engineering"; color: "#5A6672"; topPadding: 8 }
        }
    }
}
