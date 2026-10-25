import QtQuick

// Ein- und Ausgabe wie auf dem Bedienfeld der ersten Rechner-Bausaetze: Kippschalter (rastet), Taster (nur solange
// gedrueckt), rote LED. Der Zustand steht immer auch als 1/0 daneben (Farbe nie allein). Flache Form (»compact«,
// 56 dp hoch) fuer Bedienfelder mit vielen Schaltern: Bezeichnung und Wert neben dem Bedienelement.
Item {
    id: root
    property string name
    property string label // Bedeutung, z. B. »Haustuer«; sonst der Name
    property string kind: "kipp" // kipp | taster | led
    property string level: "0"
    property bool enabled: true
    property bool compact: false
    readonly property bool on: level === "1"
    readonly property bool big: !compact
    width: 124
    height: compact ? 56 : 132

    Text { // Bezeichnung oben (bis zwei Zeilen)
        visible: root.big
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.top
        anchors.bottomMargin: -36
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        maximumLineCount: 2
        text: root.label !== "" ? root.label : root.name
        color: ZTheme.text
        font.pixelSize: 15; font.bold: true
    }

    // Kippschalter: Metallplatte, Hebel oben = 1, unten = 0
    Rectangle {
        visible: root.big && root.kind === "kipp"
        anchors.horizontalCenter: parent.horizontalCenter
        y: 40
        width: 52; height: 68; radius: 10
        color: "#3A434D"
        border.color: "#8E99A4"; border.width: 2
        Rectangle { // Drehpunkt
            anchors.centerIn: parent
            width: 20; height: 20; radius: 10
            color: "#6B7580"
        }
        Rectangle { // Hebel
            anchors.horizontalCenter: parent.horizontalCenter
            width: 14; height: 30; radius: 7
            y: root.on ? 4 : parent.height - height - 4
            color: "#D5DCE3"
            border.color: "#8E99A4"
            Behavior on y { NumberAnimation { duration: 90 } }
        }
    }

    // Taster: runde Kappe, eingedrueckt solange gedrueckt
    Rectangle {
        visible: root.big && root.kind === "taster"
        anchors.horizontalCenter: parent.horizontalCenter
        y: 42
        width: 62; height: 62; radius: 31
        color: "#3A434D"
        border.color: "#8E99A4"; border.width: 2
        Rectangle {
            anchors.centerIn: parent
            width: root.on ? 40 : 48; height: width; radius: width / 2
            color: root.on ? "#B8C1CA" : "#D5DCE3"
        }
    }

    // LED: rot, leuchtet bei 1
    Item {
        visible: root.big && root.kind === "led"
        anchors.horizontalCenter: parent.horizontalCenter
        y: 42
        width: 62; height: 62
        Rectangle { // Schein
            anchors.centerIn: parent
            width: 62; height: 62; radius: 31
            color: "#FF3B30"
            opacity: root.on ? 0.3 : 0
        }
        Rectangle {
            anchors.centerIn: parent
            width: 36; height: 36; radius: 18
            color: root.on ? "#FF3B30" : "#F3C9C6"
            border.color: root.on ? "#B91C1C" : "#D49A95"; border.width: 2
        }
    }

    Text { // Wert
        visible: root.big
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        text: root.level
        color: root.on ? (root.kind === "led" ? "#B91C1C" : ZTheme.accentText) : ZTheme.muted
        font.family: ZTheme.mono; font.pixelSize: 18; font.bold: true
    }

    // ---- flache Form: Eingang = [Bezeichnung/Wert | Schalter], LED = [LED | Bezeichnung/Wert] (Pad liegt innen)
    Row {
        visible: root.compact
        anchors.verticalCenter: parent.verticalCenter
        x: root.kind === "led" ? 4 : 0
        spacing: 8
        layoutDirection: root.kind === "led" ? Qt.RightToLeft : Qt.LeftToRight
        Column {
            width: 64
            anchors.verticalCenter: parent.verticalCenter
            Text {
                width: parent.width
                horizontalAlignment: root.kind === "led" ? Text.AlignLeft : Text.AlignRight
                text: root.label !== "" ? root.label : root.name
                color: ZTheme.text
                font.pixelSize: 13; font.bold: true
                elide: Text.ElideRight
            }
            Text {
                width: parent.width
                horizontalAlignment: root.kind === "led" ? Text.AlignLeft : Text.AlignRight
                text: root.level
                color: root.on ? (root.kind === "led" ? "#B91C1C" : ZTheme.accentText) : ZTheme.muted
                font.family: ZTheme.mono; font.pixelSize: 16; font.bold: true
            }
        }
        Item {
            width: 44; height: 52
            anchors.verticalCenter: parent.verticalCenter
            Rectangle { // Kippschalter
                visible: root.kind === "kipp"
                anchors.fill: parent
                radius: 8
                color: "#3A434D"
                border.color: "#8E99A4"; border.width: 2
                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 12; height: 22; radius: 6
                    y: root.on ? 4 : parent.height - height - 4
                    color: "#D5DCE3"
                    border.color: "#8E99A4"
                    Behavior on y { NumberAnimation { duration: 90 } }
                }
            }
            Rectangle { // Taster
                visible: root.kind === "taster"
                anchors.centerIn: parent
                width: 44; height: 44; radius: 22
                color: "#3A434D"
                border.color: "#8E99A4"; border.width: 2
                Rectangle {
                    anchors.centerIn: parent
                    width: root.on ? 26 : 32; height: width; radius: width / 2
                    color: root.on ? "#B8C1CA" : "#D5DCE3"
                }
            }
            Item { // LED
                visible: root.kind === "led"
                anchors.fill: parent
                Rectangle {
                    anchors.centerIn: parent
                    width: 44; height: 44; radius: 22
                    color: "#FF3B30"
                    opacity: root.on ? 0.3 : 0
                }
                Rectangle {
                    anchors.centerIn: parent
                    width: 28; height: 28; radius: 14
                    color: root.on ? "#FF3B30" : "#F3C9C6"
                    border.color: root.on ? "#B91C1C" : "#D49A95"; border.width: 2
                }
            }
        }
    }

    TapHandler {
        enabled: root.enabled && root.kind === "kipp"
        onTapped: workbench.toggleInput(root.name)
    }
    TapHandler {
        enabled: root.enabled && root.kind === "taster"
        onPressedChanged: workbench.setInput(root.name, pressed)
    }
}
