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

    property string page: startPage // "home" (Startseite) oder "lab" (Experiment, Freies Labor, Vorfuehrung)
    readonly property bool building: workbench.stageKind === "bauen"
    readonly property bool inLab: page === "lab"

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
                font.pixelSize: 22; font.bold: true
            }
            Item { Layout.preferredWidth: 6 }
            HeaderButton { // zurueck zur Startseite wie Volta
                visible: win.inLab
                text: "⌂"
                font.family: "Segoe UI Symbol"
                font.pixelSize: 24
                onClicked: win.goHome()
                Accessible.name: qsTr("Startseite")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Startseite")
            }
            StagePicker { visible: win.inLab; Layout.maximumWidth: 360 }
            HeaderButton {
                visible: win.inLab && win.building && !workbench.benchLocked
                text: "↶"
                font.family: "Segoe UI Symbol"
                font.pixelSize: 22
                enabled: workbench.canUndo && !workbench.inside
                onClicked: workbench.undo()
                Accessible.name: qsTr("Rückgängig")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Rückgängig")
            }
            HeaderButton {
                visible: win.inLab && win.building && !workbench.benchLocked
                text: "↷"
                font.family: "Segoe UI Symbol"
                font.pixelSize: 22
                enabled: workbench.canRedo && !workbench.inside
                onClicked: workbench.redo()
                Accessible.name: qsTr("Wiederholen")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Wiederholen")
            }
            Item { Layout.fillWidth: true }
            HeaderButton { // Zahlenleiste ein/aus
                visible: win.inLab
                text: "0b 0x"
                on: workbench.numbersVisible
                onClicked: workbench.numbersVisible = !workbench.numbersVisible
                Accessible.name: qsTr("Zahlensysteme anzeigen")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Zahlen binär, dezimal und hexadezimal anzeigen")
            }
            HeaderButton {
                text: qsTr("Werkzeugkiste") + " (" + workbench.tools.length + ")"
                onClicked: toolbox.open()
            }
        }
    }

    Home {
        anchors.fill: parent
        visible: !win.inLab
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
                        Text { visible: index > 0; text: "›"; color: ZTheme.muted; font.pixelSize: 22 }
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
                    color: ZTheme.muted; font.pixelSize: 14
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
        }

        TaskPanel {
            Layout.preferredWidth: 360
            Layout.fillHeight: true
            onDemoPickerRequested: demoPicker.open()
            onHomeRequested: win.goHome()
        }
    }

    ToolboxPopup { id: toolbox; objectName: "toolbox" }
    DemoPicker {
        id: demoPicker
        objectName: "demoPicker"
        onChosen: index => { workbench.startDemo(index); win.page = "lab" }
    }
    NumbersPopup { id: numbers; objectName: "numbers" }

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
            font.family: ZTheme.mono; font.pixelSize: 30; font.bold: true
        }
    }
}
