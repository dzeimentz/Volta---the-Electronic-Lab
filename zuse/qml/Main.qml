import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Zuse 1 (Prototyp Z0) in der Grundschale des DC Learning Design Standards (wie Volta): Startseite mit Lernpfad,
// Freiem Labor und Vorfuehrung; im Experiment dunkelblaue Kopfzeile mit Startseite und Stufenauswahl, links die Bibliothek (Werkzeuge bzw. Raetselkisten), in der Mitte Steckplatz oder Werkbank, rechts
// die Aufgabe mit Zetti, Vermutung/Tabelle und Aktion. Unter der Mitte (zuschaltbar): Schalter und LEDs als Zahl.
ApplicationWindow {
    id: win
    width: 1280
    height: 800
    minimumWidth: 1024
    minimumHeight: 640
    visible: true
    color: ZTheme.graphite
    title: qsTr("Zuse 1 – Computertechnik für Einsteiger (Prototyp)")

    Binding { target: ZTheme; property: "fontScale"; value: settings.fontScale }

    property string page: startPage // "home" (Startseite), "lab" (Experiment, Freies Labor, Vorfuehrung), "journal"
    property string journalReturn: "home" // von hier aus wurde das Forscherheft geoeffnet
    readonly property bool building: workbench.stageKind === "bauen"
    readonly property bool inLab: page === "lab"
    readonly property bool choosingProfile: workbench.profilesEnabled && workbench.currentProfile.id === undefined

    function openJournal() {
        if (page !== "journal") journalReturn = page
        page = "journal"
    }
    function goHome() {
        workbench.leaveSpecial()
        page = "home"
    }
    readonly property bool discovering: workbench.stageKind === "entdecken"

    Connections {
        target: workbench
        function onStageChanged() { bench.clearSelection() }
    }

    function removeSelection() {
        if (bench.selectedPart !== "") workbench.removePart(bench.selectedPart)
        else if (bench.selectedWire >= 0) workbench.removeWire(bench.selectedWire)
        bench.clearSelection()
    }

    Shortcut { sequence: StandardKey.Undo; onActivated: workbench.undo() }
    Shortcut { sequences: [StandardKey.Redo, "Ctrl+Y"]; onActivated: workbench.redo() }
    Shortcut { sequences: [StandardKey.Delete, "Backspace"]; onActivated: win.removeSelection() }
    Shortcut { sequence: "Escape"; onActivated: workbench.inside ? workbench.leaveTo(workbench.path.length - 2) : bench.clearSelection() }

    // Kopfzeile wie Volta: dunkelblau, Produkt, Stufe, Bearbeiten, Ansicht, Werkzeugkiste
    header: Rectangle {
        height: 64
        color: ZTheme.shell
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 10
            Image {
                source: "qrc:/zuse/zuse1_64.png"
                sourceSize: Qt.size(64, 64)
                Layout.preferredWidth: 44; Layout.preferredHeight: 44
            }
            Text {
                text: win.inLab ? "Zuse 1" : qsTr("Zuse 1 – Computertechnik für Einsteiger")
                color: ZTheme.textOnShell
                font.pixelSize: Math.round(22 * ZTheme.fontScale); font.bold: true
            }
            Item { Layout.preferredWidth: 6 }
            HeaderButton { // zurueck zur Startseite wie Volta
                visible: win.inLab
                text: "⌂"
                font.family: "Segoe UI Symbol"
                font.pixelSize: Math.round(24 * ZTheme.fontScale)
                onClicked: win.goHome()
                Accessible.name: qsTr("Startseite")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Startseite")
            }
            StagePicker { visible: win.inLab; Layout.fillWidth: true; Layout.minimumWidth: 160; Layout.maximumWidth: 360 }
            HeaderButton {
                visible: win.inLab && win.building && !workbench.benchLocked
                text: "↶"
                font.family: "Segoe UI Symbol"
                font.pixelSize: Math.round(22 * ZTheme.fontScale)
                enabled: workbench.canUndo && !workbench.inside
                onClicked: workbench.undo()
                Accessible.name: qsTr("Rückgängig")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Rückgängig")
            }
            HeaderButton {
                visible: win.inLab && win.building && !workbench.benchLocked
                text: "↷"
                font.family: "Segoe UI Symbol"
                font.pixelSize: Math.round(22 * ZTheme.fontScale)
                enabled: workbench.canRedo && !workbench.inside
                onClicked: workbench.redo()
                Accessible.name: qsTr("Wiederholen")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Wiederholen")
            }
            Item { Layout.fillWidth: true }
            HeaderButton { // Zahlenleiste ein/aus: Symbol »101 = 5« (Bits ergeben eine Zahl)
                visible: win.inLab
                text: "101 = 5"
                font.family: ZTheme.mono
                on: workbench.numbersVisible
                onClicked: workbench.numbersVisible = !workbench.numbersVisible
                Accessible.name: qsTr("Zahlenleiste zeigen")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Zahlenleiste: Schalter und Lampen als Zahl lesen (binär, dezimal, hexadezimal)")
            }
            HeaderButton { // Forscherheft (Experiment bleibt, wie es ist)
                objectName: "journalButton"
                visible: win.inLab && workbench.mode !== "vorfuehrung"
                text: "📓"
                font.family: "Segoe UI Emoji"
                onClicked: win.openJournal()
                Accessible.name: qsTr("Forscherheft")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Forscherheft")
            }
            HeaderButton { // Schaltplan neben der Werkbank ein/aus (wie Volta)
                objectName: "schematicToggle"
                visible: win.inLab && win.building
                text: qsTr("Schaltplan")
                on: settings.schematicVisible
                onClicked: settings.schematicVisible = !settings.schematicVisible
                Accessible.name: settings.schematicVisible ? qsTr("Schaltplan ausblenden") : qsTr("Schaltplan zeigen")
                ToolTip.visible: hovered; ToolTip.text: Accessible.name
            }
            ToolButton { // Werkzeugkiste wie Volta: Symbol mit Zahl der Werkzeuge
                objectName: "toolboxButton"
                visible: !win.choosingProfile
                implicitWidth: ZTheme.control; implicitHeight: ZTheme.control
                onClicked: toolbox.opened ? toolbox.close() : toolbox.open()
                Accessible.name: qsTr("Werkzeugkiste")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Werkzeugkiste")
                contentItem: Item {
                    Image {
                        anchors.centerIn: parent
                        width: 26; height: 26
                        sourceSize: Qt.size(64, 64)
                        source: "qrc:/zuse/icons/toolbox_light.svg"
                    }
                    Rectangle { // Zahl der Werkzeuge
                        visible: workbench.tools.length > 0
                        anchors { right: parent.right; top: parent.top; margins: 2 }
                        width: Math.max(18, badge.implicitWidth + 8); height: 18; radius: 9
                        color: ZTheme.pad
                        Text {
                            id: badge
                            anchors.centerIn: parent
                            text: workbench.tools.length
                            color: ZTheme.shell
                            font.pixelSize: 11; font.bold: true
                        }
                    }
                }
                background: Rectangle {
                    radius: 8
                    color: toolbox.opened ? ZTheme.accent : (parent.hovered ? "#16385C" : "transparent")
                    border.color: Qt.rgba(1, 1, 1, 0.45)
                }
            }
            TopBar { // wie Volta: Schrift, Sprache, Hilfe, Kurzanleitung, Info, Menue
                Layout.alignment: Qt.AlignVCenter
                helpInMenu: win.inLab
                onHelpRequested: help.show("")
                onQuickGuideRequested: quickGuide.open()
                onInfoRequested: info.open()
                onJournalRequested: win.openJournal()
                onParentAreaRequested: parentGate.ask(() => parentArea.open())
            }
        }
    }

    JournalPage {
        objectName: "journalPage"
        anchors.fill: parent
        visible: win.page === "journal"
        onBackRequested: win.page = win.journalReturn
        onExperimentRequested: index => { workbench.leaveSpecial(); workbench.selectStage(index); win.page = "lab" }
    }

    Home {
        anchors.fill: parent
        visible: win.page === "home"
        onJournalRequested: win.openJournal()
        onExperimentRequested: index => { workbench.selectStage(index); win.page = "lab" }
        onFreeLabRequested: { workbench.startFreeLab(); win.page = "lab" }
        onDemoRequested: demoPicker.open()
    }

    RowLayout {
        anchors.fill: parent
        visible: win.inLab
        spacing: 0

        LibraryPanel {
            Layout.preferredWidth: 190
            Layout.fillHeight: true
            bench: bench
            ghost: ghost
            onRemoveSelection: win.removeSelection()
        }

        // Mitte: Raetselkisten-Steckplatz oder Werkbank, darunter die Zahlenleiste
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 12
            spacing: 8

            DiscoverStage {
                visible: win.discovering
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            RowLayout { // Pfad beim Blick hinein
                visible: win.building
                spacing: 6
                Repeater {
                    model: workbench.path
                    delegate: RowLayout {
                        required property string modelData
                        required property int index
                        spacing: 6
                        Text { visible: index > 0; text: "›"; color: ZTheme.muted; font.pixelSize: Math.round(22 * ZTheme.fontScale) }
                        ZButton {
                            text: modelData
                            primary: index === workbench.path.length - 1
                            onClicked: workbench.leaveTo(index)
                        }
                    }
                }
                Text {
                    Layout.leftMargin: 8
                    text: workbench.inside ? qsTr("Zurück: auf den Pfad tippen")
                                           : qsTr("Doppelt tippen: in eine ★-Erfindung hineinschauen")
                    color: ZTheme.muted; font.pixelSize: Math.round(14 * ZTheme.fontScale)
                }
            }
            Workbench {
                id: bench
                visible: !win.discovering
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            NumberStrip {
                visible: workbench.numbersVisible
                Layout.fillWidth: true
                onExplain: numbers.open()
            }
            Item { // Platz fuer die Vorfuehrungsleiste unten links
                visible: workbench.mode === "vorfuehrung"
                Layout.preferredHeight: ZTheme.control + 12
            }
        }

        SchematicView { // Schaltplan parallel zum Aufbau
            objectName: "schematic"
            visible: settings.schematicVisible && win.building
            Layout.preferredWidth: Math.max(280, (win.width - 190 - 360) * 0.45)
            Layout.fillHeight: true
            Layout.topMargin: 12; Layout.bottomMargin: 12
        }

        TaskPanel {
            Layout.preferredWidth: 360
            Layout.fillHeight: true
            onDemoPickerRequested: demoPicker.open()
            onHomeRequested: win.goHome()
            onComicRequested: key => comic.show(key)
        }
    }

    // Vorfuehrung laeuft: Leiste unten links wie bei Volta mit »Andere Vorführung« und »Beenden«
    component BannerButton: Button {
        id: bannerButton
        implicitHeight: ZTheme.control - 8
        leftPadding: 16
        rightPadding: 16
        font.pixelSize: 15
        font.bold: true
        contentItem: Text {
            text: bannerButton.text
            font: bannerButton.font
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: height / 2
            color: bannerButton.down ? "#2E5C8A" : (bannerButton.hovered ? "#1F4670" : "#14304D")
            border.color: ZTheme.pad
            border.width: bannerButton.visualFocus ? 3 : 1.5
        }
        Accessible.name: text
    }
    Rectangle {
        objectName: "demoBanner"
        visible: win.inLab && workbench.mode === "vorfuehrung"
        z: 900
        anchors { left: parent.left; bottom: parent.bottom; margins: 12 }
        width: demoRow.implicitWidth + 24
        height: ZTheme.control + 8
        radius: height / 2
        color: ZTheme.navy
        border.color: ZTheme.pad
        border.width: 2
        Row {
            id: demoRow
            anchors.centerIn: parent
            spacing: 10
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "🎬 " + qsTr("Vorführung")
                color: "white"
                font.bold: true
                font.pixelSize: 15
            }
            BannerButton {
                objectName: "demoOther"
                text: qsTr("Andere Vorführung")
                onClicked: demoPicker.open()
            }
            BannerButton {
                objectName: "demoEnd"
                text: qsTr("Beenden")
                onClicked: win.goHome()
            }
        }
    }

    ToolboxPanel {
        id: toolbox
        objectName: "toolbox"
        y: win.header.height // unter der Kopfzeile, der Knopf bleibt erreichbar
        height: win.height - win.header.height
        onPartSelected: id => bench.selectedPart = id
        onComicRequested: key => comic.show(key)
    }
    DemoPicker {
        id: demoPicker
        objectName: "demoPicker"
        onChosen: index => { workbench.startDemo(index); win.page = "lab" }
    }
    NumbersPopup { id: numbers; objectName: "numbers" }
    HelpDialog { id: help }
    QuickGuide { id: quickGuide; onHelpRequested: help.show("") }
    InfoDialog { id: info }
    ComicDialog { id: comic; objectName: "comic" }
    ParentGate { id: parentGate }
    ParentArea { id: parentArea }

    // Forscherprofile: ohne gewaehltes Profil fragt Zuse zuerst »Wer forscht heute?«
    ProfileChooser {
        anchors.fill: parent
        z: 800
        visible: win.choosingProfile
        onVisibleChanged: if (visible) { creating = false; win.page = "home" }
        onChosen: id => { workbench.selectProfile(id); win.page = "home" }
    }

    Rectangle { // Werkzeug, das gerade auf die Werkbank gezogen wird
        id: ghost
        property string symbol
        visible: false
        z: 100
        width: 120; height: 112
        radius: ZTheme.radius
        color: ZTheme.surface
        border.color: ZTheme.accent; border.width: 2
        opacity: 0.85
        Text {
            anchors.centerIn: parent
            text: ghost.symbol; color: ZTheme.text
            font.family: ZTheme.mono; font.pixelSize: Math.round(30 * ZTheme.fontScale); font.bold: true
        }
    }
}
