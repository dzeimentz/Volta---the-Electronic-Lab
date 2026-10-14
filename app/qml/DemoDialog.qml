// Vorfuehrung (Startseite, abschaltbar im Elternbereich): zehn fertige Aufbauten aus allen Stufen fuer die Schule oder
// ein Gespraech. Antippen startet das Experiment mit Musteraufbau und schaltet ein (LabController.startDemo); nichts
// davon landet in Fortschritt, Forscherheft oder Laborstand. Titel kommen aus den Inhalten, die Kurztexte von hier.
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: demo
    required property var controller
    signal demoStarted(string experimentId)

    objectName: "demoDialog"
    modal: true
    focus: true
    anchors.centerIn: Overlay.overlay
    width: Math.min(900, parent ? parent.width - 32 : 900)
    height: parent ? parent.height - 32 : 700
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    background: Rectangle { radius: 12; color: "#FFFDF7"; border.color: Theme.navy; border.width: 2 }

    // Auswahl: je Stufe mindestens ein Aufbau; recorder = Feld fuer Kanal 1 des Zeitschreibers
    readonly property var entries: [
        { id: "volta1.0.1", text: qsTr("Das Blinkmodul lässt eine Leuchtdiode blinken – innen arbeiten echte Transistoren.") },
        { id: "volta1.1.9", text: qsTr("Taster drücken: Summer und Lämpchen melden sich zugleich.") },
        { id: "volta1.2.6", text: qsTr("Zwei Wechselschalter: Das Licht lässt sich von beiden Enden des Flurs schalten.") },
        { id: "volta1.2.4", text: qsTr("Ohne Schutzwiderstand brennt die Leuchtdiode durch – »Rückgängig« macht sie wieder heil.") },
        { id: "volta1.2.14", text: qsTr("Ein Magnet an der Tür startet eine Melodie.") },
        { id: "volta1.3.12", text: qsTr("Der Zeitschreiber zeichnet auf, wie sich ein Kondensator füllt."), recorder: "B4" },
        { id: "volta1.3.15", text: qsTr("Ein kleiner Strom schaltet über das Relais einen großen.") },
        { id: "volta1.4.3", text: qsTr("Wird es dunkel, schaltet der Transistor das Licht ein. Unten bei »Licht, Wärme, Wasser« das Raumlicht ändern.") },
        { id: "volta1.4.8", text: qsTr("Taster drücken: Das Licht bleibt eine Weile an und geht von selbst wieder aus.") },
        { id: "volta1.4.10", text: qsTr("Zwei Transistoren schalten sich gegenseitig – die Leuchtdioden blinken im Wechsel.") }
    ]
    // Titel und Stufe aus dem Lernpfad
    function info(id) {
        const stages = controller.stages
        for (let s = 0; s < stages.length; ++s) {
            const exps = stages[s].experiments
            for (let e = 0; e < exps.length; ++e)
                if (exps[e].id === id) return { title: exps[e].title, stage: stages[s].number }
        }
        return { title: id, stage: 0 }
    }
    function start(entry) {
        close()
        if (!controller.startDemo(entry.id)) return
        if (entry.recorder) {
            controller.setRecorderChannel(0, entry.recorder)
            controller.recorderWindow = 10
        }
        demoStarted(entry.id)
    }

    // Kopf mit Tick
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
            Tick { visible: Theme.tick; mood: "motivierend"; width: 52; height: 52; anchors.verticalCenter: parent.verticalCenter }
            Label {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Vorführung")
                color: "white"
                font.pixelSize: Math.round(20 * Theme.fontScale)
                font.bold: true
            }
        }
        ToolButton {
            objectName: "demoClose"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.control; height: Theme.control
            contentItem: Item { DrawnIcon { anchors.centerIn: parent; kind: "close"; color: "white"; size: 20 } }
            onClicked: demo.close()
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
            spacing: 12

            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: qsTr("Zehn fertige Aufbauten aus allen Stufen. Antippen baut den Aufbau auf und schaltet ein. Es wird nichts gespeichert: Fortschritt, Forscherheft und der eigene Aufbau bleiben, wie sie sind.")
                font.pixelSize: Math.round(15 * Theme.fontScale)
                color: "#1E2B38"
            }
            Grid {
                id: grid
                width: parent.width
                columns: width > 640 ? 2 : 1
                spacing: 10
                Repeater {
                    model: demo.entries
                    Rectangle {
                        id: card
                        required property var modelData
                        required property int index
                        readonly property var meta: demo.info(modelData.id)
                        objectName: "demo_" + modelData.id
                        width: (grid.width - (grid.columns - 1) * grid.spacing) / grid.columns
                        height: Math.max(88, cardCol.implicitHeight + 20)
                        radius: 12
                        color: cardTap.pressed ? "#F2C894" : "#F7E3C2"
                        border.color: Theme.navy
                        border.width: 2
                        Rectangle {
                            id: badge
                            x: 12
                            anchors.verticalCenter: parent.verticalCenter
                            width: 44; height: 44; radius: 22
                            color: Theme.navy
                            Label {
                                anchors.centerIn: parent
                                text: card.meta.stage
                                color: "white"
                                font.bold: true
                                font.pixelSize: 20
                            }
                        }
                        Column {
                            id: cardCol
                            anchors.left: badge.right
                            anchors.leftMargin: 12
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4
                            Label {
                                width: parent.width
                                wrapMode: Text.Wrap
                                text: card.meta.title
                                font.bold: true
                                font.pixelSize: Math.round(16 * Theme.fontScale)
                                color: Theme.navy
                            }
                            Label {
                                width: parent.width
                                wrapMode: Text.Wrap
                                text: card.modelData.text
                                font.pixelSize: Math.round(13 * Theme.fontScale)
                                color: "#4A5866"
                            }
                        }
                        TapHandler { id: cardTap; onTapped: demo.start(card.modelData) }
                        Accessible.role: Accessible.Button
                        Accessible.name: qsTr("Stufe %1: %2").arg(card.meta.stage).arg(card.meta.title)
                    }
                }
            }
        }
    }
}
