// Erklaerung eines Pultbauteils (»?« oben rechts am Rahmen, docs/ui-spec.md, Bedienpult): was das Bauteil tut, welche
// Pultfelder es hat und wofuer sie da sind. Werte wie in der Simulation (Poti 10 kOhm linear, Lautsprecher 8 Ohm).
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: info
    property string part: ""

    function show(name) {
        part = name
        open()
    }

    // Inhalt je Bauteil: Titel, Einleitung, Felder [[Feld, Bedeutung]], Hinweise
    readonly property var entry: {
        const pot = (n) => ({
            title: qsTr("Potentiometer %1").arg(n),
            intro: qsTr("Ein Potentiometer (kurz Poti) ist ein Drehwiderstand mit 10 kΩ. Mit dem Drehknopf darüber verstellst du ihn; der Leuchtring zeigt die Stellung."),
            fields: [["P" + n + "a", qsTr("Anschluss a: ein Ende der Widerstandsbahn.")],
                     ["P" + n + "s", qsTr("Schleifer: gleitet beim Drehen über die Bahn.")],
                     ["P" + n + "b", qsTr("Anschluss b: das andere Ende der Bahn.")]],
            tips: [qsTr("Drehst du auf, wandert der Schleifer von a nach b: Zwischen a und s wird der Widerstand größer, zwischen s und b kleiner."),
                   qsTr("Als Spannungsteiler: a an 0V, b an V+. Am Schleifer s greifst du dann eine Spannung ab, die du mit dem Knopf einstellst."),
                   qsTr("Als regelbarer Widerstand nutzt du nur a und s.")]
        })
        switch (part) {
        case "batterie": return {
            title: qsTr("Batterie"),
            intro: qsTr("Die Batterie ist die Stromquelle deines Labors. Sie besteht aus Zellen mit je 1,5 Volt; oben wählst du 3, 4,5 oder 9 Volt."),
            fields: [["V+", qsTr("Pluspol (rot): Von hier fließt der Strom durch deine Schaltung.")],
                     ["0V", qsTr("Minuspol (schwarz), auch GND oder Masse: Hierhin fließt der Strom zurück. Spannungen misst du gegen 0V.")]],
            tips: [qsTr("Der Hauptschalter schaltet den Strom für die ganze Schaltung ein und aus (»Ein« / »Aus«)."),
                   qsTr("Verbinde V+ nie direkt mit 0V – das ist ein Kurzschluss.")]
        }
        case "p1": return pot(1)
        case "p2": return pot(2)
        case "schalter": return {
            title: qsTr("Schalter"),
            intro: qsTr("Der Kippschalter verbindet zwei Felder oder trennt sie. Tippe auf ihn, um ihn umzulegen."),
            fields: [["S1", qsTr("Kontakt 1")], ["S2", qsTr("Kontakt 2")]],
            tips: [qsTr("Knebel oben (»ein«): S1 und S2 sind verbunden, Strom kann fließen."),
                   qsTr("Knebel unten (»aus«): Die Verbindung ist getrennt.")]
        }
        case "lautsprecher": return {
            title: qsTr("Lautsprecher"),
            intro: qsTr("Der Lautsprecher macht aus einem Strom, der sich schnell ändert, einen Ton. Seine Spule hat 8 Ω."),
            fields: [["LS1", qsTr("Anschluss 1")], ["LS2", qsTr("Anschluss 2")]],
            tips: [qsTr("Fließt ein gleichbleibender Strom, bleibt er stumm."),
                   qsTr("Der Leuchtring zeigt, ob und wie laut ein Ton erklingt."),
                   qsTr("Wie herum du ihn anschließt, ist egal.")]
        }
        }
        return {title: "", intro: "", fields: [], tips: []}
    }

    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(parent ? parent.width - 32 : 520, 520)
    height: Math.min(parent ? parent.height - 32 : 560, body.implicitHeight + header.height + 28)
    padding: 0
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    Rectangle {
        id: header
        anchors { left: parent.left; right: parent.right; top: parent.top }
        height: Theme.control + 8
        radius: 12
        color: Theme.desk
        Rectangle { anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom; height: 12; color: Theme.desk }
        Label {
            objectName: "deskInfoTitle"
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.right: close.left
            anchors.verticalCenter: parent.verticalCenter
            elide: Text.ElideRight
            text: info.entry.title
            color: "white"
            font.pixelSize: Math.round(18 * Theme.fontScale)
            font.bold: true
        }
        ToolButton {
            id: close
            objectName: "deskInfoClose"
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: "white"; size: 18 } }
            onClicked: info.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    Flickable {
        anchors { left: parent.left; right: parent.right; top: header.bottom; bottom: parent.bottom; margins: 14 }
        contentHeight: body.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        Column {
            id: body
            width: parent.width
            spacing: 12

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: info.entry.intro
                font.pixelSize: Math.round(15 * Theme.fontScale)
                color: "#1E2B38"
            }
            Label {
                text: qsTr("Anschlüsse")
                font.pixelSize: Math.round(15 * Theme.fontScale)
                font.bold: true
                color: Theme.navy
            }
            // je Pultfeld: Feld wie am Pult, daneben die Bedeutung
            Repeater {
                model: info.entry.fields
                Row {
                    required property var modelData
                    width: body.width
                    spacing: 12
                    FieldChip {
                        id: chip
                        anchors.verticalCenter: parent.verticalCenter
                        name: parent.modelData[0]
                    }
                    Label {
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - chip.width - 12
                        wrapMode: Text.Wrap
                        text: "<b>" + parent.modelData[0] + "</b> – " + parent.modelData[1]
                        textFormat: Text.StyledText
                        font.pixelSize: Math.round(15 * Theme.fontScale)
                        color: "#1E2B38"
                    }
                }
            }
            Repeater {
                model: info.entry.tips
                Label {
                    required property string modelData
                    width: body.width
                    leftPadding: 18
                    wrapMode: Text.Wrap
                    text: "•  " + modelData
                    font.pixelSize: Math.round(15 * Theme.fontScale)
                    lineHeight: 1.1
                    color: "#1E2B38"
                }
            }
        }
    }
}
