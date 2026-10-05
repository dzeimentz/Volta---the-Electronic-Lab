// Experimentansicht (docs/ui-spec.md, Lab): Werkzeugleiste, Bauteilfach, Pultleiste, Grundplatte, Modulleiste.
// Bedienung: Bauteile aus dem Fach ziehen, Bauteile verschieben, Leitungen von Feld zu Feld ziehen, Leitungsenden
// umsetzen; langer Druck oder Rechtsklick oeffnet Menues; Rueckgaengig/Wiederholen auch per Tastenkuerzel.
// Alle Regeln (Spannweite, volle Felder ...) prueft das Modell; die Oberflaeche zeigt nur den Hinweis an.
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: lab
    required property var controller
    property var languageService: null // Anzeigesprache (main.cpp); ohne: Knopf ausgeblendet
    property var settings: null        // Einstellungen (main.cpp); ohne: Grundwerte
    signal homeRequested()  // zurueck zur Startseite
    signal infoRequested()  // Info-Fenster (liegt im Hauptfenster)
    Binding { target: Theme; property: "reduceMotion"; value: lab.settings ? lab.settings.reduceMotion : false }
    Binding { target: Theme; property: "showCurrentFlow"; value: lab.settings ? lab.settings.showCurrentFlow : true }

    // ------------------------------------------------------------------ Geometrie
    readonly property var deskNames: ["P:+9V", "P:0V", "P:P1a", "P:P1s", "P:P1b", "P:P2a", "P:P2s", "P:P2b",
                                      "P:S1", "P:S2", "P:LS1", "P:LS2"]
    // Buchsen des Dreifach-Messgeraets: unten im Messgeraet rechts neben der Grundplatte (nur eingeblendet)
    readonly property var meterNames: ["MESS:COM", "MESS:VOHM", "MESS:A"]
    readonly property int deskStripWidth: deskNames.length * (Theme.deskFieldWidth + Theme.deskFieldGap) - Theme.deskFieldGap
    readonly property int meterX: boardWidth + Theme.fieldGap
    readonly property var rows: ["A", "B", "C", "D", "E"]
    readonly property int columns: controller.boardColumns // 8, 10 oder 12 (+/- in der Werkzeugleiste)
    readonly property int slots: 16
    // Pult (Bedienelemente), Gruppenlinien, Pultleiste, Grundplatte, Modulleiste von oben nach unten
    readonly property int deskPanelHeight: 128
    readonly property int groupLineY: deskPanelHeight + 6
    readonly property int deskY: groupLineY + 26
    readonly property int boardY: deskY + Theme.fieldHeight + 2 * Theme.fieldGap
    readonly property int boardWidth: columns * Theme.fieldWidth + (columns - 1) * Theme.fieldGap
    readonly property int moduleY: boardY + rows.length * (Theme.fieldHeight + Theme.fieldGap) + Theme.fieldGap
    readonly property var allAnchors: {
        let a = controller.meterVisible ? deskNames.concat(meterNames) : deskNames.slice()
        for (let r = 0; r < rows.length; ++r)
            for (let c = 1; c <= columns; ++c) a.push(rows[r] + c)
        for (let m = 1; m <= slots; ++m) a.push("M:" + m)
        return a
    }

    function deskLabel(name) { return name.startsWith("MESS:") ? name.substring(5).replace("VOHM", "V/Ω") : name.substring(2) }
    function isBoardField(name) { return /^[A-E](1[0-2]|[1-9])$/.test(name) }

    function anchorSize(name) {
        if (deskNames.indexOf(name) >= 0 || meterNames.indexOf(name) >= 0)
            return Qt.size(Theme.deskFieldWidth, Theme.fieldHeight)
        if (name.startsWith("M:")) return Qt.size(Theme.slotWidth, Theme.fieldHeight)
        return Qt.size(Theme.fieldWidth, Theme.fieldHeight)
    }

    function anchorPoint(name) {
        const m = meterNames.indexOf(name)
        if (m >= 0)
            return Qt.point(meterX + (Theme.meterAreaWidth - 3 * Theme.deskFieldWidth - 2 * Theme.deskFieldGap) / 2
                            + Theme.deskFieldWidth / 2 + m * (Theme.deskFieldWidth + Theme.deskFieldGap),
                            boardY + 4 * (Theme.fieldHeight + Theme.fieldGap) + Theme.fieldHeight / 2)
        let i = deskNames.indexOf(name)
        if (i >= 0)
            return Qt.point(i * (Theme.deskFieldWidth + Theme.deskFieldGap) + Theme.deskFieldWidth / 2,
                            deskY + Theme.fieldHeight / 2)
        if (name.startsWith("M:")) {
            const n = parseInt(name.substring(2)) - 1
            return Qt.point(n * (Theme.slotWidth + Theme.slotGap) + Theme.slotWidth / 2, moduleY + Theme.fieldHeight / 2)
        }
        const r = rows.indexOf(name.charAt(0))
        const c = parseInt(name.substring(1)) - 1
        return Qt.point(c * (Theme.fieldWidth + Theme.fieldGap) + Theme.fieldWidth / 2,
                        boardY + r * (Theme.fieldHeight + Theme.fieldGap) + Theme.fieldHeight / 2)
    }

    // Mittelpunkt eines Stecklochs (0 links ... 4 rechts); Felder mit einem Loch: Feldmitte
    function socketPitch(name) {
        return deskNames.indexOf(name) >= 0 || meterNames.indexOf(name) >= 0 ? Theme.deskSocketPitch : Theme.socketPitch
    }
    function socketPoint(name, index) {
        const c = anchorPoint(name)
        if (name.startsWith("M:") || index === undefined) return c
        return Qt.point(c.x + (index - 2) * socketPitch(name), c.y)
    }

    // Anschlusspunkt unter einem Punkt (canvas-Koordinaten), Fangbereich Feld + 8 dp; "" wenn keiner
    function anchorAt(p, boardOnly) {
        for (let i = 0; i < allAnchors.length; ++i) {
            const name = allAnchors[i]
            if (boardOnly && !isBoardField(name)) continue
            const c = anchorPoint(name)
            const s = anchorSize(name)
            if (Math.abs(p.x - c.x) <= s.width / 2 + 8 && Math.abs(p.y - c.y) <= s.height / 2 + 8) return name
        }
        return ""
    }

    function catalogEntry(type) {
        const list = controller.catalogParts
        for (let i = 0; i < list.length; ++i) if (list[i].type === type) return list[i]
        return ({})
    }
    // Nennspannung einer Gluehlampe (Wert; ohne Wert die Katalogvorgabe)
    function lampVolt(part) { return part && part.value > 0 ? part.value : (catalogEntry("gluehlampe").defaultVolt || 0) }
    function toCanvas(scenePos) { return canvas.mapFromItem(null, scenePos.x, scenePos.y) }
    // Mitte und Rand einer Gruppe von Pultfeldern (Index in deskNames)
    function deskX(i) { return anchorPoint(deskNames[i]).x }
    function groupCenter(first, last) { return (deskX(first) + deskX(last)) / 2 }
    property string highlightGroup: "" // Gruppe, deren Felder hervorgehoben sind (Tippen auf Schaltzeichen)
    readonly property var groups: [
        {name: "batterie", symbol: "−|+", first: 0, last: 1},
        {name: "p1", symbol: "P1", first: 2, last: 4},
        {name: "p2", symbol: "P2", first: 5, last: 7},
        {name: "schalter", symbol: "S", first: 8, last: 9},
        {name: "lautsprecher", symbol: "LS", first: 10, last: 11}
    ]
    function inHighlightedGroup(name) {
        if (meterNames.indexOf(name) >= 0) return highlightGroup === "messgeraet"
        const i = deskNames.indexOf(name)
        for (let g = 0; g < groups.length; ++g)
            if (groups[g].name === highlightGroup && i >= groups[g].first && i <= groups[g].last) return true
        return false
    }

    // ------------------------------------------------------------------ Bedienzustand
    property string selectedPart: ""
    property bool schematicVisible: false   // Schaltplan parallel zum Aufbau
    property var highlightFields: []        // Felder eines im Schaltplan angetippten Knotens
    property string selectedWire: ""
    property string newWireColor: "rot"
    property string highlightAnchor: ""
    // Leitung ziehen (neu: wireFrom gesetzt) bzw. Ende umsetzen (moveWireId gesetzt)
    property string wireFrom: ""
    property string moveWireId: ""
    property bool moveWireFromEnd: false
    property point wirePreviewEnd
    // Bauteil aus dem Fach
    property string trayType: ""
    property point trayPos

    Connections {
        target: lab.controller
        function onEditFailed() { toast.show(lab.controller.lastEditError) }
    }

    // ------------------------------------------------------------------ Werkzeugleiste
    Rectangle {
        id: toolbar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Theme.control + 16
        color: Theme.desk

        // Oben rechts: Sprache, Info, Einstellungen (gemeinsam mit der Startseite)
        TopBarActions {
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            controller: lab.controller
            languageService: lab.languageService
            settings: lab.settings
            onInfoRequested: lab.infoRequested()
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: Theme.margin
            spacing: 12

            ToolButton {
                objectName: "homeButton"
                width: Theme.control; height: Theme.control
                text: "⌂"
                font.pixelSize: Math.round((26) * Theme.fontScale)
                palette.buttonText: "white"
                onClicked: lab.homeRequested()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Startseite")
                Accessible.name: qsTr("Startseite")
                background: Rectangle {
                    radius: 6
                    color: parent.down ? "#33FFFFFF" : (parent.hovered ? "#1FFFFFFF" : "transparent")
                }
            }
            // Experimentauswahl als Aufklappfeld: »Freies Labor« und die freigeschalteten Experimente; zeigt den Titel
            ComboBox {
                id: experimentBox
                objectName: "experimentBox"
                anchors.verticalCenter: parent.verticalCenter
                width: 200 // laengere Titel werden gekuerzt; die Liste ist breiter
                height: Theme.control
                textRole: "text"
                readonly property var entries: {
                    let e = [{text: qsTr("Freies Labor"), id: ""}]
                    const stages = lab.controller.stages
                    for (let s = 0; s < stages.length; ++s) {
                        if (!stages[s].unlocked) continue
                        const list = stages[s].experiments
                        for (let i = 0; i < list.length; ++i)
                            e.push({text: qsTr("Stufe %1: %2").arg(list[i].stage).arg(list[i].title)
                                          + (list[i].destructive ? "  ⚠" : ""), id: list[i].id})
                    }
                    return e
                }
                model: entries
                currentIndex: {
                    if (lab.controller.freeLab) return 0
                    for (let i = 1; i < entries.length; ++i)
                        if (entries[i].id === lab.controller.experimentId) return i
                    return -1
                }
                displayText: currentIndex >= 0 ? entries[currentIndex].text : lab.controller.title
                onActivated: (index) => {
                    if (index === 0) lab.controller.startFreeLab()
                    else lab.controller.startExperiment(entries[index].id)
                }
                font.pixelSize: Math.round((16) * Theme.fontScale)
                font.bold: true
                indicator: Label {
                    x: experimentBox.width - width - 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: "▾"
                    color: "white"
                    font.pixelSize: Math.round((18) * Theme.fontScale)
                }
                contentItem: Label {
                    leftPadding: 12
                    rightPadding: experimentBox.indicator.width + 16
                    text: experimentBox.displayText
                    color: "white"
                    font: experimentBox.font
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: experimentBox.pressed ? "#2A4A6E" : (experimentBox.hovered ? "#24425F" : "#1F3A57")
                    border.color: "#7F93A8"
                }
                delegate: ItemDelegate {
                    required property var modelData
                    required property int index
                    width: experimentBox.popup.width
                    height: Theme.control
                    text: modelData.text
                    font.pixelSize: Math.round((15) * Theme.fontScale)
                    font.bold: index === experimentBox.currentIndex
                    highlighted: experimentBox.highlightedIndex === index
                }
                popup.width: Math.max(width, 460)
                ToolTip.visible: hovered && !pressed
                ToolTip.text: qsTr("Experiment wählen")
                Accessible.name: qsTr("Experiment wählen")
            }
            ToolButton {
                objectName: "undoButton"
                text: "↶"
                font.pixelSize: Math.round((24) * Theme.fontScale)
                width: Theme.control; height: Theme.control
                enabled: lab.controller.canUndo
                onClicked: lab.controller.undo()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Rückgängig: %1").arg(lab.controller.undoText)
                Accessible.name: qsTr("Rückgängig")
            }
            ToolButton {
                objectName: "redoButton"
                text: "↷"
                font.pixelSize: Math.round((24) * Theme.fontScale)
                width: Theme.control; height: Theme.control
                enabled: lab.controller.canRedo
                onClicked: lab.controller.redo()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Wiederholen: %1").arg(lab.controller.redoText)
                Accessible.name: qsTr("Wiederholen")
            }
            // Breite der Grundplatte: Felder je Reihe 8 / 10 / 12 (Befehl, rueckgaengig machbar)
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                ToolButton {
                    objectName: "columnsMinus"
                    text: "−"
                    font.pixelSize: Math.round((24) * Theme.fontScale)
                    width: Theme.control; height: Theme.control
                    enabled: lab.columns > 8
                    onClicked: lab.controller.setBoardColumns(lab.columns - 2)
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Weniger Felder je Reihe")
                    Accessible.name: qsTr("Weniger Felder je Reihe")
                }
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 52
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("1–%1").arg(lab.columns)
                    color: "white"
                    font.pixelSize: Math.round((15) * Theme.fontScale)
                    font.bold: true
                    Accessible.name: qsTr("Felder je Reihe: %1").arg(lab.columns)
                }
                ToolButton {
                    objectName: "columnsPlus"
                    text: "+"
                    font.pixelSize: Math.round((24) * Theme.fontScale)
                    width: Theme.control; height: Theme.control
                    enabled: lab.columns < 12
                    onClicked: lab.controller.setBoardColumns(lab.columns + 2)
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Mehr Felder je Reihe")
                    Accessible.name: qsTr("Mehr Felder je Reihe")
                }
            }
            // Werkzeugkasten (Farbcode, E-Reihen, Lexikon)
            ToolButton {
                objectName: "toolboxButton"
                width: Theme.control; height: Theme.control
                text: "🧰"
                font.pixelSize: 24
                onClicked: toolbox.opened ? toolbox.close() : toolbox.open()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Werkzeugkasten")
                Accessible.name: qsTr("Werkzeugkasten")
                background: Rectangle {
                    radius: 6
                    color: toolbox.opened ? Theme.accent : (parent.hovered ? "#2A4A6E" : "transparent")
                }
            }
            // Messgeraet ein-/ausblenden
            ToolButton {
                objectName: "meterToggle"
                height: Theme.control
                width: 72
                checkable: true
                checked: lab.controller.meterVisible
                onClicked: lab.controller.meterVisible = !lab.controller.meterVisible
                ToolTip.visible: hovered
                ToolTip.text: lab.controller.meterVisible ? qsTr("Messgerät ausblenden") : qsTr("Messgerät einblenden")
                Accessible.name: ToolTip.text
                contentItem: Label {
                    text: "V A Ω"
                    color: "white"
                    font.pixelSize: Math.round((15) * Theme.fontScale)
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 6
                    color: parent.checked ? Theme.accent : (parent.hovered ? "#2A4A6E" : "transparent")
                    border.color: "#7F93A8"
                }
            }
            // Schaltplan ein-/ausblenden (ab Stufe 2 und im freien Labor)
            ToolButton {
                objectName: "schematicToggle"
                visible: lab.controller.schematicAvailable
                width: Theme.control; height: Theme.control
                checkable: true
                checked: lab.schematicVisible
                onClicked: lab.schematicVisible = !lab.schematicVisible
                ToolTip.visible: hovered
                ToolTip.text: lab.schematicVisible ? qsTr("Schaltplan ausblenden") : qsTr("Schaltplan zeigen")
                Accessible.name: ToolTip.text
                contentItem: Item {
                    SchematicSymbol { anchors.centerIn: parent; width: 40; height: 20; type: "widerstand"; stroke: "white" }
                }
                background: Rectangle {
                    radius: 6
                    color: parent.checked ? Theme.accent : (parent.hovered ? "#2A4A6E" : "transparent")
                    border.color: "#7F93A8"
                }
            }
            // Farbe fuer neue Leitungen: ein Knopf mit Auswahl (spart Platz in der Werkzeugleiste)
            ToolButton {
                id: wireColorButton
                objectName: "wireColorButton"
                width: Theme.control; height: Theme.control
                onClicked: wireColorMenu.open()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Leitungsfarbe: %1").arg(Texts.colorName(lab.newWireColor))
                Accessible.name: ToolTip.text
                contentItem: Item {
                    Rectangle {
                        anchors.centerIn: parent
                        width: 28; height: 28; radius: 14
                        color: Theme.wireColor(lab.newWireColor)
                        border.color: Theme.solderAmber
                        border.width: 3
                    }
                }
                background: Rectangle { radius: 6; color: parent.hovered ? "#2A4A6E" : "transparent" }
                Menu {
                    id: wireColorMenu
                    y: parent.height
                    Repeater {
                        model: Texts.wireColors
                        MenuItem {
                            id: colorItem
                            required property string modelData
                            objectName: "wireColor_" + modelData
                            height: Theme.control
                            text: Texts.colorName(modelData)
                            checkable: true
                            checked: lab.newWireColor === modelData
                            onTriggered: lab.newWireColor = modelData
                            // Farbe nie allein: Name daneben
                            contentItem: Row {
                                spacing: 10
                                leftPadding: 28
                                Rectangle {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 22; height: 22; radius: 11
                                    color: Theme.wireColor(colorItem.modelData)
                                    border.color: "#5A6F85"
                                }
                                Label { anchors.verticalCenter: parent.verticalCenter; text: colorItem.text; font.pixelSize: Math.round(15 * Theme.fontScale) }
                            }
                        }
                    }
                }
            }
            Label { // Entwicklerhilfe (VOLTA_SHOW_TIME=1): Simulationszeit, Echtzeitpruefung
                visible: lab.controller.showSimTime
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Zeit %1 s").arg(lab.controller.simTime.toLocaleString(Qt.locale(), "f", 1))
                color: Theme.solderAmber
                font.pixelSize: Math.round((15) * Theme.fontScale)
            }
        }
    }

    ExperimentPanel {
        id: experimentPanel
        objectName: "experimentPanel"
        controller: lab.controller
        anchors { right: parent.right; top: toolbar.bottom; bottom: parent.bottom }
        width: lab.controller.phase > 0 ? (collapsed ? Theme.control + 8 : 340) : 0
        visible: width > 0
    }

    SchematicView {
        id: schematicPanel
        objectName: "schematicPanel"
        controller: lab.controller
        anchors { right: experimentPanel.left; top: toolbar.bottom; bottom: parent.bottom }
        readonly property bool shown: lab.schematicVisible && lab.controller.schematicAvailable
        visible: shown
        width: shown ? Math.round((lab.width - tray.width - experimentPanel.width) * 0.42) : 0
        selectedPart: lab.selectedPart
        onPartSelected: (id) => { lab.selectedPart = id; lab.selectedWire = "" }
        onNodeSelected: (anchors) => lab.highlightFields = anchors
        onVisibleChanged: if (!visible) lab.highlightFields = []
    }

    PartTray {
        id: tray
        anchors { left: parent.left; top: toolbar.bottom; bottom: parent.bottom }
        parts: lab.controller.trayParts
        onDragUpdated: (type, scenePos) => {
            lab.trayType = type
            lab.trayPos = lab.toCanvas(scenePos)
            lab.highlightAnchor = lab.anchorAt(lab.trayPos, true)
        }
        onDragFinished: (type, scenePos) => {
            const field = lab.anchorAt(lab.toCanvas(scenePos), true)
            lab.trayType = ""
            lab.highlightAnchor = ""
            if (field !== "") lab.controller.addPart(type, field, false, type === "motor")
        }
    }

    Rectangle {
        id: boardArea
        anchors { left: tray.right; right: schematicPanel.left; top: toolbar.bottom; bottom: parent.bottom }
        color: Theme.board
        clip: true

        // Tippen ins Leere hebt die Auswahl auf
        TapHandler { onTapped: { lab.selectedPart = ""; lab.selectedWire = "" } }

        Item {
            id: canvas
            objectName: "canvas"
            x: Theme.margin
            y: Theme.margin
            width: Math.max(lab.controller.meterVisible ? lab.meterX + Theme.meterAreaWidth : lab.boardWidth,
                            lab.deskStripWidth)
            height: lab.moduleY + Theme.fieldHeight
            // passt die ganze Arbeitsflaeche ins Fenster (auch 1280 x 720)
            transformOrigin: Item.TopLeft
            scale: Math.min(1, (boardArea.width - 2 * Theme.margin) / width, (boardArea.height - 2 * Theme.margin) / height)

            ControlDesk {
                controller: lab.controller
                x: -8
                y: 0
                width: canvas.width + 16
                height: lab.deskPanelHeight
                batteryX: lab.groupCenter(0, 1) + 8
                p1X: lab.groupCenter(2, 4) + 8
                p2X: lab.groupCenter(5, 7) + 8
                switchX: lab.groupCenter(8, 9) + 8
                speakerX: lab.groupCenter(10, 11) + 8
            }

            Repeater {
                model: lab.groups
                GroupLine {
                    required property var modelData
                    y: lab.groupLineY
                    fromX: lab.deskX(modelData.first) - Theme.deskFieldWidth / 2 + 4
                    toX: lab.deskX(modelData.last) + Theme.deskFieldWidth / 2 - 4
                    symbol: modelData.symbol
                    onTapped: lab.highlightGroup = lab.highlightGroup === modelData.name ? "" : modelData.name
                }
            }

            // Dreifach-Messgeraet (eingeblendet ueber die Werkzeugleiste); die Buchsen liegen unten darin
            MeterPanel {
                objectName: "meterPanel"
                visible: lab.controller.meterVisible
                controller: lab.controller
                x: lab.meterX
                y: lab.boardY - 6
                width: Theme.meterAreaWidth
                height: 5 * Theme.fieldHeight + 4 * Theme.fieldGap + 12
            }

            // Alle Anschlusspunkte: Pultleiste, Messgeraet, Grundplatte, Modulleiste
            Repeater {
                model: lab.allAnchors
                ContactField {
                    id: field
                    required property string modelData
                    objectName: "field_" + modelData
                    anchorName: modelData
                    label: lab.isBoardField(modelData) ? modelData
                         : modelData.startsWith("M:") ? "M" + modelData.substring(2) : lab.deskLabel(modelData)
                    sockets: modelData.startsWith("M:") ? 1 : 5
                    pitch: lab.socketPitch(modelData)
                    width: lab.anchorSize(modelData).width
                    height: lab.anchorSize(modelData).height
                    x: lab.anchorPoint(modelData).x - width / 2
                    y: lab.anchorPoint(modelData).y - height / 2
                    color: modelData.startsWith("MESS:") ? "#D9DEE6" : Theme.field
                    highlighted: lab.highlightAnchor === modelData || lab.inHighlightedGroup(modelData)
                                 || lab.highlightFields.indexOf(modelData) >= 0
                    showVoltage: lab.controller.voltageFields.indexOf(modelData) >= 0

                    onWireDragUpdated: (scenePos) => {
                        lab.wireFrom = field.modelData
                        lab.wirePreviewEnd = lab.toCanvas(scenePos)
                        const target = lab.anchorAt(lab.wirePreviewEnd, false)
                        lab.highlightAnchor = target !== field.modelData ? target : ""
                    }
                    onWireDragFinished: (scenePos) => {
                        const target = lab.anchorAt(lab.toCanvas(scenePos), false)
                        lab.wireFrom = ""
                        lab.highlightAnchor = ""
                        // Loslassen ausserhalb eines Ziels verwirft die Leitung
                        if (target !== "" && target !== field.modelData)
                            lab.controller.addWire(field.modelData, target, lab.newWireColor)
                    }
                    onMenuRequested: (scenePos) => {
                        fieldMenu.anchorName = field.modelData
                        const p = lab.toCanvas(scenePos)
                        fieldMenu.popup(canvas, p.x, p.y)
                    }
                    Connections {
                        target: lab.controller
                        function onStateChanged() {
                            if (field.showVoltage) field.bars = lab.controller.voltageBars(field.anchorName)
                        }
                    }
                }
            }

            // Bauteile
            Repeater {
                model: lab.controller.parts
                PartItem {
                    id: part
                    required property var modelData
                    objectName: "part_" + modelData.id
                    partId: modelData.id
                    partType: modelData.type
                    value: modelData.type === "gluehlampe" ? lab.lampVolt(modelData) : modelData.value
                    closed: partState.closed === true
                    selected: lab.selectedPart === modelData.id
                    points: modelData.anchors.map((a, i) => lab.socketPoint(a, modelData.sockets[i]))
                    onTapped: {
                        lab.selectedPart = part.modelData.id
                        lab.selectedWire = ""
                        // Reedkontakt und Wechselschalter: Tippen schaltet um (Bedienhandlung)
                        if (part.modelData.type === "reedkontakt" || part.modelData.type === "wechselschalter")
                            lab.controller.setPartClosed(part.modelData.id, part.partState.closed !== true)
                    }
                    onPushed: (down) => {
                        if (part.modelData.type === "taster") lab.controller.setPartClosed(part.modelData.id, down)
                    }
                    onDragFinished: (delta) => {
                        const p = lab.anchorPoint(part.modelData.start)
                        const target = lab.anchorAt(Qt.point(p.x + delta.x, p.y + delta.y), true)
                        if (target !== "" && target !== part.modelData.start)
                            lab.controller.movePart(part.modelData.id, target, part.modelData.vertical,
                                                    part.modelData.longSpan)
                    }
                    onMenuRequested: (scenePos) => {
                        lab.selectedPart = part.modelData.id
                        partMenu.part = part.modelData
                        const p = lab.toCanvas(scenePos)
                        partMenu.popup(canvas, p.x, p.y)
                    }
                    Connections {
                        target: lab.controller
                        function onStateChanged() { part.partState = lab.controller.partState(part.partId) }
                    }
                }
            }

            // Leitungen
            Repeater {
                model: lab.controller.wires
                Item {
                    id: wire
                    required property var modelData
                    anchors.fill: parent
                    readonly property bool movingFrom: lab.moveWireId === modelData.id && lab.moveWireFromEnd
                    readonly property bool movingTo: lab.moveWireId === modelData.id && !lab.moveWireFromEnd
                    readonly property point fromPoint: movingFrom ? lab.wirePreviewEnd : lab.socketPoint(modelData.from, modelData.fromSocket)
                    readonly property point toPoint: movingTo ? lab.wirePreviewEnd : lab.socketPoint(modelData.to, modelData.toSocket)

                    WireItem {
                        id: wireShape
                        from: wire.fromPoint
                        to: wire.toPoint
                        wireColor: Theme.wireColor(wire.modelData.color)
                        selected: lab.selectedWire === wire.modelData.id
                        Connections {
                            target: lab.controller
                            function onStateChanged() { wireShape.currentmA = lab.controller.wireCurrent(wire.modelData.id) }
                        }
                    }
                    Repeater {
                        model: [true, false] // Anfang, Ende
                        WireEndHandle {
                            id: endHandle
                            required property bool modelData
                            objectName: "wireEnd_" + wire.modelData.id + (modelData ? "_von" : "_nach")
                            at: modelData ? wire.fromPoint : wire.toPoint
                            wireColor: Theme.wireColor(wire.modelData.color)
                            onMoved: (scenePos) => {
                                lab.moveWireId = wire.modelData.id
                                lab.moveWireFromEnd = endHandle.modelData
                                lab.wirePreviewEnd = lab.toCanvas(scenePos)
                                lab.highlightAnchor = lab.anchorAt(lab.wirePreviewEnd, false)
                            }
                            onReleased: (scenePos) => {
                                const target = lab.anchorAt(lab.toCanvas(scenePos), false)
                                lab.moveWireId = ""
                                lab.highlightAnchor = ""
                                if (target !== "") lab.controller.moveWireEnd(wire.modelData.id, endHandle.modelData, target)
                            }
                            onMenuRequested: (scenePos) => {
                                lab.selectedWire = wire.modelData.id
                                lab.selectedPart = ""
                                wireMenu.wireId = wire.modelData.id
                                const p = lab.toCanvas(scenePos)
                                wireMenu.popup(canvas, p.x, p.y)
                            }
                        }
                    }
                }
            }

            // Vorschau: neue Leitung
            WireItem {
                visible: lab.wireFrom !== ""
                from: lab.wireFrom !== "" ? lab.anchorPoint(lab.wireFrom) : Qt.point(0, 0)
                to: lab.wirePreviewEnd
                wireColor: Theme.wireColor(lab.newWireColor)
                opacity: 0.7
            }

            // Vorschau: Bauteil aus dem Fach
            Rectangle {
                visible: lab.trayType !== ""
                width: 64; height: 32; radius: 8
                x: lab.trayPos.x - width / 2
                y: lab.trayPos.y - height / 2
                color: Theme.solderAmber
                opacity: 0.8
                border.color: Theme.navy
                Label { anchors.centerIn: parent; text: Texts.partName(lab.trayType); font.pixelSize: Math.round((10) * Theme.fontScale) }
            }
        }

        // Fehler der Netzliste (Entwicklung)
        Column {
            anchors { right: parent.right; bottom: parent.bottom; margins: Theme.margin }
            Repeater {
                model: lab.controller.errors
                Label { required property string modelData; text: modelData; color: "#B00020" }
            }
        }
    }

    // ------------------------------------------------------------------ Menues
    Menu {
        id: fieldMenu
        property string anchorName
        MenuItem {
            text: qsTr("Spannungsanzeige ein/aus")
            onTriggered: lab.controller.toggleVoltageDisplay(fieldMenu.anchorName)
        }
    }

    Menu {
        id: partMenu
        objectName: "partMenu"
        property var part: ({})
        MenuItem {
            text: qsTr("Drehen")
            onTriggered: partMenu.part.pins.length === 4
                         ? lab.controller.rotatePart(partMenu.part.id) // Platine: 90 Grad im Uhrzeigersinn
                         : lab.controller.movePart(partMenu.part.id, partMenu.part.start, !partMenu.part.vertical,
                                                 partMenu.part.longSpan)
        }
        MenuItem {
            text: qsTr("Umdrehen (Polung)")
            onTriggered: lab.controller.flipPart(partMenu.part.id)
        }
        MenuItem {
            text: partMenu.part.longSpan ? qsTr("Kurz stecken") : qsTr("Lang stecken")
            enabled: partMenu.part.anchors !== undefined && partMenu.part.anchors.length === 2
            onTriggered: lab.controller.movePart(partMenu.part.id, partMenu.part.start, partMenu.part.vertical,
                                                 !partMenu.part.longSpan)
        }
        Menu {
            id: valueMenu
            title: qsTr("Wert")
            enabled: partMenu.part.type === "widerstand"
            Instantiator {
                model: Texts.resistorValues
                delegate: MenuItem {
                    required property var modelData
                    text: Texts.ohm(modelData)
                    checkable: true
                    checked: partMenu.part.value === modelData
                    onTriggered: lab.controller.setPartValue(partMenu.part.id, modelData)
                }
                onObjectAdded: (index, object) => valueMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => valueMenu.removeItem(object)
            }
        }
        Menu {
            id: voltMenu
            title: qsTr("Nennspannung")
            enabled: partMenu.part.type === "gluehlampe"
            Instantiator {
                model: lab.catalogEntry("gluehlampe").voltages || []
                delegate: MenuItem {
                    required property var modelData
                    text: qsTr("%1 V").arg(Number(modelData).toLocaleString(Qt.locale(), "f", modelData % 1 === 0 ? 0 : 1))
                    checkable: true
                    checked: Math.abs(lab.lampVolt(partMenu.part) - modelData) < 1e-9
                    onTriggered: lab.controller.setPartValue(partMenu.part.id, modelData)
                }
                onObjectAdded: (index, object) => voltMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => voltMenu.removeItem(object)
            }
        }
        MenuItem {
            text: qsTr("Reparieren")
            onTriggered: lab.controller.repairPart(partMenu.part.id)
        }
        MenuItem {
            text: qsTr("Entfernen")
            onTriggered: { lab.controller.removePart(partMenu.part.id); lab.selectedPart = "" }
        }
    }

    Menu {
        id: wireMenu
        property string wireId
        Instantiator {
            model: Texts.wireColors
            delegate: MenuItem {
                required property string modelData
                text: qsTr("Farbe: %1").arg(Texts.colorName(modelData))
                onTriggered: lab.controller.setWireColor(wireMenu.wireId, modelData)
            }
            onObjectAdded: (index, object) => wireMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => wireMenu.removeItem(object)
        }
        MenuItem {
            text: qsTr("Leitung löschen")
            onTriggered: { lab.controller.removeWire(wireMenu.wireId); lab.selectedWire = "" }
        }
    }

    // ------------------------------------------------------------------ Tastenkuerzel
    Shortcut { sequences: [StandardKey.Undo]; onActivated: lab.controller.undo() }
    Shortcut { sequences: ["Ctrl+Y", "Ctrl+Shift+Z"]; onActivated: lab.controller.redo() } // docs/ui-spec.md
    Shortcut {
        sequences: [StandardKey.Delete, "Backspace"]
        onActivated: {
            if (lab.selectedPart !== "") lab.controller.removePart(lab.selectedPart)
            else if (lab.selectedWire !== "") lab.controller.removeWire(lab.selectedWire)
            lab.selectedPart = ""
            lab.selectedWire = ""
        }
    }


    ToolboxPanel {
        id: toolbox
        objectName: "toolbox"
        controller: lab.controller
        y: toolbar.height
        height: lab.height - toolbar.height
        selectedPartId: lab.selectedPart
        selectedPartType: {
            const list = lab.controller.parts
            for (let i = 0; i < list.length; ++i) if (list[i].id === lab.selectedPart) return list[i].type
            return ""
        }
    }

    Toast {
        id: toast
        objectName: "toast"
        anchors.horizontalCenter: boardArea.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.margin
    }
}
