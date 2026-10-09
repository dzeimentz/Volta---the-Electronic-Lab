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
    readonly property var audio: typeof volta_audio !== "undefined" ? volta_audio : null
    readonly property var speech: typeof volta_speech !== "undefined" ? volta_speech : null
    property bool inLab: false
    property bool inJournal: false

    // Einstellung »Alle Bauteile freigeben« wirkt auf Lernpfad und Bauteilfach
    Binding { target: labController; property: "unlockAll"; value: root.settings ? root.settings.unlockAll : false }
    // Ton nur im Labor und wenn eingeschaltet; Vorlesen in der Anzeigesprache
    Binding { target: root.audio; when: root.audio !== null; property: "enabled"; value: root.settings ? root.settings.soundOn : true }
    Binding { target: root.audio; when: root.audio !== null; property: "active"; value: root.inLab }
    Binding { target: root.speech; when: root.speech !== null && root.languageService !== null; property: "language"; value: root.languageService ? root.languageService.language : "de" }
    Binding { target: labController; property: "preferredColumns"; value: root.settings ? root.settings.boardColumns : 12 }
    Binding { target: labController; property: "preferMeter"; value: root.settings ? root.settings.meterVisible : false }
    Connections {
        target: labController
        function onColumnsChosen(columns) { if (root.settings) root.settings.boardColumns = columns }
    }
    // Schriftgroesse in drei Stufen (A-/A+ oben rechts)
    Binding { target: Theme; property: "fontScale"; value: root.settings ? [0.8, 0.9, 1.0][root.settings.fontLevel] : 1.0 }
    font.pixelSize: Math.round(15 * Theme.fontScale)

    Home {
        objectName: "home"
        anchors.fill: parent
        visible: !root.inLab && !root.inJournal
        controller: labController
        languageService: root.languageService
        settings: root.settings
        onExperimentRequested: (id) => { if (labController.startExperiment(id)) root.inLab = true }
        onFreeLabRequested: { labController.startFreeLab(); root.inLab = true }
        onJournalRequested: root.inJournal = true
        onInfoRequested: infoDialog.open()
    }

    Lab {
        objectName: "lab"
        anchors.fill: parent
        visible: root.inLab
        controller: labController
        languageService: root.languageService
        settings: root.settings
        speech: root.speech
        onHomeRequested: { root.inLab = false; if (root.speech) root.speech.stop() }
        onInfoRequested: infoDialog.open()
    }

    JournalView {
        objectName: "journal"
        anchors.fill: parent
        visible: root.inJournal && !root.inLab
        controller: labController
        languageService: root.languageService
        settings: root.settings
        onBackRequested: root.inJournal = false
        onInfoRequested: infoDialog.open()
        onExperimentRequested: (id) => { if (labController.startExperiment(id)) { root.inJournal = false; root.inLab = true } }
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
