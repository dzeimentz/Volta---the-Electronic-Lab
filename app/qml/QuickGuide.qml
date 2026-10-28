// Kurzanleitung (neben der Hilfe) fuer Kinder ab etwa 9 Jahren: acht Schritte zum ersten Experiment, jeder mit einem
// vergroesserten Bildausschnitt (orange Rahmen und Pfeile zeigen, wo man tippt oder zieht), dazu die Bedienung auf einen
// Blick. Die Bilder sind je Sprache aus der App erzeugt (tools/manual_pdfs.py --app-bilder).
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
        { title: qsTr("Experiment aussuchen"),
          text: qsTr("Tippe auf der Startseite im Lernpfad auf ein Experiment. Fang ganz oben bei Stufe 0 an.") },
        { title: qsTr("Frage lesen und raten"),
          text: qsTr("Tick stellt dir eine Forscherfrage. Was glaubst du? Tippe auf deine Antwort. Raten ist erlaubt!") },
        { title: qsTr("Bauteil holen"),
          text: qsTr("Zieh das Bauteil mit dem Finger aus dem Bauteilfach auf die Felder. Alles, was im selben Feld steckt, ist verbunden.") },
        { title: qsTr("Leitungen ziehen"),
          text: qsTr("Zieh mit dem Finger von einem Feld zum anderen. Eine Leitung von Plus (V+) wird rot, eine von Minus (0V) schwarz.") },
        { title: qsTr("Strom einschalten"),
          text: qsTr("Tippe an der Batterie auf den Hauptschalter, bis »Ein« dasteht. Oben steht jetzt »Betrieb«. Zum Umbauen schaltest du wieder aus.") },
        { title: qsTr("Hinschauen und »Fertig«"),
          text: qsTr("Leuchtet das Lämpchen? Dann hat es geklappt! Tippe auf »Fertig«.") },
        { title: qsTr("Sagen, was du gesehen hast"),
          text: qsTr("Tippe an, was passiert ist. Danach erklärt dir Tick, warum es so ist.") },
        { title: qsTr("Hilfe holen"),
          text: qsTr("»Schritt für Schritt bauen« zeigt dir jedes Teil. »Ich brauche einen Tipp« gibt dir einen Hinweis. Ging etwas schief? Der Pfeil oben macht es wieder rückgängig.") }
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
                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("In acht Schritten zum ersten Experiment")
                font.pixelSize: Math.round(19 * Theme.fontScale)
                font.bold: true
                color: Theme.navy
            }
            // je Schritt eine Karte: Nummer, Ueberschrift, kurzer Text, Bild mit Rahmen und Pfeil
            Repeater {
                model: guide.steps
                Rectangle {
                    id: card
                    required property var modelData
                    required property int index
                    objectName: "quickGuideStep_" + (index + 1)
                    width: col.width
                    height: cardCol.implicitHeight + 24
                    radius: 10
                    color: "white"
                    border.color: Theme.fieldOutline
                    Column {
                        id: cardCol
                        x: 12; y: 12
                        width: parent.width - 24
                        spacing: 8
                        Row {
                            width: parent.width
                            spacing: 12
                            Rectangle {
                                width: 44; height: 44; radius: 22
                                color: "#FFD23F"
                                border.color: Theme.navy
                                border.width: 2
                                Label {
                                    anchors.centerIn: parent
                                    text: card.index + 1
                                    font.pixelSize: 21
                                    font.bold: true
                                    color: Theme.navy
                                }
                            }
                            Label {
                                width: parent.width - 56
                                anchors.verticalCenter: parent.verticalCenter
                                wrapMode: Text.Wrap
                                text: card.modelData.title
                                font.pixelSize: Math.round(18 * Theme.fontScale)
                                font.bold: true
                                color: Theme.navy
                            }
                        }
                        Label {
                            width: parent.width
                            wrapMode: Text.Wrap
                            text: card.modelData.text
                            font.pixelSize: Math.round(16 * Theme.fontScale)
                            lineHeight: 1.15
                            color: "#1E2B38"
                        }
                        Image {
                            objectName: "quickGuideImage_" + (card.index + 1)
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: Math.min(parent.width, 640)
                            height: width / 2
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            source: "qrc:/guide/schritt" + (card.index + 1) + "_" + guide.language + ".jpg"
                            Accessible.role: Accessible.Graphic
                            Accessible.name: card.modelData.title
                        }
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
