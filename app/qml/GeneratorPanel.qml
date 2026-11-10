// Generatorfenster (ab Volta 2, Konzept 5.2): Signalform, Frequenz in Dekaden mit Feineinstellung, Spitze und
// Gleichanteil. Alles Bedienhandlungen (kein Befehl); jeder Wert steht zusaetzlich als Zahl da. Nicht modal und an
// der Titelzeile verschiebbar, damit Hauptschalter, Oszilloskop und Lautsprecher daneben bedienbar bleiben; »?«
// blendet eine kurze Hilfe ein. Der Generator laeuft mit dem Hauptschalter (Lampe im Pult).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Shapes

Popup {
    id: panel
    property var controller: null
    property bool showHelp: false
    // Amplitudenmodulation gibt es ab Volta 3 (Konzept Volta 3, 5.2); Volta 1 und 2 zeigen sie nicht
    readonly property bool hasAm: controller !== null && controller.band !== "" && controller.band !== "volta1"
                                  && controller.band !== "volta2"

    // Dekade der Frequenz: 1, 10, 100 oder 1000 Hz; der Regler waehlt das 1- bis 10-Fache
    readonly property int decade: controller ? (controller.genFrequency < 10 ? 1 : controller.genFrequency < 100 ? 10
                                               : controller.genFrequency < 1000 ? 100 : 1000) : 1
    function number(v, digits) { return Number(v).toLocaleString(Qt.locale(), "f", digits) }
    function frequencyText(hz) {
        return hz >= 1000 ? qsTr("%1 kHz").arg(number(hz / 1000, hz >= 10000 ? 0 : 2))
                          : qsTr("%1 Hz").arg(number(hz, hz >= 100 ? 0 : 1))
    }

    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape
    // mittig, bis man das Fenster an der Titelzeile verschiebt; es bleibt im Fenster der App
    property bool moved: false
    property point movedTo: Qt.point(0, 0)
    readonly property real homeX: parent ? (parent.width - width) / 2 : 0
    readonly property real homeY: parent ? Math.max(0, (parent.height - height) / 2) : 0
    function clampX(v) { return parent ? Math.max(0, Math.min(parent.width - width, v)) : v }
    function clampY(v) { return parent ? Math.max(0, Math.min(parent.height - Theme.control - 2 * padding, v)) : v }
    x: moved ? clampX(movedTo.x) : homeX
    y: moved ? clampY(movedTo.y) : homeY
    width: Math.min(parent ? parent.width - 32 : 440, 440)
    padding: 16
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    contentItem: Column {
        spacing: 10
        Row {
            width: parent.width
            spacing: 4
            // Betriebslampe wie im Pult: der Generator laeuft mit dem Hauptschalter
            Item {
                width: 22; height: Theme.control
                PowerLed {
                    objectName: "generatorPanelLed"
                    anchors.centerIn: parent
                    on: panel.controller !== null && panel.controller.mainSwitch
                    accessibleName: on ? qsTr("Generator läuft") : qsTr("Generator aus")
                }
            }
            Label {
                objectName: "generatorTitle"
                width: parent.width - 22 - helpButton.width - closeButton.width - 3 * parent.spacing
                height: Theme.control
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Funktionsgenerator")
                color: Theme.navy
                font.pixelSize: Math.round(18 * Theme.fontScale)
                font.bold: true
                // Titelzeile ziehen verschiebt das Fenster (Maus, Finger und Stift), in Fensterkoordinaten
                HoverHandler { cursorShape: Qt.SizeAllCursor }
                DragHandler {
                    target: null
                    property point start
                    onActiveChanged: if (active) { start = Qt.point(panel.x, panel.y); panel.movedTo = start; panel.moved = true }
                    onCentroidChanged: if (active) panel.movedTo = Qt.point(
                        panel.clampX(start.x + centroid.scenePosition.x - centroid.scenePressPosition.x),
                        panel.clampY(start.y + centroid.scenePosition.y - centroid.scenePressPosition.y))
                }
            }
            HelpToggle {
                id: helpButton
                objectName: "generatorHelpButton"
                checked: panel.showHelp
                onClicked: panel.showHelp = !panel.showHelp
            }
            PanelButton {
                id: closeButton
                width: Theme.control; height: Theme.control
                contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 18 } }
                onClicked: panel.close()
                Accessible.name: qsTr("Schließen")
            }
        }

        // Kurze Hilfe (»?«)
        HelpBox {
            objectName: "generatorHelp"
            visible: panel.showHelp
            width: parent.width
            lines: [qsTr("Der Generator erzeugt eine Wechselspannung an G. G0 ist fest mit 0V verbunden."),
                    qsTr("Er läuft nur, wenn der Hauptschalter an ist. Dann leuchtet die Lampe im Pult."),
                    qsTr("Signalform: Sinus klingt weich, Rechteck hart, Dreieck dazwischen."),
                    qsTr("Frequenz: Wähle zuerst den Bereich, dann stellst du mit dem Regler fein ein. Mehr Hertz heißt höherer Ton."),
                    qsTr("Spitze: So weit schwingt die Spannung nach oben und nach unten."),
                    qsTr("Gleichanteil: Er schiebt die ganze Kurve nach oben oder nach unten.")]
                   .concat(panel.hasAm ? [qsTr("Modulation: Ein Ton macht die Kurve abwechselnd größer und kleiner. Die Spitzen bilden dann die Form des Tons nach.")] : [])
        }

        Label { text: qsTr("Signalform"); font.bold: true; color: Theme.navy }
        Row {
            spacing: 6
            Repeater {
                model: [qsTr("Sinus"), qsTr("Dreieck"), qsTr("Rechteck")]
                PanelButton {
                    id: waveButton
                    required property string modelData
                    required property int index
                    objectName: "genWave" + index
                    height: 64
                    width: (panel.availableWidth - 12) / 3
                    checkable: true
                    checked: panel.controller && panel.controller.genWaveform === index
                    text: modelData
                    onClicked: panel.controller.genWaveform = index
                    Accessible.name: modelData
                    // Kurvenbild ueber dem Namen (eine Periode, beginnt steigend wie im Generator)
                    contentItem: Column {
                        spacing: 4
                        Shape {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 48; height: 22
                            preferredRendererType: Shape.CurveRenderer
                            ShapePath {
                                strokeColor: waveButton.checked ? "white" : Theme.navy
                                strokeWidth: 2
                                fillColor: "transparent"
                                capStyle: ShapePath.RoundCap
                                joinStyle: ShapePath.RoundJoin
                                PathSvg {
                                    path: waveButton.index === 0
                                          ? "M0 11 C6 -3 18 -3 24 11 C30 25 42 25 48 11"
                                          : waveButton.index === 1
                                          ? "M0 11 L12 1 L36 21 L48 11"
                                          : "M0 11 V1 H24 V21 H48 V11"
                                }
                            }
                        }
                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: waveButton.text
                            color: waveButton.checked ? "white" : Theme.navy
                            font.pixelSize: Math.round(13 * Theme.fontScale)
                        }
                    }
                }
            }
        }

        // Frequenz: Bereich und Feinregler gehoeren zusammen (gemeinsamer Rahmen)
        Rectangle {
            objectName: "genFrequencyFrame"
            width: parent.width
            height: frequencyColumn.implicitHeight + 20
            radius: 8
            color: "transparent"
            border.color: "#9AA9B8"
            border.width: 1.5
            Column {
                id: frequencyColumn
                x: 10; y: 10
                width: parent.width - 20
                spacing: 6
                Label {
                    text: qsTr("Frequenz: %1").arg(panel.controller ? panel.frequencyText(panel.controller.genFrequency) : "")
                    font.bold: true
                    color: Theme.navy
                }
                Row {
                    spacing: 6
                    Repeater {
                        model: [1, 10, 100, 1000]
                        PanelButton {
                            required property int modelData
                            objectName: "genDecade" + modelData
                            height: Theme.control
                            width: (frequencyColumn.width - 18) / 4
                            checkable: true
                            checked: panel.decade === modelData
                            text: (modelData >= 1000 ? qsTr("%1 kHz").arg(modelData / 1000) : qsTr("%1 Hz").arg(modelData)) + " …"
                            onClicked: panel.controller.genFrequency = modelData
                            Accessible.name: qsTr("Frequenzbereich ab %1").arg(panel.frequencyText(modelData))
                        }
                    }
                }
                Slider {
                    objectName: "genFrequencySlider"
                    width: parent.width
                    height: Theme.control
                    from: 1; to: 10; stepSize: 0.1
                    value: panel.controller ? panel.controller.genFrequency / panel.decade : 1
                    onMoved: panel.controller.genFrequency = Math.round(value * 10) / 10 * panel.decade
                    Accessible.name: qsTr("Frequenz fein")
                }
            }
        }

        Label {
            text: qsTr("Spitze: %1 V").arg(panel.controller ? panel.number(panel.controller.genAmplitude, 1) : "")
            font.bold: true
            color: Theme.navy
        }
        Slider {
            objectName: "genAmplitudeSlider"
            width: parent.width
            height: Theme.control
            from: 0; to: 4; stepSize: 0.1
            value: panel.controller ? panel.controller.genAmplitude : 0
            onMoved: panel.controller.genAmplitude = Math.round(value * 10) / 10
            Accessible.name: qsTr("Spitze")
        }

        Label {
            text: qsTr("Gleichanteil: %1 V").arg(panel.controller ? panel.number(panel.controller.genOffset, 1) : "")
            font.bold: true
            color: Theme.navy
        }
        Slider {
            objectName: "genOffsetSlider"
            width: parent.width
            height: Theme.control
            from: -4; to: 4; stepSize: 0.1
            value: panel.controller ? panel.controller.genOffset : 0
            onMoved: panel.controller.genOffset = Math.round(value * 10) / 10
            Accessible.name: qsTr("Gleichanteil")
        }

        // Amplitudenmodulation (ab Volta 3): Tiefe und Ton gehoeren zusammen (gemeinsamer Rahmen)
        Rectangle {
            objectName: "genAmFrame"
            visible: panel.hasAm
            width: parent.width
            height: amColumn.implicitHeight + 20
            radius: 8
            color: "transparent"
            border.color: "#9AA9B8"
            border.width: 1.5
            Column {
                id: amColumn
                x: 10; y: 10
                width: parent.width - 20
                spacing: 6
                Label {
                    objectName: "genAmLabel"
                    text: panel.controller && panel.controller.genAmDepth > 0
                          ? qsTr("Modulation: %1 %").arg(panel.number(panel.controller.genAmDepth * 100, 0))
                          : qsTr("Modulation: aus")
                    font.bold: true
                    color: Theme.navy
                }
                Slider {
                    objectName: "genAmDepthSlider"
                    width: parent.width
                    height: Theme.control
                    from: 0; to: 1; stepSize: 0.05
                    value: panel.controller ? panel.controller.genAmDepth : 0
                    onMoved: panel.controller.genAmDepth = Math.round(value * 20) / 20
                    Accessible.name: qsTr("Modulation")
                }
                Label {
                    text: qsTr("Ton: %1").arg(panel.controller ? panel.frequencyText(panel.controller.genAmTone) : "")
                    color: Theme.navy
                }
                Row {
                    spacing: 6
                    Repeater {
                        model: [100, 200, 500]
                        PanelButton {
                            required property int modelData
                            objectName: "genAmTone" + modelData
                            height: Theme.control
                            width: (amColumn.width - 12) / 3
                            checkable: true
                            checked: panel.controller !== null && panel.controller.genAmTone === modelData
                            text: qsTr("%1 Hz").arg(modelData)
                            onClicked: panel.controller.genAmTone = modelData
                            Accessible.name: qsTr("Ton %1 Hz").arg(modelData)
                        }
                    }
                }
            }
        }
        Rectangle {
            objectName: "genSweepFrame"
            visible: panel.hasAm
            width: parent.width
            height: sweepColumn.implicitHeight + 20
            radius: 8
            color: "transparent"
            border.color: "#9AA9B8"
            border.width: 1.5
            readonly property real endHz: panel.controller ? Math.min(panel.controller.genFrequency * panel.controller.genSweepFactor, 10000) : 0
            Column {
                id: sweepColumn
                x: 10; y: 10
                width: parent.width - 20
                spacing: 6
                Label {
                    objectName: "genSweepLabel"
                    width: parent.width
                    wrapMode: Text.WordWrap
                    text: panel.controller && panel.controller.genSweepFactor > 1
                          ? qsTr("Wobbeln: %1 bis %2 in %3 s").arg(panel.frequencyText(panel.controller.genFrequency))
                                .arg(panel.frequencyText(parent.parent.endHz)).arg(panel.number(panel.controller.genSweepTime, 0))
                          : qsTr("Wobbeln: aus")
                    font.bold: true
                    color: Theme.navy
                }
                Row {
                    spacing: 6
                    Repeater {
                        model: [1, 10, 100, 1000]
                        PanelButton {
                            required property int modelData
                            objectName: "genSweep" + modelData
                            height: Theme.control
                            width: (sweepColumn.width - 18) / 4
                            checkable: true
                            checked: panel.controller !== null && panel.controller.genSweepFactor === modelData
                            text: modelData === 1 ? qsTr("aus") : "×" + modelData
                            onClicked: panel.controller.genSweepFactor = modelData
                            Accessible.name: modelData === 1 ? qsTr("Wobbeln aus") : qsTr("Wobbeln bis zum %1-Fachen").arg(modelData)
                        }
                    }
                }
                Row {
                    spacing: 6
                    visible: panel.controller !== null && panel.controller.genSweepFactor > 1
                    Repeater {
                        model: [1, 5, 10]
                        PanelButton {
                            required property int modelData
                            objectName: "genSweepTime" + modelData
                            height: Theme.control
                            width: (sweepColumn.width - 12) / 3
                            checkable: true
                            checked: panel.controller !== null && panel.controller.genSweepTime === modelData
                            text: qsTr("%1 s").arg(modelData)
                            onClicked: panel.controller.genSweepTime = modelData
                            Accessible.name: qsTr("Durchlauf %1 s").arg(modelData)
                        }
                    }
                }
                Label {
                    width: parent.width
                    wrapMode: Text.WordWrap
                    visible: panel.controller !== null && panel.controller.genSweepFactor > 1
                    text: qsTr("Die Frequenz steigt immer wieder vom eingestellten Wert bis zum Endwert. Am Oszilloskop siehst du so, bei welcher Frequenz deine Schaltung am meisten durchlässt.")
                    color: "#4A5560"
                    font.pixelSize: Math.round(12 * Theme.fontScale)
                }
            }
        }
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            color: "#4A5560"
            text: panel.controller && panel.controller.mainSwitch
                  ? qsTr("Der Ausgang G hat 50 Ω Innenwiderstand, G0 ist fest mit 0V verbunden.")
                  : qsTr("Der Generator ist aus. Schalte den Hauptschalter ein.")
        }
    }
}
