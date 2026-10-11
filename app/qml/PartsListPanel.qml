// Teileliste mit Aufbaukarte (Werkzeugkasten, docs/ui-spec.md): was fuer den Aufbau gebraucht wird - im Experiment
// die Vorlage, im freien Labor der eigene Aufbau. »Aufbaukarte speichern« schreibt ein PDF mit Zeichnung,
// Teileliste und Schritten zum Nachbauen mit echten Bauteilen. Zerstoerende Versuche (»Nur in der App!«) bekommen
// keine Karte.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs

Flickable {
    id: list
    required property var controller
    property var lines: []
    readonly property bool hasParts: lines.some(l => l.kind !== "batterie")
    readonly property bool appOnly: !controller.freeLab && controller.destructive

    contentHeight: col.implicitHeight + 16
    clip: true

    function refresh() { list.lines = list.controller.partsList() }
    Component.onCompleted: refresh()
    onVisibleChanged: if (visible) refresh()
    Connections {
        target: list.controller
        function onBoardChanged() { if (list.visible) list.refresh() }
        function onExperimentChanged() { list.refresh() }
        function onDeskChanged() { if (list.visible) list.refresh() }
    }

    // Texte der Karte, fertig uebersetzt (Zeichnung und Satz macht LabController::exportBuildCard)
    function cardTexts() {
        const c = list.controller
        const labels = {}
        for (const l of list.lines) {
            if (l.kind === "bauteil") {
                const v = Texts.partValue(l.type, l.value)
                for (const id of l.ids) labels[id] = v !== "" ? id + " " + v : id
            } else if (l.kind === "modul") {
                for (const id of l.ids) labels[id] = id + " " + Texts.moduleName(l.type)
            }
        }
        const steps = c.buildSteps().map(s => Texts.stepText(s)
            + (s.kind === "leitung" ? " " + qsTr("Farbe: %1.").arg(Texts.colorName(s.color)) : ""))
        steps.push(Texts.stepText({ kind: "hauptschalter" }))
        const parts = c.experimentId.split(".")
        return {
            brand: qsTr("Volta 1 – Aufbaukarte"),
            title: c.freeLab ? qsTr("Mein eigener Aufbau") : c.title,
            subtitle: c.freeLab || parts.length < 2 ? "" : qsTr("Stufe %1").arg(parts[1]),
            question: c.freeLab ? "" : c.question,
            warning: "",
            boardHeading: qsTr("So sieht der Aufbau aus"),
            partsHeading: qsTr("Das brauchst du"),
            stepsHeading: qsTr("So baust du es auf"),
            parts: list.lines.map(l => Texts.partsLine(l)),
            steps: steps,
            footer: qsTr("Sicher experimentieren: nur mit Batterien bis 9 V, nie an der Steckdose. Erstellt mit Volta 1 – Das Elektronik-Labor."),
            labels: labels
        }
    }

    FileDialog {
        id: saveDialog
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("PDF-Dokument (*.pdf)")]
        defaultSuffix: "pdf"
        currentFile: "file:///" + list.controller.buildCardFileName()
        onAccepted: result.text = list.controller.exportBuildCard(selectedFile, list.cardTexts())
                    ? "✓ " + qsTr("Aufbaukarte gespeichert.") : qsTr("Speichern hat nicht geklappt.")
    }

    Column {
        id: col
        width: parent.width
        spacing: 10

        TickSays {
            width: parent.width
            figure: 76
            visible: !list.appOnly
            mood: "erklaerend"
            text: list.controller.freeLab
                  ? qsTr("Das steckt in deinem Aufbau. Als Aufbaukarte kannst du ihn ausdrucken und mit echten Bauteilen nachbauen.")
                  : qsTr("Das brauchst du für dieses Experiment. Als Aufbaukarte kannst du es ausdrucken und mit echten Bauteilen nachbauen.")
        }
        TickSays {
            objectName: "partsAppOnly"
            width: parent.width
            figure: 76
            visible: list.appOnly
            mood: "warnend"
            text: "⚠ " + qsTr("Nur in der App! Bei diesem Versuch geht ein Bauteil kaputt – darum gibt es dafür keine Aufbaukarte.")
            textColor: "#8A3200"
            bubbleColor: "#FFE9DC"
            borderColor: Theme.signalOrange
        }
        Label {
            width: parent.width
            wrapMode: Text.Wrap
            visible: !list.hasParts
            text: qsTr("Noch ist nichts aufgebaut.")
            font.pixelSize: Math.round(14 * Theme.fontScale)
            color: "#4A5866"
        }
        Repeater {
            model: list.lines
            Rectangle {
                id: row
                required property int index
                required property var modelData
                objectName: "partsLine_" + index
                width: col.width
                height: Math.max(40, lineText.implicitHeight + 14)
                radius: 6
                color: index % 2 === 0 ? "#FFFDF7" : "#F4EFE4"
                border.color: Theme.fieldOutline
                // Leitungsfarbe als Streifen, der Farbname steht im Text (Farbe nie allein)
                Rectangle {
                    visible: row.modelData.kind === "leitung"
                    x: 8
                    anchors.verticalCenter: parent.verticalCenter
                    width: 18; height: 6; radius: 3
                    color: ({ rot: "#D32F2F", schwarz: "#222222", blau: "#1E63C6", gelb: "#E0B000", gruen: "#2E7D32", weiss: "#FAFAFA" })[row.modelData.type] || "#888888"
                    border.color: "#707070"
                }
                Label {
                    id: lineText
                    x: row.modelData.kind === "leitung" ? 34 : 10
                    width: parent.width - x - 10
                    anchors.verticalCenter: parent.verticalCenter
                    wrapMode: Text.Wrap
                    text: Texts.partsLine(row.modelData)
                    font.pixelSize: Math.round(14 * Theme.fontScale)
                    color: "#1E2B38"
                }
            }
        }
        Button {
            objectName: "saveBuildCard"
            visible: !list.appOnly
            enabled: list.hasParts
            width: parent.width
            height: Theme.control
            text: qsTr("Aufbaukarte speichern (PDF)")
            font.bold: true
            onClicked: { result.text = ""; saveDialog.open() }
        }
        Label {
            id: result
            objectName: "buildCardResult"
            width: parent.width
            wrapMode: Text.Wrap
            visible: text !== ""
            font.pixelSize: Math.round(14 * Theme.fontScale)
            color: "#2E7D32"
        }
    }
}
