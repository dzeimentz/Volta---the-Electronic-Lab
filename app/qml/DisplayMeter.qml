// Anzeigegeraet ab Volta 2 (Konzept 5.2, 9.3): zeigt Messstellen der Grundplatte ohne Messleitungen, einfach,
// doppelt oder dreifach. Je Anzeige: Wahl der Messstelle (nur gesteckte), Handbereichswahl mit Ueberlauf »1« oder selbsttaetig,
// Gleich-/Wechselgroesse (Effektivwert, »V~« / »A~«). Ab zwei Anzeigen rechnet es auf Wunsch die Leistung aus V und A.
// Rahmen und Kennung jeder Anzeige in der Farbe ihrer Messstelle (1 blau, 2 orange, 3 violett), Kennung immer dabei.
// Zwei Plaetze wie das Messgeraet von Volta 1: neben der Grundplatte (Anzeigen untereinander) oder oben im Pult
// (nebeneinander). Alles Bedienhandlungen; die Werte rechnet der LabController.
import QtQuick
import QtQuick.Controls.Basic

Rectangle {
    id: meter
    required property var controller
    property bool docked: false
    signal dockToggled()

    readonly property int count: controller.displayCount
    property var states: [({}), ({}), ({})]
    property var power: ({})

    radius: 12
    color: "#2B3440"
    border.color: "#151A20"
    border.width: 2

    function refresh() {
        meter.states = [meter.controller.displayState(0), meter.controller.displayState(1), meter.controller.displayState(2)]
        meter.power = meter.controller.displayPowerState()
    }
    // Ziffern mit 3 Hz wie ein echtes Geraet; Aenderungen der Einstellung sofort
    Timer { interval: 333; repeat: true; running: meter.visible; triggeredOnStart: true; onTriggered: meter.refresh() }
    Connections {
        target: meter.controller
        function onDeskChanged() { meter.refresh() }
        function onBoardChanged() { meter.refresh() }
    }
    function tagColor(tag) { return tag > 0 ? Theme.measureTagColors[(tag - 1) % Theme.measureTagColors.length] : "#8E9AA8" }

    // Platz wechseln (Bild wie beim Messgeraet: Balken oben = ins Pult, Balken rechts = neben die Grundplatte)
    ToolButton {
        id: dockButton
        objectName: "displayDockButton"
        x: 0; y: 0
        width: Theme.control; height: Theme.control
        onClicked: meter.dockToggled()
        ToolTip.visible: hovered
        ToolTip.text: meter.docked ? qsTr("Messgerät neben die Grundplatte") : qsTr("Messgerät oben ins Pult")
        Accessible.name: ToolTip.text
        background: Rectangle { anchors.fill: parent; anchors.margins: 6; radius: 6; color: dockButton.down ? "#4A5868" : (dockButton.hovered ? "#3A4655" : "transparent") }
        contentItem: Item {
            Rectangle {
                anchors.centerIn: parent
                width: 22; height: 16; radius: 2
                color: "transparent"
                border.color: "#C9D3DD"
                border.width: 1.5
                Rectangle {
                    color: Theme.solderAmber
                    x: meter.docked ? parent.width - width - 2 : 2
                    y: 2
                    width: meter.docked ? 6 : parent.width - 4
                    height: meter.docked ? parent.height - 4 : 5
                }
            }
        }
    }

    // Zahl der Anzeigen 1 -> 2 -> 3 -> 1 (Ziffer im Knopf)
    ToolButton {
        id: twinButton
        objectName: "displayCountButton"
        x: meter.docked ? 0 : parent.width - width
        y: meter.docked ? Theme.control : 0
        width: Theme.control; height: Theme.control
        onClicked: meter.controller.displayCount = meter.count % 3 + 1
        ToolTip.visible: hovered
        ToolTip.text: meter.count === 1 ? qsTr("Doppelmessgerät") : meter.count === 2 ? qsTr("Dreifachmessgerät")
                                        : qsTr("Einfachmessgerät")
        Accessible.name: ToolTip.text
        background: Rectangle { anchors.fill: parent; anchors.margins: 6; radius: 6; color: twinButton.down ? "#4A5868" : (twinButton.hovered ? "#3A4655" : "transparent"); border.color: "#C9D3DD" }
        contentItem: Label {
            text: meter.count
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            color: "white"; font.bold: true; font.pixelSize: Math.round(15 * Theme.fontScale)
        }
    }

    Label {
        id: title
        visible: !meter.docked
        x: Theme.control
        width: parent.width - 2 * Theme.control
        height: Theme.control
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        fontSizeMode: Text.HorizontalFit
        minimumPixelSize: 9
        text: meter.count === 3 ? qsTr("Dreifachmessgerät") : meter.count === 2 ? qsTr("Doppelmessgerät") : qsTr("Messgerät")
        color: "white"
        font.pixelSize: Math.round(14 * Theme.fontScale)
        font.bold: true
    }

    // Anzeigen: neben der Grundplatte untereinander, im Pult nebeneinander
    Grid {
        id: units
        x: meter.docked ? Theme.control + 2 : 10
        y: meter.docked ? 6 : Theme.control + 4
        columns: meter.docked ? 3 : 1
        spacing: meter.docked || meter.count < 3 ? 8 : 6
        readonly property real unitWidth: meter.docked ? (meter.width - x - 8 - (meter.count - 1) * spacing) / meter.count
                                                       : meter.width - 20
        Repeater {
            model: meter.count
            // Eine Anzeige mit Wahl der Messstelle, Bereich und Gleich-/Wechselgroesse
            Column {
                id: unit
                required property int index
                readonly property int slot: index
                width: units.unitWidth
                readonly property var st: meter.states[slot] || ({})
                readonly property bool valid: st.valid === true
                readonly property color tint: meter.tagColor(st.tag || 0)
                spacing: 4

                Rectangle {
                    id: lcd
                    objectName: "displayLcd" + unit.slot
                    width: unit.width
                    height: meter.docked || meter.count === 3 ? 56 : 70
                    radius: 6
                    color: "#B9CBA4"
                    border.color: unit.valid ? unit.tint : "#10151B"
                    border.width: 3
                    Rectangle { // Kennung der Messstelle in ihrer Farbe (Buchstabe und Nummer stehen immer dabei)
                        visible: unit.valid
                        x: 5; y: 5
                        width: tagText.implicitWidth + 10; height: tagText.implicitHeight + 2; radius: 4
                        color: unit.tint
                        Text { id: tagText; anchors.centerIn: parent; text: unit.st.label || ""; color: "white"; font.bold: true; font.pixelSize: Math.round(12 * Theme.fontScale) }
                    }
                    Label {
                        visible: unit.valid && (unit.st.ranges || []).length > 0
                        anchors.right: parent.right; anchors.rightMargin: 6; y: 4
                        text: unit.st.range >= 0 ? unit.st.ranges[unit.st.range] : qsTr("Auto")
                        font.pixelSize: Math.round(10 * Theme.fontScale)
                        color: "#33402C"
                    }
                    Row {
                        visible: unit.valid
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom; anchors.bottomMargin: 4
                        spacing: 4
                        Label {
                            objectName: "displayDigits" + unit.slot
                            text: unit.st.text || ""
                            font.family: "Consolas"
                            font.pixelSize: Math.round((meter.docked || meter.count === 3 ? 24 : 30) * Theme.fontScale)
                            font.bold: true
                            color: "#18221A"
                        }
                        Label {
                            anchors.baseline: parent.children[0].baseline
                            text: unit.st.unit || ""
                            font.pixelSize: Math.round(15 * Theme.fontScale)
                            font.bold: true
                            color: "#18221A"
                        }
                    }
                    Label { // ohne Messstelle: was zu tun ist
                        visible: !unit.valid
                        anchors.fill: parent; anchors.margins: 6
                        horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                        wrapMode: Text.Wrap
                        text: qsTr("Stecke eine Messstelle V, A oder Ω auf die Grundplatte.")
                        font.pixelSize: Math.round(11 * Theme.fontScale)
                        color: "#33402C"
                    }
                    Accessible.role: Accessible.StaticText
                    Accessible.name: !unit.valid ? qsTr("keine Messstelle")
                        : (unit.st.label || "") + ", " + (unit.st.foreignVoltage ? qsTr("Spannung abschalten!")
                          : unit.st.overrange ? qsTr("Messbereich überschritten")
                          : (unit.st.text || "") + " " + (unit.st.unit || ""))
                }

                Row {
                    visible: unit.valid
                    spacing: 4
                    readonly property real cell: (unit.width - 8) / 3
                    Button { // Messstelle waehlen
                        id: sourceButton
                        objectName: "displaySource" + unit.slot
                        width: parent.cell; height: Theme.control
                        text: (unit.st.label || "") + " ▾"
                        font.bold: true
                        onClicked: sourceMenu.open()
                        Accessible.name: qsTr("Messstelle wählen, jetzt %1").arg(unit.st.label || "")
                        Menu {
                            id: sourceMenu
                            y: sourceButton.height
                            Repeater {
                                model: meter.controller.measurePoints()
                                MenuItem {
                                    required property var modelData
                                    height: Theme.control
                                    text: modelData.label
                                    font.bold: modelData.id === unit.st.id
                                    onTriggered: meter.controller.setDisplaySource(unit.slot, modelData.id)
                                }
                            }
                        }
                    }
                    Button { // Bereich waehlen
                        id: rangeButton
                        objectName: "displayRange" + unit.slot
                        width: parent.cell; height: Theme.control
                        text: unit.st.range >= 0 ? (unit.st.ranges || [])[unit.st.range] : qsTr("Auto")
                        font.pixelSize: Math.round(11 * Theme.fontScale)
                        leftPadding: 2; rightPadding: 2
                        onClicked: rangeMenu.open()
                        Accessible.name: qsTr("Messbereich wählen")
                        Menu {
                            id: rangeMenu
                            y: rangeButton.height
                            MenuItem {
                                height: Theme.control
                                text: qsTr("selbsttätig")
                                font.bold: unit.st.range === -1
                                onTriggered: meter.controller.setDisplayRange(unit.slot, -1)
                            }
                            Repeater {
                                model: unit.st.ranges || []
                                MenuItem {
                                    required property int index
                                    required property string modelData
                                    height: Theme.control
                                    text: modelData
                                    font.bold: unit.st.range === index
                                    onTriggered: meter.controller.setDisplayRange(unit.slot, index)
                                }
                            }
                        }
                    }
                    Button { // Gleich- oder Wechselgroesse (nicht bei Ohm)
                        objectName: "displayAc" + unit.slot
                        width: parent.cell; height: Theme.control
                        enabled: unit.st.kind !== 2
                        text: unit.st.ac ? "~" : "⎓"
                        font.pixelSize: Math.round(18 * Theme.fontScale)
                        font.bold: true
                        onClicked: meter.controller.setDisplayAc(unit.slot, !unit.st.ac)
                        ToolTip.visible: hovered
                        ToolTip.text: unit.st.ac ? qsTr("Wechselgröße (Effektivwert)") : qsTr("Gleichgröße")
                        Accessible.name: ToolTip.text
                    }
                }
            }
        }
    }

    // Leistung aus V und A (ab zwei Anzeigen); Knopf »P« schaltet sie zu
    ToolButton {
        id: powerButton
        objectName: "displayPowerButton"
        visible: meter.count > 1
        x: units.x
        y: units.y + units.height + 4
        width: Theme.control; height: Theme.control
        checkable: true
        checked: meter.controller.displayPower
        onClicked: meter.controller.displayPower = checked
        ToolTip.visible: hovered
        ToolTip.text: qsTr("Leistung aus Spannung und Strom")
        Accessible.name: ToolTip.text
        background: Rectangle { anchors.fill: parent; anchors.margins: 6; radius: 6; color: powerButton.checked ? Theme.solderAmber : "transparent"; border.color: "#C9D3DD" }
        contentItem: Label {
            text: "P"
            horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            color: powerButton.checked ? "#1E2B38" : "white"; font.bold: true; font.pixelSize: Math.round(15 * Theme.fontScale)
        }
    }
    Label {
        objectName: "displayPowerText"
        visible: meter.count > 1 && meter.power.shown === true
        x: powerButton.x + powerButton.width + 4
        y: powerButton.y
        width: meter.width - x - 10
        height: Theme.control
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.Wrap
        text: meter.power.valid ? qsTr("P = %1 %2").arg(meter.power.text).arg(meter.power.unit)
            : meter.power.needsDc ? qsTr("Leistung nur bei Gleichgrößen")
            : qsTr("Leistung: eine V- und eine A-Anzeige wählen")
        color: "white"
        font.pixelSize: Math.round((meter.power.valid ? 16 : 11) * Theme.fontScale)
        font.bold: meter.power.valid === true
    }
}
