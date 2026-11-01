// Erklaerung eines Pultbauteils (»?« oben rechts am Rahmen, docs/ui-spec.md, Bedienpult): was das Bauteil tut, welche
// Pultfelder es hat und wofuer sie da sind. Werte wie in der Simulation (Poti 10 kOhm linear, Lautsprecher 8 Ohm).
import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: info
    property string part: ""
    property var controller: null
    property bool confirmRemove: false // Rueckfrage: Leitungen stecken noch an den Feldern
    // Pult nach Bedarf: jedes Pultteil ausser der Batterie laesst sich herausnehmen (Befehl)
    readonly property bool removable: controller !== null && controller.deskOnDemand && part !== "batterie"

    function show(name) {
        part = name
        confirmRemove = false
        open()
    }
    function removePart() {
        if (!confirmRemove && controller.deskPartWires(part) > 0) {
            confirmRemove = true
            return
        }
        controller.removeDeskPart(part)
        close()
    }

    // Inhalt je Bauteil: Titel, Einleitung, Felder [[Feld, Bedeutung]], Hinweise
    readonly property var entry: {
        const pot = (n) => ({
            title: qsTr("Potentiometer %1").arg(n),
            intro: qsTr("Ein Potentiometer – auch Drehregler oder kurz Poti genannt – ist ein Drehwiderstand mit 10 kΩ. Mit dem Drehknopf darüber verstellst du ihn; der Leuchtring zeigt die Stellung."),
            fields: [["P" + n + "a", qsTr("Anschluss a: ein Ende der Widerstandsbahn.")],
                     ["P" + n + "s", qsTr("Schleifer: gleitet beim Drehen über die Bahn.")],
                     ["P" + n + "b", qsTr("Anschluss b: das andere Ende der Bahn.")]],
            tips: [qsTr("Drehst du auf, wandert der Schleifer von a nach b: Zwischen a und s wird der Widerstand größer, zwischen s und b kleiner."),
                   qsTr("Als Spannungsteiler: V+ an a, 0V an b. Am Schleifer s greifst du dann eine Spannung ab, die du mit dem Knopf einstellst. Drehst du auf, wird sie kleiner."),
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
        case "batterie2": return {
            title: qsTr("Batterie 2"),
            intro: qsTr("Die zweite Batterie ist von der ersten getrennt. Du wählst 4,5 oder 9 Volt; der Hauptschalter schaltet beide Batterien zugleich."),
            fields: [["+B2", qsTr("Pluspol (rot) der zweiten Batterie.")],
                     ["\u2212B2", qsTr("Minuspol (blau) der zweiten Batterie.")]],
            tips: [qsTr("Verbindest du +B2 mit 0V, liegt −B2 unter 0V: Dort misst du eine negative Spannung. So entsteht eine Versorgung mit Plus, Minus und der Mitte 0V."),
                   qsTr("Verbinde +B2 nie direkt mit −B2 – das ist ein Kurzschluss.")]
        }
        case "wechselquelle": return {
            title: qsTr("Wechselquelle"),
            intro: qsTr("Die Wechselquelle liefert eine Wechselspannung wie ein kleiner Transformator: 6 oder 9 Volt (Effektivwert), 50 Schwingungen je Sekunde. Plus und Minus tauschen ständig die Seiten."),
            fields: [["W~a", qsTr("Anschluss a der Wicklung.")],
                     ["W~b", qsTr("Anschluss b der Wicklung.")]],
            tips: [qsTr("Die Wechselquelle hat keine Verbindung zu 0V – wie die Wicklung eines echten Transformators. Willst du die Spannung gegen 0V messen oder am Oszilloskop sehen, verbinde einen Anschluss mit 0V."),
                   qsTr("Der Hauptschalter schaltet sie zusammen mit der Batterie ein und aus."),
                   qsTr("Bei 9 V~ ist die Spitze 9 V · 1,41 = 12,7 V. Das ist eine Kleinspannung – mit der Steckdose experimentierst du nie!")]
        }
        case "generator": return {
            title: qsTr("Funktionsgenerator"),
            intro: qsTr("Der Funktionsgenerator erzeugt Sinus, Dreieck oder Rechteck mit 1 Hz bis 10 kHz. Tippe auf seinen Knopf, um Form, Frequenz, Spitze und Gleichanteil einzustellen."),
            fields: [["G", qsTr("Ausgang des Generators (50 Ω Innenwiderstand).")],
                     ["G0", qsTr("Masse des Generators (schwarz), fest mit 0V verbunden.")]],
            tips: [qsTr("Weil G0 schon an 0V liegt, misst das Oszilloskop das Signal an G direkt."),
                   qsTr("Am Lautsprecher hörst du die Frequenz als Tonhöhe.")]
        }
        case "kennlinienschreiber": return {
            title: qsTr("Kennlinienschreiber"),
            intro: qsTr("Der Kennlinienschreiber zeigt, wie viel Strom durch ein Bauteil fließt, wenn die Spannung daran steigt. Er fährt die Spannung selbst durch und zeichnet den Strom darüber. Tippe auf seinen Knopf, um die Kennlinie zu sehen."),
            fields: [["KS+", qsTr("Prüfspannung: hier fließt der Strom aus dem Schreiber in das Bauteil.")],
                     ["KS−", qsTr("Rückleitung des Schreibers. Er hat ein eigenes Netzteil und braucht den Hauptschalter nicht.")]],
            tips: [qsTr("Ein Widerstand ergibt eine Gerade: doppelte Spannung, doppelter Strom."),
                   qsTr("Bei einer Diode oder LED bleibt der Strom erst fast null und steigt dann steil an."),
                   qsTr("Ein Vorwiderstand im Schreiber begrenzt den Strom. So gehen die Bauteile nicht kaputt, solange du die Strombegrenzung klein lässt.")]
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
            Label {
                visible: info.removable && info.confirmRemove
                width: body.width
                wrapMode: Text.Wrap
                text: qsTr("An den Feldern stecken noch Leitungen. Sie werden mit abgezogen – »Rückgängig« holt alles zurück.")
                font.pixelSize: Math.round(15 * Theme.fontScale)
                font.bold: true
                color: "#8A3A00"
            }
            Button {
                objectName: "deskPartRemove"
                visible: info.removable
                height: Theme.control
                text: info.confirmRemove ? qsTr("Trotzdem herausnehmen") : qsTr("Aus dem Pult nehmen")
                onClicked: info.removePart()
            }
        }
    }
}
