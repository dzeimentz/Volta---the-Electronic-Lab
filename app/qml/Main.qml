// Hauptfenster. Phase 1: oeffnet direkt das erste gefuehrte Experiment (Auswahl ueber die Werkzeugleiste).
// Startseite (Dashboard), Lernpfad und Readme folgen in Phase 1-3 (docs/ui-spec.md).
import QtQuick
import QtQuick.Controls.Basic
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
        languageService: typeof volta_languageService !== "undefined" ? volta_languageService : null
        Component.onCompleted: labController.startExperiment("volta1.1.1")
    }
}
