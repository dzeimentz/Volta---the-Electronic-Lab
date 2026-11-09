// Zahlensysteme im Werkzeugkasten (ab Volta 3, Stufe 8, Konzept Kap. 7): eine Zahl 0 ... 15 bzw. 0 ... 255 als Bits
// zum Antippen (Wertigkeit steht ueber jedem Bit), dezimal mit - und + sowie hexadezimal. Gleiche Reihenfolge wie
// am Zaehler: rechts Q0 (1), links das hoechste Bit. Farbe nie allein: jedes Bit traegt 0 oder 1.
import QtQuick
import QtQuick.Controls.Basic

Flickable {
    id: ns
    contentHeight: col.implicitHeight + 20
    clip: true
    property int bits: 4
    property int value: 5
    readonly property int maxValue: (1 << bits) - 1

    function setValue(v) { ns.value = Math.max(0, Math.min(ns.maxValue, v)) }

    Column {
        id: col
        x: 12; y: 12
        width: ns.width - 24
        spacing: 12

        Label {
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Math.round(14 * Theme.fontScale)
            color: "#1E2B38"
            text: qsTr("Tippe auf ein Bit, um es zwischen 0 und 1 umzuschalten. Über jedem Bit steht, wie viel es zählt. Die Zahl ist die Summe der Bits mit einer 1.")
        }
        Row {
            spacing: 8
            Repeater {
                model: [4, 8]
                Button {
                    required property int modelData
                    objectName: "numberBits" + modelData
                    height: Theme.control
                    width: 96
                    text: qsTr("%1 Bit").arg(modelData)
                    font.bold: ns.bits === modelData
                    highlighted: ns.bits === modelData
                    onClicked: { ns.bits = modelData; ns.setValue(ns.value) }
                }
            }
        }
        Row {
            spacing: 4
            Repeater {
                model: ns.bits
                Column {
                    required property int index
                    readonly property int weight: 1 << (ns.bits - 1 - index)
                    readonly property bool on: (ns.value & weight) !== 0
                    spacing: 2
                    Label {
                        width: 44
                        horizontalAlignment: Text.AlignHCenter
                        text: parent.weight
                        font.pixelSize: Math.round(11 * Theme.fontScale)
                        color: "#4A5866"
                    }
                    Button {
                        objectName: "numberBit" + parent.weight
                        width: 44; height: Theme.control
                        text: parent.on ? "1" : "0"
                        font.pixelSize: Math.round(20 * Theme.fontScale)
                        font.bold: true
                        palette.buttonText: parent.on ? "#1E2B38" : "#4A5866"
                        background: Rectangle {
                            radius: 6
                            color: parent.parent.on ? "#FFC83D" : "white"
                            border.color: parent.parent.on ? Theme.solderAmber : Theme.fieldOutline
                            border.width: parent.parent.on ? 3 : 1
                        }
                        onClicked: ns.value = ns.value ^ parent.weight
                        Accessible.name: qsTr("Bit %1: %2").arg(parent.weight).arg(parent.on ? 1 : 0)
                    }
                }
            }
        }
        Row {
            spacing: 8
            Label {
                anchors.verticalCenter: parent.verticalCenter
                width: 110
                text: qsTr("dezimal")
                font.pixelSize: Math.round(14 * Theme.fontScale)
                color: Theme.navy
                font.bold: true
            }
            Button {
                objectName: "numberMinus"
                width: Theme.control; height: Theme.control
                text: "−"
                font.pixelSize: 20
                onClicked: ns.setValue(ns.value - 1)
                Accessible.name: qsTr("eins weniger")
            }
            Label {
                objectName: "numberDecimal"
                anchors.verticalCenter: parent.verticalCenter
                width: 64
                horizontalAlignment: Text.AlignHCenter
                text: ns.value
                font.family: "Consolas"
                font.pixelSize: Math.round(26 * Theme.fontScale)
                font.bold: true
                color: "#1E2B38"
            }
            Button {
                objectName: "numberPlus"
                width: Theme.control; height: Theme.control
                text: "+"
                font.pixelSize: 20
                onClicked: ns.setValue(ns.value + 1)
                Accessible.name: qsTr("eins mehr")
            }
        }
        Row {
            spacing: 8
            Label {
                width: 110
                text: qsTr("dual")
                font.pixelSize: Math.round(14 * Theme.fontScale)
                color: Theme.navy
                font.bold: true
            }
            Label {
                objectName: "numberBinary"
                text: (ns.value >>> 0).toString(2).padStart(ns.bits, "0")
                font.family: "Consolas"
                font.pixelSize: Math.round(18 * Theme.fontScale)
                color: "#1E2B38"
            }
        }
        Row {
            spacing: 8
            Label {
                width: 110
                text: qsTr("hexadezimal")
                font.pixelSize: Math.round(14 * Theme.fontScale)
                color: Theme.navy
                font.bold: true
            }
            Label {
                objectName: "numberHex"
                text: ns.value.toString(16).toUpperCase().padStart(ns.bits / 4, "0")
                font.family: "Consolas"
                font.pixelSize: Math.round(18 * Theme.fontScale)
                color: "#1E2B38"
            }
        }
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            font.pixelSize: Math.round(13 * Theme.fontScale)
            color: "#4A5866"
            text: qsTr("Ein Bit ist eine Stelle mit 0 oder 1. Vier Bits reichen bis 15, acht Bits – ein Byte – bis 255. Hexadezimal fasst je vier Bits zu einer Ziffer 0 … 9, A … F zusammen.")
        }
    }
}
