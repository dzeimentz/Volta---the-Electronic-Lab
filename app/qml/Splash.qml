// Startbildschirm der Desktop-Ausgabe (Windows/Linux x86-64, macOS): Volta-Symbol, Name, Version und Herstellerlogo.
// Blendet nach kurzer Zeit aus; Klick/Tippen oder eine Taste ueberspringt ihn. Mobile Plattformen nutzen den
// Startbildschirm des Systems (Phase 3/5b).
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: splash
    required property var controller
    readonly property var app: controller.appInfo()
    property int holdMs: 2500

    color: Theme.navy
    z: 1000
    focus: true

    function dismiss() {
        if (!fade.running && opacity === 1) fade.start()
    }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -40
        spacing: 14
        Image {
            anchors.horizontalCenter: parent.horizontalCenter
            source: "qrc:/icons/volta_256.png"
            width: 168; height: 168
            smooth: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: splash.app.name
            color: "white"
            font.pixelSize: Math.round((34) * Theme.fontScale)
            font.bold: true
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Version %1").arg(splash.app.version)
            color: Theme.solderAmber
            font.pixelSize: Math.round((16) * Theme.fontScale)
        }
    }

    // Herstellerlogo unten (Datei app/resources/brand/dc_logo.png, sonst Schriftzug)
    Item {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 48
        width: 340
        height: 80
        Image {
            objectName: "splashLogo"
            anchors.fill: parent
            visible: splash.app.hasLogo
            source: splash.app.hasLogo ? "qrc:/brand/dc_logo.png" : ""
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
        }
        Label {
            visible: !splash.app.hasLogo
            anchors.centerIn: parent
            text: "DC SOFTWARE ENGINEERING"
            color: "#2E8BEA"
            font.pixelSize: Math.round((20) * Theme.fontScale)
            font.bold: true
        }
    }

    Timer { interval: splash.holdMs; running: true; onTriggered: splash.dismiss() }
    NumberAnimation on opacity { id: fade; running: false; to: 0; duration: 400; onFinished: splash.visible = false }
    TapHandler { onTapped: splash.dismiss() }
    Keys.onPressed: splash.dismiss()

    Accessible.role: Accessible.Pane
    Accessible.name: splash.app.name
}
