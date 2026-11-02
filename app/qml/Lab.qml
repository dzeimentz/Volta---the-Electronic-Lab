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
    signal helpRequested()  // Hilfe mit Tick (liegt im Hauptfenster)
    signal quickGuideRequested()
    signal helpTopicRequested(string key) // Hilfe bei einem Kapitel
    signal parentAreaRequested()
    Binding { target: Theme; property: "reduceMotion"; value: lab.settings ? lab.settings.reduceMotion : false }
    Binding { target: Theme; property: "showCurrentFlow"; value: lab.settings ? lab.settings.showCurrentFlow : true }

    // ------------------------------------------------------------------ Geometrie
    // Pultbauteile mit ihren Feldern; die Stufenanpassung blendet nicht benoetigte Bauteile aus (controller.deskGroups).
    // Jedes Bauteil steht in einem Rahmen, der Bedienelement und Felder einfasst (ControlDesk).
    readonly property var deskGroupDefs: [
        {name: "batterie", fields: ["P:+9V", "P:0V"], minWidth: 128},
        {name: "batterie2", fields: ["P:+B2", "P:-B2"], minWidth: 128}, // ab Volta 2 (Pult nach Bedarf)
        {name: "wechselquelle", fields: ["P:W~a", "P:W~b"], minWidth: 128}, // ab Volta 2
        {name: "generator", fields: ["P:G", "P:G0"], minWidth: 128},        // ab Volta 2
        {name: "kennlinienschreiber", fields: ["P:KS+", "P:KS-"], minWidth: 128}, // ab Volta 2
        {name: "p1", fields: ["P:P1a", "P:P1s", "P:P1b"], minWidth: 120},
        {name: "p2", fields: ["P:P2a", "P:P2s", "P:P2b"], minWidth: 120},
        {name: "schalter", fields: ["P:S1", "P:S2"], minWidth: 96},
        {name: "lautsprecher", fields: ["P:LS1", "P:LS2"], minWidth: 104}
    ]
    readonly property int deskFramePad: 8   // Rand zwischen Rahmen und Feldern
    readonly property int deskFrameGap: 12  // Abstand zwischen zwei Rahmen
    // sichtbare Gruppen mit Lage: {name, fields, left, right (Rahmen), first, last (Index in deskNames)}
    readonly property var groups: {
        const shown = controller.deskGroups
        let out = [], x = 0, idx = 0
        for (let g = 0; g < deskGroupDefs.length; ++g) {
            const d = deskGroupDefs[g]
            if (shown.indexOf(d.name) < 0) continue
            const fw = d.fields.length * (Theme.deskFieldWidth + Theme.deskFieldGap) - Theme.deskFieldGap
            const w = Math.max(fw + 2 * deskFramePad, d.minWidth)
            out.push({name: d.name, fields: d.fields, left: x, right: x + w, fieldsLeft: x + (w - fw) / 2,
                      first: idx, last: idx + d.fields.length - 1})
            x += w + deskFrameGap
            idx += d.fields.length
        }
        return out
    }
    readonly property var deskNames: {
        let a = []
        for (let g = 0; g < groups.length; ++g) a = a.concat(groups[g].fields)
        return a
    }
    function openTracer() { tracerPanel.open() }
    function groupOf(name) {
        for (let g = 0; g < groups.length; ++g) if (groups[g].name === name) return groups[g]
        return null
    }
    // Buchsen des Dreifach-Messgeraets: unten im Messgeraet rechts neben der Grundplatte (nur eingeblendet)
    readonly property var meterNames: ["MESS:COM", "MESS:VOHM", "MESS:A"]
    // Pult nach Bedarf: rechts neben dem letzten Rahmen der Knopf »+« zum Geraetefach
    readonly property int deskTrayX: (groups.length > 0 ? groups[groups.length - 1].right : 0) + deskFrameGap
    readonly property int deskStripWidth: controller.deskOnDemand ? deskTrayX + Theme.control
                                        : groups.length > 0 ? groups[groups.length - 1].right : 0
    // runde Knoepfe +/- fuer die Breite der Grundplatte: rechts neben den Feldern, in der Hoehe zentriert
    readonly property int sizerX: boardWidth + Theme.fieldGap
    readonly property int sizerY: boardY + (rows.length * (Theme.fieldHeight + Theme.fieldGap) - Theme.fieldGap) / 2
                                  - (2 * Theme.control + 16) / 2
    readonly property int meterX: sizerX + Theme.control + Theme.fieldGap
    // Muelleimer unten rechts in Hoehe der Modulleiste, buendig mit dem rechten Feldrand (nie ueber M16)
    readonly property int slotsRight: slots * (Theme.slotWidth + Theme.slotGap) - Theme.slotGap
    readonly property bool showModules: controller.moduleBarVisible // Stufenanpassung: nur wenn Module gebraucht werden
    readonly property int trashX: showModules ? Math.max(boardWidth - Theme.control, slotsRight + 12) : boardWidth - Theme.control
    readonly property int trashY: moduleY + (Theme.fieldHeight - Theme.control) / 2
    property bool trashHot: false
    property var checkMarks: [] // Befunde der Fehlersuche: nummerierte Marken an ihrer ersten Stelle // ein Bauteil oder Modul wird gerade ueber den Muelleimer gezogen
    // Verschiebung beim Ziehen (Szenenpunkte) in Punkte der skalierten Arbeitsflaeche umrechnen
    function toCanvasDelta(d) { return Qt.point(d.x / canvas.scale, d.y / canvas.scale) }
    function overTrash(p) {
        return p.x >= trashX - 12 && p.x <= trashX + Theme.control + 12
            && p.y >= trashY - 16 && p.y <= trashY + Theme.control + 16
    }
    // Mitte eines Bauteils (Durchschnitt seiner Felder) in canvas-Koordinaten
    function partCenter(part) {
        let x = 0, y = 0
        for (let i = 0; i < part.anchors.length; ++i) { const p = anchorPoint(part.anchors[i]); x += p.x; y += p.y }
        return Qt.point(x / part.anchors.length, y / part.anchors.length)
    }
    // Messgeraet oben im Pult: rechts neben der Pultleiste; seine Buchsen liegen dann in der Reihe der Pultleiste
    readonly property bool meterDocked: settings ? settings.meterDocked : false
    // Volta 1: Dreifach-Messgeraet mit Buchsen; ab Volta 2 das Anzeigegeraet fuer die Messstellen (Konzept 5.2)
    readonly property bool classicMeter: controller.band === "" || controller.band === "volta1"
    readonly property int meterDockX: deskStripWidth + Theme.fieldGap
    // Anzeigegeraet mit drei Anzeigen: im Pult breiter (Knoepfe >= 48 dp), neben der Grundplatte eine Reihe hoeher
    readonly property bool tripleDisplay: !classicMeter && controller.displayCount >= 3
    readonly property int meterDockWidthNow: Theme.meterDockWidth + (tripleDisplay ? 160 : 0)
    readonly property int meterRows: tripleDisplay ? 6 : 5
    readonly property int meterJacksWidth: 3 * Theme.deskFieldWidth + 2 * Theme.deskFieldGap
    readonly property var rows: ["A", "B", "C", "D", "E"].slice(0, controller.boardRows) // Stufenanpassung: ab 2 Reihen
    readonly property int columns: controller.boardColumns // 5, 6 (Stufenanpassung), 8, 10 oder 12 (+/-)
    // +/- fuer die Breite: im freien Labor; im gefuehrten Experiment nur ohne Stufenanpassung
    // Volta 1: im Experiment waechst die Grundplatte mit der Stufe; ab Volta 2 bleiben 8, 10 und 12 waehlbar
    readonly property bool boardResizable: controller.freeLab || !controller.stageLayout || controller.startColumns > 0
    readonly property int slots: 16
    // Pult (Bedienelemente), Gruppenlinien, Pultleiste, Grundplatte, Modulleiste von oben nach unten
    readonly property int deskPanelHeight: 140 // Bedienelemente und Schaltzeichen; darunter im selben Pult die Felder
    readonly property int groupLineY: deskPanelHeight + 6
    readonly property int deskY: deskPanelHeight + 4
    readonly property int deskBottom: deskY + Theme.fieldHeight + 12 // Unterkante des Pults (schliesst die Felder ein)
    readonly property int boardY: deskBottom + Theme.fieldGap + 4
    readonly property int boardWidth: columns * Theme.fieldWidth + (columns - 1) * Theme.fieldGap
    // Messgeraet neben der Grundplatte ist 5 Reihen hoch: die Modulleiste rutscht bei kleinem Steckbrett darunter
    readonly property int moduleY: boardY + Math.max(rows.length, controller.meterVisible && !meterDocked ? meterRows : 0)
                                   * (Theme.fieldHeight + Theme.fieldGap) + Theme.fieldGap
    readonly property var allAnchors: {
        let a = controller.meterVisible && classicMeter ? deskNames.concat(meterNames) : deskNames.slice()
        for (let r = 0; r < rows.length; ++r)
            for (let c = 1; c <= columns; ++c) a.push(rows[r] + c)
        if (showModules) for (let m = 1; m <= slots; ++m) a.push("M:" + m)
        return a
    }

    // Pluspol der Batterie heisst V+ (intern weiter P:+9V), Minuspol 0V mit GND darunter
    function deskLabel(name) {
        if (name.startsWith("MESS:")) return name.substring(5).replace("VOHM", "V/Ω")
        return name === "P:+9V" ? "V+" : name.substring(2).replace("-", "\u2212")
    }
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
        for (let g = 0; g < groups.length; ++g) {
            const k = groups[g].fields.indexOf(name)
            if (k >= 0)
                return Qt.point(groups[g].fieldsLeft + k * (Theme.deskFieldWidth + Theme.deskFieldGap)
                                + Theme.deskFieldWidth / 2, deskY + Theme.fieldHeight / 2)
        }
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
        if (!showModules) return ""
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
    // Abdeckkarte (ab Volta 2): sobald der Fototransistor da ist (Lichtschranke unterbrechen)
    readonly property bool cardAvailable: catalogEntry("fototransistor").type !== undefined

    // Licht und Waerme gibt es, sobald Fotowiderstand oder Heissleiter verfuegbar oder gesteckt sind
    readonly property bool environmentAvailable: {
        const tray = controller.trayParts
        for (let i = 0; i < tray.length; ++i)
            if (["fotowiderstand", "heissleiter", "feuchtesonde", "fototransistor"].indexOf(tray[i].type) >= 0
                    && !tray[i].locked) return true
        const parts = controller.parts
        for (let k = 0; k < parts.length; ++k)
            if (["fotowiderstand", "heissleiter", "feuchtesonde", "fototransistor"].indexOf(parts[k].type) >= 0) return true
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

    // ------------------------------------------------------------------ Ansicht: Zoom und Verschieben
    // Selbsttaetig (autoView): die ganze Arbeitsflaeche passt ins Fenster (die Breite der Grundplatte waehlt die
    // Automatik 8/10/12). Zwei Finger, Strg + Mausrad, Mausrad, Leertaste + Ziehen und mittlere Maustaste stellen
    // Massstab und Ausschnitt von Hand ein; der Knopf oben rechts in der Arbeitsflaeche kehrt zur ganzen Platte zurueck.
    // Kleine Steckbretter (Stufenanpassung) werden dabei groesser gezeigt: die Felder nutzen die freie Flaeche
    readonly property real fitScale: Math.min(Theme.maxFitZoom, (boardArea.width - 2 * Theme.margin) / canvas.width,
                                              (boardArea.height - 2 * Theme.margin) / canvas.height)
    property bool autoView: true
    property bool spaceHeld: false // Leertaste gedrueckt: Ziehen mit der Maus verschiebt den Ausschnitt
    property real userScale: 1
    property point userPos: Qt.point(Theme.margin, Theme.margin)
    readonly property real viewScale: autoView ? fitScale : Math.max(fitScale, Math.min(Theme.maxZoom, userScale))
    // Ausschnitt begrenzen: passt die Flaeche, bleibt sie oben links; sonst kein leerer Rand ueber Theme.margin
    function clampOffset(offset, scaled, view) {
        if (scaled + 2 * Theme.margin <= view) return Theme.margin
        return Math.max(view - scaled - Theme.margin, Math.min(Theme.margin, offset))
    }
    // Punkt der Arbeitsflaeche unter einem Punkt in boardArea
    function contentAt(p) { return Qt.point((p.x - canvas.x) / canvas.scale, (p.y - canvas.y) / canvas.scale) }
    // Massstab s einstellen, so dass der Flaechenpunkt c unter dem Punkt p (boardArea) bleibt
    function zoomTo(s, c, p) {
        const scale = Math.max(fitScale, Math.min(Theme.maxZoom, s))
        userPos = Qt.point(p.x - c.x * scale, p.y - c.y * scale)
        userScale = scale
        autoView = false
    }
    // Ziehen: Flaeche ab der Startlage um die Verschiebung des Zeigers versetzen
    function panTo(start, translation) {
        userScale = canvas.scale
        userPos = Qt.point(start.x + translation.x, start.y + translation.y)
        autoView = false
    }
    function panBy(dx, dy) {
        userScale = canvas.scale
        userPos = Qt.point(canvas.x + dx, canvas.y + dy)
        autoView = false
    }
    // Mitte eines Pultrahmens (-1: Bauteil ausgeblendet)
    function groupCenter(name) { const g = groupOf(name); return g ? (g.left + g.right) / 2 : -1 }
    property string highlightGroup: "" // Gruppe, deren Felder hervorgehoben sind (Tippen auf den Rahmen)
    function inHighlightedGroup(name) {
        if (meterNames.indexOf(name) >= 0) return highlightGroup === "messgeraet"
        const g = groupOf(highlightGroup)
        return g !== null && g.fields.indexOf(name) >= 0
    }
    // Hintergrund des Steckbretts: jede Reihe eine eigene Farbe, nach rechts eine Abstufung wie ein Verlauf
    // (Reihenbuchstabe steht immer im Feld - Farbe nie allein)
    function boardTint(row, col) { return Theme.fieldTint(row, col, columns) }

    // ------------------------------------------------------------------ Leitungsfuehrung
    // Leitungen laufen moeglichst nicht ueber Bauteile: nach jeder Aenderung am Aufbau oder an der Flaeche sucht
    // der Router (LabController.routeWires) Wege um die Bauteilkoerper herum. {Leitungs-ID: {points, arc}}
    property var wireRoutes: ({})
    function routeWires() {
        const obstacles = []
        for (let i = 0; i < partRepeater.count; ++i) {
            const it = partRepeater.itemAt(i)
            if (it) obstacles.push(it.bodyRect)
        }
        const list = controller.wires
        const ends = list.map(w => ({from: socketPoint(w.from, w.fromSocket), to: socketPoint(w.to, w.toSocket)}))
        const routes = controller.routeWires(Qt.rect(-Theme.margin, -Theme.margin, canvas.width + 2 * Theme.margin,
                                                     canvas.height + 2 * Theme.margin), obstacles, ends)
        const out = {}
        for (let i = 0; i < list.length && i < routes.length; ++i) out[list[i].id] = routes[i]
        wireRoutes = out
    }
    Timer { id: routeTimer; interval: 20; onTriggered: lab.routeWires() }
    Connections {
        target: lab.controller
        function onBoardChanged() { routeTimer.restart() }
        function onDeskChanged() { routeTimer.restart() }
    }
    Connections {
        target: canvas
        function onWidthChanged() { routeTimer.restart() }
        function onHeightChanged() { routeTimer.restart() }
    }
    Component.onCompleted: {
        Texts.band = lab.controller ? lab.controller.band : ""
        routeTimer.restart()
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
    // Drehen (Menue oder Doppeltippen): Platine um 90 Grad, sonst waagerecht <-> senkrecht ab demselben Feld
    function turnPart(p) {
        lab.beginEdit()
        if (p.pins.length === 4) lab.controller.rotatePart(p.id)
        else lab.controller.movePart(p.id, p.start, !p.vertical, p.longSpan)
    }
    property bool menuHintShown: false
    function showMenuHint() {
        if (menuHintShown) return
        menuHintShown = true
        toast.show(qsTr("Doppeltippen dreht ein Bauteil. Langer Druck oder Rechtsklick öffnet das Menü (Farbe, Drehen, Löschen …)."))
    }
    property bool schematicVisible: settings ? settings.schematicVisible : false // Schaltplan parallel zum Aufbau
    property bool recorderVisible: false // Zeitschreiber unter der Arbeitsflaeche
    property var highlightFields: []        // Felder eines im Schaltplan angetippten Knotens
    property string selectedWire: ""
    // Lieblingsfarbe (Stern im Farbmenue): Leitungen ohne Verbindung zu einem Pol. Mit der Farbautomatik zeigt der
    // Farbknopf schon beim Druck auf ein Feld, welche Farbe die Leitung von dort bekommt (Plus rot, Minus schwarz)
    property string newWireColor: settings ? settings.wireColor : "blau"
    property string colorField: ""
    readonly property string nextWireColor: {
        lab.controller.wires; lab.settings && lab.settings.autoWireColor // neu nach Aenderungen (Verbindungen zum Pol)
        return lab.controller.wireColorFor(lab.wireFrom !== "" ? lab.wireFrom : lab.colorField,
                                           lab.wireFrom !== "" ? lab.highlightAnchor : "", lab.newWireColor)
    }
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
    property point trayScenePos // Finger bzw. Maus in Lab-Koordinaten (schwebende Vorschau ueber Fach und Pult)

    Connections {
        target: lab.controller
        function onEditFailed() { toast.show(lab.controller.lastEditError) }
        // Ersatzton offen ansagen (Konzept 5.3)
        function onToneModeChanged() {
            if (lab.controller.toneMode === "ersatz")
                toast.show(qsTr("Diese Schaltung ist zu aufwendig, um sie Schritt für Schritt hörbar zu rechnen. Du hörst einen Ton mit der Frequenz, die die Simulation ergibt."))
        }
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
            onHelpRequested: lab.helpRequested()
            onQuickGuideRequested: lab.quickGuideRequested()
            helpInMenu: true
            onParentAreaRequested: lab.parentAreaRequested()
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
                indicator: DrawnIcon {
                    x: experimentBox.width - width - 12
                    anchors.verticalCenter: parent.verticalCenter
                    kind: "down"
                    size: 16
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
                contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "undo"; color: "white"; size: 22; opacity: parent.parent.enabled ? 1 : 0.35 } }
                background: Rectangle {
                    radius: 6
                    color: parent.down ? "#33FFFFFF" : (parent.hovered && parent.enabled ? "#2A4A6E" : "transparent")
                    border.color: parent.enabled ? "#7F93A8" : "#4A6178"
                }
                width: Theme.control; height: Theme.control
                enabled: lab.controller.canUndo
                onClicked: { lab.beginEdit(); lab.controller.undo() }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Rückgängig: %1").arg(lab.controller.undoText)
                Accessible.name: qsTr("Rückgängig")
            }
            ToolButton {
                objectName: "redoButton"
                contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "redo"; color: "white"; size: 22; opacity: parent.parent.enabled ? 1 : 0.35 } }
                background: Rectangle {
                    radius: 6
                    color: parent.down ? "#33FFFFFF" : (parent.hovered && parent.enabled ? "#2A4A6E" : "transparent")
                    border.color: parent.enabled ? "#7F93A8" : "#4A6178"
                }
                width: Theme.control; height: Theme.control
                enabled: lab.controller.canRedo
                onClicked: { lab.beginEdit(); lab.controller.redo() }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Wiederholen: %1").arg(lab.controller.redoText)
                Accessible.name: qsTr("Wiederholen")
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
                    border.color: "#7F93A8"
                }
            }
            // Spannungsanzeige automatisch: Balken auf allen belegten Feldern der Grundplatte
            ToolButton {
                objectName: "voltageAutoToggle"
                width: Theme.control; height: Theme.control
                checkable: true
                checked: lab.settings ? lab.settings.autoVoltageBars : false
                enabled: lab.settings !== null
                onClicked: {
                    lab.settings.autoVoltageBars = !lab.settings.autoVoltageBars
                    toast.show(lab.settings.autoVoltageBars ? qsTr("Spannungsanzeige auf allen belegten Feldern")
                                                            : qsTr("Spannungsanzeige automatisch aus"))
                }
                ToolTip.visible: hovered
                ToolTip.text: checked ? qsTr("Spannungsanzeige automatisch ausschalten")
                                      : qsTr("Spannungsanzeige auf allen belegten Feldern zeigen")
                Accessible.name: ToolTip.text
                contentItem: Item {
                    VoltageBars { anchors.centerIn: parent; bars: 3 }
                }
                background: Rectangle {
                    radius: 6
                    color: parent.checked ? Theme.accent : (parent.hovered ? "#2A4A6E" : "transparent")
                    border.color: "#7F93A8"
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
                ToolTip.text: !lab.classicMeter ? (lab.recorderVisible ? qsTr("Oszilloskop ausblenden") : qsTr("Oszilloskop zeigen"))
                            : lab.recorderVisible ? qsTr("Zeitschreiber ausblenden") : qsTr("Zeitschreiber zeigen")
                Accessible.name: ToolTip.text
                background: Rectangle {
                    radius: 6
                    color: parent.checked ? Theme.accent : (parent.hovered ? "#2A4A6E" : "transparent")
                    border.color: "#7F93A8"
                }
            }
            // Zeitlupe (ab Volta 2, Konzept 5.3): 1, 1/10, 1/100, 1/1000 der Echtzeit; Ton ist dann stumm. Symbol: Lupe
            // mit Uhrzeigern (gezeichnet); die Stufe steht als Text daneben (Farbe nie allein)
            ToolButton {
                objectName: "slowMotionButton"
                visible: !lab.classicMeter
                readonly property var scales: [1, 0.1, 0.01, 0.001]
                readonly property int level: Math.max(0, scales.indexOf(lab.controller.timeScale))
                width: Math.max(Theme.control, implicitWidth); height: Theme.control
                checkable: true
                checked: level > 0
                onClicked: lab.controller.timeScale = scales[(level + 1) % scales.length]
                leftPadding: 10; rightPadding: 10
                contentItem: Row {
                    spacing: 4
                    DrawnIcon { anchors.verticalCenter: parent.verticalCenter; kind: "slowmo"; color: "white"; size: 24 }
                    Label {
                        visible: parent.parent.level > 0
                        anchors.verticalCenter: parent.verticalCenter
                        text: "1/" + Math.round(1 / parent.parent.scales[parent.parent.level])
                        color: "white"
                        font.pixelSize: 15
                        font.bold: true
                    }
                }
                ToolTip.visible: hovered
                ToolTip.text: level === 0 ? qsTr("Zeitlupe: aus (Echtzeit). Antippen: 1/10")
                              : qsTr("Zeitlupe: 1/%1, Ton stumm. Antippen: langsamer bzw. aus").arg(Math.round(1 / scales[level]))
                Accessible.name: ToolTip.text
                background: Rectangle {
                    radius: 6
                    color: parent.checked ? Theme.accent : (parent.hovered ? "#2A4A6E" : "transparent")
                    border.color: "#7F93A8"
                }
            }
            // Farbe fuer neue Leitungen: ein Knopf mit Auswahl (spart Platz in der Werkzeugleiste). Das »A« oben rechts
            // zeigt die Automatik (Plus rot, Minus schwarz): rot = an, grau = aus. Umschalten: Klick auf das »A« (Maus,
            // Stift) oder langer Druck auf den Knopf (Touch); Antippen oeffnet die Farbauswahl
            ToolButton {
                id: wireColorButton
                objectName: "wireColorButton"
                readonly property bool auto: lab.settings ? lab.settings.autoWireColor : true
                function toggleAuto() {
                    if (!lab.settings) return
                    lab.settings.autoWireColor = !lab.settings.autoWireColor
                    toast.show(lab.settings.autoWireColor ? qsTr("Leitungsfarbe automatisch: Plus rot, Minus schwarz")
                                                          : qsTr("Leitungsfarbe automatisch: aus"))
                }
                width: Theme.control; height: Theme.control
                onClicked: wireColorMenu.open()
                onPressAndHold: toggleAuto()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Lieblingsfarbe: %1").arg(Texts.colorName(lab.newWireColor)) + " – "
                              + (auto ? qsTr("Automatik an (Plus rot, Minus schwarz)") : qsTr("Automatik aus"))
                Accessible.name: ToolTip.text
                Accessible.description: qsTr("Langer Druck schaltet die Automatik um.")
                contentItem: Item {
                    Rectangle {
                        anchors.centerIn: parent
                        width: 22; height: 22; radius: 11
                        color: Theme.wireColor(lab.nextWireColor)
                        border.color: Theme.solderAmber
                        border.width: 3
                    }
                    // Automatik-Zeichen: Buchstabe und Farbe (nie Farbe allein: aus = grau und durchgestrichen)
                    Rectangle {
                        id: autoBadge
                        objectName: "wireAutoBadge"
                        x: parent.width - width + 2
                        y: -2
                        width: 17; height: 17; radius: 8.5
                        color: wireColorButton.auto ? "#D32F2F" : "#7A838C"
                        border.color: "white"
                        border.width: 1
                        Label {
                            anchors.centerIn: parent
                            text: "A"
                            color: "white"
                            font.pixelSize: 11
                            font.bold: true
                            font.strikeout: !wireColorButton.auto
                        }
                        // Maus und Stift: Klick auf das »A« schaltet um (nimmt den Druck, die Farbauswahl bleibt zu)
                        TapHandler {
                            objectName: "wireAutoTap"
                            acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus | PointerDevice.TouchPad
                            gesturePolicy: TapHandler.WithinBounds
                            margin: 4
                            onTapped: wireColorButton.toggleAuto()
                        }
                    }
                }
                background: Rectangle {
                    radius: 6
                    color: parent.hovered ? "#2A4A6E" : "transparent"
                    border.color: "#7F93A8"
                }
                Menu {
                    id: wireColorMenu
                    margins: 8 // immer ganz im Fenster
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
                            // Farbe nie allein: Name daneben; Stern = Lieblingsfarbe (gefuellt) bzw. waehlbar (leer)
                            indicator: null
                            contentItem: Row {
                                spacing: 10
                                DrawnIcon { // gezeichnet: ★/☆ fehlen in den Schriften von Qt auf Android
                                    objectName: "wireFavorite_" + colorItem.modelData
                                    anchors.verticalCenter: parent.verticalCenter
                                    size: 22
                                    kind: colorItem.checked ? "star" : "starOutline"
                                    color: colorItem.checked ? Theme.solderAmber : "#8A96A3"
                                }
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
        partImages: !tray.symbols // Aufbautext zeigt dieselbe Ansicht wie das Bauteilfach
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
        anchors { right: experimentPanel.left; top: toolbar.bottom; bottom: scopePanel.top }
        readonly property bool shown: lab.schematicVisible && lab.controller.schematicAvailable
        visible: shown
        // verkleinert wird auch die Spalte im selben Verhaeltnis schmaler (mehr Platz fuer das Steckbrett);
        // vergroessert bleibt sie so breit und der Plan laesst sich verschieben
        fullWidth: shown ? Math.round((lab.width - tray.width - experimentPanel.width) * 0.5) : 0
        width: shown ? Math.round(Math.max(headerWidth, fullWidth * Math.min(1, zoomPercent / 100))) : 0
        selectedPart: lab.selectedPart
        onPartSelected: (id) => { lab.selectedPart = id; lab.selectedWire = "" }
        onNodeSelected: (anchors) => lab.highlightFields = anchors
        onVisibleChanged: if (!visible) lab.highlightFields = []
    }

    PartTray {
        id: tray
        objectName: "partTray"
        anchors { left: parent.left; top: toolbar.bottom; bottom: parent.bottom }
        parts: lab.controller.trayParts
        // Aufbauhilfe: das Fach zeigt, was der naechste Schritt braucht
        Connections {
            target: lab.controller
            function onGuideChanged() {
                const k = lab.controller.guideStep.kind
                if (k === "modul") tray.showModules = tray.hasModules
                else if (k === "bauteil") { tray.showModules = false; tray.showType(lab.controller.guideStep.type) }
            }
            // neues Experiment bzw. freies Labor: wieder alle Themen
            function onExperimentChanged() {
                tray.filter = ""
                if (lab.controller.experimentScope) lab.recorderVisible = true // Vorlage stellt das Oszilloskop ein
            }
        }
        onLockedTapped: (type, stage) => toast.show(qsTr("%1 wird mit Stufe %2 freigeschaltet.").arg(Texts.partName(type)).arg(stage))
        onDragUpdated: (type, scenePos) => {
            lab.beginEdit()
            lab.trayType = type
            lab.trayPos = lab.toCanvas(scenePos)
            lab.trayScenePos = lab.mapFromItem(null, scenePos.x, scenePos.y)
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
        symbols: lab.settings ? lab.settings.traySymbols : false // Bauteil / Symbol, gemerkt
        onSymbolsChanged: if (lab.settings) lab.settings.traySymbols = symbols
        onModuleLockedTapped: (type, stage) => toast.show(qsTr("%1 wird mit Stufe %2 freigeschaltet.").arg(Texts.moduleName(type)).arg(stage))
        onModuleDragUpdated: (type, scenePos) => {
            lab.beginEdit()
            lab.trayModule = type
            lab.trayPos = lab.toCanvas(scenePos)
            lab.trayScenePos = lab.mapFromItem(null, scenePos.x, scenePos.y)
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

    // Volta 1: Zeitschreiber; ab Volta 2 an derselben Stelle das Oszilloskop (der Zeitschreiber ist dort sein
    // Rollbetrieb). Nur eingeblendet tastet die Simulation fuer das Oszilloskop ab.
    RecorderPanel {
        id: recorderPanel
        objectName: "recorderPanel"
        controller: lab.controller
        readonly property bool shown: lab.recorderVisible && lab.controller.recorderAvailable && lab.classicMeter
        visible: shown
        anchors { left: tray.right; right: experimentPanel.left; bottom: parent.bottom }
        height: shown ? Math.max(170, Math.min(240, lab.height * 0.28)) : 0
        onCloseRequested: lab.recorderVisible = false
    }
    ScopePanel {
        id: scopePanel
        objectName: "scopePanel"
        controller: lab.controller
        readonly property bool shown: lab.recorderVisible && lab.controller.recorderAvailable && !lab.classicMeter
        visible: shown
        onShownChanged: lab.controller.scopeActive = shown
        Component.onCompleted: lab.controller.scopeActive = shown
        anchors { left: tray.right; right: experimentPanel.left; bottom: recorderPanel.top }
        height: shown ? Math.max(190, Math.min(260, lab.height * 0.3)) : 0
        onCloseRequested: lab.recorderVisible = false
    }

    Rectangle {
        id: boardArea
        objectName: "boardArea"
        anchors { left: tray.right; right: schematicPanel.left; top: toolbar.bottom; bottom: scopePanel.top }
        color: Theme.board
        clip: true

        // Tippen ins Leere hebt die Auswahl auf (und gibt der Arbeitsflaeche den Tastaturfokus fuer die Leertaste)
        TapHandler {
            onTapped: { lab.selectedPart = ""; lab.selectedWire = ""; lab.selectedModule = ""; spaceKeys.forceActiveFocus() }
        }

        // Leertaste + Ziehen (Desktop): solange die Leertaste gehalten wird, nimmt eine Ebene ueber der Arbeitsflaeche
        // das Ziehen mit der Maus als Verschieben (Felder und Bauteile bleiben unberuehrt)
        Item {
            id: spaceKeys
            objectName: "spaceKeys"
            Keys.onPressed: (event) => {
                if (event.key === Qt.Key_Space) { if (!event.isAutoRepeat) lab.spaceHeld = true; event.accepted = true }
            }
            Keys.onReleased: (event) => {
                if (event.key === Qt.Key_Space) { if (!event.isAutoRepeat) lab.spaceHeld = false; event.accepted = true }
            }
            onActiveFocusChanged: if (!activeFocus) lab.spaceHeld = false
        }
        Item {
            objectName: "spacePanLayer"
            anchors.fill: parent
            z: 10
            visible: lab.spaceHeld
            HoverHandler { cursorShape: spacePan.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor }
            DragHandler {
                id: spacePan
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything // vor dem Ziehen einer Leitung
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                property point startPos
                onActiveChanged: if (active) startPos = Qt.point(canvas.x, canvas.y)
                onActiveTranslationChanged: if (active) lab.panTo(startPos, activeTranslation)
            }
        }

        // Zoom und Verschieben mit zwei Fingern: der Flaechenpunkt unter dem Mittelpunkt der Finger bleibt darunter
        PinchHandler {
            id: pinch
            objectName: "boardPinch"
            target: null
            minimumPointCount: 2
            maximumPointCount: 2
            // zwei Finger gehoeren dem Zoom, auch wenn einer auf einem Bauteil oder Feld liegt
            grabPermissions: PointerHandler.CanTakeOverFromAnything
            property real startScale: 1
            property point startContent
            onActiveChanged: {
                if (active) {
                    startScale = canvas.scale
                    startContent = lab.contentAt(centroid.position)
                }
            }
            onActiveScaleChanged: if (active) lab.zoomTo(startScale * activeScale, startContent, centroid.position)
            onCentroidChanged: if (active) lab.zoomTo(startScale * activeScale, startContent, centroid.position)
        }
        // Strg + Mausrad: Zoom am Mauszeiger; Mausrad bzw. Umschalt + Mausrad: senkrecht bzw. waagrecht verschieben
        WheelHandler {
            acceptedModifiers: Qt.ControlModifier
            target: null
            onWheel: (event) => lab.zoomTo(canvas.scale * Math.pow(1.1, event.angleDelta.y / 120),
                                          lab.contentAt(point.position), point.position)
        }
        WheelHandler {
            acceptedModifiers: Qt.NoModifier
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            target: null
            enabled: canvas.width * canvas.scale + 2 * Theme.margin > boardArea.width
                     || canvas.height * canvas.scale + 2 * Theme.margin > boardArea.height
            onWheel: (event) => lab.panBy(event.angleDelta.x / 2, event.angleDelta.y / 2)
        }
        WheelHandler {
            acceptedModifiers: Qt.ShiftModifier
            target: null
            onWheel: (event) => lab.panBy(event.angleDelta.y / 2, 0)
        }
        // Mittlere Maustaste: Ausschnitt ziehen
        DragHandler {
            target: null
            acceptedButtons: Qt.MiddleButton
            acceptedDevices: PointerDevice.Mouse
            property point startPos
            onActiveChanged: if (active) startPos = Qt.point(canvas.x, canvas.y)
            onActiveTranslationChanged: if (active) lab.panTo(startPos, activeTranslation)
        }

        // Ganze Platte zeigen: nur sichtbar, wenn von Hand gezoomt oder verschoben wurde
        ToolButton {
            objectName: "viewReset"
            anchors { right: parent.right; top: parent.top; margins: 8 }
            z: 11
            width: Theme.control; height: Theme.control
            visible: !lab.autoView
            onClicked: lab.autoView = true
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Ganze Platte zeigen")
            Accessible.name: ToolTip.text
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "fit"; size: 24; color: Theme.navy } }
            background: Rectangle { radius: 8; color: parent.down ? "#D7DEE6" : "#EEF2F6"; border.color: "#7F93A8"; border.width: 1.5 }
        }

        Item {
            id: canvas
            objectName: "canvas"
            width: Math.max(lab.trashX + Theme.control, lab.meterX, !lab.controller.meterVisible ? lab.boardWidth
                            : lab.meterDocked ? Math.max(lab.boardWidth, lab.meterDockX + lab.meterDockWidthNow + 12) // mit Rahmen
                            : lab.meterX + Theme.meterAreaWidth,
                            lab.deskStripWidth)
            height: lab.showModules ? lab.moduleY + Theme.fieldHeight + Theme.control + 8 // Modulleiste mit Namensschildern
                                    : lab.trashY + Theme.control + 8
            // Massstab und Ausschnitt aus der Ansicht (oben); selbsttaetig passt die ganze Flaeche ins Fenster
            transformOrigin: Item.TopLeft
            scale: lab.viewScale
            x: lab.autoView ? Theme.margin : lab.clampOffset(lab.userPos.x, width * scale, boardArea.width)
            y: lab.autoView ? Theme.margin : lab.clampOffset(lab.userPos.y, height * scale, boardArea.height)

            // Pult: blaue Flaeche ueber Bedienelemente und Pultfelder, je Bauteil ein Rahmen um Bedienelement und Felder
            ControlDesk {
                controller: lab.controller
                x: -8
                y: 0
                width: canvas.width + 16
                height: lab.deskBottom
                groups: lab.groups
                offsetX: 8
                fieldsBottom: lab.deskY + Theme.fieldHeight + 6
                fieldsTop: lab.deskY
                meterFrame: lab.controller.meterVisible && lab.meterDocked
                meterLeft: lab.meterDockX
                meterWidth: lab.meterDockWidthNow
                highlightGroup: lab.highlightGroup
                onGroupTapped: (name) => lab.highlightGroup = lab.highlightGroup === name ? "" : name
                onInfoRequested: (name) => deskPartInfo.show(name)
                trayX: lab.deskTrayX
                onTrayRequested: deskTray.openFor(false)
                onTrayRemoveRequested: deskTray.openFor(true)
                onGeneratorRequested: generatorPanel.open()
                onTracerRequested: tracerPanel.open()
            }

            // Farbiger Hintergrund des Steckbretts: eine Kachel je Feld (Reihenfarbe, nach rechts dunkler)
            Repeater {
                model: lab.rows.length * lab.columns
                Rectangle {
                    required property int index
                    readonly property int row: Math.floor(index / lab.columns)
                    readonly property int col: index % lab.columns + 1
                    readonly property point at: lab.anchorPoint(lab.rows[row] + col)
                    width: Theme.fieldWidth + Theme.fieldGap - 6
                    height: Theme.fieldHeight + Theme.fieldGap - 6
                    x: at.x - width / 2
                    y: at.y - height / 2
                    radius: 8
                    color: lab.boardTint(row, col)
                }
            }

            // Dreifach-Messgeraet (eingeblendet ueber die Werkzeugleiste); die Buchsen liegen unten darin
            MeterPanel {
                objectName: "meterPanel"
                visible: lab.controller.meterVisible && lab.classicMeter
                controller: lab.controller
                docked: lab.meterDocked
                onDockToggled: if (lab.settings) lab.settings.meterDocked = !lab.settings.meterDocked
                x: docked ? lab.meterDockX : lab.meterX
                y: docked ? 14 : lab.boardY - 6
                width: docked ? Theme.meterDockWidth : Theme.meterAreaWidth
                height: docked ? lab.deskPanelHeight - 22 : 5 * Theme.fieldHeight + 4 * Theme.fieldGap + 12
            }
            DisplayMeter {
                objectName: "displayMeter"
                visible: lab.controller.meterVisible && !lab.classicMeter
                controller: lab.controller
                docked: lab.meterDocked
                onDockToggled: if (lab.settings) lab.settings.meterDocked = !lab.settings.meterDocked
                x: docked ? lab.meterDockX : lab.meterX
                y: docked ? 14 : lab.boardY - 6
                width: docked ? lab.meterDockWidthNow : Theme.meterAreaWidth
                // im Pult bis in die Feldreihe (ohne Buchsen frei): Platz fuer die Leistung unter den Anzeigen
                height: docked ? lab.deskBottom - 34
                               : lab.meterRows * Theme.fieldHeight + (lab.meterRows - 1) * Theme.fieldGap + 12
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
                    dragScale: canvas.scale
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
                    onDragMoved: (sceneDelta) => {
                        const delta = lab.toCanvasDelta(sceneDelta)
                        lab.trashHot = lab.overTrash(Qt.point(moduleItem.x + moduleItem.width / 2 + delta.x, lab.trashY + 10))
                    }
                    onDragFinished: (sceneDelta) => {
                        const delta = lab.toCanvasDelta(sceneDelta)
                        lab.trashHot = false
                        if (lab.overTrash(Qt.point(moduleItem.x + moduleItem.width / 2 + delta.x, lab.trashY + 10))) {
                            trashDialog.ask("module", moduleItem.modelData.id, moduleItem.modelData.type)
                            return
                        }
                        const p = lab.anchorPoint("M:" + moduleItem.modelData.slot)
                        const target = lab.slotAt(Qt.point(p.x + delta.x, p.y))
                        if (target !== "" && target !== "M:" + moduleItem.modelData.slot)
                            lab.controller.moveModule(moduleItem.modelData.id, target)
                    }
                }
            }

            // Fehlersuche: nummerierte Marke je Befund an seiner ersten Stelle (gleiche Nummer wie in der Liste)
            Repeater {
                model: lab.checkMarks
                Rectangle {
                    required property int index
                    required property var modelData
                    objectName: "checkMark_" + index
                    readonly property point at: lab.anchorPoint(modelData.anchors[0])
                    readonly property size area: lab.anchorSize(modelData.anchors[0])
                    // mehrere Marken am selben Feld nebeneinander
                    readonly property int same: {
                        let n = 0
                        for (let i = 0; i < index; ++i) if (lab.checkMarks[i].anchors[0] === modelData.anchors[0]) ++n
                        return n
                    }
                    x: at.x + area.width / 2 - width / 2 - same * (width + 2)
                    y: at.y - area.height / 2 - height / 2
                    z: 50
                    width: 26; height: 26; radius: 13
                    color: Theme.signalOrange
                    border.color: "white"
                    border.width: 2
                    Label { anchors.centerIn: parent; text: parent.index + 1; color: "white"; font.bold: true; font.pixelSize: 14 }
                }
            }

            // Breite der Grundplatte: Felder je Reihe 8 / 10 / 12 (Befehl, rueckgaengig machbar). Runde Knoepfe, gelb mit
            // schwarzem Ring; was nicht geht (- bei 8, + bei 12), ist grau. Dazwischen die aktuelle Breite.
            component SizeButton: ToolButton {
                id: sizeButton
                property bool plus: true
                // Tippflaeche 48 dp, sichtbarer Kreis kleiner; Zeichen als Balken gezeichnet (exakt mittig)
                width: Theme.control; height: Theme.control
                readonly property color ink: enabled ? "#111111" : "#7A838C"
                contentItem: Item {
                    Rectangle {
                        anchors.centerIn: parent
                        width: 14; height: 3; radius: 1.5
                        color: sizeButton.ink
                    }
                    Rectangle {
                        visible: sizeButton.plus
                        anchors.centerIn: parent
                        width: 3; height: 14; radius: 1.5
                        color: sizeButton.ink
                    }
                }
                background: Item {
                    Rectangle {
                        anchors.centerIn: parent
                        width: 34; height: 34; radius: 17
                        color: !sizeButton.enabled ? "#C9CED4" : (sizeButton.down ? "#F2B705" : "#FFD23F")
                        border.color: sizeButton.enabled ? "#111111" : "#9AA3AD"
                        border.width: 2.5
                    }
                }
                ToolTip.visible: hovered
                Accessible.name: ToolTip.text
            }
            Column {
                visible: lab.boardResizable
                x: lab.sizerX
                y: lab.sizerY
                spacing: 0
                SizeButton {
                    objectName: "columnsPlus"
                    enabled: lab.columns < 12
                    onClicked: { lab.beginEdit(); lab.controller.setBoardColumns(lab.columns + 2) }
                    ToolTip.text: qsTr("Mehr Felder je Reihe")
                }
                Label {
                    width: Theme.control
                    height: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("1–%1").arg(lab.columns)
                    color: Theme.navy
                    font.pixelSize: 12
                    font.bold: true
                    Accessible.name: qsTr("Felder je Reihe: %1").arg(lab.columns)
                }
                SizeButton {
                    objectName: "columnsMinus"
                    plus: false
                    enabled: lab.columns > (lab.controller.startColumns > 0 ? lab.controller.minBoardColumns : 8)
                    onClicked: { lab.beginEdit(); lab.controller.setBoardColumns(lab.columns - 2) }
                    ToolTip.text: qsTr("Weniger Felder je Reihe")
                }
            }

            // Muelleimer: Antippen loescht das Ausgewaehlte (ohne Auswahl: alles abbauen), Bauteile und Module lassen
            // sich darauf ziehen; immer mit Rueckfrage, rueckgaengig machbar
            ToolButton {
                id: trashButton
                objectName: "trashButton"
                x: lab.trashX
                y: lab.trashY
                width: Theme.control; height: Theme.control
                // Auswahl schon beim Druecken merken: das Tippen hebt sonst ueber den Hintergrund die Auswahl auf
                property var picked: ({ part: "", wire: "", module: "" })
                onPressed: picked = { part: lab.selectedPart, wire: lab.selectedWire, module: lab.selectedModule }
                onClicked: {
                    if (picked.part !== "") {
                        const list = lab.controller.parts
                        for (let i = 0; i < list.length; ++i)
                            if (list[i].id === picked.part) trashDialog.ask("part", list[i].id, list[i].type)
                    } else if (picked.wire !== "") trashDialog.ask("wire", picked.wire, "")
                    else if (picked.module !== "") {
                        const mods = lab.controller.modules
                        for (let m = 0; m < mods.length; ++m)
                            if (mods[m].id === picked.module) trashDialog.ask("module", mods[m].id, mods[m].type)
                    } else trashDialog.ask("all", "", "")
                }
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Löschen: Ausgewähltes antippen oder Bauteil hierher ziehen")
                Accessible.name: qsTr("Löschen")
                background: Rectangle {
                    radius: 10
                    color: lab.trashHot ? "#C62828" : (trashButton.down ? "#D7DEE6" : "#EEF2F6")
                    border.color: lab.trashHot ? "#8E1B1B" : "#7F93A8"
                    border.width: lab.trashHot ? 3 : 1.5
                    scale: lab.trashHot ? 1.15 : 1
                    Behavior on scale { enabled: !Theme.reduceMotion; NumberAnimation { duration: 100 } }
                }
                contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "trash"; size: 26; color: lab.trashHot ? "white" : Theme.navy } }
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
                    // Batteriepole: V+ rot, 0V/GND schwarz (mit Schrift - Farbe nie allein)
                    // Batterie 2: +B2 rot, -B2 blau (negative Versorgung, Farbkonzept 9.3)
                    fillColor: modelData === "P:+9V" || modelData === "P:+B2" ? "#C62828"
                             : modelData === "P:0V" || modelData === "P:G0" ? "#17191C" // G0 ist 0V
                             : modelData === "P:-B2" ? "#2C62C9" : "transparent"
                    subLabel: modelData === "P:0V" ? "GND" : ""
                    pitch: lab.socketPitch(modelData)
                    width: lab.anchorSize(modelData).width
                    height: lab.anchorSize(modelData).height
                    x: lab.anchorPoint(modelData).x - width / 2
                    y: lab.anchorPoint(modelData).y - height / 2
                    highlighted: lab.highlightAnchor === modelData || lab.inHighlightedGroup(modelData)
                                 || lab.highlightFields.indexOf(modelData) >= 0
                                 || (lab.trayPreview.fields || []).indexOf(modelData) >= 0
                    // einzeln eingeschaltet (Feldmenue) oder automatisch auf allen belegten Feldern (Werkzeugleiste)
                    showVoltage: lab.controller.voltageFields.indexOf(modelData) >= 0
                                 || (lab.settings !== null && lab.settings.autoVoltageBars
                                     && lab.controller.occupiedFields.indexOf(modelData) >= 0)
                    onShowVoltageChanged: if (showVoltage) voltage = lab.controller.voltageDisplay(anchorName)
                    recorderChannel: lab.controller.recorderChannels.indexOf(modelData.startsWith("MESS:") ? "MESS" : modelData)
                    target: lab.guideVisible && (lab.controller.guideStep.targets || []).indexOf(modelData) >= 0

                    onPressedChanged: if (pressed) lab.colorField = field.modelData
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
                            if (field.showVoltage) field.voltage = lab.controller.voltageDisplay(field.anchorName)
                        }
                    }
                }
            }

            // Bauteile
            Repeater {
                id: partRepeater
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
                    dragScale: canvas.scale
                    points: modelData.anchors.map((a, i) => lab.socketPoint(a, modelData.sockets[i]))
                    onTapped: {
                        lab.selectedPart = part.modelData.id
                        lab.selectedWire = ""
                        lab.showMenuHint()
                        // Reedkontakt und Wechselschalter: Tippen schaltet um (Bedienhandlung)
                        if (part.modelData.type === "reedkontakt" || part.modelData.type === "wechselschalter")
                            lab.controller.setPartClosed(part.modelData.id, part.partState.closed !== true)
                        // Trimmer: Tippen oeffnet das Stellfenster (Bedienhandlung)
                        if (part.modelData.type.startsWith("trimmer"))
                            trimmerPanel.openFor(part.modelData.id, part.modelData.type,
                                                 part.partState.position !== undefined ? part.partState.position : 0.5)
                    }
                    onPushed: (down) => {
                        if (part.modelData.type === "taster") lab.controller.setPartClosed(part.modelData.id, down)
                    }
                    onDoubleTapped: { lab.selectedPart = part.modelData.id; lab.turnPart(part.modelData) }
                    onDragMoved: (sceneDelta) => {
                        const delta = lab.toCanvasDelta(sceneDelta)
                        const c = lab.partCenter(part.modelData)
                        lab.trashHot = lab.overTrash(Qt.point(c.x + delta.x, c.y + delta.y))
                    }
                    onDragFinished: (sceneDelta) => {
                        const delta = lab.toCanvasDelta(sceneDelta)
                        lab.trashHot = false
                        const c = lab.partCenter(part.modelData)
                        if (lab.overTrash(Qt.point(c.x + delta.x, c.y + delta.y))) {
                            trashDialog.ask("part", part.modelData.id, part.modelData.type)
                            return
                        }
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
                        // um die Bauteile herum gefuehrt; beim Umstecken eines Endes wieder der einfache Bogen
                        waypoints: {
                            const r = lab.wireRoutes[wire.modelData.id]
                            if (!r || r.arc || wire.movingFrom || wire.movingTo) return []
                            const p = r.points
                            if (Math.hypot(p[0].x - wire.fromPoint.x, p[0].y - wire.fromPoint.y) > 1
                                    || Math.hypot(p[p.length - 1].x - wire.toPoint.x, p[p.length - 1].y - wire.toPoint.y) > 1) return []
                            return p.slice(1, p.length - 1)
                        }
                        wireColor: Theme.wireColor(wire.modelData.color)
                        selected: lab.selectedWire === wire.modelData.id
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Leitung %1 von %2 nach %3").arg(Texts.colorName(wire.modelData.color))
                                         .arg(String(wire.modelData.from).replace(/^[A-Z]+:/, ""))
                                         .arg(String(wire.modelData.to).replace(/^[A-Z]+:/, ""))
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
                wireColor: Theme.wireColor(lab.nextWireColor)
                opacity: 0.7
            }

            // Vorschau: Modul aus dem Modulfach an seinem Platz in der Modulleiste (sonst schwebt es am Finger)
            Rectangle {
                visible: lab.trayModule !== "" && lab.highlightAnchor !== ""
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
                    reach: (modelData.type === "taschenlampe" ? 1.2 : modelData.type === "wasser" ? 0.8
                            : modelData.type === "karte" ? 0.6 : 1.5) * (Theme.fieldWidth + Theme.fieldGap)
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
            count: lab.cardAvailable ? 5 : 4
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
                    model: lab.cardAvailable ? ["taschenlampe", "foehn", "wasser", "karte"] : ["taschenlampe", "foehn", "wasser"]
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
                            Text { anchors.verticalCenter: parent.verticalCenter; text: envTile.modelData === "taschenlampe" ? "🔦" : envTile.modelData === "wasser" ? "💧" : envTile.modelData === "karte" ? "▮" : "♨"; font.pixelSize: 20; color: envTile.modelData === "karte" ? "#4A5560" : "#B23A10" }
                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                text: Texts.toolName(envTile.modelData)
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
                        Accessible.name: Texts.toolName(modelData)
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
        margins: 8 // immer ganz im Fenster
        property string anchorName
        MenuItem {
            height: Theme.control
            text: qsTr("Spannungsanzeige ein/aus")
            enabled: !fieldMenu.anchorName.startsWith("MESS:")
            onTriggered: lab.controller.toggleVoltageDisplay(fieldMenu.anchorName)
        }
        // Zeitschreiber: Feld (Spannung gegen 0 V) bzw. Messgeraet (seine Anzeige) auf Kanal 1 oder 2
        Repeater {
            model: lab.controller.recorderAvailable && lab.classicMeter ? 2 : 0 // ab Volta 2: Tastkoepfe
            MenuItem {
                height: Theme.control
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
        margins: 8 // immer ganz im Fenster
        objectName: "partMenu"
        property var part: ({})
        MenuItem {
            height: Theme.control
            text: qsTr("Drehen")
            onTriggered: lab.turnPart(partMenu.part)
        }
        MenuItem {
            height: Theme.control
            text: qsTr("Umdrehen (Polung)")
            onTriggered: { lab.beginEdit(); lab.controller.flipPart(partMenu.part.id) }
        }
        MenuItem {
            height: Theme.control
            text: partMenu.part.longSpan ? qsTr("Kurz stecken") : qsTr("Lang stecken")
            enabled: partMenu.part.anchors !== undefined && partMenu.part.anchors.length === 2
            onTriggered: {
                lab.beginEdit()
                lab.controller.movePart(partMenu.part.id, partMenu.part.start, partMenu.part.vertical, !partMenu.part.longSpan)
            }
        }
        Menu {
            id: valueMenu
            margins: 8 // immer ganz im Fenster
            title: qsTr("Wert")
            // Kondensator: Werte ab Volta 2 (Folienkondensatoren)
            readonly property bool cap: partMenu.part.type === "kondensator" && Texts.band !== "" && Texts.band !== "volta1"
            enabled: partMenu.part.type === "widerstand" || cap
            Instantiator {
                model: valueMenu.cap ? Texts.capacitorValues : Texts.resistorValues
                delegate: MenuItem {
                    height: Theme.control
                    required property var modelData
                    text: valueMenu.cap ? Texts.farad(modelData) + " (" + Texts.capacitorCode(modelData) + ")" : Texts.ohm(modelData)
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
            margins: 8 // immer ganz im Fenster
            title: qsTr("Nennspannung")
            enabled: partMenu.part.type === "gluehlampe"
            Instantiator {
                model: lab.catalogEntry("gluehlampe").voltages || []
                delegate: MenuItem {
                    height: Theme.control
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
            height: Theme.control
            text: qsTr("Reparieren")
            onTriggered: { lab.beginEdit(); lab.controller.repairPart(partMenu.part.id) }
        }
        MenuSeparator {}
        MenuItem {
            height: Theme.control
            objectName: "partMenuRemove"
            text: "🗑 " + qsTr("Entfernen")
            onTriggered: { lab.beginEdit(); lab.controller.removePart(partMenu.part.id); lab.selectedPart = "" }
        }
    }

    Menu {
        id: moduleMenu
        margins: 8 // immer ganz im Fenster
        objectName: "moduleMenu"
        property string moduleId
        property string moduleType
        MenuItem {
            height: Theme.control
            text: qsTr("Blick hinein")
            onTriggered: moduleInside.show(moduleMenu.moduleId, moduleMenu.moduleType)
        }
        MenuItem {
            height: Theme.control
            text: qsTr("Herausnehmen")
            onTriggered: { lab.beginEdit(); lab.controller.removeModule(moduleMenu.moduleId); lab.selectedModule = "" }
        }
    }

    DeskPartInfo {
        id: deskPartInfo
        objectName: "deskPartInfo"
        controller: lab.controller
    }

    TrimmerPanel {
        id: trimmerPanel
        objectName: "trimmerPanel"
        controller: lab.controller
    }

    GeneratorPanel {
        id: generatorPanel
        objectName: "generatorPanel"
        controller: lab.controller
    }

    TracerPanel {
        id: tracerPanel
        objectName: "tracerPanel"
        controller: lab.controller
    }

    DeskTray {
        id: deskTray
        objectName: "deskTray"
        controller: lab.controller
    }

    ModuleInside {
        id: moduleInside
        objectName: "moduleInside"
        controller: lab.controller
    }

    Menu {
        id: toolMenu
        margins: 8 // immer ganz im Fenster
        objectName: "toolMenu"
        property string toolType
        property bool toolOn
        MenuItem {
            height: Theme.control
            text: toolMenu.toolOn ? qsTr("Ausschalten") : qsTr("Einschalten")
            onTriggered: lab.controller.setToolOn(toolMenu.toolType, !toolMenu.toolOn)
        }
        MenuItem {
            height: Theme.control
            text: qsTr("Wegräumen")
            onTriggered: lab.controller.removeTool(toolMenu.toolType)
        }
    }

    Menu {
        id: wireMenu
        margins: 8 // immer ganz im Fenster
        objectName: "wireMenu"
        property string wireId
        property string wireColor
        Instantiator {
            model: Texts.wireColors
            delegate: MenuItem {
                height: Theme.control
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
            height: Theme.control
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


    // Etwas ist kaputt gegangen oder tut sich nicht: Tick bietet die Fehlersuche an (antippen daneben schliesst).
    // »Tut sich nichts«: im Experiment, alles gesteckt, im Betrieb, aber nach 12 s noch kein Erfolg - je Aufbaustand
    // hoechstens einmal. Bei »Nur in der App!« ist das Kaputtgehen gewollt: dann kein Hinweis.
    property bool troubleShown: false
    TroublePopup {
        id: trouble
        parent: boardArea
        // mittig im sichtbaren Teil der Grundplatte (ein offener Werkzeugkasten deckt rechts einen Teil ab)
        readonly property real freeWidth: boardArea.width - (toolbox.opened ? toolbox.width : 0)
        width: Math.min(560, freeWidth - 24)
        x: Math.max(8, (freeWidth - width) / 2)
        y: Math.max(8, boardArea.height - height - 72)
        finderUnlocked: lab.controller.unlockedStage >= 2
        onFaultFinderRequested: toolbox.showFaultFinder()
        onHelpRequested: lab.helpTopicRequested("fehler")
    }
    Connections {
        target: lab.controller
        function onPartDestroyed(id, type) {
            if (lab.controller.destructive && !lab.controller.freeLab) return
            trouble.showDestroyed(Texts.partName(type) + " (" + id + ")")
        }
        function onBoardChanged() { lab.troubleShown = false }
    }

    // Glueckwunsch nach »Fertig«: mittig ueber der Grundplatte
    SuccessPopup {
        id: success
        parent: boardArea
        width: Math.min(520, boardArea.width - 24)
        x: Math.max(8, (boardArea.width - width) / 2)
        y: Math.max(8, (boardArea.height - height) / 2)
    }
    Connections {
        target: lab.controller
        function onExperimentSolved() {
            trouble.close()
            success.observe = lab.controller.phase === 3
            success.open()
        }
    }
    Timer {
        id: idleTrouble
        interval: 12000
        running: lab.operating && !lab.controller.freeLab && lab.controller.phase === 2 && lab.controller.buildComplete
                 && !lab.controller.canFinish && !lab.troubleShown && !toolbox.opened && !trouble.opened
        onTriggered: { lab.troubleShown = true; trouble.showNotWorking() }
    }

    ToolboxPanel {
        id: toolbox
        objectName: "toolbox"
        controller: lab.controller
        y: toolbar.height
        height: lab.height - toolbar.height
        selectedPartId: lab.selectedPart
        onCheckResults: (list) => { lab.checkMarks = list; if (list.length === 0) lab.highlightFields = [] }
        onFindingSelected: (anchors, partId) => {
            lab.highlightFields = anchors
            if (partId !== "") { lab.selectedPart = partId; lab.selectedWire = ""; lab.selectedModule = "" }
        }
        selectedPartType: {
            const list = lab.controller.parts
            for (let i = 0; i < list.length; ++i) if (list[i].id === lab.selectedPart) return list[i].type
            return ""
        }
    }

    // Schwebende Vorschau beim Ziehen aus dem Fach: das Bauteil (mit Draehten) bzw. Modul folgt dem Finger ueber
    // Fach, Pult und Grundplatte; passt es auf die Grundplatte, zeigt dort die Steckvorschau den genauen Platz.
    Item {
        id: dragFloat
        objectName: "trayDragFloat"
        readonly property bool partShown: lab.trayType !== "" && lab.trayPreview.ok !== true
        readonly property bool moduleShown: lab.trayModule !== "" && lab.highlightAnchor === ""
        visible: partShown || moduleShown
        z: 50
        readonly property int pins: {
            const list = lab.controller.trayParts
            for (let i = 0; i < list.length; ++i) if (list[i].type === lab.trayType) return list[i].pins
            return 2
        }
        width: (pins === 4 ? 130 : pins === 3 ? 200 : 124) * canvas.scale
        height: (pins === 4 ? 110 : 40) * canvas.scale
        x: lab.trayScenePos.x - width / 2
        y: lab.trayScenePos.y - height / 2
        PartPreview {
            visible: dragFloat.partShown
            anchors.fill: parent
            type: lab.trayType
            pins: dragFloat.pins
            opacity: 0.85
        }
        Rectangle {
            visible: dragFloat.moduleShown
            anchors.centerIn: parent
            width: (4 * (Theme.slotWidth + Theme.slotGap) - Theme.slotGap) * canvas.scale
            height: 32 * canvas.scale
            radius: 8
            color: Qt.tint(Theme.modulePlate(lab.trayModule), "#B8FFFFFF")
            border.color: Theme.modulePlate(lab.trayModule)
            border.width: 2
            opacity: 0.9
            Row {
                anchors.centerIn: parent
                spacing: 6
                ModuleIcon { anchors.verticalCenter: parent.verticalCenter; height: 22; type: lab.trayModule; color: Theme.modulePlate(lab.trayModule) }
                Label { anchors.verticalCenter: parent.verticalCenter; text: Texts.moduleName(lab.trayModule); color: Theme.navy; font.bold: true; font.pixelSize: Math.round(12 * Theme.fontScale) }
            }
        }
    }

    Toast {
        id: toast
        objectName: "toast"
        anchors.horizontalCenter: boardArea.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.margin
    }

    // Rueckfrage vor dem Loeschen (Muelleimer); Loeschen ist ein Befehl und laesst sich rueckgaengig machen
    Dialog {
        id: trashDialog
        objectName: "trashDialog"
        property string kind: ""   // part, wire, module, all
        property string itemId: ""
        property string itemType: ""
        function ask(kind, id, type) {
            trashDialog.kind = kind
            trashDialog.itemId = id
            trashDialog.itemType = type
            trashDialog.open()
        }
        anchors.centerIn: parent
        modal: true
        width: Math.min(460, lab.width - 32)
        title: kind === "all" ? qsTr("Alles abbauen?") : qsTr("Löschen?")
        contentItem: Column {
            spacing: 16
            Label {
                width: trashDialog.width - trashDialog.leftPadding - trashDialog.rightPadding
                wrapMode: Text.Wrap
                font.pixelSize: Math.round(16 * Theme.fontScale)
                text: (trashDialog.kind === "part" ? qsTr("%1 %2 von der Grundplatte nehmen?").arg(Texts.partName(trashDialog.itemType)).arg(trashDialog.itemId)
                     : trashDialog.kind === "wire" ? qsTr("Diese Leitung entfernen?")
                     : trashDialog.kind === "module" ? qsTr("%1 %2 aus der Modulleiste nehmen?").arg(Texts.moduleName(trashDialog.itemType)).arg(trashDialog.itemId)
                     : qsTr("Alle Bauteile, Leitungen und Module von der Grundplatte nehmen?"))
                     + "\n\n" + qsTr("Mit »Rückgängig« holst du es wieder zurück.")
            }
            Row {
                anchors.right: parent.right
                spacing: 12
                Button {
                    objectName: "trashCancel"
                    text: qsTr("Abbrechen")
                    height: Theme.control
                    onClicked: trashDialog.reject()
                }
                Button {
                    objectName: "trashConfirm"
                    text: trashDialog.kind === "all" ? qsTr("Alles abbauen") : qsTr("Löschen")
                    height: Theme.control
                    onClicked: trashDialog.accept()
                }
            }
        }
        onAccepted: {
            lab.beginEdit()
            if (kind === "part") { lab.controller.removePart(itemId); lab.selectedPart = "" }
            else if (kind === "wire") { lab.controller.removeWire(itemId); lab.selectedWire = "" }
            else if (kind === "module") { lab.controller.removeModule(itemId); lab.selectedModule = "" }
            else lab.controller.clearBoard()
        }
    }
}
