// Hauptfenster: Startseite (Lernpfad, Freies Labor) und Experimentansicht; Info-Fenster und Startbildschirm
// gemeinsam (docs/ui-spec.md).
import QtQuick
import QtQuick.Controls.Basic
import Volta

ApplicationWindow {
    id: root
    // Windows: eigene Fensterleiste (TitleBar.qml, main.cpp setzt volta_customTitleBar); das Fenster ist dann
    // rahmenlos und um die Leiste hoeher, damit die Arbeitsflaeche gleich gross bleibt
    readonly property bool customTitle: typeof volta_customTitleBar !== "undefined" && volta_customTitleBar
    readonly property int titleHeight: customTitle ? 40 : 0
    flags: customTitle ? (Qt.Window | Qt.FramelessWindowHint) : Qt.Window
    width: 1280
    height: 800 + titleHeight
    // Desktop: Mindestgroesse 1280 x 720. Android: Bildschirm ohne Mindestgroesse (sonst ragt das Fenster unter die
    // Navigationsleiste) und im Vollbild; die Systemleisten kommen per Wischen vom Rand zurueck.
    readonly property bool mobile: Qt.platform.os === "android" || Qt.platform.os === "ios"
    minimumWidth: mobile ? 0 : 1280
    minimumHeight: mobile ? 0 : 720 + titleHeight
    visibility: mobile ? Window.FullScreen : Window.AutomaticVisibility
    visible: true
    title: Qt.application.name
    color: Theme.board

    header: TitleBar {
        window: root
        visible: root.customTitle && root.visibility !== Window.FullScreen
        height: visible ? 40 : 0
        // oberer Rand und obere Ecken zum Groessenaendern (nicht maximiert)
        ResizeEdge { edges: Qt.TopEdge; x: 8; width: parent.width - 16; height: 5 }
        ResizeEdge { edges: Qt.TopEdge | Qt.LeftEdge; width: 8; height: 8 }
        ResizeEdge { edges: Qt.TopEdge | Qt.RightEdge; x: parent.width - 8; width: 8; height: 8 }
    }
    // Rand zum Groessenaendern des rahmenlosen Fensters: Ziehen uebergibt an das System
    component ResizeEdge: Item {
        property int edges: Qt.LeftEdge
        visible: root.customTitle && root.visibility === Window.Windowed
        z: 3000
        HoverHandler {
            cursorShape: (parent.edges === (Qt.TopEdge | Qt.LeftEdge) || parent.edges === (Qt.BottomEdge | Qt.RightEdge))
                         ? Qt.SizeFDiagCursor
                         : (parent.edges === (Qt.TopEdge | Qt.RightEdge) || parent.edges === (Qt.BottomEdge | Qt.LeftEdge))
                           ? Qt.SizeBDiagCursor
                           : (parent.edges === Qt.LeftEdge || parent.edges === Qt.RightEdge) ? Qt.SizeHorCursor
                                                                                               : Qt.SizeVerCursor
        }
        DragHandler {
            target: null
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            onActiveChanged: if (active) root.startSystemResize(parent.edges)
        }
    }
    ResizeEdge { edges: Qt.LeftEdge; y: 0; width: 5; height: parent.height - 8 }
    ResizeEdge { edges: Qt.RightEdge; x: parent.width - 5; width: 5; height: parent.height - 8 }
    ResizeEdge { edges: Qt.BottomEdge; x: 8; y: parent.height - 5; width: parent.width - 16; height: 5 }
    ResizeEdge { edges: Qt.BottomEdge | Qt.LeftEdge; y: parent.height - 8; width: 8; height: 8 }
    ResizeEdge { edges: Qt.BottomEdge | Qt.RightEdge; x: parent.width - 8; y: parent.height - 8; width: 8; height: 8 }

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
    // Praesentation: etwas groessere Schrift (Beamer)
    Binding { target: Theme; property: "fontScale"; value: (root.settings ? [0.8, 0.9, 1.0][root.settings.fontLevel] : 1.0) * (labController.presentation ? 1.1 : 1.0) }
    Binding { target: Theme; property: "silverFields"; value: root.settings ? root.settings.silverFields : true }
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
        onQuickGuideRequested: quickGuide.open()
        onTaskRequested: taskDialog.openTask()
        onDemoRequested: demoDialog.open()
        onParentAreaRequested: parentGate.ask(() => parentArea.open())
    }

    Lab {
        id: labView
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
        onQuickGuideRequested: quickGuide.open()
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
        onQuickGuideRequested: quickGuide.open()
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
    // Aufgabe der Lehrkraft (QR-Code ueber die Kamera-App oder Code zum Eintippen). Mit Profilen wartet sie, bis
    // eines gewaehlt ist.
    property string pendingTask: ""
    function offerTask() {
        if (root.pendingTask === "" || (labController.profilesEnabled && labController.currentProfile.id === undefined)) return
        taskDialog.showLink(root.pendingTask)
        root.pendingTask = ""
    }
    Connections {
        target: labController
        function onTaskLinkReceived(link) { root.pendingTask = link; root.offerTask() }
        function onProfilesChanged() { root.offerTask() }
    }
    TaskDialog {
        id: taskDialog
        controller: labController
        onExperimentRequested: (id) => { if (labController.startExperiment(id)) { root.inJournal = false; root.inLab = true } }
        onBoardRequested: { if (labController.startTaskBoard()) { root.inJournal = false; root.inLab = true } }
    }
    QuickGuide {
        id: quickGuide
        onHelpRequested: helpDialog.open()
    }
    DemoDialog {
        id: demoDialog
        controller: labController
        onDemoStarted: (experimentId) => {
            root.inJournal = false
            root.inLab = true
            labView.recorderVisible = experimentId === "volta1.3.12" // Ladekurve im Zeitschreiber
        }
    }
    // Knopf in den Leisten unten links (Vorfuehrung, Praesentation): runde Pille in der Farbe der Leiste
    component BannerButton: Button {
        id: bannerButton
        height: Theme.control - 8
        leftPadding: 16
        rightPadding: 16
        font.pixelSize: 15
        font.bold: true
        contentItem: Label {
            text: bannerButton.text
            font: bannerButton.font
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: height / 2
            color: bannerButton.down ? "#2E5C8A" : (bannerButton.hovered ? "#1F4670" : "#14304D")
            border.color: Theme.solderAmber
            border.width: bannerButton.visualFocus ? 3 : 1.5
        }
        Accessible.name: text
    }
    // Vorfuehrung laeuft: Hinweis unten links mit »Andere Vorführung« und »Beenden« (zurueck zur Startseite)
    Rectangle {
        objectName: "demoBanner"
        visible: labController.demo
        z: 900
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 12
        anchors.bottomMargin: labController.presentation ? Theme.control + 28 : 12
        width: demoRow.implicitWidth + 24
        height: Theme.control + 8
        radius: height / 2
        color: Theme.navy
        border.color: Theme.solderAmber
        border.width: 2
        Row {
            id: demoRow
            anchors.centerIn: parent
            spacing: 10
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: "🎬 " + qsTr("Vorführung")
                color: "white"
                font.bold: true
                font.pixelSize: 15
            }
            BannerButton {
                objectName: "demoOther"
                text: qsTr("Andere Vorführung")
                onClicked: demoDialog.open()
            }
            BannerButton {
                objectName: "demoEnd"
                text: qsTr("Beenden")
                onClicked: { labController.endDemo(); root.inLab = false; root.inJournal = false }
            }
        }
    }
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
    ParentArea {
        id: parentArea
        controller: labController
        settings: root.settings
        onTaskQrRequested: (link, code) => taskQr.showTask(link, code)
    }
    TaskQrDialog { id: taskQr; controller: labController }

    // Praesentationsmodus: Vollbild (Desktop), Hinweis unten links mit »Beenden«; beim Beenden zurueck zur Startseite
    property int visibilityBeforePresentation: Window.AutomaticVisibility
    Connections {
        target: labController
        function onPresentationChanged() {
            root.inLab = false
            root.inJournal = false
            if (root.mobile) return
            if (labController.presentation) {
                root.visibilityBeforePresentation = root.visibility
                root.visibility = Window.FullScreen
            } else {
                root.visibility = root.visibilityBeforePresentation === Window.FullScreen ? Window.Windowed
                                                                                          : root.visibilityBeforePresentation
            }
        }
    }
    Rectangle {
        objectName: "presentationBanner"
        visible: labController.presentation
        z: 900
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 12
        width: bannerRow.implicitWidth + 24
        height: Theme.control + 8
        radius: height / 2
        color: Theme.navy
        border.color: Theme.solderAmber
        border.width: 2
        Row {
            id: bannerRow
            anchors.centerIn: parent
            spacing: 12
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: "▶ " + qsTr("Präsentation – es wird nichts gespeichert")
                color: "white"
                font.bold: true
                font.pixelSize: 15
            }
            BannerButton {
                objectName: "presentationEnd"
                text: qsTr("Beenden")
                onClicked: labController.presentation = false
            }
        }
    }

    // Startbildschirm nur in der Desktop-Ausgabe (main.cpp setzt volta_showSplash)
    Loader {
        anchors.fill: parent
        z: 1000
        active: typeof volta_showSplash !== "undefined" && volta_showSplash
        sourceComponent: Splash { controller: labController }
    }
}
