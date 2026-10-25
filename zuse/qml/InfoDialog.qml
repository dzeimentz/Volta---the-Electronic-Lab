import QtQuick
import QtQuick.Controls

// Info wie bei Volta: Programm, Technik, Hersteller und Kontakt, Datenschutz. Web und E-Mail stehen nur als Text da
// (externe Links erst hinter der Elternschranke, CLAUDE.md Regel 6).
Popup {
    id: info
    objectName: "infoDialog"
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(600, parent ? parent.width - 48 : 600)
    height: Math.min(content.implicitHeight + 120, parent ? parent.height - 48 : 700)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: ZTheme.navy; border.width: 2 }

    component Heading: Text {
        width: parent.width
        font.pixelSize: Math.round(15 * ZTheme.fontScale)
        font.bold: true
        color: ZTheme.navy
        topPadding: 6
    }
    component Line: Text {
        width: parent.width
        wrapMode: Text.Wrap
        font.pixelSize: Math.round(14 * ZTheme.fontScale)
        color: ZTheme.text
    }

    Rectangle {
        id: head
        width: parent.width
        height: 72
        radius: 12
        color: ZTheme.navy
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 12; color: ZTheme.navy }
        Image {
            id: appIcon
            x: 16
            anchors.verticalCenter: parent.verticalCenter
            source: "qrc:/zuse/zuse1_64.png"
            width: 48; height: 48
        }
        Column {
            anchors.left: appIcon.right
            anchors.leftMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            Text {
                text: qsTr("Zuse 1 – Computertechnik für Einsteiger")
                color: "white"
                font.pixelSize: Math.round(18 * ZTheme.fontScale); font.bold: true
            }
            Text {
                text: qsTr("Version %1 (Prototyp)").arg(appInfo.version)
                color: "#E8C27A"
                font.pixelSize: Math.round(13 * ZTheme.fontScale)
            }
        }
        ZButton {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: "✕"
            onClicked: info.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    ScrollView {
        id: scroll
        anchors { top: head.bottom; left: parent.left; right: parent.right; bottom: parent.bottom; margins: 20 }
        contentWidth: availableWidth
        clip: true
        Column {
            id: content
            width: scroll.availableWidth
            spacing: 6
            Line {
                text: qsTr("Virtueller Experimentierkasten für Computertechnik ab 10 Jahren: vom Lichtschalter über "
                           + "Logikbausteine, Zahlen und Speicher bis zur selbst gebauten Rechenmaschine. Was die "
                           + "Lampen zeigen, berechnet eine echte Logiksimulation.")
            }
            Heading { text: qsTr("Plattformen") }
            Line { text: qsTr("Diese Ausgabe: %1").arg(appInfo.platform) }
            Line { text: qsTr("Vorgesehen: Windows, Android-Tablets, iPad, macOS, Linux") }
            Heading { text: qsTr("Verwendete Technik") }
            Line { text: qsTr("Qt %1 – Qt Quick, Qt Quick Controls (Lizenz LGPL v3)").arg(appInfo.qtVersion) }
            Line { text: qsTr("C++17 mit eigener Logiksimulation: ereignisgesteuert, Laufzeiten je Gatter, Pegel 0, 1 und unbestimmt") }
            Line { text: qsTr("Übersetzer: %1 · Build-System: CMake, Ninja · Prüfung: Catch2 und Qt Test").arg(appInfo.compiler) }
            Heading { text: qsTr("Hersteller") }
            Line { text: "DC Software Engineering"; font.bold: true }
            Line { text: "Daniel Zeimentz, Aufm Rothenhügel 26, 54666 Irrel, " + qsTr("Deutschland") }
            Line { text: qsTr("E-Mail: %1").arg("info@dc-software-engineering.com") }
            Line { text: qsTr("Web: %1").arg("www.dc-software-engineering.com") }
            Heading { text: qsTr("Zetti") }
            Line { text: qsTr("Zetti, der kleine Roboter, begleitet dich durch Zuse 1. Der Name erinnert an Konrad Zuse, der 1941 den ersten frei programmierbaren Rechner der Welt baute.") }
            Heading { text: qsTr("Fotos (Historische Entwicklung)") }
            Line { text: qsTr("Alle Fotos von Wikimedia Commons, verkleinert:") }
            Line { text: qsTr("Konrad Zuse, 1992: Wolfgang Hunscher, Dortmund (CC BY-SA 3.0)") }
            Line { text: qsTr("Nachbau der Z3: Venusianer, Wikipedia (CC BY-SA 3.0)") }
            Line { text: qsTr("Relais: Sir Boris (CC BY-SA 3.0)") }
            Line { text: qsTr("Elektronenröhre: Alex P. Kok (CC BY-SA 4.0)") }
            Line { text: qsTr("Nachbau des ersten Transistors: gemeinfrei") }
            Line { text: qsTr("Chip 7400 unter dem Mikroskop: Robert Baruch (CC BY-SA 4.0)") }
            Heading { text: qsTr("Datenschutz") }
            Line { text: qsTr("Zuse 1 erhebt keine Daten, zeigt keine Werbung und braucht kein Internet. Deine Erfindungen bleiben auf diesem Gerät.") }
            Line { text: "© 2026 DC Software Engineering"; color: ZTheme.muted; topPadding: 8 }
        }
    }
}
