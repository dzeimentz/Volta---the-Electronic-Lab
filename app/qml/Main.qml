// Hauptfenster: Startseite (Lernpfad, Freies Labor) und Experimentansicht; Info-Fenster und Startbildschirm
// gemeinsam (docs/ui-spec.md).
import QtQuick
import QtQuick.Controls.Basic
import Volta

ApplicationWindow {
    id: root
    width: 1280
    height: 800
    // Desktop: Mindestgroesse 1280 x 720 (docs/platforms.md); Touch: Bildschirmgroesse, Hoch- und Querformat
    minimumWidth: Theme.touchUi ? 0 : 1280
    minimumHeight: Theme.touchUi ? 0 : 720
    visible: true
    title: Qt.application.name
    color: Theme.board

    readonly property var languageService: typeof volta_languageService !== "undefined" ? volta_languageService : null
    readonly property var settings: typeof volta_settings !== "undefined" ? volta_settings : null
    property bool inLab: false

    // Einstellung »Alle Bauteile freigeben« wirkt auf Lernpfad und Bauteilfach
    Binding { target: labController; property: "unlockAll"; value: root.settings ? root.settings.unlockAll : false }
    // Entwicklerhilfe VOLTA_DEMO_TOUCH=1: Touch-Oberflaeche auf dem Desktop (main.cpp)
    Binding { target: Theme; property: "touchUi"; value: true; when: typeof volta_forceTouch !== "undefined" && volta_forceTouch }
    // Schriftgroesse in drei Stufen (A-/A+ oben rechts)
    Binding { target: Theme; property: "fontScale"; value: root.settings ? [0.8, 0.9, 1.0][root.settings.fontLevel] : 1.0 }
    font.pixelSize: Math.round(15 * Theme.fontScale)

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
