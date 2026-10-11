// Startbildschirm der Desktop-Ausgabe (Windows/Linux x86-64): Startbild mit Tick, Volta-Schriftzug und Herstellerlogo
// (app/resources/brand/splash.jpg), darunter die Version. Blendet nach kurzer Zeit aus; Klick/Tippen oder eine Taste
// ueberspringt ihn. Mobile Plattformen nutzen den Startbildschirm des Systems (Phase 3/5b).
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: splash
    required property var controller
    readonly property var app: controller.appInfo()
    property int holdMs: 2500

    color: "#0B1626"
    z: 1000
    focus: true

    function dismiss() {
        if (!fade.running && opacity === 1) fade.start()
    }

    Image {
        id: picture
        objectName: "splashImage"
        anchors.fill: parent
        source: "qrc:/brand/splash.jpg"
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        asynchronous: false
    }

    // Version unten rechts im Bild
    Label {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 14
        text: qsTr("Version %1").arg(splash.app.version)
        color: "white"
        style: Text.Outline
        styleColor: "#80000000"
        font.pixelSize: Math.round(14 * Theme.fontScale)
    }

    Timer { interval: splash.holdMs; running: true; onTriggered: splash.dismiss() }
    NumberAnimation on opacity { id: fade; running: false; to: 0; duration: 400; onFinished: splash.visible = false }
    TapHandler { onTapped: splash.dismiss() }
    Keys.onPressed: splash.dismiss()

    Accessible.role: Accessible.Pane
    Accessible.name: splash.app.name
}
