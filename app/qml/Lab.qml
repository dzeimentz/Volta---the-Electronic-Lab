// Experimentansicht (docs/ui-spec.md, Lab): Werkzeugleiste, Bauteilfach, Pultleiste, Grundplatte, Modulleiste.
// Bedienung: Bauteile aus dem Fach ziehen, Bauteile verschieben, Leitungen von Feld zu Feld ziehen, Leitungsenden
// umsetzen; langer Druck oder Rechtsklick oeffnet Menues; Rueckgaengig/Wiederholen auch per Tastenkuerzel.
// Alle Regeln (Spannweite, volle Felder ...) prueft das Modell; die Oberflaeche zeigt nur den Hinweis an.
import QtQuick
import QtQuick.Controls

Item {
    id: lab
    required property var controller

    // ------------------------------------------------------------------ Geometrie
    readonly property var deskNames: ["P:+9V", "P:0V", "P:P1a", "P:P1s", "P:P1b", "P:P2a", "P:P2s", "P:P2b",
                                      "P:S1", "P:S2", "P:LS1", "P:LS2", "MESS:COM", "MESS:VOHM", "MESS:A"]
    readonly property var rows: ["A", "B", "C", "D", "E"]
    readonly property int columns: 8
    readonly property int slots: 16
    readonly property int deskY: 0
    readonly property int boardY: Theme.fieldHeight + 2 * Theme.fieldGap
    readonly property int boardWidth: columns * Theme.fieldWidth + (columns - 1) * Theme.fieldGap
    readonly property int moduleY: boardY + rows.length * (Theme.fieldHeight + Theme.fieldGap) + Theme.fieldGap
    readonly property var allAnchors: {
        let a = deskNames.slice()
        for (let r = 0; r < rows.length; ++r)
            for (let c = 1; c <= columns; ++c) a.push(rows[r] + c)
        for (let m = 1; m <= slots; ++m) a.push("M:" + m)
        return a
    }

    function deskLabel(name) { return name.startsWith("MESS:") ? name.substring(5).replace("VOHM", "V/Ω") : name.substring(2) }
    function isBoardField(name) { return /^[A-E][1-8]$/.test(name) }

    function anchorSize(name) {
        if (deskNames.indexOf(name) >= 0) return Qt.size(Theme.deskFieldWidth, Theme.fieldHeight)
        if (name.startsWith("M:")) return Qt.size(Theme.slotWidth, Theme.fieldHeight)
        return Qt.size(Theme.fieldWidth, Theme.fieldHeight)
    }

    function anchorPoint(name) {
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

    function toCanvas(scenePos) { return canvas.mapFromItem(null, scenePos.x, scenePos.y) }

    // ------------------------------------------------------------------ Bedienzustand
    property string selectedPart: ""
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

        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: Theme.margin
            spacing: 12

            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: lab.controller.title
                color: "white"
                font.pixelSize: 18
                font.bold: true
                width: 340
                elide: Text.ElideRight
            }
            ToolButton {
                objectName: "undoButton"
                text: "↶"
                font.pixelSize: 24
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
                font.pixelSize: 24
                width: Theme.control; height: Theme.control
                enabled: lab.controller.canRedo
                onClicked: lab.controller.redo()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Wiederholen: %1").arg(lab.controller.redoText)
                Accessible.name: qsTr("Wiederholen")
            }
            // Farbe fuer neue Leitungen
            Row {
                anchors.verticalCenter: parent.verticalCenter
                Repeater {
                    model: Texts.wireColors
                    Item {
                        id: chip
                        required property string modelData
                        width: Theme.control; height: Theme.control
                        Rectangle {
                            anchors.centerIn: parent
                            width: 26; height: 26; radius: 13
                            color: Theme.wireColor(chip.modelData)
                            border.color: lab.newWireColor === chip.modelData ? Theme.solderAmber : "#5A6F85"
                            border.width: lab.newWireColor === chip.modelData ? 4 : 1
                        }
                        TapHandler { onTapped: lab.newWireColor = chip.modelData }
                        Accessible.role: Accessible.RadioButton
                        Accessible.name: qsTr("Leitungsfarbe %1").arg(Texts.colorName(chip.modelData))
                    }
                }
            }
            Switch {
                height: Theme.control
                text: qsTr("Hauptschalter")
                checked: lab.controller.mainSwitch
                onToggled: lab.controller.mainSwitch = checked
                palette.windowText: "white"
            }
            ComboBox {
                anchors.verticalCenter: parent.verticalCenter
                width: 100
                height: Theme.control
                model: ["3 V", "4,5 V", "9 V"]
                currentIndex: lab.controller.cells === 2 ? 0 : (lab.controller.cells === 3 ? 1 : 2)
                onActivated: (index) => { lab.controller.cells = [2, 3, 6][index] }
                Accessible.name: qsTr("Batteriespannung")
            }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Zeit %1 s").arg(lab.controller.simTime.toLocaleString(Qt.locale(), "f", 1))
                color: Theme.solderAmber
                font.pixelSize: 15
            }
        }
    }

    PartTray {
        id: tray
        anchors { left: parent.left; top: toolbar.bottom; bottom: parent.bottom }
        parts: lab.controller.catalogParts
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
        anchors { left: tray.right; right: parent.right; top: toolbar.bottom; bottom: parent.bottom }
        color: Theme.board
        clip: true

        // Tippen ins Leere hebt die Auswahl auf
        TapHandler { onTapped: { lab.selectedPart = ""; lab.selectedWire = "" } }

        Item {
            id: canvas
            objectName: "canvas"
            x: Theme.margin
            y: Theme.margin * 2
            width: Math.max(lab.boardWidth, lab.deskNames.length * (Theme.deskFieldWidth + Theme.deskFieldGap))
            height: lab.moduleY + Theme.fieldHeight

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
                    sockets: modelData.startsWith("M:") ? 1 : 4
                    width: lab.anchorSize(modelData).width
                    height: lab.anchorSize(modelData).height
                    x: lab.anchorPoint(modelData).x - width / 2
                    y: lab.anchorPoint(modelData).y - height / 2
                    color: modelData.startsWith("MESS:") ? "#D9DEE6" : Theme.field
                    highlighted: lab.highlightAnchor === modelData
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
                    value: modelData.value
                    closed: partState.closed === true
                    selected: lab.selectedPart === modelData.id
                    points: modelData.anchors.map(a => lab.anchorPoint(a))
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
                    readonly property point fromPoint: movingFrom ? lab.wirePreviewEnd : lab.anchorPoint(modelData.from)
                    readonly property point toPoint: movingTo ? lab.wirePreviewEnd : lab.anchorPoint(modelData.to)

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
                Label { anchors.centerIn: parent; text: Texts.partName(lab.trayType); font.pixelSize: 10 }
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
            onTriggered: lab.controller.movePart(partMenu.part.id, partMenu.part.start, !partMenu.part.vertical,
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

    Toast {
        id: toast
        objectName: "toast"
        anchors.horizontalCenter: boardArea.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.margin
    }
}
