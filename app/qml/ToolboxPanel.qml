// Werkzeugkasten (Konzept 5.7, docs/ui-spec.md): Farbcode in beide Richtungen (4 und 5 Ringe), E-Reihen als
// Zahlenstrahl mit naechstem Normwert und Abweichung, Bauteillexikon. »Uebernehmen« setzt den Wert des ausgewaehlten
// Widerstands (Befehl, rueckgaengig machbar). Rechnen im Modell (model/resistor_code.hpp).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Drawer {
    id: box
    required property var controller
    property string selectedPartId: "" // ausgewaehltes Bauteil im Labor
    property string selectedPartType: ""
    readonly property bool canApply: selectedPartType === "widerstand"
    // Fehlersuche: Befunde fuer die Marken auf der Grundplatte, Antippen hebt eine Stelle hervor
    signal checkResults(var list)
    signal findingSelected(var anchors, string partId)

    // Fehlersuche oeffnen und gleich pruefen (Hinweis von Tick, wenn etwas kaputt ist oder nicht funktioniert)
    function showFaultFinder() {
        open()
        tabs.currentIndex = 4
        Qt.callLater(faultFinder.check)
    }

    edge: Qt.RightEdge
    width: 460
    onClosed: faultFinder.reset()
    modal: false
    interactive: false // nur ueber den Knopf: kein Wischen vom Rand (rechts liegen Experimentfeld und Schaltplan)
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { color: "#FFFDF7"; border.color: Theme.fieldOutline }

    // Farben nach IEC 60062: 0 schwarz ... 9 weiss, 10 gold, 11 silber
    readonly property var ringColors: ["#000000", "#8B4513", "#E8221B", "#FF8C00", "#F2D200", "#2E9A44", "#2C62C9",
                                       "#8A2BE2", "#808080", "#FFFFFF", "#D4AF37", "#C0C0C0"]
    readonly property var ringNames: [qsTr("schwarz"), qsTr("braun"), qsTr("rot"), qsTr("orange"), qsTr("gelb"),
                                      qsTr("grün"), qsTr("blau"), qsTr("violett"), qsTr("grau"), qsTr("weiß"),
                                      qsTr("gold"), qsTr("silber")]
    function ohmText(v) {
        if (!(v > 0)) return "–"
        const unit = v >= 1e6 ? [1e6, " MΩ"] : v >= 1e3 ? [1e3, " kΩ"] : [1, " Ω"]
        const x = v / unit[0]
        const d = x < 10 ? 2 : (x < 100 ? 1 : 0)
        let s = Number(x).toLocaleString(Qt.locale(), "f", d)
        const sep = Qt.locale().decimalPoint
        if (s.indexOf(sep) >= 0) s = s.replace(new RegExp("\\" + sep + "?0+$"), "")
        return s + unit[1]
    }

    component Heading: Label { font.pixelSize: Math.round(16 * Theme.fontScale); font.bold: true; color: Theme.navy }
    component Body: Label {
        width: parent ? parent.width : 300
        wrapMode: Text.Wrap
        font.pixelSize: Math.round(14 * Theme.fontScale)
        color: "#1E2B38"
    }
    component Choice: Button {
        property bool selected: false
        height: Theme.control
        font.pixelSize: Math.round(14 * Theme.fontScale)
        font.bold: selected
        background: Rectangle {
            radius: 6
            color: parent.selected ? Theme.accent : "#EEF4FA"
            border.color: Theme.accent
        }
        palette.buttonText: selected ? "white" : Theme.navy
    }

    Column {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        Row {
            width: parent.width
            Image {
                anchors.verticalCenter: parent.verticalCenter
                width: 28; height: 28
                sourceSize: Qt.size(64, 64)
                source: "qrc:/icons/toolbox_dark.svg"
            }
            Label {
                width: parent.width - closeButton.width - 36
                leftPadding: 8
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Werkzeugkasten")
                font.pixelSize: Math.round(20 * Theme.fontScale)
                font.bold: true
                color: Theme.navy
            }
            ToolButton {
                id: closeButton
                width: Theme.control; height: Theme.control
                contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 20 } }
                onClicked: box.close()
                Accessible.name: qsTr("Schließen")
            }
        }

        // Reiterleiste mit Hinweispfeilen: Reiter, die nicht mehr in die Breite passen, liegen verdeckt rechts
        // (oder links); ein pulsierender Pfeil zeigt das, Antippen rollt die Leiste dorthin
        Item {
            id: tabArea
            width: parent.width
            height: tabs.height
            readonly property var view: tabs.contentItem
            readonly property bool moreRight: view && view.contentWidth > view.width + 1 && !view.atXEnd
            readonly property bool moreLeft: view && view.contentWidth > view.width + 1 && !view.atXBeginning

            TabBar {
                id: tabs
                objectName: "toolboxTabs"
                clip: true
                anchors.fill: parent
                TabButton { objectName: "tabColor"; text: qsTr("Farbcode"); height: Theme.control; width: implicitWidth; leftPadding: 12; rightPadding: 12 }
                TabButton { objectName: "tabSeries"; text: qsTr("E-Reihen"); height: Theme.control; width: implicitWidth; leftPadding: 12; rightPadding: 12 }
                TabButton { objectName: "tabLexicon"; text: qsTr("Lexikon"); height: Theme.control; width: implicitWidth; leftPadding: 12; rightPadding: 12 }
                TabButton { objectName: "tabCalc"; text: qsTr("Rechner"); height: Theme.control; width: implicitWidth; leftPadding: 12; rightPadding: 12 }
                TabButton { objectName: "tabCheck"; text: qsTr("Fehlersuche"); height: Theme.control; width: implicitWidth; leftPadding: 12; rightPadding: 12 }
                TabButton { objectName: "tabParts"; text: qsTr("Teileliste"); height: Theme.control; width: implicitWidth; leftPadding: 12; rightPadding: 12 }
            }

            component TabArrow: Rectangle {
                id: arrow
                property bool pointsRight: true
                signal activated()
                width: 44
                height: tabs.height
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: arrow.pointsRight ? "#00FFFDF7" : "#FFFDF7" }
                    GradientStop { position: 0.45; color: "#FFFDF7" }
                    GradientStop { position: 1.0; color: arrow.pointsRight ? "#FFFDF7" : "#00FFFDF7" }
                }
                Rectangle {
                    id: arrowDot
                    anchors.verticalCenter: parent.verticalCenter
                    x: arrow.pointsRight ? parent.width - width - 4 : 4
                    width: 30; height: 30; radius: 15
                    color: Theme.accent
                    DrawnIcon {
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: arrow.pointsRight ? 1 : -1
                        kind: arrow.pointsRight ? "right" : "left"
                        color: "white"
                        size: 22
                    }
                    SequentialAnimation on opacity {
                        running: arrow.visible && !Theme.reduceMotion
                        loops: Animation.Infinite
                        NumberAnimation { to: 0.45; duration: 700; easing.type: Easing.InOutQuad }
                        NumberAnimation { to: 1.0; duration: 700; easing.type: Easing.InOutQuad }
                    }
                }
                TapHandler { onTapped: arrow.activated() }
                Accessible.role: Accessible.Button
                Accessible.name: arrow.pointsRight ? qsTr("Weitere Reiter rechts") : qsTr("Weitere Reiter links")
            }
            TabArrow {
                objectName: "tabsMoreRight"
                anchors.right: parent.right
                visible: tabArea.moreRight
                onActivated: tabArea.view.contentX = tabArea.view.contentWidth - tabArea.view.width
            }
            TabArrow {
                objectName: "tabsMoreLeft"
                pointsRight: false
                anchors.left: parent.left
                visible: tabArea.moreLeft
                onActivated: tabArea.view.contentX = 0
            }
        }

        StackLayout {
            width: parent.width
            height: parent.height - y
            currentIndex: tabs.currentIndex

            // ------------------------------------------------------------ Farbcode
            Flickable {
                contentHeight: colorCol.implicitHeight
                clip: true
                Column {
                    id: colorCol
                    width: parent.width
                    spacing: 10
                    property int ringCount: 4
                    property var rings: [4, 7, 1, 10] // 470 Ohm, gold
                    readonly property real ohm: box.controller.decodeColors(rings)
                    function setValue(v) {
                        const c = box.controller.colorCode(v, ringCount)
                        if (c.valid) rings = c.rings
                        return c.valid
                    }
                    function setRingCount(n) {
                        const v = ohm
                        ringCount = n
                        if (!setValue(v)) rings = n === 5 ? [4, 7, 0, 0, 1] : [4, 7, 1, 10]
                    }

                    Row {
                        spacing: 8
                        Choice { objectName: "rings4"; text: qsTr("4 Ringe"); selected: colorCol.ringCount === 4; onClicked: colorCol.setRingCount(4) }
                        Choice { objectName: "rings5"; text: qsTr("5 Ringe"); selected: colorCol.ringCount === 5; onClicked: colorCol.setRingCount(5) }
                    }
                    // Widerstand mit Ringen
                    Item {
                        width: parent.width
                        height: 56
                        Rectangle { x: 10; y: 27; width: parent.width - 20; height: 3; color: "#8A8A8A" }
                        Rectangle {
                            x: 70; y: 10
                            width: parent.width - 140
                            height: 38
                            radius: 14
                            color: "#E8D2A6"
                            border.color: "#A8916A"
                            Row {
                                anchors.centerIn: parent
                                spacing: 14
                                Repeater {
                                    model: colorCol.rings
                                    Rectangle {
                                        required property int index
                                        required property int modelData
                                        width: 10
                                        height: 38
                                        // Toleranzring mit Abstand (Leserichtung)
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: box.ringColors[modelData]
                                        border.color: "#555555"
                                        border.width: modelData === 9 ? 1 : 0
                                    }
                                }
                            }
                        }
                    }
                    Label {
                        objectName: "colorValue"
                        text: box.ohmText(colorCol.ohm) + "  ± " + Number(colorCol.rings[colorCol.rings.length - 1] === 10 ? 5
                              : colorCol.rings[colorCol.rings.length - 1] === 11 ? 10 : [0, 1, 2, 0, 0, 0.5, 0.25, 0.1, 0.05][colorCol.rings[colorCol.rings.length - 1]] || 0)
                              .toLocaleString(Qt.locale(), "f", 2).replace(/[.,]?0+$/, "") + " %"
                        font.pixelSize: Math.round(24 * Theme.fontScale)
                        font.bold: true
                        color: Theme.navy
                    }
                    // Ring fuer Ring waehlen
                    Repeater {
                        model: colorCol.ringCount
                        Row {
                            id: ringRow
                            required property int index
                            spacing: 8
                            readonly property bool isMultiplier: index === colorCol.ringCount - 2
                            readonly property bool isTolerance: index === colorCol.ringCount - 1
                            readonly property var options: isTolerance ? [1, 2, 5, 6, 7, 8, 10, 11]
                                                         : isMultiplier ? [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]
                                                         : index === 0 ? [1, 2, 3, 4, 5, 6, 7, 8, 9] : [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
                            Label {
                                width: 120
                                anchors.verticalCenter: parent.verticalCenter
                                text: ringRow.isTolerance ? qsTr("Toleranz") : ringRow.isMultiplier ? qsTr("Multiplikator")
                                                           : qsTr("%1. Ziffer").arg(ringRow.index + 1)
                                font.pixelSize: Math.round(13 * Theme.fontScale)
                                color: "#4A5866"
                            }
                            ComboBox {
                                objectName: "ring_" + ringRow.index
                                width: 200
                                height: Theme.control
                                model: ringRow.options.map(c => box.ringNames[c])
                                currentIndex: ringRow.options.indexOf(colorCol.rings[ringRow.index])
                                onActivated: (i) => {
                                    const r = colorCol.rings.slice()
                                    r[ringRow.index] = ringRow.options[i]
                                    colorCol.rings = r
                                }
                                // Farbfeld zusaetzlich zum Namen
                                indicator: Rectangle {
                                    x: parent.width - width - 8
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 22; height: 22; radius: 4
                                    color: box.ringColors[colorCol.rings[ringRow.index]]
                                    border.color: "#555555"
                                }
                            }
                        }
                    }
                    Heading { text: qsTr("Wert eingeben") }
                    Row {
                        spacing: 8
                        TextField {
                            id: valueInput
                            objectName: "colorInput"
                            width: 200
                            height: Theme.control
                            placeholderText: qsTr("z. B. 4,7k")
                            font.pixelSize: Math.round(15 * Theme.fontScale)
                            onAccepted: showRings.clicked()
                        }
                        Choice {
                            id: showRings
                            objectName: "colorShow"
                            text: qsTr("Ringe zeigen")
                            onClicked: inputHint.visible = !colorCol.setValue(box.controller.parseOhm(valueInput.text))
                        }
                    }
                    Body { id: inputHint; visible: false; text: qsTr("Diesen Wert kann ich nicht lesen. Versuche z. B. 470, 4,7k oder 2M2."); color: "#8A3200" }
                    Choice {
                        objectName: "colorApply"
                        text: box.canApply ? qsTr("Auf %1 übernehmen").arg(box.selectedPartId) : qsTr("Erst einen Widerstand auswählen")
                        enabled: box.canApply && colorCol.ohm > 0
                        onClicked: box.controller.setPartValue(box.selectedPartId, colorCol.ohm)
                    }
                }
            }

            // ------------------------------------------------------------ E-Reihen
            Flickable {
                contentHeight: seriesCol.implicitHeight
                clip: true
                Column {
                    id: seriesCol
                    width: parent.width
                    spacing: 12
                    property int series: 12
                    property real input: 500
                    readonly property var values: box.controller.eSeries(series)
                    readonly property var near: box.controller.nearestE(input, series)

                    Row {
                        spacing: 8
                        Repeater {
                            model: [6, 12, 24]
                            Choice {
                                required property int modelData
                                objectName: "series_" + modelData
                                text: "E" + modelData
                                selected: seriesCol.series === modelData
                                onClicked: seriesCol.series = modelData
                            }
                        }
                    }
                    // Zahlenstrahl einer Dekade (logarithmisch), Normwerte als Striche, Eingabe als Marke
                    Item {
                        id: line
                        width: parent.width
                        height: 86
                        readonly property real x0: 14
                        readonly property real x1: width - 14
                        function pos(f) { return x0 + (x1 - x0) * Math.log10(f) }
                        Rectangle { x: line.x0; y: 40; width: line.x1 - line.x0; height: 2; color: Theme.navy }
                        Repeater {
                            model: seriesCol.values.concat([10])
                            Item {
                                required property real modelData
                                x: line.pos(modelData)
                                Rectangle { x: -1; y: 32; width: 2; height: 18; color: Theme.navy }
                                Label {
                                    visible: seriesCol.series !== 24 || [1, 1.5, 2.2, 3.3, 4.7, 6.8, 10].indexOf(modelData) >= 0
                                    x: -width / 2; y: 52
                                    text: Number(modelData).toLocaleString(Qt.locale(), "f", modelData === 10 ? 0 : 1)
                                    font.pixelSize: Math.round(11 * Theme.fontScale)
                                    color: Theme.navy
                                }
                            }
                        }
                        // Eingabe (auf eine Dekade bezogen)
                        Item {
                            readonly property real f: seriesCol.input / Math.pow(10, Math.floor(Math.log10(seriesCol.input)))
                            x: line.pos(f)
                            visible: seriesCol.input > 0
                            Label { x: -width / 2; y: 6; text: "▼"; color: Theme.signalOrange; font.pixelSize: 18 }
                        }
                    }
                    Heading { text: qsTr("Wert eingeben") }
                    Row {
                        spacing: 8
                        TextField {
                            id: seriesInput
                            objectName: "seriesInput"
                            width: 200
                            height: Theme.control
                            text: "500"
                            font.pixelSize: Math.round(15 * Theme.fontScale)
                            onTextChanged: {
                                const v = box.controller.parseOhm(text)
                                if (v > 0) seriesCol.input = v
                            }
                        }
                        Label {
                            anchors.verticalCenter: parent.verticalCenter
                            text: box.ohmText(seriesCol.input)
                            font.pixelSize: Math.round(15 * Theme.fontScale)
                            color: "#4A5866"
                        }
                    }
                    Rectangle {
                        width: parent.width
                        height: nearCol.implicitHeight + 20
                        radius: 8
                        color: seriesCol.near.isMember ? "#E6F4E7" : "#EEF4FA"
                        border.color: seriesCol.near.isMember ? "#2E7D32" : Theme.accent
                        Column {
                            id: nearCol
                            x: 10; y: 10
                            width: parent.width - 20
                            spacing: 4
                            Body {
                                objectName: "seriesResult"
                                font.bold: true
                                text: seriesCol.near.isMember
                                      ? "✓ " + qsTr("%1 ist ein Normwert der Reihe E%2.").arg(box.ohmText(seriesCol.near.value)).arg(seriesCol.series)
                                      : qsTr("Nächster Normwert in E%1: %2").arg(seriesCol.series).arg(box.ohmText(seriesCol.near.value))
                            }
                            Body {
                                visible: !seriesCol.near.isMember
                                text: qsTr("Abweichung: %1 %").arg((seriesCol.near.deviation > 0 ? "+" : "")
                                      + Number(seriesCol.near.deviation).toLocaleString(Qt.locale(), "f", 1))
                            }
                        }
                    }
                    Choice {
                        objectName: "seriesApply"
                        text: box.canApply ? qsTr("Normwert auf %1 übernehmen").arg(box.selectedPartId) : qsTr("Erst einen Widerstand auswählen")
                        enabled: box.canApply && seriesCol.near.value > 0
                        onClicked: box.controller.setPartValue(box.selectedPartId, seriesCol.near.value)
                    }
                    Body {
                        text: qsTr("Widerstände gibt es nicht in jedem Wert, sondern in genormten Stufen. In der Reihe E12 hat jede Zehnerstufe 12 Werte.")
                        color: "#4A5866"
                    }
                }
            }

            // ------------------------------------------------------------ Lexikon
            ListView {
                objectName: "lexiconList"
                clip: true
                spacing: 8
                model: box.controller.lexicon
                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: Math.max(64, lexText.implicitHeight + 34)
                    radius: 8
                    color: "white"
                    border.color: Theme.fieldOutline
                    SchematicSymbol {
                        x: 8; y: 10
                        width: 60; height: 30
                        type: modelData.type
                    }
                    Column {
                        x: 80; y: 8
                        width: parent.width - 90
                        spacing: 2
                        Label { text: Texts.partName(modelData.type); font.pixelSize: Math.round(15 * Theme.fontScale); font.bold: true; color: Theme.navy }
                        Label {
                            id: lexText
                            width: parent.width
                            text: modelData.text
                            wrapMode: Text.Wrap
                            font.pixelSize: Math.round(13 * Theme.fontScale)
                            color: "#1E2B38"
                        }
                    }
                }
            }

            // ------------------------------------------------------------ Formelrechner (ab Stufe 3)
            FormulaCalculator {
                objectName: "formulaCalculator"
                controller: box.controller
                canApply: box.canApply
                selectedPartId: box.selectedPartId
            }

            // ------------------------------------------------------------ Fehlersuche (ab Stufe 2)
            FaultFinder {
                id: faultFinder
                objectName: "faultFinder"
                controller: box.controller
                onCheckResults: (list) => box.checkResults(list)
                onFindingSelected: (anchors, partId) => box.findingSelected(anchors, partId)
            }

            // ------------------------------------------------------------ Teileliste mit Aufbaukarte (PDF)
            PartsListPanel {
                id: partsList
                objectName: "partsList"
                controller: box.controller
            }
        }
    }
}
