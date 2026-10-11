// Hauptfenster: Startseite (Lernpfad, Freies Labor) und Experimentansicht; Info-Fenster und Startbildschirm
// gemeinsam (docs/ui-spec.md).
import QtQuick
import QtQuick.Controls.Basic
import Volta

ApplicationWindow {
    id: root
    width: 1280
    height: 800
    // Desktop: Mindestgroesse 1280 x 720. Android: Bildschirm ohne Mindestgroesse (sonst ragt das Fenster unter die
    // Navigationsleiste) und im Vollbild; die Systemleisten kommen per Wischen vom Rand zurueck.
    readonly property bool mobile: Qt.platform.os === "android" || Qt.platform.os === "ios"
    minimumWidth: mobile ? 0 : 1280
    minimumHeight: mobile ? 0 : 720
    visibility: mobile ? Window.FullScreen : Window.AutomaticVisibility
    visible: true
    title: Qt.application.name
    color: Theme.board

    readonly property var languageService: typeof volta_languageService !== "undefined" ? volta_languageService : null
    readonly property var settings: typeof volta_settings !== "undefined" ? volta_settings : null
    readonly property var audio: typeof volta_audio !== "undefined" ? volta_audio : null
    readonly property var speech: typeof volta_speech !== "undefined" ? volta_speech : null
    readonly property var managed: typeof volta_managed !== "undefined" ? volta_managed : null
    readonly property var audioRoute: typeof volta_audioRoute !== "undefined" ? volta_audioRoute : null
    // Klassenmodus Ton: Vorgabe der Schule, sonst Einstellung im Elternbereich; »kopfhoerer« nur mit Kopfhoerern
    readonly property string soundMode: managed && managed.soundMode !== "" ? managed.soundMode
                                        : (settings ? settings.classroomSound : "normal")
    readonly property bool soundAllowed: soundMode === "normal"
                                         || (soundMode === "kopfhoerer" && audioRoute !== null && audioRoute.headphones)
    onSoundAllowedChanged: if (!soundAllowed && speech) speech.stop()
    // Forscherprofile: Vorgabe der Schule (0 = aus, sonst an) oder Einstellung im Elternbereich
    readonly property bool profilesWanted: managed && managed.profiles >= 0 ? managed.profiles > 0
                                           : (settings ? settings.profilesEnabled : false)
    property bool inLab: false
    property bool inJournal: false
    // Automatisches Speichern: letzten Laborstand laden; war das Labor offen, geht es dort direkt weiter.
    // Mit Forscherprofilen erst nach der Profilwahl (ProfileChooser).
    function resume() {
        root.inJournal = false
        root.inLab = labController.resumeSession() && labController.savedLabOpen()
    }
    Component.onCompleted: {
        labController.profilesEnabled = root.profilesWanted
        if (!labController.profilesEnabled) root.resume()
    }
    Connections {
        target: root.settings
        function onChanged() { labController.profilesEnabled = root.profilesWanted }
    }
    onInLabChanged: labController.setLabOpen(root.inLab)

    // Einstellung »Alle Bauteile freigeben« wirkt auf Lernpfad und Bauteilfach
    Binding { target: labController; property: "unlockAll"; value: root.settings ? root.settings.unlockAll : false }
    // Ton nur im Labor und wenn eingeschaltet; Vorlesen in der Anzeigesprache
    Binding { target: root.audio; when: root.audio !== null; property: "enabled"; value: (root.settings ? root.settings.soundOn : true) && root.soundAllowed }
    Binding { target: root.audio; when: root.audio !== null; property: "active"; value: root.inLab }
    Binding { target: root.speech; when: root.speech !== null && root.languageService !== null; property: "language"; value: root.languageService ? root.languageService.language : "de" }
    Binding { target: labController; property: "preferredColumns"; value: root.settings ? root.settings.boardColumns : 12 }
    Binding { target: labController; property: "preferMeter"; value: root.settings ? root.settings.meterVisible : false }
    Binding { target: labController; property: "autoWireColor"; value: root.settings ? root.settings.autoWireColor : true }
    Binding { target: labController; property: "autoColumns"; value: root.settings ? root.settings.autoBoardColumns : true }
    Binding { target: labController; property: "meterDocked"; value: root.settings ? root.settings.meterDocked : false }
    Connections {
        target: labController
        function onColumnsChosen(columns) { if (root.settings) root.settings.boardColumns = columns }
    }
    // Schriftgroesse in drei Stufen (A-/A+ oben rechts)
    Binding { target: Theme; property: "fontScale"; value: root.settings ? [0.8, 0.9, 1.0][root.settings.fontLevel] : 1.0 }
    Binding { target: Theme; property: "tick"; value: root.settings ? root.settings.tickEnabled : true }
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
        onContinueRequested: root.inLab = true
        onProfileSwitchRequested: { labController.leaveProfile(); root.inLab = false; root.inJournal = false }
        onJournalRequested: root.inJournal = true
        onInfoRequested: infoDialog.open()
        onHelpRequested: helpDialog.open()
        onParentAreaRequested: parentGate.ask(() => parentArea.open())
    }

    Lab {
        objectName: "lab"
        anchors.fill: parent
        visible: root.inLab
        controller: labController
        languageService: root.languageService
        settings: root.settings
        speech: root.soundAllowed ? root.speech : null // Vorlesen nur, wenn Ton erlaubt ist
        onHomeRequested: { root.inLab = false; if (root.speech) root.speech.stop() }
        onInfoRequested: infoDialog.open()
        onHelpRequested: helpDialog.open()
        onHelpTopicRequested: (key) => helpDialog.show(key)
        onParentAreaRequested: parentGate.ask(() => parentArea.open())
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
        onHelpRequested: helpDialog.open()
        onParentAreaRequested: parentGate.ask(() => parentArea.open())
        onExperimentRequested: (id) => { if (labController.startExperiment(id)) { root.inJournal = false; root.inLab = true } }
    }

    InfoDialog {
        id: infoDialog
        objectName: "infoDialog"
        controller: labController
        // Links nach aussen erst nach der Elternschranke
        onLinkRequested: (url) => { infoDialog.close(); parentGate.ask(() => Qt.openUrlExternally(url)) }
    }
    ParentGate { id: parentGate; settings: root.settings }
    HelpDialog {
        id: helpDialog
        onParentAreaRequested: parentGate.ask(() => parentArea.open())
    }

    // Fortschrittscode (QR ueber die Kamera-App): Rueckfrage, dann in das aktuelle Profil uebernehmen. Mit Profilen
    // wartet die Frage, bis eines gewaehlt ist.
    property string pendingLink: ""
    function offerLink() {
        if (root.pendingLink === "" || (labController.profilesEnabled && labController.currentProfile.id === undefined)) return
        linkDialog.count = labController.progressFromLink(root.pendingLink).length
        linkDialog.open()
    }
    Connections {
        target: labController
        function onProgressLinkReceived(link) { root.pendingLink = link; root.offerLink() }
        function onProfilesChanged() { root.offerLink() }
    }
    Dialog {
        id: linkDialog
        objectName: "linkDialog"
        property int count: 0
        anchors.centerIn: parent
        modal: true
        width: Math.min(460, root.width - 32)
        title: qsTr("Fortschritt übernehmen?")
        contentItem: Column {
            spacing: 16
            Label {
                width: linkDialog.width - linkDialog.leftPadding - linkDialog.rightPadding
                wrapMode: Text.Wrap
                font.pixelSize: Math.round(15 * Theme.fontScale)
                text: linkDialog.count === 0 ? qsTr("Dieser Code enthält keine Experimente von Volta 1.")
                    : labController.currentProfile.id !== undefined
                    ? qsTr("%1 erledigte Experimente in das Profil »%2« übernehmen?").arg(linkDialog.count).arg(labController.currentProfile.name)
                    : qsTr("%1 erledigte Experimente übernehmen?").arg(linkDialog.count)
            }
            Row {
                anchors.right: parent.right
                spacing: 12
                Button { objectName: "linkCancel"; height: Theme.control; text: qsTr("Abbrechen"); onClicked: linkDialog.reject() }
                Button {
                    objectName: "linkAccept"
                    visible: linkDialog.count > 0
                    height: Theme.control
                    text: qsTr("Übernehmen")
                    onClicked: linkDialog.accept()
                }
            }
        }
        onAccepted: labController.importProgressLink(root.pendingLink)
        onClosed: root.pendingLink = ""
    }

    // Forscherprofile: Auswahl, solange keins gewaehlt ist (ueber allen Ansichten, unter Schranke und Elternbereich)
    ProfileChooser {
        anchors.fill: parent
        visible: labController.profilesEnabled && labController.currentProfile.id === undefined
        controller: labController
        onChosen: (id) => { if (labController.selectProfile(id)) root.resume() }
        onParentAreaRequested: parentGate.ask(() => parentArea.open())
    }
    ParentArea { id: parentArea; controller: labController; settings: root.settings }

    // Startbildschirm nur in der Desktop-Ausgabe (main.cpp setzt volta_showSplash)
    Loader {
        anchors.fill: parent
        z: 1000
        active: typeof volta_showSplash !== "undefined" && volta_showSplash
        sourceComponent: Splash { controller: labController }
    }
}
