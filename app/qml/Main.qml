// Hauptfenster: Startseite (Lernpfad, Freies Labor) und Experimentansicht; Info-Fenster und Startbildschirm
// gemeinsam (docs/ui-spec.md).
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

    readonly property var languageService: typeof volta_languageService !== "undefined" ? volta_languageService : null
    readonly property var settings: typeof volta_settings !== "undefined" ? volta_settings : null
    property bool inLab: false

    // Einstellung »Alle Bauteile freigeben« wirkt auf Lernpfad und Bauteilfach
    Binding { target: labController; property: "unlockAll"; value: root.settings ? root.settings.unlockAll : false }

    Home {
        objectName: "home"
        anchors.fill: parent
        visible: !root.inLab
        controller: labController
        languageService: root.languageService
        settings: root.settings
        onExperimentRequested: (id) => { if (labController.startExperiment(id)) root.inLab = true }
        onFreeLabRequested: { labController.startFreeLab(); root.inLab = true }
        onInfoRequested: infoDialog.open()
    }

    Lab {
        objectName: "lab"
        anchors.fill: parent
        visible: root.inLab
        controller: labController
        languageService: root.languageService
        settings: root.settings
        onHomeRequested: root.inLab = false
        onInfoRequested: infoDialog.open()
    }

    InfoDialog {
        id: infoDialog
        objectName: "infoDialog"
        controller: labController
    }

    // Startbildschirm nur in der Desktop-Ausgabe (main.cpp setzt volta_showSplash)
    Loader {
        anchors.fill: parent
        z: 1000
        active: typeof volta_showSplash !== "undefined" && volta_showSplash
        sourceComponent: Splash { controller: labController }
    }
}
