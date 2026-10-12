// Kurzanleitung (neben der Hilfe): in sechs Schritten zum ersten Experiment und die Bedienung auf einen Blick.
// Das Bild ist ein Bildschirmfoto des Labors mit Nummern, je Sprache erzeugt (tools/manual_pdfs.py --app-bilder).
// Die ausfuehrliche Hilfe mit Tick bleibt im HelpDialog; die PDF-Anleitungen liegen ausserhalb der App.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: guide
    signal helpRequested()

    objectName: "quickGuide"
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(900, parent ? parent.width - 32 : 900)
    height: parent ? parent.height - 32 : 700
    padding: 0
    closePolicy: Popup.CloseOnEscape
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    readonly property string language: typeof volta_languageService !== "undefined" && volta_languageService
                                       ? volta_languageService.language : "de"
    readonly property var steps: [
        { title: qsTr("Bauteil holen"),
          text: qsTr("Ziehe ein Bauteil aus dem Bauteilfach auf die Grundplatte. Alles im selben Kontaktfeld ist miteinander verbunden.") },
        { title: qsTr("Leitungen ziehen"),
          text: qsTr("Ziehe mit dem Finger von Feld zu Feld. Leitungen an Plus werden rot, an Minus schwarz.") },
        { title: qsTr("Schritt für Schritt"),
          text: qsTr("Lies die Frage, wähle deine Vermutung und tippe auf »Schritt für Schritt bauen«. Die Zielfelder blinken blau.") },
        { title: qsTr("Einschalten"),
          text: qsTr("Tippe auf »Stecken«: Die Taste wird zu »Betrieb«, und Strom fließt. Zum Umbauen tippst du wieder darauf.") },
        { title: qsTr("Fertig"),
          text: qsTr("Funktioniert es? Dann tippe auf »Fertig« und wähle, was du beobachtet hast.") },
        { title: qsTr("Hilfe"),
          text: qsTr("Kommst du nicht weiter, tippe auf »Ich brauche einen Tipp«. Ist etwas kaputtgegangen, macht »Rückgängig« es wieder heil.") }
    ]
    readonly property var gestures: [
        [qsTr("Bauteil stecken oder verschieben"), qsTr("ziehen")],
        [qsTr("Bauteil drehen"), qsTr("zweimal schnell tippen (am Computer: Doppelklick)")],
        [qsTr("Menü: Drehen, Farbe, Wert, Löschen …"), qsTr("lange drücken (am Computer: Rechtsklick)")],
        [qsTr("Leitung ziehen"), qsTr("von Feld zu Feld ziehen")],
        [qsTr("Etwas entfernen"), qsTr("antippen und dann den Mülleimer antippen")],
        [qsTr("Rückgängig und Wiederholen"), qsTr("gebogene Pfeile oben in der Leiste")]
    ]

    onAboutToShow: flick.contentY = 0

    // Kopf mit Tick (wie die Hilfe)
    Rectangle {
        id: head
        width: parent.width
        height: 64
        radius: 12
        color: Theme.navy
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 12; color: Theme.navy }
        Row {
            x: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            Tick { visible: Theme.tick; mood: "erklaerend"; width: 52; height: 52; anchors.verticalCenter: parent.verticalCenter }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Kurzanleitung")
                color: "white"
                font.pixelSize: Math.round(20 * Theme.fontScale)
                font.bold: true
            }
        }
        ToolButton {
            objectName: "quickGuideClose"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: "white"; size: 20 } }
            onClicked: guide.close()
            Accessible.name: qsTr("Schließen")
        }
    }

    Flickable {
        id: flick
        anchors { left: parent.left; right: parent.right; top: head.bottom; bottom: parent.bottom }
        anchors.margins: 16
        contentHeight: col.implicitHeight + 16
        clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            id: col
            width: flick.width - 12
            spacing: 14

            Label {
                text: qsTr("In sechs Schritten zum ersten Experiment")
                font.pixelSize: Math.round(19 * Theme.fontScale)
                font.bold: true
                color: Theme.navy
            }
            // Labor mit den Nummern 1 bis 6
            Rectangle {
                width: parent.width
                height: width * 640 / 1024 + 4
                color: "white"
                border.color: Theme.fieldOutline
                Image {
                    objectName: "quickGuideImage"
                    anchors.fill: parent
                    anchors.margins: 2
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    source: "qrc:/guide/kurz_" + guide.language + ".jpg"
                    Accessible.role: Accessible.Graphic
                    Accessible.name: qsTr("Das Labor mit den Nummern der sechs Schritte")
                }
            }
            Repeater {
                model: guide.steps
                Row {
                    required property var modelData
                    required property int index
                    width: col.width
                    spacing: 12
                    Rectangle {
                        width: 40; height: 40; radius: 20
                        color: "#FFD23F"
                        border.color: Theme.navy
                        border.width: 2
                        Label {
                            anchors.centerIn: parent
                            text: index + 1
                            font.pixelSize: 19
                            font.bold: true
                            color: Theme.navy
                        }
                    }
                    Label {
                        width: parent.width - 52
                        anchors.verticalCenter: parent.verticalCenter
                        wrapMode: Text.Wrap
                        textFormat: Text.StyledText
                        text: "<b>" + modelData.title + "</b> – " + modelData.text
                        font.pixelSize: Math.round(15 * Theme.fontScale)
                        color: "#1E2B38"
                    }
                }
            }

            Label {
                topPadding: 6
                text: qsTr("Bedienung auf einen Blick")
                font.pixelSize: Math.round(19 * Theme.fontScale)
                font.bold: true
                color: Theme.navy
            }
            Rectangle {
                width: parent.width
                height: gestureCol.implicitHeight + 2
                radius: 8
                color: "white"
                border.color: Theme.fieldOutline
                Column {
                    id: gestureCol
                    x: 1; y: 1
                    width: parent.width - 2
                    Repeater {
                        model: guide.gestures
                        Rectangle {
                            required property var modelData
                            required property int index
                            width: gestureCol.width
                            height: Math.max(Theme.control, Math.max(what.implicitHeight, how.implicitHeight) + 12)
                            color: index % 2 === 0 ? "#FFFFFF" : "#F7F2E8"
                            radius: 7
                            Label {
                                id: what
                                x: 12
                                width: parent.width * 0.45 - 12
                                anchors.verticalCenter: parent.verticalCenter
                                wrapMode: Text.Wrap
                                text: modelData[0]
                                font.pixelSize: Math.round(15 * Theme.fontScale)
                                font.bold: true
                                color: Theme.navy
                            }
                            Label {
                                id: how
                                x: parent.width * 0.45 + 6
                                width: parent.width * 0.55 - 18
                                anchors.verticalCenter: parent.verticalCenter
                                wrapMode: Text.Wrap
                                text: modelData[1]
                                font.pixelSize: Math.round(15 * Theme.fontScale)
                                color: "#1E2B38"
                            }
                        }
                    }
                }
            }

            TickSays {
                width: parent.width
                figure: 84
                mood: "warnend"
                bubbleColor: "#FFE9DC"
                borderColor: Theme.signalOrange
                text: qsTr("Mit echten Bauteilen: nur Batterien bis 9 Volt, niemals an der Steckdose! Versuche mit »Nur in der App!« baust du nicht in echt nach.")
            }

            Button {
                objectName: "quickGuideHelp"
                height: Theme.control
                text: qsTr("Zur ausführlichen Hilfe")
                font.bold: true
                onClicked: { guide.close(); guide.helpRequested() }
            }
        }
    }
}
