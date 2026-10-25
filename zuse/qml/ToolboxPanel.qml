import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Werkzeugkiste wie bei Volta: Seitenleiste vom rechten Rand mit Reitern – eigene Werkzeuge (entdeckte Regeln und
// eingepackte Erfindungen, Wahl der Schaltzeichen), Lexikon (waechst mit den geschafften Experimenten, verraet keine
// Raetselkiste vorab), Zahlenrechner (binaer, dezimal, hexadezimal), Fehlersuche und Teileliste des Aufbaus.
Drawer {
    id: box
    signal partSelected(string partId) // Fehlersuche: Baustein auf der Werkbank hervorheben

    edge: Qt.RightEdge
    width: Math.min(460, parent ? parent.width : 460)
    height: parent ? parent.height : 600
    modal: false
    interactive: false // nur ueber den Knopf, kein Wischen vom Rand
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { color: "#FFFDF7"; border.color: ZTheme.layerEdge }

    function showTab(index) { tabs.currentIndex = index; open() }
    function has(kuerzel) { return kuerzel === "" || workbench.doneStages.indexOf(kuerzel) >= 0 }

    component Heading: Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        font.pixelSize: Math.round(17 * ZTheme.fontScale); font.bold: true
        color: ZTheme.navy
    }
    component Body: Text {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        font.pixelSize: Math.round(14 * ZTheme.fontScale)
        color: ZTheme.text
    }
    component Tab: TabButton { // hell wie bei Volta, gewaehlter Reiter: Akzentflaeche und fett (nicht nur Farbe)
        id: tab
        height: ZTheme.control
        width: implicitWidth
        leftPadding: 10; rightPadding: 10
        font.pixelSize: Math.round(14 * ZTheme.fontScale)
        font.bold: checked
        contentItem: Text {
            text: tab.text
            font: tab.font
            color: tab.checked ? ZTheme.textOnAccent : ZTheme.navy
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: 6
            color: tab.checked ? ZTheme.accentText : (tab.hovered ? "#E0F2F7" : "#EEF4FA")
            border.color: ZTheme.accentDim
        }
    }
    component Page: Flickable {
        id: page
        default property alias content: col.data
        clip: true
        contentWidth: width
        contentHeight: col.implicitHeight + 8
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {}
        ColumnLayout {
            id: col
            width: page.width - 14
            spacing: 10
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            Image {
                Layout.preferredWidth: 28; Layout.preferredHeight: 28
                sourceSize: Qt.size(64, 64)
                source: "qrc:/zuse/icons/toolbox_dark.svg"
            }
            Text {
                Layout.fillWidth: true
                leftPadding: 8
                text: qsTr("Werkzeugkiste")
                font.pixelSize: Math.round(20 * ZTheme.fontScale); font.bold: true
                color: ZTheme.navy
            }
            ToolButton {
                implicitWidth: ZTheme.control; implicitHeight: ZTheme.control
                text: "✕"
                font.pixelSize: 20
                onClicked: box.close()
                Accessible.name: qsTr("Schließen")
                ToolTip.visible: hovered; ToolTip.text: qsTr("Schließen")
            }
        }

        TabBar {
            id: tabs
            objectName: "toolboxTabs"
            Layout.fillWidth: true
            clip: true
            spacing: 4
            background: Item {}
            Tab { text: qsTr("Werkzeuge") }
            Tab { text: qsTr("Lexikon") }
            Tab { text: qsTr("Zahlen") }
            Tab { text: qsTr("Fehlersuche") }
            Tab { text: qsTr("Teileliste") }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabs.currentIndex

            // ---- Werkzeuge
            Page {
                Body {
                    visible: workbench.tools.length === 0
                    color: ZTheme.muted
                    text: qsTr("Noch leer. Jede Regel, die du in einer Rätselkiste entdeckst, und jede Erfindung, die "
                               + "du einpackst, landet hier – und du kannst sie danach überall einbauen.")
                }
                Repeater {
                    model: workbench.tools
                    delegate: Rectangle {
                        id: card
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.max(84, cardRow.implicitHeight + 20)
                        radius: 14
                        color: modelData.invention ? "#FFF7E6" : "#ECFAFD"
                        border.color: modelData.invention ? ZTheme.pad : ZTheme.accentDim
                        RowLayout {
                            id: cardRow
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 12
                            GateShape {
                                Layout.preferredWidth: 72; Layout.preferredHeight: 60
                                symbol: card.modelData.symbol
                                inverted: card.modelData.inverted
                                us: workbench.symbolStyle === "us"
                                stroke: ZTheme.navy
                                fontSize: card.modelData.symbol.length > 2 ? 20 : 26
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Heading { text: (card.modelData.invention ? "★ " : "") + card.modelData.name }
                                Body { text: card.modelData.merksatz }
                            }
                        }
                    }
                }
                Heading { Layout.topMargin: 6; text: qsTr("Schaltzeichen") }
                RowLayout {
                    spacing: 8
                    ZButton {
                        objectName: "styleDin"
                        text: "DIN"
                        primary: workbench.symbolStyle === "din"
                        onClicked: workbench.symbolStyle = "din"
                    }
                    ZButton {
                        objectName: "styleUs"
                        text: "US"
                        primary: workbench.symbolStyle === "us"
                        onClicked: workbench.symbolStyle = "us"
                    }
                }
                Body { color: ZTheme.muted; text: qsTr("DIN EN 60617 (Europa) oder IEEE Std 91 (USA)") }
            }

            // ---- Lexikon: Eintraege erscheinen mit dem Experiment, in dem man sie kennenlernt
            Page {
                Body {
                    color: ZTheme.muted
                    text: qsTr("Hier sammeln sich die Begriffe, die du kennenlernst. Mit jedem geschafften "
                               + "Experiment kommen neue dazu.")
                }
                Repeater {
                    model: [
                        { need: "", title: qsTr("Bit"), symbol: "",
                          text: qsTr("Die kleinste Information: 0 oder 1, aus oder an. Auf der Werkbank zeigt jede "
                                     + "Leitung ihr Bit: Cyan und dick für 1, grau für 0.") },
                        { need: "", title: qsTr("Offener Eingang »?«"), symbol: "",
                          text: qsTr("Hängt an einem Eingang keine Leitung, ist er weder 0 noch 1. Zuse 1 zeigt dann "
                                     + "»?« – echte Bausteine verhalten sich an offenen Eingängen unberechenbar.") },
                        { need: "NICHT", title: qsTr("NICHT"), symbol: "1", inv: true, table: ["0 → 1", "1 → 0"],
                          text: qsTr("Dreht ein Bit um. Beispiel: Das Licht im Kühlschrank ist an, wenn die Tür NICHT "
                                     + "zu ist.") },
                        { need: "UND", title: qsTr("UND"), symbol: "&", table: ["0 0 → 0", "0 1 → 0", "1 0 → 0", "1 1 → 1"],
                          text: qsTr("Nur 1, wenn alle Eingänge 1 sind. Beispiel: Die Maschine läuft nur, wenn beide "
                                     + "Hände auf den Tasten liegen.") },
                        { need: "ODER", title: qsTr("ODER"), symbol: "≥1", table: ["0 0 → 0", "0 1 → 1", "1 0 → 1", "1 1 → 1"],
                          text: qsTr("1, sobald mindestens ein Eingang 1 ist. Beispiel: Die Klingel läutet an der "
                                     + "Haustür oder an der Gartentür.") },
                        { need: "NAND", title: qsTr("NAND"), symbol: "&", inv: true,
                          table: ["0 0 → 1", "0 1 → 1", "1 0 → 1", "1 1 → 0"],
                          text: qsTr("NICHT-UND: nur 0, wenn alle Eingänge 1 sind. Aus NAND allein lässt sich jede "
                                     + "andere Schaltung bauen.") },
                        { need: "XODER", title: qsTr("XODER"), symbol: "=1",
                          table: ["0 0 → 0", "0 1 → 1", "1 0 → 1", "1 1 → 0"],
                          text: qsTr("Entweder-oder: 1, wenn die Eingänge verschieden sind. Beispiel: Ein Licht mit "
                                     + "zwei Schaltern im Treppenhaus.") },
                        { need: "ZAEHLEN", title: qsTr("Binärzahl"), symbol: "",
                          text: qsTr("Jedes Bit hat einen Wert: 1, 2, 4, 8 … – von rechts nach links verdoppelt. "
                                     + "1011 heißt 8 + 2 + 1 = 11.") },
                        { need: "ZAEHLEN", title: qsTr("Hexadezimal"), symbol: "",
                          text: qsTr("Vier Bits zusammen ergeben eine Ziffer von 0 bis F (15). So schreibt man lange "
                                     + "Bitketten kurz: 1111 1010 = FA.") },
                        { need: "HA", title: qsTr("Übertrag"), symbol: "",
                          text: qsTr("1 + 1 ergibt binär 10: Die Stelle zeigt 0, die 1 wandert als Übertrag in die "
                                     + "nächste Stelle – wie beim schriftlichen Rechnen.") },
                        { need: "HA", title: qsTr("Halbaddierer"), symbol: "",
                          text: qsTr("Addiert zwei Bits: XODER liefert die Summe, UND den Übertrag.") },
                        { need: "VA", title: qsTr("Volladdierer"), symbol: "",
                          text: qsTr("Addiert zwei Bits und den Übertrag von der Stelle davor. Mehrere hintereinander "
                                     + "addieren ganze Zahlen.") },
                        { need: "MERKER", title: qsTr("Rückkopplung und Speicher"), symbol: "",
                          text: qsTr("Führt ein Ausgang auf den eigenen Eingang zurück, kann sich die Schaltung etwas "
                                     + "merken. So entsteht ein Speicher für ein Bit.") },
                        { need: "TBIT", title: qsTr("Takt"), symbol: "",
                          text: qsTr("Ein gleichmäßiges An und Aus, das alle Speicher gemeinsam weiterschaltet. So "
                                     + "passiert alles Schritt für Schritt in der richtigen Reihenfolge.") },
                        { need: "REG4", title: qsTr("Register"), symbol: "",
                          text: qsTr("Mehrere Speicherbits nebeneinander, die eine ganze Zahl auf einmal merken.") },
                        { need: "ZAEHLER", title: qsTr("Zähler"), symbol: "",
                          text: qsTr("Speicherbits, die bei jedem Takt um eins weiterzählen – binär: 0000, 0001, 0010 …") },
                        { need: "RW", title: qsTr("Rechenwerk"), symbol: "",
                          text: qsTr("Der Teil eines Computers, der rechnet: Addierer und Register zusammen.") },
                        { need: "STEUER", title: qsTr("Steuerung"), symbol: "",
                          text: qsTr("Sagt dem Rechenwerk, wann es rechnen und wann es sich das Ergebnis merken soll. "
                                     + "Bei dir sind das noch deine Hände – später übernimmt das ein Programm.") },
                        { need: "", title: qsTr("Konrad Zuse"), symbol: "",
                          text: qsTr("Baute ab 1936 in Berlin Rechenmaschinen, die mit Bits rechneten. Seine Z3 "
                                     + "(1941) gilt als erster funktionsfähiger frei programmierbarer Computer.") }
                    ]
                    delegate: Rectangle {
                        id: entry
                        required property var modelData
                        readonly property bool open: box.has(modelData.need)
                        Layout.fillWidth: true
                        Layout.preferredHeight: entryCol.implicitHeight + 20
                        radius: 12
                        color: open ? ZTheme.surface : ZTheme.surfaceHigh
                        border.color: ZTheme.line
                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 12
                            GateShape {
                                visible: entry.open && entry.modelData.symbol !== ""
                                Layout.preferredWidth: 60; Layout.preferredHeight: 50
                                Layout.alignment: Qt.AlignTop
                                symbol: entry.modelData.symbol
                                inverted: !!entry.modelData.inv
                                us: workbench.symbolStyle === "us"
                                stroke: ZTheme.navy
                                fontSize: 22
                            }
                            ColumnLayout {
                                id: entryCol
                                Layout.fillWidth: true
                                spacing: 4
                                Heading { text: entry.open ? entry.modelData.title : "🔒 " + qsTr("Noch nicht entdeckt") }
                                Body {
                                    visible: entry.open
                                    text: entry.modelData.text
                                }
                                Text {
                                    visible: entry.open && !!entry.modelData.table
                                    text: entry.modelData.table ? entry.modelData.table.join("    ") : ""
                                    font.family: ZTheme.mono
                                    font.pixelSize: Math.round(14 * ZTheme.fontScale)
                                    color: ZTheme.accentText
                                }
                            }
                        }
                    }
                }
            }

            // ---- Zahlenrechner: 8 Bits antippen, Wert dezimal und hexadezimal
            Page {
                id: calc
                property int value: 0
                Body { text: qsTr("Tippe die Bits an. Jedes Bit hat seinen Wert – zusammen ergeben sie die Zahl.") }
                GridLayout {
                    Layout.alignment: Qt.AlignHCenter
                    columns: 8
                    columnSpacing: 4
                    rowSpacing: 2
                    Repeater {
                        model: 8
                        delegate: Text {
                            required property int index
                            Layout.preferredWidth: 48
                            horizontalAlignment: Text.AlignHCenter
                            text: 1 << (7 - index)
                            color: ZTheme.muted
                            font.pixelSize: Math.round(13 * ZTheme.fontScale)
                        }
                    }
                    Repeater {
                        model: 8
                        delegate: Button {
                            id: bitButton
                            required property int index
                            objectName: "bit" + (7 - index)
                            readonly property int mask: 1 << (7 - index)
                            readonly property bool on: (calc.value & mask) !== 0
                            Layout.preferredWidth: 48; Layout.preferredHeight: 56
                            onClicked: calc.value = calc.value ^ mask
                            Accessible.name: qsTr("Bit mit Wert %1: %2").arg(mask).arg(on ? 1 : 0)
                            contentItem: Text {
                                text: bitButton.on ? "1" : "0"
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                                font.family: ZTheme.mono; font.bold: true
                                font.pixelSize: Math.round(24 * ZTheme.fontScale)
                                color: bitButton.on ? ZTheme.textOnAccent : ZTheme.text
                            }
                            background: Rectangle {
                                radius: 8
                                color: bitButton.on ? ZTheme.accent : ZTheme.surface
                                border.width: bitButton.on ? 3 : 1
                                border.color: bitButton.on ? ZTheme.accentText : ZTheme.holeStrong
                            }
                        }
                    }
                }
                GridLayout {
                    Layout.alignment: Qt.AlignHCenter
                    columns: 2
                    columnSpacing: 16
                    rowSpacing: 6
                    Text { text: qsTr("binär"); color: ZTheme.muted; font.pixelSize: Math.round(15 * ZTheme.fontScale) }
                    Text {
                        objectName: "calcBin"
                        text: ("00000000" + calc.value.toString(2)).slice(-8).replace(/(.{4})/, "$1 ")
                        font.family: ZTheme.mono; font.pixelSize: Math.round(22 * ZTheme.fontScale); color: ZTheme.text
                    }
                    Text { text: qsTr("dezimal"); color: ZTheme.muted; font.pixelSize: Math.round(15 * ZTheme.fontScale) }
                    Text {
                        objectName: "calcDec"
                        text: calc.value
                        font.family: ZTheme.mono; font.pixelSize: Math.round(22 * ZTheme.fontScale); color: ZTheme.text
                    }
                    Text { text: qsTr("hexadezimal"); color: ZTheme.muted; font.pixelSize: Math.round(15 * ZTheme.fontScale) }
                    Text {
                        objectName: "calcHex"
                        text: ("0" + calc.value.toString(16).toUpperCase()).slice(-2)
                        font.family: ZTheme.mono; font.pixelSize: Math.round(22 * ZTheme.fontScale); color: ZTheme.text
                    }
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 8
                    ZButton { text: "− 1"; onClicked: calc.value = (calc.value + 255) % 256 }
                    ZButton { text: "+ 1"; onClicked: calc.value = (calc.value + 1) % 256 }
                    ZButton { text: qsTr("Null"); onClicked: calc.value = 0 }
                }
                Body {
                    color: ZTheme.muted
                    text: qsTr("Mit 8 Bits (einem Byte) kommst du von 0 bis 255. Nach 255 fängt der Zähler wieder bei "
                               + "0 an – das Bit für 256 fehlt.")
                }
            }

            // ---- Fehlersuche
            Page {
                Body {
                    text: qsTr("Hier steht, was an deinem Aufbau auffällt. Tippe einen Hinweis an, dann leuchtet der "
                               + "Baustein auf der Werkbank auf.")
                }
                Rectangle {
                    visible: workbench.faultFindings.length === 0
                    Layout.fillWidth: true
                    Layout.preferredHeight: okText.implicitHeight + 20
                    radius: 12
                    color: ZTheme.okBg
                    Text {
                        id: okText
                        anchors.fill: parent; anchors.margins: 10
                        wrapMode: Text.WordWrap
                        text: "✓ " + qsTr("Keine Auffälligkeiten: Alle Eingänge haben eine Leitung, und jeder Ausgang "
                                          + "führt irgendwohin. Ob die Schaltung das Richtige tut, zeigt »Prüfen«.")
                        color: ZTheme.ok
                        font.pixelSize: Math.round(14 * ZTheme.fontScale)
                    }
                }
                Repeater {
                    model: workbench.faultFindings
                    delegate: Button {
                        id: finding
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: Math.max(ZTheme.control, findingText.implicitHeight + 20)
                        enabled: modelData.part !== ""
                        onClicked: box.partSelected(modelData.part)
                        contentItem: Text {
                            id: findingText
                            wrapMode: Text.WordWrap
                            text: "⚠ " + finding.modelData.text
                            color: ZTheme.text
                            font.pixelSize: Math.round(14 * ZTheme.fontScale)
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            radius: 12
                            color: finding.down ? "#FDE68A" : "#FEF3C7"
                            border.color: "#D97706"
                        }
                    }
                }
            }

            // ---- Teileliste
            Page {
                Body {
                    visible: workbench.partsList.length === 0
                    color: ZTheme.muted
                    text: qsTr("Auf der Werkbank liegt noch kein Baustein.")
                }
                Repeater {
                    model: workbench.partsList
                    delegate: RowLayout {
                        id: row
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 12
                        GateShape {
                            Layout.preferredWidth: 56; Layout.preferredHeight: 44
                            symbol: row.modelData.symbol
                            inverted: row.modelData.inverted
                            us: workbench.symbolStyle === "us"
                            stroke: ZTheme.navy
                            fill: row.modelData.own ? "#FFF7E6" : ZTheme.surface
                            fontSize: row.modelData.symbol.length > 2 ? 14 : 20
                        }
                        Text {
                            Layout.fillWidth: true
                            text: (row.modelData.own ? "★ " : "") + row.modelData.name
                            color: ZTheme.text
                            font.pixelSize: Math.round(16 * ZTheme.fontScale); font.bold: true
                        }
                        Text {
                            text: "× " + row.modelData.count
                            color: ZTheme.navy
                            font.family: ZTheme.mono
                            font.pixelSize: Math.round(18 * ZTheme.fontScale); font.bold: true
                        }
                    }
                }
                Body {
                    visible: workbench.partsList.length > 0
                    color: ZTheme.muted
                    text: {
                        let n = 0
                        for (const p of workbench.partsList) n += p.count
                        return qsTr("Bausteine zusammen: %1").arg(n)
                    }
                }
            }
        }
    }
}
