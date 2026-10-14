import QtQuick

// Ein- und Ausgabe wie auf dem Bedienfeld der ersten Rechner-Bausaetze: Kippschalter (rastet), Taster (nur solange
// gedrueckt), rote LED. Der Zustand steht immer auch als 1/0 daneben (Farbe nie allein).
Item {
    id: root
    property string name
    property string kind: "kipp" // kipp | taster | led
    property string level: "0"
    property bool enabled: true
    readonly property bool on: level === "1"
    width: 84
    height: 120

    Text { // Bezeichnung oben
        anchors.horizontalCenter: parent.horizontalCenter
        y: 0
        text: root.name
        color: ZTheme.text
        font.family: ZTheme.mono; font.pixelSize: 18; font.bold: true
    }

    // Kippschalter: Metallplatte, Hebel oben = 1, unten = 0
    Rectangle {
        visible: root.kind === "kipp"
        anchors.horizontalCenter: parent.horizontalCenter
        y: 26
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
        visible: root.kind === "taster"
        anchors.horizontalCenter: parent.horizontalCenter
        y: 28
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
        visible: root.kind === "led"
        anchors.horizontalCenter: parent.horizontalCenter
        y: 28
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
            color: root.on ? "#FF453A" : "#4A1A18"
            border.color: root.on ? "#FFB3AD" : "#7A2E2A"; border.width: 2
        }
    }

    Text { // Wert
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        text: root.level
        color: root.on ? (root.kind === "led" ? "#FF8A80" : ZTheme.accent) : ZTheme.muted
        font.family: ZTheme.mono; font.pixelSize: 18; font.bold: true
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
