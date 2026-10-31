// Generatorfenster (ab Volta 2, Konzept 5.2): Signalform, Frequenz in Dekaden mit Feineinstellung, Spitze und
// Gleichanteil. Alles Bedienhandlungen (kein Befehl); jeder Wert steht zusaetzlich als Zahl da.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: panel
    property var controller: null

    // Dekade der Frequenz: 1, 10, 100 oder 1000 Hz; der Regler waehlt das 1- bis 10-Fache
    readonly property int decade: controller ? (controller.genFrequency < 10 ? 1 : controller.genFrequency < 100 ? 10
                                               : controller.genFrequency < 1000 ? 100 : 1000) : 1
    function number(v, digits) { return Number(v).toLocaleString(Qt.locale(), "f", digits) }
    function frequencyText(hz) {
        return hz >= 1000 ? qsTr("%1 kHz").arg(number(hz / 1000, hz >= 10000 ? 0 : 2))
                          : qsTr("%1 Hz").arg(number(hz, hz >= 100 ? 0 : 1))
    }

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - 32 : 440, 440)
    padding: 16
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    contentItem: Column {
        spacing: 10
        Row {
            width: parent.width
            Label {
                width: parent.width - closeButton.width
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Funktionsgenerator")
                color: Theme.navy
                font.pixelSize: Math.round(18 * Theme.fontScale)
                font.bold: true
            }
            ToolButton {
                id: closeButton
                width: Theme.control; height: Theme.control
                contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: Theme.navy; size: 18 } }
                onClicked: panel.close()
                Accessible.name: qsTr("Schließen")
            }
        }

        Label { text: qsTr("Signalform"); font.bold: true; color: Theme.navy }
        Row {
            spacing: 6
            Repeater {
                model: [qsTr("Sinus"), qsTr("Dreieck"), qsTr("Rechteck")]
                Button {
                    required property string modelData
                    required property int index
                    objectName: "genWave" + index
                    height: Theme.control
                    width: (panel.availableWidth - 12) / 3
                    checkable: true
                    checked: panel.controller && panel.controller.genWaveform === index
                    text: modelData
                    onClicked: panel.controller.genWaveform = index
                }
            }
        }

        Label {
            text: qsTr("Frequenz: %1").arg(panel.controller ? panel.frequencyText(panel.controller.genFrequency) : "")
            font.bold: true
            color: Theme.navy
        }
        Row {
            spacing: 6
            Repeater {
                model: [1, 10, 100, 1000]
                Button {
                    required property int modelData
                    objectName: "genDecade" + modelData
                    height: Theme.control
                    width: (panel.availableWidth - 18) / 4
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
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            color: "#4A5560"
            text: qsTr("Der Ausgang G hat 50 Ω Innenwiderstand, G0 ist fest mit 0V verbunden. Der Generator braucht den Hauptschalter nicht.")
        }
    }
}
