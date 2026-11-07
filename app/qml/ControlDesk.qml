// Bedienpult I (docs/ui-spec.md, Bedienpult): Batterie mit Zellenwahl und Hauptschalter, Potentiometer 1 und 2 mit
// Lichtring, Schalter (Kippschalter), Lautsprecher. Die blaue Pultflaeche schliesst die Pultfelder ein; jedes Bauteil
// steht mit seinen Feldern in einem eigenen Rahmen (Lage aus Lab.groups). Ausgeblendete Bauteile (Stufenanpassung)
// fehlen in groups. Alles hier sind Bedienhandlungen (kein Verlauf). Rundes »?« oben rechts je Rahmen erklaert das
// Bauteil und seine Felder (DeskPartInfo).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Rectangle {
    id: desk
    required property var controller
    property var groups: []        // [{name, left, right, ...}] in Koordinaten der Arbeitsflaeche
    property real offsetX: 0       // Pult beginnt links vor der Arbeitsflaeche
    property real fieldsBottom: 0  // Unterkante der Pultfelder (Rahmen schliessen sie ein)
    property real fieldsTop: 0     // Oberkante der Pultfelder (Schaltzeichen sitzen direkt darueber)
    property bool meterFrame: false // Messgeraet oben im Pult: eigener Rahmen um Anzeige und Buchsen
    property real meterLeft: 0
    property real meterWidth: 0
    property string highlightGroup: ""
    signal groupTapped(string name)
    signal infoRequested(string name)
    signal generatorRequested() // Generatorfenster oeffnen
    signal tracerRequested()    // Kennlinienschreiber oeffnen
    signal stationsRequested()  // Senderfenster oeffnen (ab Volta 3)
    signal trayRequested()          // Pult nach Bedarf: Geraetefach oeffnen (Einsetzen)
    signal trayRemoveRequested()    // ... zum Herausnehmen
    property real trayX: 0          // Lage des Knopfs »+« (Arbeitsflaeche)
    // Zustand der Drehregler (durchgebrannt, ueberlastet) aus der laufenden Simulation
    property var pot1: ({})
    property var pot2: ({})
    Connections {
        target: desk.controller
        function onStateChanged() {
            desk.pot1 = desk.controller.potState(1)
            desk.pot2 = desk.controller.potState(2)
        }
    }

    color: Theme.desk
    radius: 10

    function group(name) {
        for (let g = 0; g < groups.length; ++g) if (groups[g].name === name) return groups[g]
        return null
    }
    function centerOf(name) { const g = group(name); return g ? offsetX + (g.left + g.right) / 2 : 0 }
    readonly property color frameColor: "#8FA6BC"

    // Rahmen je Bauteil: Bedienelement oben, Felder unten; Tippen hebt die Felder hervor
    Repeater {
        model: desk.groups
        Rectangle {
            required property var modelData
            x: desk.offsetX + modelData.left
            y: 6
            width: modelData.right - modelData.left
            height: desk.fieldsBottom - y
            radius: 10
            color: Qt.lighter(Theme.desk, 1.12)
            border.color: desk.highlightGroup === modelData.name ? Theme.signalOrange : desk.frameColor
            border.width: 2
            TapHandler { onTapped: desk.groupTapped(modelData.name) }
            // »?«: sichtbar 18 dp in der abgerundeten Ecke (gleicher Mittelpunkt wie der Eckbogen); Tippflaeche 48 dp,
            // rechts buendig mit dem Rahmen (nicht im Nachbarrahmen) und unter den Bedienelementen, damit diese Vorrang haben
            AbstractButton {
                id: infoButton
                objectName: "deskInfo_" + parent.modelData.name
                x: parent.width - width
                y: parent.radius - height / 2
                width: Theme.control
                height: Theme.control
                onClicked: desk.infoRequested(parent.modelData.name)
                Accessible.name: qsTr("Was ist das?")
                background: Item {
                    Rectangle {
                        x: parent.width - 10 - width / 2
                        y: (parent.height - height) / 2
                        width: 18; height: 18; radius: 9
                        color: infoButton.down ? "#FFFFFF" : (infoButton.hovered ? "#E6EEF6" : "#C9D6E3")
                        border.color: "#FFFFFF"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "?"
                            color: Theme.navy
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }
                }
            }
            // Betriebslampe oben links: Generator und Wechselquelle laufen mit dem Hauptschalter, der
            // Kennlinienschreiber hat ein eigenes Netzteil und laeuft immer. An gruen mit Schein, aus dunkel ohne Schein
            // (Helligkeit und Schein, nicht nur die Farbe)
            PowerLed {
                readonly property string part: parent.modelData.name
                objectName: "deskLed_" + part
                visible: part === "generator" || part === "wechselquelle" || part === "kennlinienschreiber"
                on: part === "kennlinienschreiber" || desk.controller.mainSwitch
                x: 9; y: 9
                accessibleName: part === "generator" ? (on ? qsTr("Generator läuft") : qsTr("Generator aus"))
                              : part === "wechselquelle" ? (on ? qsTr("Wechselquelle an") : qsTr("Wechselquelle aus"))
                              : qsTr("Kennlinienschreiber an (eigenes Netzteil)")
            }
        }
    }

    // Rahmen um das Messgeraet oben im Pult (Anzeige oben, Buchsen COM, V/Ω, A unten) wie bei den Bauteilen
    Rectangle {
        objectName: "meterDeskFrame"
        visible: desk.meterFrame
        x: desk.offsetX + desk.meterLeft - 12
        y: 6
        width: desk.meterWidth + 24
        height: desk.fieldsBottom - y
        radius: 10
        color: Qt.lighter(Theme.desk, 1.12)
        border.color: desk.frameColor
        border.width: 2
    }

    // Schaltzeichen ueber den Feldern (DIN EN 60617) mit Leitungen hinunter zu jedem Feld: Batterie (langer Strich
    // Pluspol ueber V+, kurzer dicker Minuspol ueber 0V), Potentiometer (Schleifer als Pfeil zum mittleren Feld),
    // Schalter (zeigt die Stellung), Lautsprecher
    Repeater {
        model: desk.groups.filter(g => ["batterie", "batterie2", "wechselquelle", "generator", "kennlinienschreiber",
                                        "senderanlage", "p1", "p2", "schalter", "lautsprecher"].indexOf(g.name) >= 0)
        Shape {
            id: deskSymbol
            required property var modelData
            objectName: "deskSymbol_" + modelData.name
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            readonly property real fieldTop: desk.fieldsTop
            readonly property real y0: fieldTop - 20 // Hoehe der Leitung im Schaltzeichen
            function cx(k) {
                return desk.offsetX + modelData.fieldsLeft + k * (Theme.deskFieldWidth + Theme.deskFieldGap)
                       + Theme.deskFieldWidth / 2
            }
            readonly property var paths: {
                const t = fieldTop, y = y0
                switch (modelData.name) {
                case "batterie": case "batterie2": { // V+, 0V bzw. +B2, -B2
                    const a = cx(0), b = cx(1), m = (a + b) / 2
                    return {s: `M${a} ${t} V${y} H${m - 4} M${m + 4} ${y} H${b} V${t} M${m - 4} ${y - 9} V${y + 9}`,
                            f: `M${m + 2} ${y - 5} H${m + 6} V${y + 5} H${m + 2} Z`, plus: m - 11}
                }
                case "wechselquelle": case "generator": { // Kreis mit ~ (Wechselquelle) bzw. Sinus (Generator)
                    const a = cx(0), b = cx(1), m = (a + b) / 2, r = 9
                    const wave = modelData.name === "generator"
                        ? ` M${m - 6} ${y} C${m - 4} ${y - 8} ${m - 2} ${y - 8} ${m} ${y} C${m + 2} ${y + 8} ${m + 4} ${y + 8} ${m + 6} ${y}`
                        : ""
                    return {s: `M${a} ${t} V${y} H${m - r} M${m + r} ${y} H${b} V${t} M${m - r} ${y} a${r} ${r} 0 1 0 ${2 * r} 0 a${r} ${r} 0 1 0 ${-2 * r} 0` + wave,
                            f: "", mark: modelData.name === "wechselquelle" ? "~" : undefined, mx: m}
                }
                case "kennlinienschreiber": { // Kreis mit Kennlinie (Knick wie bei der Diode)
                    const a = cx(0), b = cx(1), m = (a + b) / 2, r = 9
                    return {s: `M${a} ${t} V${y} H${m - r} M${m + r} ${y} H${b} V${t} M${m - r} ${y} a${r} ${r} 0 1 0 ${2 * r} 0 a${r} ${r} 0 1 0 ${-2 * r} 0`
                               + ` M${m - 6} ${y + 4} H${m + 1} C${m + 3} ${y + 4} ${m + 4} ${y} ${m + 5} ${y - 6}`,
                            f: "", plus: a + 12}
                }
                case "senderanlage": { // Antenne (DIN EN 60617): Mast mit Dreieck, ein Feld ANT
                    const a = cx(0)
                    return {s: `M${a} ${t} V${y - 12} M${a - 9} ${y - 22} L${a} ${y - 12} L${a + 9} ${y - 22} M${a} ${y - 12} V${y - 22}`,
                            f: ""}
                }
                case "p1": case "p2": { // a, s, b: Widerstand zwischen a und b, Schleifer von s
                    const a = cx(0), s = cx(1), b = cx(2)
                    return {s: `M${a} ${t} V${y} H${s - 22} M${s + 22} ${y} H${b} V${t} M${s - 22} ${y - 6} H${s + 22} V${y + 6} H${s - 22} Z M${s} ${t} V${y + 11}`,
                            f: `M${s} ${y + 7} L${s - 4} ${y + 14} L${s + 4} ${y + 14} Z`}
                }
                case "schalter": { // Schliesser; geschlossen, wenn der Schalter ein ist
                    const a = cx(0), b = cx(1), l = a + 18, r = b - 18
                    const end = desk.controller.deskSwitch ? `L${r + 2} ${y - 2}` : `L${r} ${y - 13}`
                    return {s: `M${a} ${t} V${y} H${l} ${end} M${r} ${y} H${b} V${t}`,
                            f: `M${l - 2.5} ${y} a2.5 2.5 0 1 0 5 0 a2.5 2.5 0 1 0 -5 0 Z`}
                }
                case "lautsprecher": { // Magnet mit Trichter nach oben
                    const a = cx(0), b = cx(1), m = (a + b) / 2
                    return {s: `M${a} ${t} V${y} H${m - 8} M${m + 8} ${y} H${b} V${t} M${m - 8} ${y - 4} H${m + 8} V${y + 4} H${m - 8} Z M${m - 6} ${y - 4} L${m - 12} ${y - 14} H${m + 12} L${m + 6} ${y - 4}`,
                            f: ""}
                }
                }
                return {s: "M0 0", f: ""}
            }
            ShapePath {
                strokeColor: "#D5E2EE"
                strokeWidth: 2
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                PathSvg { path: deskSymbol.paths.s }
            }
            ShapePath {
                strokeColor: "transparent"
                fillColor: "#D5E2EE"
                PathSvg { path: deskSymbol.paths.f || "M0 0" }
            }
            Text { // Wechselzeichen im Kreis der Wechselquelle
                visible: deskSymbol.paths.mark !== undefined
                x: (deskSymbol.paths.mx || 0) - width / 2
                y: deskSymbol.y0 - height / 2 - 1
                text: deskSymbol.paths.mark || ""
                color: "#D5E2EE"
                font.pixelSize: 15
                font.bold: true
            }
            Text { // Pluszeichen am langen Strich (Polaritaet nie nur ueber die Strichlaenge)
                visible: deskSymbol.paths.plus !== undefined
                x: (deskSymbol.paths.plus || 0) - width / 2
                y: deskSymbol.y0 - 12 - height / 2
                text: "+"
                color: "#D5E2EE"
                font.pixelSize: 12
                font.bold: true
            }
        }
    }

    // Batterie: Zellenwahl und Hauptschalter
    Column {
        visible: desk.group("batterie") !== null
        x: desk.centerOf("batterie") - width / 2
        y: 12
        spacing: 4
        Label { text: qsTr("Batterie"); color: "white"; font.pixelSize: Math.round((13) * Theme.fontScale); font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
        ComboBox {
            objectName: "cellsBox"
            width: 96
            height: Theme.control - 6
            // Zahlenformat der Anzeigesprache (4,5 V / 4.5 V); Neuaufbau bei Sprachwechsel ueber qsTr-Bindung
            // ab Volta 3 auch eine Zelle (1,5 V) fuer Aufwaertswandler und Detektor (Konzept Volta 3, 5.1)
            readonly property var choices: desk.controller.band === "" || desk.controller.band === "volta1"
                                           || desk.controller.band === "volta2" ? [2, 3, 6] : [1, 2, 3, 6]
            model: choices.map(c => qsTr("%1 V").arg(c % 2 === 1 ? (1.5 * c).toLocaleString(Qt.locale(), "f", 1) : 1.5 * c))
            currentIndex: Math.max(0, choices.indexOf(desk.controller.cells))
            onActivated: (index) => { desk.controller.cells = choices[index] }
            Accessible.name: qsTr("Batteriespannung")
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 4
            Switch {
                objectName: "mainSwitch"
                checked: desk.controller.mainSwitch
                onToggled: desk.controller.mainSwitch = checked
                Accessible.name: qsTr("Hauptschalter")
            }
            // Stellung auch als Text (Farbe nie allein)
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: desk.controller.mainSwitch ? qsTr("Ein") : qsTr("Aus")
                color: "white"
                font.pixelSize: Math.round((14) * Theme.fontScale)
                font.bold: true
            }
        }
    }

    // Batterie 2 (ab Volta 2): Spannungswahl; sie haengt am Hauptschalter der Batterie
    Column {
        visible: desk.group("batterie2") !== null
        x: desk.centerOf("batterie2") - width / 2
        y: 12
        spacing: 4
        Label { text: qsTr("Batterie 2"); color: "white"; font.pixelSize: Math.round((13) * Theme.fontScale); font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
        ComboBox {
            objectName: "cells2Box"
            width: 96
            height: Theme.control - 6
            model: [qsTr("%1 V").arg((4.5).toLocaleString(Qt.locale(), "f", 1)), qsTr("%1 V").arg(9)]
            currentIndex: desk.controller.cells2 === 3 ? 0 : 1
            onActivated: (index) => { desk.controller.cells2 = [3, 6][index] }
            Accessible.name: qsTr("Spannung von Batterie 2")
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: desk.controller.mainSwitch ? qsTr("Ein") : qsTr("Aus")
            color: "white"
            font.pixelSize: Math.round((14) * Theme.fontScale)
            font.bold: true
        }
    }

    // Wechselquelle (ab Volta 2): 6 / 9 V~, fest 50 Hz; der Hauptschalter der Batterie schaltet sie mit
    Column {
        visible: desk.group("wechselquelle") !== null
        x: desk.centerOf("wechselquelle") - width / 2
        y: 12
        spacing: 4
        Label { text: qsTr("Wechselquelle"); color: "white"; font.pixelSize: Math.round((13) * Theme.fontScale); font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
        ComboBox {
            objectName: "acRmsBox"
            width: 104
            height: Theme.control - 6
            font.bold: true
            background: Rectangle { radius: 6; color: "#F4F7FA"; border.color: "#7F93A8"; border.width: 1.5 }
            model: [qsTr("%1 V~").arg(6), qsTr("%1 V~").arg(9)]
            currentIndex: desk.controller.acRms === 6 ? 0 : 1
            onActivated: (index) => { desk.controller.acRms = [6, 9][index] }
            Accessible.name: qsTr("Spannung der Wechselquelle")
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: (desk.controller.mainSwitch ? qsTr("Ein") : qsTr("Aus")) + " · 50 Hz"
            color: "white"
            font.pixelSize: Math.round((14) * Theme.fontScale)
            font.bold: true
        }
    }

    // Funktionsgenerator (ab Volta 2): Knopf mit Signalform und Werten oeffnet das Generatorfenster
    Column {
        visible: desk.group("generator") !== null
        x: desk.centerOf("generator") - width / 2
        y: 12
        spacing: 4
        Label { text: qsTr("Generator"); color: "white"; font.pixelSize: Math.round((13) * Theme.fontScale); font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
        PanelButton {
            objectName: "generatorButton"
            width: 112
            height: Theme.control
            readonly property var forms: [qsTr("Sinus"), qsTr("Dreieck"), qsTr("Rechteck")]
            readonly property real hz: desk.controller.genFrequency
            text: forms[desk.controller.genWaveform] + "\n"
                  + (hz >= 1000 ? qsTr("%1 kHz").arg(Number(hz / 1000).toLocaleString(Qt.locale(), "f", 1))
                                : qsTr("%1 Hz").arg(Number(hz).toLocaleString(Qt.locale(), "f", hz >= 100 ? 0 : 1)))
            font.pixelSize: Math.round(12 * Theme.fontScale)
            onClicked: desk.generatorRequested()
            Accessible.name: qsTr("Funktionsgenerator einstellen")
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("%1 V Spitze").arg(Number(desk.controller.genAmplitude).toLocaleString(Qt.locale(), "f", 1))
            color: "white"
            font.pixelSize: Math.round((12) * Theme.fontScale)
        }
    }

    // Kennlinienschreiber (ab Volta 2): Knopf mit Bereich und Strombegrenzung oeffnet das Fenster mit der Kennlinie
    Column {
        visible: desk.group("kennlinienschreiber") !== null
        x: desk.centerOf("kennlinienschreiber") - width / 2
        y: 12
        spacing: 4
        // Texte haengen an den Pulteinstellungen (deskChanged, hier ueber acRms)
        readonly property var st: { desk.controller.acRms; return visible ? desk.controller.tracerState() : ({}) }
        Label { text: qsTr("Kennlinie"); color: "white"; font.pixelSize: Math.round((13) * Theme.fontScale); font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
        PanelButton {
            objectName: "tracerButton"
            width: 112
            height: Theme.control
            text: qsTr("Zeichnen") + "\n" + (parent.st.rangeText || "")
            font.pixelSize: Math.round(12 * Theme.fontScale)
            onClicked: desk.tracerRequested()
            Accessible.name: qsTr("Kennlinienschreiber öffnen")
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("bis %1").arg(parent.st.limitText || "")
            color: "white"
            font.pixelSize: Math.round((12) * Theme.fontScale)
        }
    }

    // Senderanlage (ab Volta 3): Knopf mit der Zahl der Sender, die senden, oeffnet das Senderfenster
    Column {
        visible: desk.group("senderanlage") !== null
        x: desk.centerOf("senderanlage") - width / 2
        y: 12
        spacing: 4
        readonly property int onCount: { desk.controller.cells; let n = 0; for (let i = 0; i < 3; ++i) if (desk.controller.stationOn(i)) ++n; return n }
        Label { text: qsTr("Sender"); color: "white"; font.pixelSize: Math.round((13) * Theme.fontScale); font.bold: true; anchors.horizontalCenter: parent.horizontalCenter }
        PanelButton {
            objectName: "stationsButton"
            width: 112
            height: Theme.control
            text: qsTr("Mittelwelle") + "\n" + qsTr("%1 von 3 an").arg(parent.onCount)
            font.pixelSize: Math.round(12 * Theme.fontScale)
            onClicked: desk.stationsRequested()
            Accessible.name: qsTr("Senderanlage einstellen")
        }
    }

    // Pult nach Bedarf: »+« oeffnet das Geraetefach zum Einsetzen, »−« zum Herausnehmen. Uebereinander, Tippflaeche
    // je 48 dp, sichtbarer Kreis so gross wie die Knoepfe der Steckbrettbreite; Zeichen als Balken gezeichnet (mittig)
    component TrayButton: ToolButton {
        id: trayButton
        property bool plus: true
        width: Theme.control; height: Theme.control
        readonly property color ink: enabled ? Theme.navy : "#7A838C"
        contentItem: Item {
            Rectangle { anchors.centerIn: parent; width: 14; height: 3; radius: 1.5; color: trayButton.ink }
            Rectangle { visible: trayButton.plus; anchors.centerIn: parent; width: 3; height: 14; radius: 1.5; color: trayButton.ink }
        }
        background: Item {
            Rectangle {
                anchors.centerIn: parent
                width: 34; height: 34; radius: 17
                color: !trayButton.enabled ? "#8A97A5" : (trayButton.down ? "#E6EEF6" : "#C9D6E3")
                border.color: trayButton.enabled ? "white" : "#A9B4BF"
                border.width: 2
            }
        }
        ToolTip.visible: hovered
        Accessible.name: ToolTip.text
    }
    Column {
        visible: desk.controller.deskOnDemand
        x: desk.offsetX + desk.trayX
        y: (6 + desk.fieldsBottom - height) / 2 // mittig in der Hoehe der Rahmen
        spacing: 0
        TrayButton {
            objectName: "deskTrayButton"
            plus: true
            onClicked: desk.trayRequested()
            ToolTip.text: qsTr("Gerätefach: Pultteile einsetzen")
        }
        TrayButton {
            objectName: "deskTrayRemoveButton"
            plus: false
            enabled: desk.controller.deskTray.some(t => t.present && t.part !== "batterie")
            onClicked: desk.trayRemoveRequested()
            ToolTip.text: qsTr("Pultteile herausnehmen")
        }
    }

    KnobRing {
        objectName: "knobP1"
        visible: desk.group("p1") !== null
        x: desk.centerOf("p1") - width / 2
        y: 8
        label: qsTr("Potentiometer 1")
        value: desk.controller.pot1
        broken: desk.pot1.destroyed === true
        overloaded: desk.pot1.overloaded === true
        onMoved: (v) => desk.controller.pot1 = v
        onRepairRequested: desk.controller.repairPot(1)
    }
    KnobRing {
        objectName: "knobP2"
        visible: desk.group("p2") !== null
        x: desk.centerOf("p2") - width / 2
        y: 8
        label: qsTr("Potentiometer 2")
        value: desk.controller.pot2
        broken: desk.pot2.destroyed === true
        overloaded: desk.pot2.overloaded === true
        onMoved: (v) => desk.controller.pot2 = v
        onRepairRequested: desk.controller.repairPot(2)
    }

    // Schalter: Kippschalter (Knebel oben = ein, unten = aus), Beschriftung darunter
    Item {
        objectName: "deskSwitch"
        visible: desk.group("schalter") !== null
        x: desk.centerOf("schalter") - width + 23 // Platte mittig, Beschriftung links davon
        y: 12
        width: 70
        height: 108
        readonly property bool on: desk.controller.deskSwitch
        Rectangle { // Metallplatte
            id: plate
            anchors.right: parent.right
            width: 46; height: 72; radius: 9
            color: "#3A434D"
            border.color: "#8E99A4"; border.width: 2
            Rectangle { // Drehpunkt
                anchors.centerIn: parent
                width: 18; height: 18; radius: 9
                color: "#6B7580"
            }
            Rectangle { // Knebel
                anchors.horizontalCenter: parent.horizontalCenter
                width: 14; height: 32; radius: 7
                y: parent.parent.on ? 4 : parent.height - height - 4
                color: "#D5DCE3"
                border.color: "#8E99A4"
                Behavior on y { enabled: !Theme.reduceMotion; NumberAnimation { duration: 90 } }
            }
        }
        // Stellungen wie auf einem echten Pult beschriftet (nie nur ueber die Lage)
        Text {
            anchors.right: plate.left; anchors.rightMargin: 3; y: 4
            text: qsTr("ein"); color: parent.on ? "white" : "#8FA6BC"
            font.pixelSize: Math.round((11) * Theme.fontScale); font.bold: parent.on
        }
        Text {
            anchors.right: plate.left; anchors.rightMargin: 3; y: plate.height - height - 4
            text: qsTr("aus"); color: parent.on ? "#8FA6BC" : "white"
            font.pixelSize: Math.round((11) * Theme.fontScale); font.bold: !parent.on
        }
        Label {
            anchors.horizontalCenter: plate.horizontalCenter
            anchors.top: plate.bottom
            anchors.topMargin: 6
            text: qsTr("Schalter")
            color: "white"
            font.pixelSize: Math.round((13) * Theme.fontScale)
            font.bold: true
        }
        TapHandler { onTapped: desk.controller.deskSwitch = !desk.controller.deskSwitch }
        Accessible.role: Accessible.CheckBox
        Accessible.name: qsTr("Schalter")
        Accessible.checkable: true
        Accessible.checked: on
        Accessible.onPressAction: desk.controller.deskSwitch = !desk.controller.deskSwitch
    }

    // Lautsprecher mit Leuchtring (Klanganzeige)
    Column {
        visible: desk.group("lautsprecher") !== null
        x: desk.centerOf("lautsprecher") - width / 2
        y: 12
        spacing: 2
        SpeakerRing {
            anchors.horizontalCenter: parent.horizontalCenter
            controller: desk.controller
        }
        // Name; ab Volta 2 steht dort, woher der Ton kommt (Tonanzeige, Konzept 5.3) - als Text, Farbe nie allein
        Label {
            objectName: "toneModeLabel"
            anchors.horizontalCenter: parent.horizontalCenter
            readonly property string mode: desk.controller.toneMode
            text: mode === "echt" ? qsTr("♪ gerechnet") : mode === "ersatz" ? qsTr("♪ Ersatzton")
                  : mode === "zeitlupe" ? qsTr("Zeitlupe: stumm") : qsTr("Lautsprecher")
            color: mode === "ersatz" ? "#FFD27A" : "white"
            font.pixelSize: Math.round(12 * Theme.fontScale)
            font.bold: mode === "ersatz"
        }
    }
}
