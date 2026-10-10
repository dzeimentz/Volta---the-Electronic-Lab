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
    property var speech: null          // Vorlesen (main.cpp); ohne: kein Vorlese-Knopf
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
    // Messgeraet oben im Pult: rechts neben der Pultleiste; seine Buchsen liegen dann in der Reihe der Pultleiste
    readonly property bool meterDocked: settings ? settings.meterDocked : false
    readonly property int meterDockX: deskStripWidth + Theme.fieldGap
    readonly property int meterJacksWidth: 3 * Theme.deskFieldWidth + 2 * Theme.deskFieldGap
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
        if (m >= 0 && meterDocked)
            return Qt.point(meterDockX + (Theme.meterDockWidth - meterJacksWidth) / 2 + Theme.deskFieldWidth / 2
                            + m * (Theme.deskFieldWidth + Theme.deskFieldGap), deskY + Theme.fieldHeight / 2)
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

    // Feld der Modulleiste unter einem Punkt ("" wenn keins)
    function slotAt(p) {
        for (let m = 1; m <= slots; ++m) {
            const c = anchorPoint("M:" + m)
            if (Math.abs(p.x - c.x) <= Theme.slotWidth / 2 + Theme.slotGap / 2 && Math.abs(p.y - c.y) <= Theme.fieldHeight / 2 + 40)
                return "M:" + m
        }
        return ""
    }
    // Anschlussname des Moduls auf einem Feld der Modulleiste ("" wenn frei)
    function modulePinAt(slotName) {
        const n = parseInt(slotName.substring(2))
        const list = controller.modules
        for (let i = 0; i < list.length; ++i)
            if (n >= list[i].slot && n < list[i].slot + list[i].width) return list[i].pins[n - list[i].slot] || ""
        return ""
    }

    // Werkzeuge (Taschenlampe, Foehn): Feldeinheiten (col 1..12, row 0..4 = A..E) <-> Arbeitsflaeche
    function toolPoint(col, row) {
        return Qt.point((col - 1) * (Theme.fieldWidth + Theme.fieldGap) + Theme.fieldWidth / 2,
                        boardY + row * (Theme.fieldHeight + Theme.fieldGap) + Theme.fieldHeight / 2)
    }
    function toolCell(p) {
        return Qt.point((p.x - Theme.fieldWidth / 2) / (Theme.fieldWidth + Theme.fieldGap) + 1,
                        (p.y - boardY - Theme.fieldHeight / 2) / (Theme.fieldHeight + Theme.fieldGap))
    }
    function toolOf(type) {
        const list = controller.tools
        for (let i = 0; i < list.length; ++i) if (list[i].type === type) return list[i]
        return null
    }
    // Licht und Waerme gibt es, sobald Fotowiderstand oder Heissleiter verfuegbar oder gesteckt sind
    readonly property bool environmentAvailable: {
        const tray = controller.trayParts
        for (let i = 0; i < tray.length; ++i)
            if ((tray[i].type === "fotowiderstand" || tray[i].type === "heissleiter" || tray[i].type === "feuchtesonde")
                    && !tray[i].locked) return true
        const parts = controller.parts
        for (let k = 0; k < parts.length; ++k)
            if (parts[k].type === "fotowiderstand" || parts[k].type === "heissleiter" || parts[k].type === "feuchtesonde") return true
        return controller.tools.length > 0 || controller.roomLight !== 2
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
    // Aufbauhilfe sichtbar (Aufbauphase, vom Kind eingeschaltet)
    readonly property bool guideVisible: controller.guideActive && controller.phase === 2
    property string selectedModule: ""
    // Steck- und Betriebsmodus (docs/ui-spec.md): Betrieb = Hauptschalter ein. Im Betrieb sind Bauteile, Leitungen
    // und Module fest; Taster, Schalter und Regler bleiben bedienbar. Wer im Betrieb umbauen will (aus dem Fach
    // ziehen, Leitung ziehen, aendernde Menueaktion, Rueckgaengig), schaltet damit automatisch aus: Steckmodus.
    readonly property bool operating: controller.mainSwitch
    function beginEdit() {
        if (!controller.mainSwitch) return
        controller.mainSwitch = false
        toast.show(qsTr("Strom aus – Steckmodus. Zum Testen den Hauptschalter wieder einschalten."))
    }
    // Aktionen (Farbe, Drehen, Loeschen ...) gibt es nur ueber das Kontextmenue: langer Druck bzw. Rechtsklick.
    // Beim ersten Auswaehlen einmal darauf hinweisen (Antippen selbst loest nie etwas Unumkehrbares aus).
    property bool menuHintShown: false
    function showMenuHint() {
        if (menuHintShown) return
        menuHintShown = true
        toast.show(qsTr("Langer Druck oder Rechtsklick öffnet das Menü (Farbe, Drehen, Löschen …)."))
    }
    property bool schematicVisible: settings ? settings.schematicVisible : false // Schaltplan parallel zum Aufbau
    property bool recorderVisible: false // Zeitschreiber unter der Arbeitsflaeche
    property var highlightFields: []        // Felder eines im Schaltplan angetippten Knotens
    property string selectedWire: ""
    property string newWireColor: settings ? settings.wireColor : "rot"
    // Arbeitsumgebung merken (AppSettings)
    onSchematicVisibleChanged: if (settings) settings.schematicVisible = schematicVisible
    onNewWireColorChanged: if (settings) settings.wireColor = newWireColor
    Connections {
        target: lab.controller
        // Messgeraet im freien Labor merken
        function onDeskChanged() { if (lab.settings && lab.controller.freeLab) lab.settings.meterVisible = lab.controller.meterVisible }
    }
    property string highlightAnchor: ""
    // Leitung ziehen (neu: wireFrom gesetzt) bzw. Ende umsetzen (moveWireId gesetzt)
    property string wireFrom: ""
    property string moveWireId: ""
    property bool moveWireFromEnd: false
    property point wirePreviewEnd
    // Modul aus dem Modulfach
    property string trayModule: ""
    // Bauteil aus dem Fach
    property string trayType: ""
    property var trayPreview: ({}) // Steckvorschau: alle Felder, die das gezogene Bauteil belegen wuerde
    property point trayPos

    Connections {
        target: lab.controller
        function onEditFailed() { toast.show(lab.controller.lastEditError) }
    }

    // ------------------------------------------------------------------ Werkzeugleiste
    Rectangle {
        id: toolbar
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Theme.control + 8 // Knoepfe 48 dp (Touch), Symbole darin kleiner
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
                contentItem: Item {
                    Image {
                        anchors.centerIn: parent
                        width: 24; height: 24
                        sourceSize: Qt.size(48, 48)
                        source: "qrc:/icons/home_light.svg"
                    }
                }
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
                font.pixelSize: Math.round(14 * Theme.fontScale)
                font.bold: true
                indicator: Label {
                    x: experimentBox.width - width - 12
                    anchors.verticalCenter: parent.verticalCenter
                    text: "▾"
                    color: "white"
                    font.pixelSize: 14
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
            // Modus: ✋ Stecken / ⚡ Betrieb (Symbol und Text, Farbe nie allein); schaltet den Hauptschalter
            ToolButton {
                id: modeButton
                objectName: "modeButton"
                anchors.verticalCenter: parent.verticalCenter
                height: Theme.control
                width: 118
                onClicked: lab.controller.mainSwitch = !lab.controller.mainSwitch
                ToolTip.visible: hovered
                ToolTip.text: lab.operating ? qsTr("Betrieb: Schaltung testen. Antippen schaltet aus und wechselt in den Steckmodus.")
                                            : qsTr("Steckmodus: aufbauen und ändern. Antippen schaltet ein und wechselt in den Betrieb.")
                Accessible.name: lab.operating ? qsTr("Betrieb") : qsTr("Stecken")
                contentItem: Label {
                    text: lab.operating ? "⚡ " + qsTr("Betrieb") : "✋ " + qsTr("Stecken")
                    color: lab.operating ? Theme.navy : "white"
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
                background: Rectangle {
                    radius: 6
                    color: lab.operating ? Theme.solderAmber : (modeButton.hovered ? "#2A4A6E" : "#1F3A57")
                    border.color: lab.operating ? "#FFE08A" : "#7F93A8"
                    border.width: 2
                }
            }
            ToolButton {
                objectName: "undoButton"
                text: "↶"
                font.pixelSize: 19
                width: Theme.control; height: Theme.control
                enabled: lab.controller.canUndo
                onClicked: { lab.beginEdit(); lab.controller.undo() }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Rückgängig: %1").arg(lab.controller.undoText)
                Accessible.name: qsTr("Rückgängig")
            }
            ToolButton {
                objectName: "redoButton"
                text: "↷"
                font.pixelSize: 19
                width: Theme.control; height: Theme.control
                enabled: lab.controller.canRedo
                onClicked: { lab.beginEdit(); lab.controller.redo() }
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
                    font.pixelSize: 19
                    width: Theme.control; height: Theme.control
                    enabled: lab.columns > 8
                    onClicked: { lab.beginEdit(); lab.controller.setBoardColumns(lab.columns - 2) }
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
                    font.pixelSize: 13
                    font.bold: true
                    Accessible.name: qsTr("Felder je Reihe: %1").arg(lab.columns)
                }
                ToolButton {
                    objectName: "columnsPlus"
                    text: "+"
                    font.pixelSize: 19
                    width: Theme.control; height: Theme.control
                    enabled: lab.columns < 12
                    onClicked: { lab.beginEdit(); lab.controller.setBoardColumns(lab.columns + 2) }
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Mehr Felder je Reihe")
                    Accessible.name: qsTr("Mehr Felder je Reihe")
                }
            }
            // Werkzeugkasten (Farbcode, E-Reihen, Lexikon)
            ToolButton {
                objectName: "toolboxButton"
                width: Theme.control; height: Theme.control
                contentItem: Item {
                    Image {
                        anchors.centerIn: parent
                        width: 26; height: 26
                        sourceSize: Qt.size(64, 64)
                        source: "qrc:/icons/toolbox_light.svg"
                        smooth: true
                    }
                }
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
                width: 60
                checkable: true
                checked: lab.controller.meterVisible
                onClicked: lab.controller.meterVisible = !lab.controller.meterVisible
                ToolTip.visible: hovered
                ToolTip.text: lab.controller.meterVisible ? qsTr("Messgerät ausblenden") : qsTr("Messgerät einblenden")
                Accessible.name: ToolTip.text
                contentItem: Label {
                    text: "V A Ω"
                    color: "white"
                    font.pixelSize: 13
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
                    SchematicSymbol { anchors.centerIn: parent; width: 30; height: 15; type: "widerstand"; stroke: "white" }
                }
                background: Rectangle {
                    radius: 6
                    color: parent.checked ? Theme.accent : (parent.hovered ? "#2A4A6E" : "transparent")
                    border.color: "#7F93A8"
                }
            }
            // Zeitschreiber ein-/ausblenden (ab Stufe 3 und im freien Labor)
            ToolButton {
                objectName: "recorderToggle"
                visible: lab.controller.recorderAvailable
                width: Theme.control; height: Theme.control
                checkable: true
                checked: lab.recorderVisible
                onClicked: lab.recorderVisible = !lab.recorderVisible
                text: "📈"
                font.pixelSize: 18
                ToolTip.visible: hovered
                ToolTip.text: lab.recorderVisible ? qsTr("Zeitschreiber ausblenden") : qsTr("Zeitschreiber zeigen")
                Accessible.name: ToolTip.text
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
                        width: 22; height: 22; radius: 11
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
        speech: lab.speech
        anchors { right: parent.right; top: toolbar.bottom; bottom: parent.bottom }
        width: lab.controller.phase > 0 ? (collapsed ? Theme.control + 8 : 340) : 0
        collapsed: lab.settings ? lab.settings.experimentPanelCollapsed : false
        onCollapsedChanged: if (lab.settings) lab.settings.experimentPanelCollapsed = collapsed
        visible: width > 0
    }

    SchematicView {
        id: schematicPanel
        objectName: "schematicPanel"
        controller: lab.controller
        anchors { right: experimentPanel.left; top: toolbar.bottom; bottom: recorderPanel.top }
        readonly property bool shown: lab.schematicVisible && lab.controller.schematicAvailable
        visible: shown
        width: shown ? Math.round((lab.width - tray.width - experimentPanel.width) * 0.5) : 0
        selectedPart: lab.selectedPart
        onPartSelected: (id) => { lab.selectedPart = id; lab.selectedWire = "" }
        onNodeSelected: (anchors) => lab.highlightFields = anchors
        onVisibleChanged: if (!visible) lab.highlightFields = []
    }

    PartTray {
        id: tray
        anchors { left: parent.left; top: toolbar.bottom; bottom: parent.bottom }
        parts: lab.controller.trayParts
        // Aufbauhilfe: das Fach zeigt, was der naechste Schritt braucht
        Connections {
            target: lab.controller
            function onGuideChanged() {
                const k = lab.controller.guideStep.kind
                if (k === "modul") tray.showModules = tray.hasModules
                else if (k === "bauteil") tray.showModules = false
            }
        }
        onLockedTapped: (type, stage) => toast.show(qsTr("%1 wird mit Stufe %2 freigeschaltet.").arg(Texts.partName(type)).arg(stage))
        onDragUpdated: (type, scenePos) => {
            lab.beginEdit()
            lab.trayType = type
            lab.trayPos = lab.toCanvas(scenePos)
            const start = lab.anchorAt(lab.trayPos, true)
            lab.highlightAnchor = start
            lab.trayPreview = start !== "" ? lab.controller.partPreview(type, start, false, type === "motor") : ({})
        }
        onDragFinished: (type, scenePos) => {
            const field = lab.anchorAt(lab.toCanvas(scenePos), true)
            lab.trayType = ""
            lab.highlightAnchor = ""
            lab.trayPreview = ({})
            if (field !== "") lab.controller.addPart(type, field, false, type === "motor")
        }
        modules: lab.controller.trayModules
        onModuleLockedTapped: (type, stage) => toast.show(qsTr("%1 wird mit Stufe %2 freigeschaltet.").arg(Texts.moduleName(type)).arg(stage))
        onModuleDragUpdated: (type, scenePos) => {
            lab.beginEdit()
            lab.trayModule = type
            lab.trayPos = lab.toCanvas(scenePos)
            lab.highlightAnchor = lab.slotAt(lab.trayPos)
        }
        onModuleDragFinished: (type, scenePos) => {
            const slot = lab.slotAt(lab.toCanvas(scenePos))
            lab.trayModule = ""
            lab.highlightAnchor = ""
            if (slot !== "") lab.controller.addModule(type, slot)
            else toast.show(qsTr("Module gehören in die Modulleiste unten."))
        }
    }

    RecorderPanel {
        id: recorderPanel
        objectName: "recorderPanel"
        controller: lab.controller
        readonly property bool shown: lab.recorderVisible && lab.controller.recorderAvailable
        visible: shown
        anchors { left: tray.right; right: experimentPanel.left; bottom: parent.bottom }
        height: shown ? Math.max(170, Math.min(240, lab.height * 0.28)) : 0
        onCloseRequested: lab.recorderVisible = false
    }

    Rectangle {
        id: boardArea
        anchors { left: tray.right; right: schematicPanel.left; top: toolbar.bottom; bottom: recorderPanel.top }
        color: Theme.board
        clip: true

        // Tippen ins Leere hebt die Auswahl auf
        TapHandler { onTapped: { lab.selectedPart = ""; lab.selectedWire = ""; lab.selectedModule = "" } }

        Item {
            id: canvas
            objectName: "canvas"
            x: Theme.margin
            y: Theme.margin
            width: Math.max(!lab.controller.meterVisible ? lab.boardWidth
                            : lab.meterDocked ? Math.max(lab.boardWidth, lab.meterDockX + Theme.meterDockWidth)
                            : lab.meterX + Theme.meterAreaWidth,
                            lab.deskStripWidth)
            height: lab.moduleY + Theme.fieldHeight + Theme.control + 8 // Modulleiste mit Namensschildern
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
                docked: lab.meterDocked
                onDockToggled: if (lab.settings) lab.settings.meterDocked = !lab.settings.meterDocked
                x: docked ? lab.meterDockX : lab.meterX
                y: docked ? 4 : lab.boardY - 6
                width: docked ? Theme.meterDockWidth : Theme.meterAreaWidth
                height: docked ? lab.deskPanelHeight - 8 : 5 * Theme.fieldHeight + 4 * Theme.fieldGap + 12
            }
            // Gruppenlinie ueber den Buchsen, wenn das Messgeraet im Pult sitzt
            GroupLine {
                visible: lab.controller.meterVisible && lab.meterDocked
                y: lab.groupLineY
                fromX: lab.meterDockX + (Theme.meterDockWidth - lab.meterJacksWidth) / 2 + 4
                toX: fromX + lab.meterJacksWidth - 8
                symbol: "V A Ω"
                onTapped: lab.highlightGroup = lab.highlightGroup === "messgeraet" ? "" : "messgeraet"
            }

            // Module in der Modulleiste (unter den Feldern, die sie belegen)
            Repeater {
                model: lab.controller.modules
                ModuleItem {
                    id: moduleItem
                    required property var modelData
                    objectName: "module_" + modelData.id
                    controller: lab.controller
                    moduleId: modelData.id
                    moduleType: modelData.type
                    slotCount: modelData.width
                    selected: lab.selectedModule === modelData.id
                    movable: !lab.operating
                    x: lab.anchorPoint("M:" + modelData.slot).x - Theme.slotWidth / 2 - 6
                    y: lab.moduleY - 6
                    onTapped: { lab.selectedModule = modelData.id; lab.selectedPart = ""; lab.selectedWire = "" }
                    onInsideRequested: moduleInside.show(modelData.id, modelData.type)
                    onMenuRequested: (scenePos) => {
                        lab.selectedModule = modelData.id
                        moduleMenu.moduleId = modelData.id
                        moduleMenu.moduleType = modelData.type
                        const p = lab.toCanvas(scenePos)
                        moduleMenu.popup(canvas, p.x, p.y)
                    }
                    onDragFinished: (delta) => {
                        const p = lab.anchorPoint("M:" + moduleItem.modelData.slot)
                        const target = lab.slotAt(Qt.point(p.x + delta.x, p.y))
                        if (target !== "" && target !== "M:" + moduleItem.modelData.slot)
                            lab.controller.moveModule(moduleItem.modelData.id, target)
                    }
                }
            }

            // Alle Anschlusspunkte: Pultleiste, Messgeraet, Grundplatte, Modulleiste
            Repeater {
                model: lab.allAnchors
                ContactField {
                    id: field
                    required property string modelData
                    objectName: "field_" + modelData
                    anchorName: modelData
                    // Modulleiste: unter einem Modul der Anschlussname, sonst die Feldnummer
                    readonly property string modulePin: modelData.startsWith("M:") ? lab.modulePinAt(modelData) : ""
                    label: lab.isBoardField(modelData) ? modelData
                         : modulePin !== "" ? Texts.pinName(modulePin)
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
                                 || (lab.trayPreview.fields || []).indexOf(modelData) >= 0
                    showVoltage: lab.controller.voltageFields.indexOf(modelData) >= 0
                    recorderChannel: lab.controller.recorderChannels.indexOf(modelData.startsWith("MESS:") ? "MESS" : modelData)
                    target: lab.guideVisible && (lab.controller.guideStep.targets || []).indexOf(modelData) >= 0

                    onWireDragUpdated: (scenePos) => {
                        lab.beginEdit()
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
                    movable: !lab.operating
                    points: modelData.anchors.map((a, i) => lab.socketPoint(a, modelData.sockets[i]))
                    onTapped: {
                        lab.selectedPart = part.modelData.id
                        lab.selectedWire = ""
                        lab.showMenuHint()
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
                            movable: !lab.operating
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
                            onTapped: {
                                lab.selectedWire = wire.modelData.id
                                lab.selectedPart = ""
                                lab.selectedModule = ""
                                lab.showMenuHint()
                            }
                            onMenuRequested: (scenePos) => {
                                lab.selectedWire = wire.modelData.id
                                lab.selectedPart = ""
                                wireMenu.wireId = wire.modelData.id
                                wireMenu.wireColor = wire.modelData.color
                                const p = lab.toCanvas(scenePos)
                                wireMenu.popup(canvas, p.x, p.y)
                            }
                        }
                    }
                }
            }

            // Aufbauhilfe: das naechste Teil schemenhaft an seinem Platz (nimmt keine Eingaben an)
            PartItem {
                objectName: "guideGhostPart"
                readonly property var step: lab.controller.guideStep
                visible: lab.guideVisible && step.kind === "bauteil"
                enabled: false
                opacity: 0.45
                partId: ""
                partType: step.type || ""
                value: step.value || 0
                points: visible ? step.pins.map(p => lab.socketPoint(p.field, 2)) : []
            }
            WireItem {
                objectName: "guideGhostWire"
                readonly property var step: lab.controller.guideStep
                visible: lab.guideVisible && step.kind === "leitung"
                from: visible ? lab.anchorPoint(step.from) : Qt.point(0, 0)
                to: visible ? lab.anchorPoint(step.to) : Qt.point(0, 0)
                wireColor: Theme.wireColor(step.color || "rot")
                opacity: 0.45
            }
            Rectangle {
                readonly property var step: lab.controller.guideStep
                readonly property var slots: step.targets || []
                visible: lab.guideVisible && step.kind === "modul" && slots.length > 0
                x: visible ? lab.anchorPoint(slots[0]).x - Theme.slotWidth / 2 - 6 : 0
                y: lab.moduleY - 6
                width: slots.length * (Theme.slotWidth + Theme.slotGap) - Theme.slotGap + 12
                height: Theme.fieldHeight + 6 + Theme.control + 8
                radius: 8
                color: "#556A4C8C"
                border.color: "#1E7BE0"
                border.width: 2
                Label {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 14
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: Texts.moduleName(parent.step.type || "")
                    color: "white"
                    font.bold: true
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

            // Vorschau: Modul aus dem Modulfach
            Rectangle {
                visible: lab.trayModule !== ""
                width: 4 * (Theme.slotWidth + Theme.slotGap) - Theme.slotGap; height: 32; radius: 8
                x: lab.trayPos.x - Theme.slotWidth / 2
                y: lab.trayPos.y - height / 2
                color: "#D8CCE8"
                opacity: 0.85
                border.color: Theme.navy
                Label { anchors.centerIn: parent; text: Texts.moduleName(lab.trayModule); font.pixelSize: Math.round((11) * Theme.fontScale) }
            }

            // Raumlicht: gedaempfte Arbeitsflaeche bei Daemmerung und Dunkelheit (zusaetzlich die Anzeige im Werkzeugfeld)
            Rectangle {
                x: -Theme.margin; y: lab.boardY - Theme.fieldGap
                width: canvas.width + 2 * Theme.margin
                height: canvas.height - y + Theme.margin
                color: "#0B1A2A"
                opacity: lab.controller.roomLight === 0 ? 0.45 : lab.controller.roomLight === 1 ? 0.2 : 0
                Behavior on opacity { enabled: !Theme.reduceMotion; NumberAnimation { duration: 250 } }
            }
            // Werkzeuge auf der Grundplatte
            Repeater {
                model: lab.controller.tools
                ToolToken {
                    required property var modelData
                    objectName: "tool_" + modelData.type
                    readonly property point at: lab.toolPoint(modelData.col, modelData.row)
                    toolType: modelData.type
                    on: modelData.on
                    reach: (modelData.type === "taschenlampe" ? 1.2 : modelData.type === "wasser" ? 0.8 : 1.5) * (Theme.fieldWidth + Theme.fieldGap)
                    x: at.x - width / 2
                    y: at.y - height / 2
                    z: 20
                    onMoved: (delta) => {
                        const c = lab.toolCell(Qt.point(at.x + delta.x, at.y + delta.y))
                        lab.controller.placeTool(modelData.type, c.x, c.y)
                    }
                    onToggled: lab.controller.setToolOn(modelData.type, !modelData.on)
                    onMenuRequested: (scenePos) => {
                        toolMenu.toolType = modelData.type
                        toolMenu.toolOn = modelData.on
                        const p = lab.toCanvas(scenePos)
                        toolMenu.popup(canvas, p.x, p.y)
                    }
                }
            }

            // Vorschau: Bauteil aus dem Fach schemenhaft an dem Platz, an dem es landen wuerde (alle Felder sichtbar)
            PartItem {
                objectName: "trayGhostPart"
                visible: lab.trayType !== "" && lab.trayPreview.ok === true
                enabled: false
                opacity: 0.6
                partId: ""
                partType: lab.trayType
                points: visible ? lab.trayPreview.pins.map(p => lab.socketPoint(p.field, 2)) : []
            }
            Rectangle {
                visible: lab.trayType !== "" && lab.trayPreview.ok !== true
                width: 64; height: 32; radius: 8
                x: lab.trayPos.x - width / 2
                y: lab.trayPos.y - height / 2
                color: Theme.solderAmber
                opacity: 0.8
                border.color: Theme.navy
                Label { anchors.centerIn: parent; text: Texts.partName(lab.trayType); font.pixelSize: Math.round((10) * Theme.fontScale) }
            }
        }

        // Werkzeugleiste fuer Licht und Waerme (unten links, klappt automatisch aus und ein): Raumlicht umschalten,
        // Taschenlampe und Foehn auf die Grundplatte ziehen oder antippen (legt sie in die Mitte); Bedienhandlungen,
        // auch im Betrieb. Weitere Werkzeuge kommen als weitere Kinder dazu.
        FlyoutBar {
            id: environmentBar
            objectName: "environmentBar"
            visible: lab.environmentAvailable
            // unten mittig, wie die Werkzeuggruppen in Visual Technic Studio
            anchors { horizontalCenter: parent.horizontalCenter; bottom: parent.bottom; bottomMargin: 4 }
            z: 30
            title: qsTr("Licht, Wärme, Wasser")
            count: 4
                Button {
                    objectName: "roomLightButton"
                    height: Theme.control
                    width: 132
                    readonly property int level: lab.controller.roomLight
                    text: level === 2 ? "☀ " + qsTr("Raum hell") : level === 1 ? "⛅ " + qsTr("Dämmerung") : "🌙 " + qsTr("Dunkel")
                    onClicked: lab.controller.roomLight = (level + 2) % 3 // hell -> Daemmerung -> dunkel -> hell
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Raumlicht umschalten")
                    font.pixelSize: Math.round(13 * Theme.fontScale)
                }
                Repeater {
                    model: ["taschenlampe", "foehn", "wasser"]
                    Rectangle {
                        id: envTile
                        required property string modelData
                        objectName: "envTool_" + modelData
                        width: 112; height: Theme.control
                        radius: 8
                        readonly property bool placed: lab.toolOf(modelData) !== null
                        color: envDrag.active ? Theme.solderAmber : (placed ? "#D5DCE3" : "#FFF6DD")
                        border.color: Theme.navy
                        Row {
                            anchors.centerIn: parent
                            spacing: 6
                            Text { anchors.verticalCenter: parent.verticalCenter; text: envTile.modelData === "taschenlampe" ? "🔦" : envTile.modelData === "wasser" ? "💧" : "♨"; font.pixelSize: 20; color: "#B23A10" }
                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                text: envTile.modelData === "taschenlampe" ? qsTr("Taschenlampe") : envTile.modelData === "wasser" ? qsTr("Wassertropfen") : qsTr("Föhn")
                                font.pixelSize: Math.round(12 * Theme.fontScale)
                                color: Theme.navy
                            }
                        }
                        property point lastScenePos
                        DragHandler {
                            id: envDrag
                            target: null
                            onCentroidChanged: if (active) envTile.lastScenePos = centroid.scenePosition
                            onActiveChanged: {
                                environmentBar.busy = active // waehrend des Ziehens offen bleiben
                                if (active) return
                                const c = lab.toolCell(lab.toCanvas(envTile.lastScenePos))
                                lab.controller.placeTool(envTile.modelData, c.x, c.y)
                            }
                        }
                        TapHandler {
                            onTapped: {
                                const t = lab.toolOf(envTile.modelData)
                                if (t === null) lab.controller.placeTool(envTile.modelData, Math.max(1, lab.columns / 2), 2)
                                else lab.controller.setToolOn(envTile.modelData, !t.on)
                            }
                        }
                        Accessible.name: (modelData === "taschenlampe" ? qsTr("Taschenlampe") : modelData === "wasser" ? qsTr("Wassertropfen") : qsTr("Föhn"))
                    }
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
            enabled: !fieldMenu.anchorName.startsWith("MESS:")
            onTriggered: lab.controller.toggleVoltageDisplay(fieldMenu.anchorName)
        }
        // Zeitschreiber: Feld (Spannung gegen 0 V) bzw. Messgeraet (seine Anzeige) auf Kanal 1 oder 2
        Repeater {
            model: lab.controller.recorderAvailable ? 2 : 0
            MenuItem {
                required property int index
                objectName: "recordChannel_" + index
                text: fieldMenu.anchorName.startsWith("MESS:")
                      ? qsTr("Zeitschreiber Kanal %1: Messgerät").arg(index + 1)
                      : qsTr("Zeitschreiber Kanal %1: hier messen").arg(index + 1)
                onTriggered: {
                    lab.controller.setRecorderChannel(index, fieldMenu.anchorName.startsWith("MESS:") ? "MESS" : fieldMenu.anchorName)
                    lab.recorderVisible = true
                }
            }
        }
    }

    Menu {
        id: partMenu
        objectName: "partMenu"
        property var part: ({})
        MenuItem {
            text: qsTr("Drehen")
            onTriggered: {
                lab.beginEdit()
                if (partMenu.part.pins.length === 4) lab.controller.rotatePart(partMenu.part.id) // Platine: 90 Grad
                else lab.controller.movePart(partMenu.part.id, partMenu.part.start, !partMenu.part.vertical,
                                             partMenu.part.longSpan)
            }
        }
        MenuItem {
            text: qsTr("Umdrehen (Polung)")
            onTriggered: { lab.beginEdit(); lab.controller.flipPart(partMenu.part.id) }
        }
        MenuItem {
            text: partMenu.part.longSpan ? qsTr("Kurz stecken") : qsTr("Lang stecken")
            enabled: partMenu.part.anchors !== undefined && partMenu.part.anchors.length === 2
            onTriggered: {
                lab.beginEdit()
                lab.controller.movePart(partMenu.part.id, partMenu.part.start, partMenu.part.vertical, !partMenu.part.longSpan)
            }
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
                    onTriggered: { lab.beginEdit(); lab.controller.setPartValue(partMenu.part.id, modelData) }
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
                    onTriggered: { lab.beginEdit(); lab.controller.setPartValue(partMenu.part.id, modelData) }
                }
                onObjectAdded: (index, object) => voltMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => voltMenu.removeItem(object)
            }
        }
        MenuItem {
            text: qsTr("Reparieren")
            onTriggered: { lab.beginEdit(); lab.controller.repairPart(partMenu.part.id) }
        }
        MenuSeparator {}
        MenuItem {
            objectName: "partMenuRemove"
            text: "🗑 " + qsTr("Entfernen")
            onTriggered: { lab.beginEdit(); lab.controller.removePart(partMenu.part.id); lab.selectedPart = "" }
        }
    }

    Menu {
        id: moduleMenu
        objectName: "moduleMenu"
        property string moduleId
        property string moduleType
        MenuItem {
            text: qsTr("Blick hinein")
            onTriggered: moduleInside.show(moduleMenu.moduleId, moduleMenu.moduleType)
        }
        MenuItem {
            text: qsTr("Herausnehmen")
            onTriggered: { lab.beginEdit(); lab.controller.removeModule(moduleMenu.moduleId); lab.selectedModule = "" }
        }
    }

    ModuleInside {
        id: moduleInside
        objectName: "moduleInside"
        controller: lab.controller
    }

    Menu {
        id: toolMenu
        objectName: "toolMenu"
        property string toolType
        property bool toolOn
        MenuItem {
            text: toolMenu.toolOn ? qsTr("Ausschalten") : qsTr("Einschalten")
            onTriggered: lab.controller.setToolOn(toolMenu.toolType, !toolMenu.toolOn)
        }
        MenuItem {
            text: qsTr("Wegräumen")
            onTriggered: lab.controller.removeTool(toolMenu.toolType)
        }
    }

    Menu {
        id: wireMenu
        objectName: "wireMenu"
        property string wireId
        property string wireColor
        Instantiator {
            model: Texts.wireColors
            delegate: MenuItem {
                required property string modelData
                objectName: "wireMenuColor_" + modelData
                text: qsTr("Farbe: %1").arg(Texts.colorName(modelData))
                checkable: true
                checked: wireMenu.wireColor === modelData
                onTriggered: lab.controller.setWireColor(wireMenu.wireId, modelData)
            }
            onObjectAdded: (index, object) => wireMenu.insertItem(index, object)
            onObjectRemoved: (index, object) => wireMenu.removeItem(object)
        }
        MenuSeparator {}
        MenuItem {
            objectName: "wireMenuRemove"
            text: "🗑 " + qsTr("Leitung löschen")
            onTriggered: { lab.beginEdit(); lab.controller.removeWire(wireMenu.wireId); lab.selectedWire = "" }
        }
    }

    // ------------------------------------------------------------------ Tastenkuerzel
    Shortcut { sequences: [StandardKey.Undo]; onActivated: { lab.beginEdit(); lab.controller.undo() } }
    Shortcut { sequences: ["Ctrl+Y", "Ctrl+Shift+Z"]; onActivated: { lab.beginEdit(); lab.controller.redo() } } // docs/ui-spec.md
    Shortcut {
        sequences: [StandardKey.Delete, "Backspace"]
        onActivated: {
            if (lab.selectedPart !== "" || lab.selectedWire !== "" || lab.selectedModule !== "") lab.beginEdit()
            if (lab.selectedPart !== "") lab.controller.removePart(lab.selectedPart)
            else if (lab.selectedWire !== "") lab.controller.removeWire(lab.selectedWire)
            else if (lab.selectedModule !== "") lab.controller.removeModule(lab.selectedModule)
            lab.selectedPart = ""
            lab.selectedWire = ""
            lab.selectedModule = ""
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
