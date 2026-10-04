// Platzhalter fuer die Startseite (Dashboard). Siehe docs/ui-spec.md.
import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 1280
    height: 800
    minimumWidth: 1280
    minimumHeight: 720
    visible: true
    title: Qt.application.name
    color: "#F7F3EA"

    Label {
        anchors.centerIn: parent
        text: qsTr("Volta - Das Elektronik-Labor")
        font.pixelSize: 32
        color: "#0A2540"
    }
}
