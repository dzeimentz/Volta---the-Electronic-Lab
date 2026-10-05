// Hauptfenster. Phase 0: oeffnet direkt die Experimentansicht mit der Vorlage von Experiment 2.4.
// Startseite (Dashboard), Lernpfad und Readme folgen in Phase 1-3 (docs/ui-spec.md).
import QtQuick
import QtQuick.Controls
import Volta

ApplicationWindow {
    id: root
    width: 1280
    height: 800
    minimumWidth: 1280
    minimumHeight: 720
    visible: true
    title: Qt.application.name
    color: Theme.board

    Lab {
        anchors.fill: parent
        controller: labController
        Component.onCompleted: labController.loadExperiment("volta1.2.4")
    }
}
